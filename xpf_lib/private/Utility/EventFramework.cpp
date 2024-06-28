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
    _Inout_ IEvent* Event
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

    this->NotifyListeners(Event);
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::EventBus::NotifyListeners(
    _Inout_ IEvent* Event
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
    xpf::SharedPointer<ListenersList> listenersSnapshot;
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
        xpf::SharedPointer<xpf::EventListenerData> currentListener = currentListenersList[i];
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

/**
 * @brief   Everything here can be paged.
 */
XPF_SECTION_PAGED;

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
    // First we acquire the listeners lock - this will prevent other operations while we are working.
    // We run down the listeners with the shared lock guard taken as we'll not modify the listeners list lock.
    //
    {
        xpf::SharedLockGuard listenersGuard{ this->m_ListenersLock };
        if (!this->m_Listeners.IsEmpty())
        {
            xpf::EventBus::ListenersList& listenersList = (*this->m_Listeners);
            for (size_t i = 0; i < listenersList.Size(); ++i)
            {
                xpf::SharedPointer<xpf::EventListenerData> currentListener = listenersList[i];
                if (!currentListener.IsEmpty())
                {
                    (*currentListener).Rundown.WaitForRelease();
                }
            }
        }
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
    auto listenerDataSharedPtr = xpf::MakeSharedWithAllocator<xpf::EventListenerData>(this->m_Listeners.GetAllocator());
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
    xpf::SharedPointer<ListenersList> newListenersList = this->CloneListeners();
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
            xpf::SharedPointer<xpf::EventListenerData> currentListener = currentListenersList[i];
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
        xpf::SharedPointer<ListenersList> newListenersList = this->CloneListeners();
        if (!newListenersList.IsEmpty())
        {
            this->m_Listeners = newListenersList;
        }
    }

    return STATUS_SUCCESS;
}

xpf::SharedPointer<xpf::EventBus::ListenersList>
XPF_API
xpf::EventBus::CloneListeners(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::SharedPointer<xpf::EventBus::ListenersList> clone;

    //
    // On insufficient resources, return empty list.
    //
    clone = xpf::MakeSharedWithAllocator<xpf::EventBus::ListenersList>(this->m_Listeners.GetAllocator());
    if (clone.IsEmpty())
    {
        return clone;
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
        xpf::SharedPointer<xpf::EventListenerData> currentListenerSharedPtr = currentListeners[i];

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
        auto newListenerSharedPtr = xpf::MakeSharedWithAllocator<xpf::EventListenerData>(this->m_Listeners.GetAllocator());
        if (newListenerSharedPtr.IsEmpty())
        {
            clone.Reset();
            return clone;
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
            clone.Reset();
            return clone;
        }
    }

    //
    // Returned the cloned list.
    //
    return clone;
}
