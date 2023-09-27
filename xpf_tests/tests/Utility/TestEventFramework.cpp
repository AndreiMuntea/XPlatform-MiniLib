/**
 * @file        xpf_tests/tests/Utility/TestEventFramework.cpp
 *
 * @brief       This contains tests for event framework.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"
#include "xpf_tests/Mocks/TestMocks.hpp"
#include "xpf_tests/Mocks/TestMockEvents.hpp"

/**
 * @brief   This is a helper method which will create a mock event
 *          and dispatch it to the event bus.
 * 
 * @param[in] EventId - The ID of the event to be created.
 * 
 * @param[in] EventValue - The value of the event.
 * 
 * @param[in] EventBus - the bus to throw the event to.
 * 
 * @param[in] DispatchType - The way to throw the event on bus.
 * 
 * @return a proper NTSTATUS error code.
 */
static NTSTATUS
XpfTesEventDispatchHelper(
    _In_ _Const_ const xpf::EVENT_ID& EventId,
    _In_ uint32_t EventValue,
    _Inout_ xpf::Optional<xpf::EventBus>* EventBus,
    _In_ xpf::EventDispatchType DispatchType
) noexcept(true)
{
    xpf::EventBus& eventBus = (*(*EventBus));

    auto mockEvent = xpf::MakeShared<xpf::mocks::MockEvent>(EventValue, EventId);
    if (mockEvent.IsEmpty())
    {
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    return eventBus.Dispatch(xpf::DynamicSharedPointerCast<xpf::IEvent>(mockEvent),
                             DispatchType);
}


/**
 * @brief       This is a thread pool method which will dispatch a lot of events.
 *              in all possible ways (async, auto and sync).
 *
 * @param[in] Context - a pointer to the event bus where to dispatch the events.
 */
static void XPF_API
XpfTesEventDispatchTpMethod(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto eventBus = reinterpret_cast<xpf::Optional<xpf::EventBus>*>(Context);
    if (nullptr != eventBus)
    {
        for (uint32_t eventId = 0; eventId < 100; ++eventId)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            status = XpfTesEventDispatchHelper(eventId, 5, eventBus, xpf::EventDispatchType::kAsync);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(status));
            status = XpfTesEventDispatchHelper(eventId, 5, eventBus, xpf::EventDispatchType::kAuto);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(status));
            status = XpfTesEventDispatchHelper(eventId, 5, eventBus, xpf::EventDispatchType::kSync);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(status));
        }
    }
}

/**
 * @brief       This tests the creation of the event bus.
 */
XPF_TEST_SCENARIO(TestEventBus, Create)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::EventBus> eventBus;

    status = xpf::EventBus::Create(&eventBus);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(eventBus.HasValue());
}

/**
 * @brief       This tests the registration of an event listener.
 */
XPF_TEST_SCENARIO(TestEventBus, RegisterListener)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::EventBus> eventBus;

    status = xpf::EventBus::Create(&eventBus);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::EVENT_ID eventId = 1;
    xpf::EVENT_LISTENER_ID listenerId = { 0 };
    xpf::mocks::MockEventListener listener{ eventId };

    //
    // Register the listener.
    //
    status = (*eventBus).RegisterListener(&listener,
                                          &listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Create an event to dispatch it.
    //
    status = XpfTesEventDispatchHelper(eventId, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*eventBus).Rundown();
    XPF_TEST_EXPECT_TRUE(listener.IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listener.SkippedEvents() == 0);
}

/**
 * @brief       This tests the de-registration of an event listener.
 */
