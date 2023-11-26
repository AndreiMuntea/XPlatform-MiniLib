/**
 * @file        xpf_tests/tests/Locks/TestReadWriteLock.cpp
 *
 * @brief       This contains tests for ReadWriteLock.
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
struct MockTestReadWriteLockContext
{
    /**
     * @brief Current object which we want to test.
     */
    xpf::Optional<xpf::ReadWriteLock> Lock;

    /**
     * @brief True if the lock is taken, false otherwise.
     */
    bool IsLockTaken = false;

    /**
     * @brief Specifies whether the lock should be acquired exclusive.
     *        If false, it will be acquired shared.
     */
    bool AcquireExclusive = false;
};

/**
 * @brief       This is a mock callback used for testing ReadWriteLock.
 *              Acquires the lock in the specified manner.
 *
 * @param[in] Context - a MockTestReadWriteLockContext pointer.
 */
static void XPF_API
MockThreadReadWriteLockCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = static_cast<MockTestReadWriteLockContext*>(Context);

    if (nullptr != mockContext)
    {
        if (mockContext->AcquireExclusive)
        {
            (*mockContext->Lock).LockExclusive();
            mockContext->IsLockTaken = true;
            (*mockContext->Lock).UnLockExclusive();
        }
        else
        {
            (*mockContext->Lock).LockShared();
            mockContext->IsLockTaken = true;
            (*mockContext->Lock).UnLockShared();
        }
    }
}

/**
 * @brief       This tests the creation of a read write lock
 */
XPF_TEST_SCENARIO(TestReadwriteLock, Create)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::ReadWriteLock> rwLock;

    status = xpf::ReadWriteLock::Create(&rwLock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(rwLock.HasValue());
}

/**
 * @brief       This tests the Acquire and then Release exclusive methods.
 */
XPF_TEST_SCENARIO(TestReadwriteLock, AcquireReleaseExclusive)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::ReadWriteLock> rwLock;

    status = xpf::ReadWriteLock::Create(&rwLock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Classic way.
    //
    (*rwLock).LockExclusive();
    (*rwLock).UnLockExclusive();

    //
    // LockGuard way.
    //
    {
        xpf::ExclusiveLockGuard guard{ (*rwLock) };
    }
}

/**
 * @brief       This tests the Acquire and then Release shared methods.
 */
XPF_TEST_SCENARIO(TestReadwriteLock, AcquireReleaseShared)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::ReadWriteLock> rwLock;

    status = xpf::ReadWriteLock::Create(&rwLock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Classic way.
    //
    (*rwLock).LockShared();
    (*rwLock).LockShared();

    (*rwLock).UnLockShared();
    (*rwLock).UnLockShared();

    //
    // LockGuard way.
    //
    {
        xpf::SharedLockGuard guard1{ (*rwLock) };
        xpf::SharedLockGuard guard2{ (*rwLock) };
        xpf::SharedLockGuard guard3{ (*rwLock) };
    }
}

/**
 * @brief       This tests that if we try to acquire exclusive twice,
 *              the execution will block.
 */
XPF_TEST_SCENARIO(TestReadwriteLock, AcquireExclusiveTwice)
{
    MockTestReadWriteLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    status = xpf::ReadWriteLock::Create(&context.Lock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*context.Lock).LockExclusive();

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = true;
    context.IsLockTaken = false;

    status = thread.Run(&MockThreadReadWriteLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The lock shouldn't be taken.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        XPF_TEST_EXPECT_TRUE(!context.IsLockTaken);
        xpf::ApiYieldProcesor();
    }

    //
    // Release once.
    //
    (*context.Lock).UnLockExclusive();

    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsLockTaken)
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
XPF_TEST_SCENARIO(TestReadwriteLock, AcquireExclusiveBlocksShared)
{
    MockTestReadWriteLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    status = xpf::ReadWriteLock::Create(&context.Lock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    (*context.Lock).LockExclusive();

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = false;
    context.IsLockTaken = false;

    status = thread.Run(&MockThreadReadWriteLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The lock shouldn't be taken.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        XPF_TEST_EXPECT_TRUE(!context.IsLockTaken);
        xpf::ApiYieldProcesor();
    }

    //
    // Release once.
    //
    (*context.Lock).UnLockExclusive();

    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsLockTaken)
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
XPF_TEST_SCENARIO(TestReadwriteLock, AcquireExclusiveWaitsUntilSharedIsReleased)
{
    MockTestReadWriteLockContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::thread::Thread thread;

    status = xpf::ReadWriteLock::Create(&context.Lock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    for (size_t i = 0; i < 20; ++i)
    {
        (*context.Lock).LockShared();
    }

    //
    // Create a separated thread to acquire again - which should block until we release once
    //
    context.AcquireExclusive = true;
    context.IsLockTaken = false;

    status = thread.Run(&MockThreadReadWriteLockCallback, &context);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // The lock shouldn't be taken until all references are released.
    //
    for (size_t i = 0; i < 20; ++i)
    {
        for (size_t j = 0; j < 10; ++j)
        {
            XPF_TEST_EXPECT_TRUE(!context.IsLockTaken);
            xpf::ApiYieldProcesor();
        }
        (*context.Lock).UnLockShared();
    }



    //
    // Wait until spinlock is taken then wait for the thread to finish.
    //
    while (!context.IsLockTaken)
    {
        xpf::ApiYieldProcesor();
    }

    //
    // And now wait for thread to finish - the spinlock should be released.
    //
    thread.Join();
}
