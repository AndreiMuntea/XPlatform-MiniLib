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
    auto mockContext = reinterpret_cast<MockTestThreadContext*>(Context);
    EXPECT_TRUE(mockContext != nullptr);

    if (nullptr != mockContext)
    {
        xpf::ApiAtomicIncrement(&mockContext->Increment);
    }
}

/**
 * @brief       This tests the default constructor and destructor of thread.
 */
TEST(TestThread, DefaultConstructorDestructor)
{
    xpf::thread::Thread thread;
    EXPECT_TRUE(nullptr == thread.ThreadHandle());
    EXPECT_FALSE(thread.IsJoinable());
}

/**
 * @brief       This tests one simple callback.
 */
TEST(TestThread, OneCallbackRun)
{
    xpf::thread::Thread thread;
    MockTestThreadContext context;

    NTSTATUS status = thread.Run(&MockThreadCallback, &context);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    EXPECT_TRUE(thread.IsJoinable());
    thread.Join();

    EXPECT_FALSE(thread.IsJoinable());
    EXPECT_EQ(uint64_t{ 1 }, context.Increment);
}

/**
 * @brief       This tests that join is automatically calleed when thread is destroyed.
 */
TEST(TestThread, JoinOnDestroy)
{
    MockTestThreadContext context;

    {
        xpf::thread::Thread thread;
        NTSTATUS status = thread.Run(&MockThreadCallback, &context);
        EXPECT_TRUE(NT_SUCCESS(status));
        _Analysis_assume_(NT_SUCCESS(status));
    }

    EXPECT_EQ(uint64_t{ 1 }, context.Increment);
}

/**
 * @brief       This tests that multiple threads can run.
 */
TEST(TestThread, MultipleCallbackRun)
{
    MockTestThreadContext context;
    xpf::thread::Thread threads[10];

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        NTSTATUS status = threads[i].Run(&MockThreadCallback, &context);
        EXPECT_TRUE(NT_SUCCESS(status));
        _Analysis_assume_(NT_SUCCESS(status));
    }

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    EXPECT_EQ(uint64_t{ XPF_ARRAYSIZE(threads) }, context.Increment);
}

/**
 * @brief       This tests running after a callback was run
 */
TEST(TestThread, RunOnSameObject)
{
    xpf::thread::Thread thread;
    MockTestThreadContext context;

    NTSTATUS status = thread.Run(&MockThreadCallback, &context);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    EXPECT_TRUE(thread.IsJoinable());

    //
    // A callback is already running.
    //
    status = thread.Run(&MockThreadCallback, &context);
    EXPECT_FALSE(NT_SUCCESS(status));
    _Analysis_assume_(!NT_SUCCESS(status));

    //
    // Wait to finish.
    //
    thread.Join();
    EXPECT_FALSE(thread.IsJoinable());

    //
    // Now we can enqueue.
    //
    status = thread.Run(&MockThreadCallback, &context);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    thread.Join();
    EXPECT_EQ(uint64_t{ 2 }, context.Increment);
}
