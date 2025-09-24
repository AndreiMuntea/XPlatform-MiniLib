/**
 * @file        xpf_tests/tests/Containers/TestTwoLockQueue.cpp
 *
 * @brief       This contains tests for two lock queue.
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

/**
 * @brief       This is a mock struct for two lock queue.
 */
struct MockTestTlqElement
{
    /**
     * @brief An int_8 element holding a dummy value.
     */
    int8_t DummyValue = -1;

    /**
     * @brief The required SINGLE_LIST_ENTRY which will be used
     *        when inserting the element in list.    
     */
    xpf::XPF_SINGLE_LIST_ENTRY ListEntry = { 0 };
};

/**
 * @brief       This is a mock callback used for testing Atomic list.
 *              Does a lot of link and unlink operations.
 * 
 * @param[in] Context - A pointer to an atomic list.
 */
static void XPF_API
MockTlqStressCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = static_cast<xpf::TwoLockQueue*>(Context);
    if (nullptr != mockContext)
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            void* memory = xpf::MemoryAllocator::AllocateMemory(sizeof(MockTestTlqElement));
            _Analysis_assume_(nullptr != memory);
            XPF_DEATH_ON_FAILURE(nullptr != memory);

            MockTestTlqElement* element = static_cast<MockTestTlqElement*>(memory);
            xpf::MemoryAllocator::Construct(element);

            xpf::TlqPush(*mockContext, &element->ListEntry);

            auto popElement = xpf::TlqPop(*mockContext);
            _Analysis_assume_(nullptr != popElement);
            XPF_DEATH_ON_FAILURE(nullptr != popElement);

            auto crtElement = XPF_CONTAINING_RECORD(popElement, MockTestTlqElement, ListEntry);

            xpf::MemoryAllocator::Destruct(crtElement);
            xpf::MemoryAllocator::FreeMemory(crtElement);
            crtElement = nullptr;
        }
    }
}


/**
 * @brief       This tests the default constructor of two lock queue list.
 */
XPF_TEST_SCENARIO(TestTwoLockQueue, DefaultConstructorDestructor)
{
    xpf::TwoLockQueue tlq;

    XPF_TEST_EXPECT_TRUE(tlq.Head == nullptr);
    XPF_TEST_EXPECT_TRUE(tlq.Tail == nullptr);
}

/**
 * @brief       This tests the Insert method.
 */
XPF_TEST_SCENARIO(TestTwoLockQueue, Insert)
{
    xpf::TwoLockQueue tlq;

    xpf::XPF_SINGLE_LIST_ENTRY* nullEntry = nullptr;
    _Analysis_assume_(nullEntry != nullptr);
    xpf::TlqPush(tlq, nullEntry);

    //
    // [1]
    //
    MockTestTlqElement firstElement;
    firstElement.DummyValue = 1;
    xpf::TlqPush(tlq, &firstElement.ListEntry);

    //
    // [1] --> [2]
    //
    MockTestTlqElement secondElement;
    secondElement.DummyValue = 2;
    xpf::TlqPush(tlq, &secondElement.ListEntry);

    //
    // [1] --> [2] --> [3]
    //
    MockTestTlqElement thirdElement;
    thirdElement.DummyValue = 3;
    xpf::TlqPush(tlq, &thirdElement.ListEntry);

    xpf::XPF_SINGLE_LIST_ENTRY* listHead = xpf::TlqFlush(tlq);

    //
    // Grab the first element.
    //
    MockTestTlqElement* crtElement = nullptr;

    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestTlqElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 1 } == crtElement->DummyValue);

    //
    // Grab the second element.
    //
    listHead = listHead->Next;
    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestTlqElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 2 } == crtElement->DummyValue);

    //
    // Grab the third element.
    //
    listHead = listHead->Next;
    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestTlqElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 3 } == crtElement->DummyValue);
}

/**
 * @brief       This tests the link and unlink in a stress scenario
 */
XPF_TEST_SCENARIO(TestTwoLockQueue, LinkUnlinkStress)
{
    xpf::thread::Thread threads[10];
    xpf::TwoLockQueue tlq;

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(threads[i].Run(MockTlqStressCallback, &tlq)));
    }

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    XPF_TEST_EXPECT_TRUE(tlq.Head == nullptr);
    XPF_TEST_EXPECT_TRUE(tlq.Tail == nullptr);
}
