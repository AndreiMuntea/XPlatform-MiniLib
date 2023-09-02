/**
 * @file        xpf_tests/tests/Containers/TestAtomicList.cpp
 *
 * @brief       This contains tests for atomic list
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
 * @brief       This is a mock struct for atomic list.
 */
struct MockTestAtomicListElement
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
MockAtomicListStressCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    MockTestAtomicListElement elements[100];

    auto mockContext = reinterpret_cast<xpf::AtomicList*>(Context);
    if (nullptr != mockContext)
    {
        for (size_t i = 0; i < 10000; ++i)
        {
            for (size_t k = 0; k < XPF_ARRAYSIZE(elements); ++k)
            {
                mockContext->Insert(&elements[k].ListEntry);
            }
            mockContext->Flush(nullptr);

            for (size_t k = 0; k < XPF_ARRAYSIZE(elements); ++k)
            {
                mockContext->Insert(&elements[k].ListEntry);
            }
            mockContext->Flush(nullptr);
        }
    }
}


/**
 * @brief       This tests the default constructor of atomic list.
 */
XPF_TEST_SCENARIO(TestAtomicList, DefaultConstructorDestructor)
{
    xpf::AtomicList atomicList;
    XPF_TEST_EXPECT_TRUE(atomicList.IsEmpty());
}

/**
 * @brief       This tests the Insert method for head.
 */
XPF_TEST_SCENARIO(TestAtomicList, Insert)
{
    xpf::AtomicList atomicList;
    XPF_TEST_EXPECT_TRUE(atomicList.IsEmpty());

    atomicList.Insert(nullptr);

    //
    // [1]
    //
    MockTestAtomicListElement firstElement;
    firstElement.DummyValue = 1;
    atomicList.Insert(&firstElement.ListEntry);

    //
    // [2] --> [1]
    //
    MockTestAtomicListElement secondElement;
    secondElement.DummyValue = 2;
    atomicList.Insert(&secondElement.ListEntry);

    //
    // [3] --> [2] --> [1]
    //
    MockTestAtomicListElement thirdElement;
    thirdElement.DummyValue = 3;
    atomicList.Insert(&thirdElement.ListEntry);

    xpf::XPF_SINGLE_LIST_ENTRY* listHead = nullptr;

    atomicList.Flush(&listHead);
    XPF_TEST_EXPECT_TRUE(atomicList.IsEmpty());

    //
    // Grab the first element.
    //
    MockTestAtomicListElement* crtElement = nullptr;

    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestAtomicListElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 3 } == crtElement->DummyValue);

    //
    // Grab the second element.
    //
    listHead = listHead->Next;
    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestAtomicListElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 2 } == crtElement->DummyValue);

    //
    // Grab the third element.
    //
    listHead = listHead->Next;
    crtElement = XPF_CONTAINING_RECORD(listHead, MockTestAtomicListElement, ListEntry);
    XPF_TEST_EXPECT_TRUE(int8_t{ 1 } == crtElement->DummyValue);
}

/**
 * @brief       This tests the Link and flush in a stress scenario
 */
XPF_TEST_SCENARIO(TestAtomicList, LinkUnlinkStress)
{
    xpf::thread::Thread threads[10];
    xpf::AtomicList atomicList;

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(threads[i].Run(MockAtomicListStressCallback, &atomicList)));
    }

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    //
    // We need the list empty to not assert.
    //
    atomicList.Flush(nullptr);
}
