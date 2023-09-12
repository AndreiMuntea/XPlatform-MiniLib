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

    status = (*eventBus).RegisterListener(&listener,
                                          &listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    auto event = xpf::MakeShared<xpf::mocks::MockEvent>(5, eventId);
    XPF_TEST_EXPECT_TRUE(!event.IsEmpty());

    status = (*eventBus).Dispatch(xpf::DynamicSharedPointerCast<xpf::IEvent>(event), xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*eventBus).Rundown();
    XPF_TEST_EXPECT_TRUE(listener.IncrementedValue() == 5);
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

    status = (*eventBus).RegisterListener(&listener,
        &listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    auto event = xpf::MakeShared<xpf::mocks::MockEvent>(5, eventId);
    XPF_TEST_EXPECT_TRUE(!event.IsEmpty());

    status = (*eventBus).Dispatch(xpf::DynamicSharedPointerCast<xpf::IEvent>(event), xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    status = (*eventBus).UnregisterListener(listenerId);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    status = (*eventBus).Dispatch(xpf::DynamicSharedPointerCast<xpf::IEvent>(event), xpf::EventDispatchType::kSync);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*eventBus).Rundown();
    XPF_TEST_EXPECT_TRUE(listener.IncrementedValue() == 5);
}