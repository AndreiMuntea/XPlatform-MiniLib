/**
  * @file        xpf_tests/Mocks/TestMockEvents.hpp
  *
  * @brief       This contains mocks eventsdefinitions for tests.
  *
  * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
  *
  * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
  *              All rights reserved.
  *
  * @license     See top-level directory LICENSE file.
  */


#pragma once


#include "xpf_tests/XPF-TestIncludes.hpp"

namespace xpf
{
namespace mocks
{

/**
 * @brief This is a dummy mock event containing a value.
 */
class MockEvent : public xpf::IEvent
{
 public:
/**
 * @brief Default constructor.
 *
 * @param Value - the value of this event.
 *
 * @param EventId - the ID of this event.
 */
MockEvent(
    _In_ uint32_t Value,
    _In_ _Const_ const xpf::EVENT_ID& EventId
) noexcept(true) : IEvent()
{
    this->m_Value = Value;
    this->m_EventId = EventId;
}

/**
 * @brief Destructor.
 */
~MockEvent(
    void
) = default;

/**
 * @brief Copy constructor - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 */
MockEvent(
    _In_ _Const_ const MockEvent& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
MockEvent(
    _Inout_ MockEvent&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
MockEvent&
operator=(
    _In_ _Const_ const MockEvent& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
MockEvent&
operator=(
    _Inout_ MockEvent&& Other
) noexcept(true) = delete;

/**
 * @brief Getter for underlying value.
 *
 * @return The underlying stored value.
 */
inline uint32_t
Value(
    void
) const noexcept(true)
{
    return this->m_Value;
}

/**
 * @brief This method is used to retrieve the event id of an Event.
 *        This uniquely identifies the event.
 * 
 * @return An uniquely identifier for this type of event.
 */
inline xpf::EVENT_ID
XPF_API
EventId(
    void
) const noexcept(true) override
{
    return this->m_EventId;
}

 private:
    uint32_t m_Value = 0;
    xpf::EVENT_ID m_EventId = { 0 };
};  // class MockEvent

/**
 * @brief This is a dummy mock event listener.
 *        It registers to a single event, specified in the constructor.
 *        Counts the number of skipped events and also increments the value for MockEvents.
 */
class MockEventListener : public xpf::IEventListener
{
 public:
/**
 * @brief Default constructor.
 *
 * @param EventId - the value of the interesting event.
 */
MockEventListener(
    _In_ xpf::EVENT_ID EventId
) noexcept(true) : IEventListener()
{
    this->m_EventId = EventId;
}

/**
 * @brief Destructor.
 */
~MockEventListener(
    void
) = default;

/**
 * @brief Copy constructor - default
 * 
 * @param[in] Other - The other object to construct from.
 */
MockEventListener(
    _In_ _Const_ const MockEventListener& Other
) noexcept(true) = default;

/**
 * @brief Move constructor - default
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
MockEventListener(
    _Inout_ MockEventListener&& Other
) noexcept(true) = default;

/**
 * @brief Copy assignment - default
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
MockEventListener&
operator=(
    _In_ _Const_ const MockEventListener& Other
) noexcept(true) = default;

/**
 * @brief Move assignment - default
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
MockEventListener&
operator=(
    _Inout_ MockEventListener&& Other
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
 * @return void.
 */
inline void
XPF_API
OnEvent(
    _In_ _Const_ const xpf::SharedPointer<xpf::IEvent>& Event,
    _Inout_ xpf::EventBus* Bus
) noexcept(true) override
{
    XPF_DEATH_ON_FAILURE(nullptr != Bus);

    if ((*Event).EventId() == this->m_EventId)
    {
        auto mockEvent = xpf::DynamicSharedPointerCast<xpf::mocks::MockEvent>(Event);
        for (uint32_t i = 0; i < (*mockEvent).Value(); ++i)
        {
            xpf::ApiAtomicIncrement(&this->m_IncrementedValue);
        }
    }
    else
    {
        xpf::ApiAtomicIncrement(&this->m_SkippedEvents);
    }
}

/**
 * @brief Getter for skipped events.
 *
 * @return The number of skipped events.
 */
inline uint32_t
SkippedEvents(
    void
) const noexcept(true)
{
    return this->m_SkippedEvents;
}

/**
 * @brief Getter for incremented value.
 *
 * @return The incremented value.
 */
inline uint32_t
IncrementedValue(
    void
) const noexcept(true)
{
    return this->m_IncrementedValue;
}

 private:
    xpf::EVENT_ID m_EventId = { 0 };

    alignas(uint32_t) volatile uint32_t m_SkippedEvents = 0;
    alignas(uint32_t) volatile uint32_t m_IncrementedValue = 0;
};  // class MockEventListener
};  // namespace mocks
};  // namespace xpf