XPF_TEST_SCENARIO(TestEventBus, UnregisterListener)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::EventBus> eventBus;

    status = xpf::EventBus::Create(&eventBus);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::EVENT_ID eventId = 1;
    xpf::EVENT_LISTENER_ID listenerId = { 0 };
    xpf::mocks::MockEventListener listener{ eventId };

    //
    // Create listener and dispatch an event.
    //
    status = (*eventBus).RegisterListener(&listener,
                                          &listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    status = XpfTesEventDispatchHelper(eventId, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Unregister the listener.
    //
    status = (*eventBus).UnregisterListener(listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // After the listener is unregistered it shouldn't receive any other events.
    // But the dispatch should succeed
    //
    status = XpfTesEventDispatchHelper(eventId, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*eventBus).Rundown();
    eventBus.Reset();

    XPF_TEST_EXPECT_TRUE(listener.IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listener.SkippedEvents() == 0);
}


/**
 * @brief       This tests the registration of multiple listeners.
 */
XPF_TEST_SCENARIO(TestEventBus, RegisterMultipleListener)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::EventBus> eventBus;

    status = xpf::EventBus::Create(&eventBus);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::mocks::MockEventListener listeners[] =
    {
        {xpf::EVENT_ID{1}},
        {xpf::EVENT_ID{1}},
        {xpf::EVENT_ID{2}},
    };

    xpf::EVENT_LISTENER_ID listenersId[] =
    {
        {},
        {},
        {},
    };

    //
    // Register the listeners
    //
    for (size_t i = 0; i < XPF_ARRAYSIZE(listeners); ++i)
    {
        status = (*eventBus).RegisterListener(&listeners[i],
                                              &listenersId[i]);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    //
    // Dispatch event with id 1.
    // First two listeners should get it.
    //
    status = XpfTesEventDispatchHelper(xpf::EVENT_ID{ 1 }, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(listeners[0].IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listeners[0].SkippedEvents() == 0);

    XPF_TEST_EXPECT_TRUE(listeners[1].IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listeners[1].SkippedEvents() == 0);

    XPF_TEST_EXPECT_TRUE(listeners[2].IncrementedValue() == 0);
    XPF_TEST_EXPECT_TRUE(listeners[2].SkippedEvents() == 1);

    //
    // Dispatch event with id 3.
    // No listener should get it.
    //
    status = XpfTesEventDispatchHelper(xpf::EVENT_ID{ 3 }, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(listeners[0].IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listeners[0].SkippedEvents() == 1);

    XPF_TEST_EXPECT_TRUE(listeners[1].IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listeners[1].SkippedEvents() == 1);

    XPF_TEST_EXPECT_TRUE(listeners[2].IncrementedValue() == 0);
    XPF_TEST_EXPECT_TRUE(listeners[2].SkippedEvents() == 2);

    //
    // Unregister second listener.
    //
    status = (*eventBus).UnregisterListener(listenersId[1]);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Dispatch event with id 1.
    // Only first listener should get it
    //
    status = XpfTesEventDispatchHelper(xpf::EVENT_ID{ 1 }, 5, &eventBus, xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(listeners[0].IncrementedValue() == 10);
    XPF_TEST_EXPECT_TRUE(listeners[0].SkippedEvents() == 1);

    XPF_TEST_EXPECT_TRUE(listeners[1].IncrementedValue() == 5);
    XPF_TEST_EXPECT_TRUE(listeners[1].SkippedEvents() == 1);

    XPF_TEST_EXPECT_TRUE(listeners[2].IncrementedValue() == 0);
    XPF_TEST_EXPECT_TRUE(listeners[2].SkippedEvents() == 3);

    (*eventBus).Rundown();
    eventBus.Reset();
}


/**
 * @brief       This is a stress test.
 */
XPF_TEST_SCENARIO(TestEventBus, Stress)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::EventBus> eventBus;

    status = xpf::EventBus::Create(&eventBus);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::Vector<xpf::mocks::MockEventListener> listeners;

    //
    // Create the listeners: 5 listeners per event and 100 different events.
    //
    for (uint32_t eventId = 0; eventId < 100; ++eventId)
    {
        for (uint32_t listenerCount = 0; listenerCount < 5; ++listenerCount)
        {
            status = listeners.Emplace(eventId);
            XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
        }
    }

    //
    // Register the listeners.
    //
    for (size_t i = 0; i < listeners.Size(); ++i)
    {
        xpf::EVENT_LISTENER_ID id;
        xpf::ApiZeroMemory(&id, sizeof(id));

        status = (*eventBus).RegisterListener(&listeners[i],
                                              &id);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    //
    // Now prepare to dispatch on bus. Enqueue the items.
    //
    xpf::Optional<xpf::ThreadPool> pool;
    status = xpf::ThreadPool::Create(&pool);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    for (size_t i = 0; i < 100; ++i)
    {
        status = (*pool).Enqueue(XpfTesEventDispatchTpMethod,
                                 XpfTesEventDispatchTpMethod,
                                 &eventBus);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    //
    // 100 items, each dispatching the same event 3 times with a value of 5
    //              => each listener should get 1500 incremented value.
    //
    // 100 items, each dispatching 100 different events 3 times.
    //              => 29700 skipped events
    //
    for (size_t i = 0; i < listeners.Size(); ++i)
    {
        while (listeners[i].IncrementedValue() != 1500)
        {
            xpf::ApiYieldProcesor();
        }
        while (listeners[i].SkippedEvents() != 29700)
        {
            xpf::ApiYieldProcesor();
        }
    }

    (*pool).Rundown();
    (*eventBus).Rundown();

    pool.Reset();
    eventBus.Reset();
}
