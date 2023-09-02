/**
 * @file        xpf_tests/tests/Locks/TestBusyLock.cpp
 *
 * @brief       This contains tests for Busy Lock.
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
 * @brief       This is a context used in busy lock tests.
 */
struct MockTestBusyLockContext
{
    /**
     * @brief Current object which we want to test.
     */
    xpf::BusyLock BusyLock;

    /**
     * @brief True if the busy lock is taken, false otherwise.
     */
    bool IsBusyLockTaken = false;

    /**
     * @brief Specifies whether the lock should be acquired exclusive.
     *        If false, it will be acquired shared.
     */
    bool AcquireExclusive = false;
};

/**
 * @brief       This is a mock callback used for testing BusyLock.
 *              Acquires the lock in the specified manner.
 *
 * @param[in] Context - a MockTestThreadContext pointer.
 */
static void XPF_API
MockThreadBusyLockCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockTestBusyLockContext*>(Context);

    if (nullptr != mockContext)
    {
        if (mockContext->AcquireExclusive)
        {
            mockContext->BusyLock.LockExclusive();
            mockContext->IsBusyLockTaken = true;
            mockContext->BusyLock.UnLockExclusive();
        }
        else
        {
            mockContext->BusyLock.LockShared();
            mockContext->IsBusyLockTaken = true;
            mockContext->BusyLock.UnLockShared();
        }
    }
}


/**
 * @brief       This tests the default constructor and destructor of busy lock.
 */
XPF_TEST_SCENARIO(TestBusyLock, DefaultConstructorDestructor)
{
    xpf::BusyLock lock;
}

/**
 * @brief       This tests the Acquire and then Release exclusive methods.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireReleaseExclusive)
{
    xpf::BusyLock lock;

    //
    // Classic way.
    //
    lock.LockExclusive();
    lock.UnLockExclusive();

    //
    // LockGuard way.
    //
    {
        xpf::ExclusiveLockGuard guard{ lock };
    }
}

/**
 * @brief       This tests the Acquire and then Release shared methods.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireReleaseShared)
{
    xpf::BusyLock lock;

    //
    // Classic way.
    //
    lock.LockShared();
    lock.LockShared();

    lock.UnLockShared();
    lock.UnLockShared();

    //
    // LockGuard way.
    //
    {
        xpf::SharedLockGuard guard1{ lock };
        xpf::SharedLockGuard guard2{ lock };
        xpf::SharedLockGuard guard3{ lock };
    }
}

/**
 * @brief       This tests the shared acquisition up to 2^15 times.
 *              Then we check that we can't acquire the thread until
 *              We release one acquisition.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireSharedMaxTimes)
{
    MockTestBusyLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    //
    // Acquire max 2^15 times.
    //
    for (size_t i = 0; i < 0x7FFF; ++i)
    {
        context.BusyLock.LockShared();
    }

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = false;
    context.IsBusyLockTaken = false;

    status = thread.Run(&MockThreadBusyLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The lock shouldn't be taken.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        XPF_TEST_EXPECT_TRUE(!context.IsBusyLockTaken);
        xpf::ApiYieldProcesor();
    }

    //
    // Release once.
    //
    context.BusyLock.UnLockShared();

    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsBusyLockTaken)
    {
        xpf::ApiYieldProcesor();
    }

    //
    // And now wait for thread to finish - the spinlock should be released.
    //
    thread.Join();

    //
    // Release all other aquisitions.
    //
    for (size_t i = 0; i < 0x7fff - 1; ++i)
    {
        context.BusyLock.UnLockShared();
    }
}

/**
 * @brief       This tests that if we try to acquire exclusive twice,
 *              the execution will block.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireExclusiveTwice)
{
    MockTestBusyLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    context.BusyLock.LockExclusive();

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = true;
    context.IsBusyLockTaken = false;

    status = thread.Run(&MockThreadBusyLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The busylock shouldn't be taken.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        XPF_TEST_EXPECT_TRUE(!context.IsBusyLockTaken);
        xpf::ApiYieldProcesor();
    }

    //
    // Release once.
    //
    context.BusyLock.UnLockExclusive();

    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsBusyLockTaken)
    {
        xpf::ApiYieldProcesor();
    }

    //
    // And now wait for thread to finish - the spinlock should be released.
    //
    thread.Join();
}

/**
 * @brief       This tests that if we try to acquire shared,
 *              after getting an exclusive lock, the execution will block.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireExclusiveBlocksShared)
{
    MockTestBusyLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    context.BusyLock.LockExclusive();

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = false;
    context.IsBusyLockTaken = false;

    status = thread.Run(&MockThreadBusyLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The busylock shouldn't be taken.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        XPF_TEST_EXPECT_TRUE(!context.IsBusyLockTaken);
        xpf::ApiYieldProcesor();
    }

    //
    // Release once.
    //
    context.BusyLock.UnLockExclusive();

    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsBusyLockTaken)
    {
        xpf::ApiYieldProcesor();
    }

    //
    // And now wait for thread to finish - the spinlock should be released.
    //
    thread.Join();
}

/**
 * @brief       This tests that if we try to acquire exclusive,
 *              while the lock is hold shared, the execution blocks
 *              until all references are released.
 */
XPF_TEST_SCENARIO(TestBusyLock, AcquireExclusiveWaitsUntilSharedIsReleased)
{
    MockTestBusyLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    for (size_t i = 0; i < 20; ++i)
    {
        context.BusyLock.LockShared();
    }

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = true;
    context.IsBusyLockTaken = false;

    status = thread.Run(&MockThreadBusyLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The busylock shouldn't be taken until all references are released.
    //
    for (size_t i = 0; i < 20; ++i)
    {
        for (size_t j = 0; j < 10; ++j)
        {
            XPF_TEST_EXPECT_TRUE(!context.IsBusyLockTaken);
            xpf::ApiYieldProcesor();
        }
        context.BusyLock.UnLockShared();
    }



    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsBusyLockTaken)
    {
        xpf::ApiYieldProcesor();
    }

    //
    // And now wait for thread to finish - the spinlock should be released.
    //
    thread.Join();
}
