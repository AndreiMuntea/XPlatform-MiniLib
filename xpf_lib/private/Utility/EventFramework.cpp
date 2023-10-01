/**
 * @file        xpf_lib/private/utility/EventFramework.cpp
 *
 * @brief       This implements a simple and fast event framework.
 *              It can have multiple listeners registered.
 *              The events must inherit the IEvent interface.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into default section.
 */
XPF_SECTION_DEFAULT;

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::Dispatch(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event,
    _In_ xpf::EventDispatchType DispatchType
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Can't throw empty event.
    //
    if (Event.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Prevent the event bus from being run down.
    //
    xpf::RundownGuard busGuard{ this->m_EventBusRundown };
    if (!busGuard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    //
    // If auto was selected, we'll send the event async for now.
    // In future we may want to add further optimizations (like when the stack is too low,
    // or we have too many events in queue).
    //
    if (DispatchType == xpf::EventDispatchType::kAuto)
    {
        if (this->m_EnqueuedAsyncItems >= this->ASYNC_THRESHOLD)
        {
            DispatchType = xpf::EventDispatchType::kSync;
        }
        else
        {
            DispatchType = xpf::EventDispatchType::kAsync;
        }
    }

    if (DispatchType == xpf::EventDispatchType::kSync)
    {
        this->NotifyListeners(Event);
        return STATUS_SUCCESS;
    }
    else
    {
        XPF_DEATH_ON_FAILURE(DispatchType == xpf::EventDispatchType::kAsync);
        return this->EnqueueAsync(Event);
    }
}

void
XPF_API
xpf::EventBus::NotifyListeners(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Prevent the event bus from being run down.
    //
    xpf::RundownGuard busGuard{ this->m_EventBusRundown };
    if (!busGuard.IsRundownAcquired())
    {
        return;
    }

    //
    // Now walk all listeners and notify the event.
    //
    XPF_SINGLE_LIST_ENTRY* crtEntry = this->m_Listeners.Head;
    while (crtEntry != nullptr)
    {
        xpf::EventListenerData* eventListenerData = XPF_CONTAINING_RECORD(crtEntry, xpf::EventListenerData, ListEntry);
        crtEntry = crtEntry->Next;

        if (nullptr == eventListenerData)
        {
            continue;
        }

        xpf::RundownGuard listenerGuard{ eventListenerData->Rundown };
        if (!listenerGuard.IsRundownAcquired())
        {
            continue;
        }

        if (nullptr != eventListenerData->NakedPointer)
        {
            eventListenerData->NakedPointer->OnEvent(Event, this);
        }
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::EnqueueAsync(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Prevent the event bus from being run down.
    //
    xpf::RundownGuard busGuard{ this->m_EventBusRundown };
    if (!busGuard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    //
    // Now we allocate the event data structure.
    //
    void* memoryBlock = this->m_Allocator.AllocateMemory(sizeof(xpf::EventData));
    if (nullptr == memoryBlock)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::ApiZeroMemory(memoryBlock, sizeof(xpf::EventData));

    //
    // Now construct the object.
    //
    xpf::EventData* eventData = reinterpret_cast<xpf::EventData*>(memoryBlock);
    xpf::MemoryAllocator::Construct(eventData);

    eventData->Event = Event;
    eventData->Bus = this;

    xpf::ApiAtomicIncrement(&this->m_EnqueuedAsyncItems);
    const NTSTATUS status = (*this->m_AsyncPool).Enqueue(this->AsyncCallback,
                                                         this->AsyncCallback,
                                                         eventData);
    if (!NT_SUCCESS(status))
    {
        xpf::ApiAtomicDecrement(&this->m_EnqueuedAsyncItems);
        goto CleanUp;
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        xpf::MemoryAllocator::Destruct(eventData);
        this->m_Allocator.FreeMemory(reinterpret_cast<void**>(&eventData));
    }
    return status;
}

/**
 * @brief   Threadpool related callback code can be paged out.
 *          Our threads are running at passive anyway.
 *          Everything here can be paged.
 */
XPF_SECTION_PAGED;

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::Create(
    _Inout_ xpf::Optional<xpf::EventBus>* EventBusToCreate
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized event bus.
    // Assert here and bail early.
    //
    if ((nullptr == EventBusToCreate) || (EventBusToCreate->HasValue()))
    {
        XPF_DEATH_ON_FAILURE(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new event bus. This will be an empty one.
    // It will be initialized below.
    //
    EventBusToCreate->Emplace();

    //
    // We failed to create an event. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!EventBusToCreate->HasValue())
    {
        XPF_DEATH_ON_FAILURE(false);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Grab a reference to the newly created event bus. Makes the code easier to read.
    // It will be optimized away on release anyway.
    //
    xpf::EventBus& newEventBus = (*(*EventBusToCreate));

    //
    // Create the threadpool.
    //
    status = xpf::ThreadPool::Create(&newEventBus.m_AsyncPool);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    if (!NT_SUCCESS(status))
    {
        EventBusToCreate->Reset();
        XPF_DEATH_ON_FAILURE(!EventBusToCreate->HasValue());
    }
    else
    {
        XPF_DEATH_ON_FAILURE(EventBusToCreate->HasValue());
    }
    return status;
}

void
XPF_API
xpf::EventBus::Rundown(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Block further event listeners registrations and bus operations.
    //
    this->m_EventBusRundown.WaitForRelease();

    //
    // Walk all listeners and run down.
    //
    XPF_SINGLE_LIST_ENTRY* crtEntry = this->m_Listeners.Head;
    while (crtEntry != nullptr)
    {
        xpf::EventListenerData* eventListenerData = XPF_CONTAINING_RECORD(crtEntry, xpf::EventListenerData, ListEntry);
        crtEntry = crtEntry->Next;

        if (nullptr != eventListenerData)
        {
            //
            // Wait for all callbacks to finish.
            //
            eventListenerData->Rundown.WaitForRelease();

            //
            // Invalidate the naked pointer and the id.
            //
            eventListenerData->NakedPointer = nullptr;
            xpf::ApiZeroMemory(&eventListenerData->Id, sizeof(xpf::EVENT_LISTENER_ID));
        }
    }

    //
    // Clean the threadpool. This will also ensure all elements are flushed.
    //
    if (this->m_AsyncPool.HasValue())
    {
        (*this->m_AsyncPool).Rundown();
    }

    //
    // And now flush the event listeners and free resources.
    //
    crtEntry = TlqFlush(this->m_Listeners);
    while (crtEntry != nullptr)
    {
        xpf::EventListenerData* eventListenerData = XPF_CONTAINING_RECORD(crtEntry, xpf::EventListenerData, ListEntry);
        crtEntry = crtEntry->Next;

        //
        // And now free the resources.
        //
        if (nullptr != eventListenerData)
        {
            xpf::MemoryAllocator::Destruct(eventListenerData);
            this->m_Allocator.FreeMemory(reinterpret_cast<void**>(&eventListenerData));
        }
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::RegisterListener(
    _In_ xpf::IEventListener* Listener,
    _Out_ xpf::EVENT_LISTENER_ID* ListenerId
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::EventListenerData* listenerData = nullptr;

    //
    // Parameters validation. Sanity check.
    //
    if ((nullptr == Listener) || (nullptr == ListenerId))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // First the event bus rundown. If the bus was destroyed, we can't enqueue.
    //
    xpf::RundownGuard guard{ this->m_EventBusRundown };
    if (!guard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    //
    // Now we allocate the event data listener structure.
    //
    void* memoryBlock = this->m_Allocator.AllocateMemory(sizeof(xpf::EventListenerData));
    if (nullptr == memoryBlock)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }
    xpf::ApiZeroMemory(memoryBlock, sizeof(xpf::EventListenerData));

    //
    // And now create the structure.
    //
    listenerData = reinterpret_cast<xpf::EventListenerData*>(memoryBlock);
    xpf::MemoryAllocator::Construct(listenerData);

    //
    // Now generate the random id for the listener.
    // And fill the other details. From this point below, we can't fail.
    //
    static_assert(sizeof(xpf::EVENT_LISTENER_ID) == sizeof(uuid_t), "Invariant violation!");
    xpf::ApiRandomUuid(&listenerData->Id);
    listenerData->NakedPointer = Listener;

    //
    // All good. insert the listener in list and set the output parameters.
    //
    TlqPush(this->m_Listeners, &listenerData->ListEntry);
    xpf::ApiCopyMemory(ListenerId, &listenerData->Id, sizeof(listenerData->Id));
    status = STATUS_SUCCESS;

CleanUp:
    if (!NT_SUCCESS(status))
    {
        if (nullptr != listenerData)
        {
            xpf::MemoryAllocator::Destruct(listenerData);
            this->m_Allocator.FreeMemory(reinterpret_cast<void**>(&listenerData));
        }
    }
    return status;
}


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::UnregisterListener(
    _In_ _Const_ const xpf::EVENT_LISTENER_ID& ListenerId
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // First the event bus rundown. If the bus was destroyed, we can't do anything.
    //
    xpf::RundownGuard guard{ this->m_EventBusRundown };
    if (!guard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    //
    // Now walk the listeners list and search for the one with the given id.
    //
    XPF_SINGLE_LIST_ENTRY* crtEntry = this->m_Listeners.Head;
    while (crtEntry != nullptr)
    {
        xpf::EventListenerData* eventListenerData = XPF_CONTAINING_RECORD(crtEntry, xpf::EventListenerData, ListEntry);
        crtEntry = crtEntry->Next;

        //
        // Further events will not be sent to this listener.
        //
        if ((nullptr != eventListenerData) && (xpf::ApiAreUuidsEqual(ListenerId, eventListenerData->Id)))
        {
            eventListenerData->Rundown.WaitForRelease();
            return STATUS_SUCCESS;
        }
    }

    //
    // At this point we couldn't find the listener with the given id.
    //
    return STATUS_NOT_FOUND;
}

void
XPF_API
xpf::EventBus::AsyncCallback(
    _In_opt_ xpf::thread::CallbackArgument EventData
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::EventData* eventData = reinterpret_cast<xpf::EventData*>(EventData);
    if (nullptr == eventData)
    {
        XPF_DEATH_ON_FAILURE(false);
        return;
    }

    if (nullptr == eventData->Bus)
    {
        XPF_DEATH_ON_FAILURE(false);
        return;
    }

    //
    // Get a reference to the allocator from event bus.
    // We'll be destroying the event data structure at the end.
    //
    auto& allocator = eventData->Bus->m_Allocator;

    //
    // Notify the listeners. This comes from a thread pool.
    // We have a guarantee that the event bus is not ran down yet.
    // The threadpool will wait for all outstanding items to be processed.
    //
    eventData->Bus->NotifyListeners(eventData->Event);
    xpf::ApiAtomicDecrement(&eventData->Bus->m_EnqueuedAsyncItems);

    //
    // And now do the actual cleanup. First call the destructor.
    // And then free the memory.
    //
    xpf::MemoryAllocator::Destruct(eventData);
    allocator.FreeMemory(reinterpret_cast<void**>(&eventData));
}
