/**
 * @file        xpf_tests/tests/Multithreading/TestThreadPool.cpp
 *
 * @brief       This contains tests for threadpool.
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
 * @brief       This is a context used in threadpool tests.
 */
struct MockTestThreadPoolContext
{
    /**
     * @brief   This will be incremented by each thread.
     */
    alignas(uint64_t) volatile uint64_t Increment = 0;

    /**
     * @brief   This is the number of iterations that each thread
     *          will do to increment the Increment.
     */
    const size_t Iterations = 10000;

    /**
     * @brief   This is the threadpool used for testing.
     */
    xpf::Optional<xpf::ThreadPool> Threadpool;
};

/**
 * @brief       This is a mock callback used for testing threadpool.
 *              It does 10k additions.
 *
 * @param[in] Context - a MockTestThreadPoolContext pointer.
 */
static void XPF_API
MockThreadPoolIncrementCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockTestThreadPoolContext*>(Context);
    EXPECT_TRUE(mockContext != nullptr);

    if (nullptr != mockContext)
    {
        EXPECT_TRUE(mockContext->Threadpool.HasValue());

        for (size_t i = 0; i < mockContext->Iterations; ++i)
        {
            xpf::ApiAtomicIncrement(&mockContext->Increment);
        }
    }
}

/**
 * @brief       This is a mock callback used for testing threadpool.
 *              It enqueues 1000 callbacks for threadpool.
 *
 * @param[in] Context - a MockTestThreadPoolContext pointer.
 */
static void XPF_API
MockThreadPoolEnqueueCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockTestThreadPoolContext*>(Context);
    EXPECT_TRUE(mockContext != nullptr);

    if (nullptr != mockContext)
    {
        EXPECT_TRUE(mockContext->Threadpool.HasValue());
        for (size_t i = 0; i < 1000; ++i)
        {
            NTSTATUS status = (*mockContext->Threadpool).Enqueue(MockThreadPoolIncrementCallback,
                                                                 MockThreadPoolIncrementCallback,
                                                                 Context);
            EXPECT_TRUE(NT_SUCCESS(status));
            _Analysis_assume_(NT_SUCCESS(status));
        }
    }
}


/**
 * @brief       This tests the creation of the threadpool.
 */
TEST(TestThreadPool, Create)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::ThreadPool> threadpool;

    status = xpf::ThreadPool::Create(&threadpool);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));
}

/**
 * @brief       This tests the enqueue of one work item.
 */
TEST(TestThreadPool, EnqueueRundown)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MockTestThreadPoolContext threadpoolContext;

    status = xpf::ThreadPool::Create(&threadpoolContext.Threadpool);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    //
    // Enqueue one work item.
    //
    status = (*threadpoolContext.Threadpool).Enqueue(MockThreadPoolIncrementCallback,
                                                     MockThreadPoolIncrementCallback,
                                                     &threadpoolContext);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    //
    // Rundown should wait until the item is processsed.
    //
    (*threadpoolContext.Threadpool).Rundown();
    EXPECT_EQ(uint64_t{ threadpoolContext.Iterations }, threadpoolContext.Increment);

    //
    // Further insert is blocked.
    //
    status = (*threadpoolContext.Threadpool).Enqueue(MockThreadPoolIncrementCallback,
                                                     MockThreadPoolIncrementCallback,
                                                     &threadpoolContext);
    EXPECT_FALSE(NT_SUCCESS(status));
    _Analysis_assume_(!NT_SUCCESS(status));
}


/**
 * @brief       This tests the threadpool under stress.
 *              It will enqueue 10 work items that each
 *              will enqueue 1000 work items => 1000 work items.
 *              Each work item will do 10000 increments => 100 000 000 operations.
 */
TEST(TestThreadPool, Stress)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MockTestThreadPoolContext threadpoolContext;

    status = xpf::ThreadPool::Create(&threadpoolContext.Threadpool);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    //
    // Enqueue 10 work items.
    //
    for (size_t i = 0; i < 10; ++i)
    {
        status = (*threadpoolContext.Threadpool).Enqueue(MockThreadPoolEnqueueCallback,
                                                         MockThreadPoolEnqueueCallback,
                                                         &threadpoolContext);
        EXPECT_TRUE(NT_SUCCESS(status));
        _Analysis_assume_(NT_SUCCESS(status));
    }

    //
    // Now spin until this work is done.
    //
    while (threadpoolContext.Increment != 100000000)
    {
        xpf::ApiYieldProcesor();
    }
}
