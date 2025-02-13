/**
 * @file        xpf_lib/public/Utility/EventFramework.hpp
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


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/PlatformApi.hpp"

#include "xpf_lib/public/Multithreading/RundownProtection.hpp"
#include "xpf_lib/public/Multithreading/ThreadPool.hpp"

#include "xpf_lib/public/Memory/SharedPointer.hpp"
#include "xpf_lib/public/Containers/Vector.hpp"


namespace xpf
{

/**
 * @brief   The allocations performed by the event framework need to be non paged on windows kernel, as it is
 *          is required to run at dispatch level as well. We define a separated allocator which will point to the
 *          critical memory allocator.
 */
#define XPF_EVENT_FRAMEWORK_ALLOCATOR  xpf::PolymorphicAllocator{                                       \
                                       .AllocFunction = &xpf::CriticalMemoryAllocator::AllocateMemory,  \
                                       .FreeFunction  = &xpf::CriticalMemoryAllocator::FreeMemory }

/**
 * @brief       Define this here separetely as it is easier to
 *              change in future if the need arise.
 *              Uniquely identifies an event.
 */
using EVENT_ID = uint32_t;

/**
 * @brief       Define this here separetely as it is easier to
 *              change in future if the need arise.
 *              Uniquely identifies an event listener.
 */
using EVENT_LISTENER_ID = uuid_t;

/**
 * @brief       Forward definition for the event bus class.
 *              This is required by the OnEvent() handler.
 */
class EventBus;

/**
 * @brief   This is the interface for the Event class.
 *          All other events must inherit this one.
 */
class IEvent
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IEvent, default);

 public:
/**
 * @brief IEvent constructor - default.
 */
IEvent(
    void
) noexcept(true) = default;

/**
 * @brief IEvent destructor - default.
 */
virtual ~IEvent(
    void
) noexcept(true) = default;

/**
 * @brief This method is used to retrieve the event id of an Event.
 *        This uniquely identifies the event.
 * 
 * @return An uniquely identifier for this type of event.
 */
virtual xpf::EVENT_ID
XPF_API
EventId(
    void
) const noexcept(true) = 0;
};  // class IEvent;

/**
 * @brief   This is the interface for the Event Listener class.
 *          All other event listeners must inherit this one.
 */
class IEventListener
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IEventListener, default);

 public:
/**
 * @brief IEventListener constructor - default.
 */
IEventListener(
    void
) noexcept(true) = default;

/**
 * @brief IEventListener destructor - default.
 */
virtual ~IEventListener(
    void
) noexcept(true) = default;

/**
 * @brief                 This method is used to generically handle an event.
 *                        It will be automatically invoked for each listener
 *                        registered with the event bus.
 * 
 * @param[in,out] Event - A  reference to the event.
 *                        Its internal data should not be modified by the event handler.
 *                        It is the caller responsibility to downcast this to the proper event class.
 * 
 * @param[in,out] Bus   - The event bus where this particular event has been thrown to.
 *                        It has strong guarantees that the bus will be valid until the OnEvent() is called.
 *                        Can be safely used to throw new events from the OnEvent() method itself.
 *
 * @return - void.
 */
virtual void
XPF_API
OnEvent(
    _Inout_ xpf::SharedPointer<IEvent>& Event,
    _Inout_ xpf::EventBus* Bus
) noexcept(true) = 0;
};  // class IEventListener;

/**
 * @brief       This is used internally in the event bus.
 *              Stores details about each registered IEventListener.
 */
struct EventListenerData
{
   /**
    * @brief       This will be acquired every time the OnEvent() callback is called.
    *              It will prevent the listener from being unregistered while there
    *              are outstanding callbacks to it.
    */
    xpf::RundownProtection Rundown;
   /**
    * @brief       This uniquely identifies an event listener inside an event bus.
    *              When the listener is registered this will be returned to the caller.
    *              Can be further used to unregister the listener.
    */
    xpf::EVENT_LISTENER_ID Id = { 0 };
   /**
    * @brief       This is an actual pointer to the IEventListener object.
    *              It will be invalidated once the listener has been ran down.
    */
    xpf::IEventListener* NakedPointer = nullptr;
};  // struct EventListenerData;

/**
 * @brief       This is the event bus class.
 *              You can register listeners, unregister listeners
 *              and throw events on it.
 */
class EventBus final
{
 public:
/**
 * @brief This is useful to ease the access to the Listeners.
 */
using ListenersList = xpf::Vector<xpf::SharedPointer<xpf::EventListenerData>>;
 public:
/**
 * @brief EventBus constructor - default.
 */
EventBus(void) noexcept(true) = default;

/**
 * @brief Copy and move are deleted
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(EventBus, delete);

/**
 * @brief EventBus destructor - default.
 */
~EventBus(
    void
) noexcept(true)
{
    this->Rundown();
}

/**
 * @brief This method is used to register a new listener within the event framework.
 *
 * @param[in] Listener - To be registered within the event framework.
 *                       It will start receiving events as soon as it is registered.
 *
 * @param[out] ListenerId - Will uniquely identify the Listener within the event framework.
 * 
 * @return A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
RegisterListener(
    _In_ xpf::IEventListener* Listener,
    _Out_ xpf::EVENT_LISTENER_ID* ListenerId
) noexcept(true);

/**
 * @brief This method is used to unregister an existing listener within the event framework.
 *
 * @param[in] ListenerId - An unique identifier returned by the RegisterListener above.
 * 
 * @return A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
UnregisterListener(
    _In_ _Const_ const xpf::EVENT_LISTENER_ID& ListenerId
) noexcept(true);

/**
 * @brief Waits for all outstanding events to finish.
 *        Prevents further enqueues of events.
 *
 * @return void
 */
void
XPF_API
Rundown(
    void
) noexcept(true);

/**
 * @brief This method is used to dispatch an event to all listeners registered
 *        in the Event bus.
 *
 * @param[in,out]   Event - The event to be dispatched.
 * 
 * @return A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Dispatch(
    _Inout_ xpf::SharedPointer<IEvent>& Event
) noexcept(true);

 private:
/**
 * @brief This method is used to notify all listeners that a particular
 *        event has been thrown on the bus.
 *
 * @param[in] Event - The event to be dispatched.
 */
void
XPF_API
NotifyListeners(
    _Inout_ xpf::SharedPointer<IEvent>& Event
) noexcept(true);

/**
 * @brief This method is used to clone the current listeners.
 *        This is helpful for Register / Unregister so we won't hold the busy lock during dispatch.
 * 
 *
 * @return a Clone of the currently registered listeners.
 *
 * @note That the already run down listeners will not be cloned.
 *       The method must be called with the listeners lock taken.
 */
xpf::SharedPointer<ListenersList>
XPF_API
CloneListeners(
    void
) noexcept(true);


 private:
    /**
     * @brief   This is used to block further operations within the event bus.
     */
     xpf::RundownProtection m_EventBusRundown;

    /**
     * @brief       This list stores the details about all registered listeners.
     *              This is a shared pointer<vector> as we want the event framework
     *              to have a minimal-scope lock.
     *              During register and unregister we'll clone the vector and replace it,
     *              so the in-progress listeners will consume events with the old list,
     *              and won't be affected.
     */
     xpf::SharedPointer<ListenersList> m_Listeners{ XPF_EVENT_FRAMEWORK_ALLOCATOR };
     /**
      * @brief  This will guard the access to the m_Listeners. 
      */
     xpf::BusyLock m_ListenersLock;

    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class EventBus
};  // namespace xpf
