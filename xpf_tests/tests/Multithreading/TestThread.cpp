/**
 * @file        xpf_tests/tests/Multithreading/TestThread.cpp
 *
 * @brief       This contains tests for thread.
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
 * @brief       This is a context used in thread tests.
 */
struct MockTestThreadContext
{
    /**
     * @brief   This will be incremented by each thread.
     */
    alignas(uint64_t) uint64_t Increment = 0;
};

/**
 * @brief       This is a mock callback used for testing thread.
 *              It increments the Increment with one unit.
 * 
 * @param[in] Context - a MockTestThreadContext pointer.
 */
static void XPF_API
MockThreadCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = static_cast<MockTestThreadContext*>(Context);
    if (nullptr != mockContext)
    {
        xpf::ApiAtomicIncrement(&mockContext->Increment);
    }
}

/**
 * @brief       This tests the default constructor and destructor of thread.
 */
XPF_TEST_SCENARIO(TestThread, DefaultConstructorDestructor)
{
    xpf::thread::Thread thread;
    XPF_TEST_EXPECT_TRUE(nullptr == thread.ThreadHandle());
    XPF_TEST_EXPECT_TRUE(!thread.IsJoinable());
}

/**
 * @brief       This tests one simple callback.
 */
XPF_TEST_SCENARIO(TestThread, OneCallbackRun)
{
    xpf::thread::Thread thread;
    MockTestThreadContext context;

    NTSTATUS status = thread.Run(&MockThreadCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(thread.IsJoinable());
    thread.Join();

    XPF_TEST_EXPECT_TRUE(!thread.IsJoinable());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 1 } == context.Increment);
}

/**
 * @brief       This tests that join is automatically calleed when thread is destroyed.
 */
XPF_TEST_SCENARIO(TestThread, JoinOnDestroy)
{
    MockTestThreadContext context;

    {
        xpf::thread::Thread thread;
        NTSTATUS status = thread.Run(&MockThreadCallback, &context);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    XPF_TEST_EXPECT_TRUE(uint64_t{ 1 } == context.Increment);
}

/**
 * @brief       This tests that multiple threads can run.
 */
XPF_TEST_SCENARIO(TestThread, MultipleCallbackRun)
{
    MockTestThreadContext context;
    xpf::thread::Thread threads[10];

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        NTSTATUS status = threads[i].Run(&MockThreadCallback, &context);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    XPF_TEST_EXPECT_TRUE(uint64_t{ XPF_ARRAYSIZE(threads) } == context.Increment);
}

/**
 * @brief       This tests running after a callback was run
 */
XPF_TEST_SCENARIO(TestThread, RunOnSameObject)
{
    xpf::thread::Thread thread;
    MockTestThreadContext context;

    NTSTATUS status = thread.Run(&MockThreadCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(thread.IsJoinable());

    //
    // A callback is already running - expect false.
    //
    status = thread.Run(&MockThreadCallback, &context);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));

    //
    // Wait to finish.
    //
    thread.Join();
    XPF_TEST_EXPECT_TRUE(!thread.IsJoinable());

    //
    // Now we can enqueue.
    //
    status = thread.Run(&MockThreadCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    thread.Join();
    XPF_TEST_EXPECT_TRUE(uint64_t{ 2 } == context.Increment);
}
