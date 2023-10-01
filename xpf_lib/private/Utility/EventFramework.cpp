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
    // Grab a reference to the current listeners list.
    //
    xpf::SharedPointer<ListenersList, xpf::CriticalMemoryAllocator> listenersSnapshot;
    {
        xpf::SharedLockGuard listenersLock{ this->m_ListenersLock };
        listenersSnapshot = this->m_Listeners;
    }

    //
    // No listener was registered, so we have nothing to do.
    //
    if (listenersSnapshot.IsEmpty())
    {
        return;
    }
    xpf::EventBus::ListenersList& currentListenersList = (*listenersSnapshot);

    //
    // Now walk all listeners and notify the event.
    //
    for (size_t i = 0; i < currentListenersList.Size(); ++i)
    {
        xpf::SharedPointer<xpf::EventListenerData, xpf::CriticalMemoryAllocator> currentListener = currentListenersList[i];
        if (currentListener.IsEmpty())
        {
            continue;
        }
        xpf::EventListenerData& eventListenerData = (*currentListener);

        xpf::RundownGuard listenerGuard{ eventListenerData.Rundown };
        if (!listenerGuard.IsRundownAcquired())
        {
            continue;
        }
        if (nullptr != eventListenerData.NakedPointer)
        {
            eventListenerData.NakedPointer->OnEvent(Event, this);
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
    // Frist we acquire the listeners lock - this will prevent other operations while we are working.
    // We run down the listeners with the shared lock guard taken as we'll not modify the listeners list lock.
    //
    {
        xpf::SharedLockGuard listenersGuard{ this->m_ListenersLock };
        if (!this->m_Listeners.IsEmpty())
        {
            xpf::EventBus::ListenersList& currentListenersList = (*this->m_Listeners);
            for (size_t i = 0; i < currentListenersList.Size(); ++i)
            {
                xpf::SharedPointer<xpf::EventListenerData, xpf::CriticalMemoryAllocator> currentListener = currentListenersList[i];
                if (!currentListener.IsEmpty())
                {
                    (*currentListener).Rundown.WaitForRelease();
                }
            }
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
    {
        xpf::ExclusiveLockGuard listenersGuard{ this->m_ListenersLock };
        this->m_Listeners.Reset();
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
    // Now we create the event data listener structure.
    //
    auto listenerDataSharedPtr = xpf::MakeShared<xpf::EventListenerData, xpf::CriticalMemoryAllocator>();
    if (listenerDataSharedPtr.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::EventListenerData& listenerData = (*listenerDataSharedPtr);

    //
    // Now generate the random id for the listener.
    // And fill the other details. From this point below, we can't fail.
    //
    static_assert(sizeof(xpf::EVENT_LISTENER_ID) == sizeof(uuid_t), "Invariant violation!");
    xpf::ApiRandomUuid(&listenerData.Id);
    listenerData.NakedPointer = Listener;

    //
    // All good. insert the listener in list and set the output parameters.
    // Here we need to clone the listeners first. Hold the lock with minimal scope.
    //
    xpf::ExclusiveLockGuard listenersGuard{ this->m_ListenersLock };
    xpf::SharedPointer<ListenersList, xpf::CriticalMemoryAllocator> newListenersList = this->CloneListeners();
    if (newListenersList.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we can't insert the new listener in the clone, we're done.
    //
    NTSTATUS status = (*newListenersList).Emplace(listenerDataSharedPtr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now we need to replace the listeners list with the cloned one.
    //
    this->m_Listeners = newListenersList;
    xpf::ApiCopyMemory(ListenerId, &listenerData.Id, sizeof(listenerData.Id));

    return STATUS_SUCCESS;
}


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::EventBus::UnregisterListener(
    _In_ _Const_ const xpf::EVENT_LISTENER_ID& ListenerId
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    bool foundListener = false;

    //
    // First the event bus rundown. If the bus was destroyed, we can't do anything.
    //
    xpf::RundownGuard guard{ this->m_EventBusRundown };
    if (!guard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    //
    // Frist we acquire the listeners lock - this will prevent other operations while we are working.
    // We run down the listener with the shared lock guard taken as we'll not modify the listeners list lock.
    // This will still allow other events to be dispatched.
    //
    {
        xpf::SharedLockGuard listenersGuard{ this->m_ListenersLock };
        if (this->m_Listeners.IsEmpty())
        {
            return STATUS_NOT_FOUND;
        }
        xpf::EventBus::ListenersList& currentListenersList = (*this->m_Listeners);

        for (size_t i = 0; i < currentListenersList.Size(); ++i)
        {
            xpf::SharedPointer<xpf::EventListenerData, xpf::CriticalMemoryAllocator> currentListener = currentListenersList[i];
            if (currentListener.IsEmpty())
            {
                continue;
            }
            if (xpf::ApiAreUuidsEqual(ListenerId, (*currentListener).Id))
            {
                (*currentListener).Rundown.WaitForRelease();
                foundListener = true;
                break;
            }
        }
    }

    //
    // No point in cloning the listeners list as we did not find the element.
    //
    if (!foundListener)
    {
        return STATUS_NOT_FOUND;
    }

    //
    // And now we'll update the list. This requires exclusive lock.
    // If we fail to clone the listeners list we won't fail the operation.
    // The listener was ran down, and won't receive other events.
    // So we just move on.
    //
    {
        xpf::ExclusiveLockGuard listenersGuard{ this->m_ListenersLock };
        xpf::SharedPointer<ListenersList, xpf::CriticalMemoryAllocator> newListenersList = this->CloneListeners();
        if (!newListenersList.IsEmpty())
        {
            this->m_Listeners = newListenersList;
        }
    }

    return STATUS_SUCCESS;
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

xpf::SharedPointer<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator>
XPF_API
xpf::EventBus::CloneListeners(
    void
) noexcept(true)
{
    xpf::SharedPointer<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator> clone;

    //
    // On insufficient resources, return empty list.
    //
    clone = xpf::MakeShared<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator>();
    if (clone.IsEmpty())
    {
        return xpf::SharedPointer<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator>();
    }

    //
    // If the current list is empty, we're done - return the empty clone.
    //
    if (this->m_Listeners.IsEmpty())
    {
        return clone;
    }
    xpf::EventBus::ListenersList& currentListeners = (*this->m_Listeners);

    for (size_t i = 0; i < currentListeners.Size(); ++i)
    {
        xpf::SharedPointer<xpf::EventListenerData, xpf::CriticalMemoryAllocator> currentListenerSharedPtr = currentListeners[i];

        //
        // Don't enqueue empty listeners.
        //
        if (currentListenerSharedPtr.IsEmpty())
        {
            continue;
        }
        xpf::EventListenerData& currentListener = *currentListenerSharedPtr;

        //
        // Don't enqueue already ran down listeners.
        //
        xpf::RundownGuard listenerGuard{ (currentListener).Rundown };
        if (!listenerGuard.IsRundownAcquired())
        {
            continue;
        }

        //
        // If any allocation is failing, we return an empty list.
        //
        auto newListenerSharedPtr = xpf::MakeShared<xpf::EventListenerData, xpf::CriticalMemoryAllocator>();
        if (newListenerSharedPtr.IsEmpty())
        {
            return xpf::SharedPointer<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator>();
        }
        xpf::EventListenerData& newListener = *newListenerSharedPtr;

        //
        // Now copy the details.
        //
        xpf::ApiCopyMemory(&newListener.Id, &currentListener.Id, sizeof(newListener.Id));
        newListener.NakedPointer = currentListener.NakedPointer;

        //
        // And finally insert to the clone.
        //
        auto status = (*clone).Emplace(newListenerSharedPtr);
        if (!NT_SUCCESS(status))
        {
            return xpf::SharedPointer<xpf::EventBus::ListenersList, xpf::CriticalMemoryAllocator>();
        }
    }

    //
    // Returned the cloned list.
    //
    return clone;
}