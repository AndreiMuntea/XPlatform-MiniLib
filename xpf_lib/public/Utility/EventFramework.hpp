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
#include "xpf_lib/public/Containers/TwoLockQueue.hpp"


namespace xpf
{

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
 * @brief      This is an enum  coordinating the dispatch type for an event bus.
 *
 *             kSync means that the event will be dispatched on the original thread.
 *             This should be used with caution as there might be locks taken.
 * 
 *             kAsync means the event will be placed in a queue and processed some time
 *             in the future.
 *
 *             kAuto means the event framework will choose the best (optimal) way to throw
 *             the event.
 */
enum class EventDispatchType
{
    kSync  = 1,
    kAsync = 2,
    kAuto  = 3,
};


/**
 * @brief   This is the interface for the Event class.
 *          All other events must inherit this one.
 */
class IEvent
{
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

//
// Copy and move should be handled by each event class.
// We'll default them in the base interface here.
//
 protected:
/**
 * @brief Copy constructor - default.
 * 
 * @param[in] Other - The other object to construct from.
 */
IEvent(
    _In_ _Const_ const IEvent& Other
) noexcept(true) = default;

/**
 * @brief Move constructor - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IEvent(
    _Inout_ IEvent&& Other
) noexcept(true) = default;

/**
 * @brief Copy assignment - default.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IEvent&
operator=(
    _In_ _Const_ const IEvent& Other
) noexcept(true) = default;

/**
 * @brief Move assignment - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IEvent&
operator=(
    _Inout_ IEvent&& Other
) noexcept(true) = default;
};  // class IEvent;

/**
 * @brief   This is the interface for the Event Listener class.
 *          All other event listeners must inherit this one.
 */
class IEventListener
{
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
 * @param[in] Event     - A const reference to the event.
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
    _In_ _Const_ const xpf::SharedPointer<xpf::IEvent>& Event,
    _Inout_ xpf::EventBus* Bus
) noexcept(true) = 0;


//
// Copy and move should be handled by each event listener class.
// We'll default them in the base interface here.
//
 protected:
/**
 * @brief Copy constructor - default.
 * 
 * @param[in] Other - The other object to construct from.
 */
IEventListener(
    _In_ _Const_ const IEventListener& Other
) noexcept(true) = default;

/**
 * @brief Move constructor - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IEventListener(
    _Inout_ IEventListener&& Other
) noexcept(true) = default;

/**
 * @brief Copy assignment - default.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IEventListener&
operator=(
    _In_ _Const_ const IEventListener& Other
) noexcept(true) = default;

/**
 * @brief Move assignment - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IEventListener&
operator=(
    _Inout_ IEventListener&& Other
) noexcept(true) = default;
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
    xpf::Optional<xpf::RundownProtection> Rundown;
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
   /**
    * @brief       This will be used to enqueue the EventListenerData structure inside a list.
    *              The list is stored internally by the event bus itself.
    */
    xpf::XPF_SINGLE_LIST_ENTRY ListEntry = { 0 };
};  // struct EventListenerData;

/**
 * @brief       This is used internally in the event bus.
 *              Stores details about each Event ready to be dispatched async.
 */
struct EventData
{
   /**
    * @brief       This is the event ready to be dispatched.
    */
    xpf::SharedPointer<xpf::IEvent> Event;
   /**
    * @brief       This is the event bus on which the event is thrown.
    */
    xpf::EventBus* Bus = nullptr;
};  // struct EventData;


/**
 * @brief       This is the event bus class.
 *              You can register listeners, unregister listeners
 *              and throw events on it.
 */
class EventBus final
{
 private:
/**
 * @brief EventBus constructor - default.
 */
EventBus(
    void
) noexcept(true) : m_Allocator(sizeof(xpf::EventListenerData), true)
{
    XPF_NOTHING();
}

 public:
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
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
EventBus(
    _In_ _Const_ const EventBus& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
EventBus(
    _Inout_ EventBus&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
EventBus&
operator=(
    _In_ _Const_ const EventBus& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
EventBus&
operator=(
    _Inout_ EventBus&& Other
) noexcept(true) = delete;

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
 * @param[in] Event - The event to be dispatched.
 *
 * @param[in] DispatchType - Controls how the event should be dispatched.
 *                           See the xpf::EventDispatchType structure above
 *                           for more information.
 * 
 * @return A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Dispatch(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event,
    _In_ xpf::EventDispatchType DispatchType = xpf::EventDispatchType::kAuto
) noexcept(true);

/**
 * @brief Create and initialize an EventBus. This must be used instead of constructor.
 *        It ensures the EventBus is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] EventBusToCreate - the EventBus to be created. On input it will be empty.
 *                                    On output it will contain a fully initialized EventBus
 *                                    or an empty one on fail.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success EventBusToCreate has a value
 *       and on fail EventBusToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::EventBus>* EventBusToCreate
) noexcept(true);

 private:
/**
 * @brief This method is used to notify all listeners that a particular
 *        event has been thrown on the bus. It can be called from threadpool
 *        or directly from Dispatch method itself.
 *
 * @param[in] Event - The event to be dispatched.
 */
void
XPF_API
NotifyListeners(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event
) noexcept(true);

/**
 * @brief This method is used to enqueue an event to be dispatched Async.
 *
 * @param[in] Event - The event to be dispatched.
 * 
 * @return A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
EnqueueAsync(
    _In_ _Const_ const xpf::SharedPointer<IEvent>& Event
) noexcept(true);

/**
 * @brief This method is passed to m_AsyncPool.
 * 
 * @param[in] EventData - a pointer to the EventData that is currently being dispatched.
 */
static void
XPF_API
AsyncCallback(
    _In_opt_ xpf::thread::CallbackArgument EventData
) noexcept(true);

 private:
    /**
     * @brief   This is used to block further operations within the event bus.
     */
     xpf::Optional<xpf::RundownProtection> m_EventBusRundown;
    /**
     * @brief       This is the allocator for EventData. Use the lookaside allocator
     *              as we'll have a lot of allocations. We want to recycle some of it.
     */
     xpf::LookasideListAllocator m_Allocator;
    /**
     * @brief       This is a threadpool which will be responsible for handling async events.
     *              The workers will start processing from m_EventDataList.
     */
     xpf::Optional<xpf::ThreadPool> m_AsyncPool;
    /**
     * @brief       This list stores the details about all registered listeners.
     *              Note that "removing" a listener simply means running down the listener.
     *              It will be stored in this list forever.
     */
     xpf::TwoLockQueue m_Listeners;

    /**
     * @brief       This stores the number of currently enqueued async items.
     *              When this exceeds the threshold, we start stealing threads.
     */
     alignas(uint32_t) volatile  uint32_t m_EnqueuedAsyncItems = 0;
    /**
     * @brief       This is the async threshold. When this is reached, we'll favor stealing threads.
     */
     static constexpr uint32_t ASYNC_THRESHOLD = 1024;


    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class EventBus
};  // namespace xpf
