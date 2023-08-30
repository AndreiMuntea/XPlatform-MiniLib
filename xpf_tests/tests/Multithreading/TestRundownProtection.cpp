/**
 * @file        xpf_tests/tests/Multithreading/TestRundownProtection.cpp
 *
 * @brief       This contains tests for rundown protection.
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
 * @brief       This is a context used in rundown protection tests.
 */
struct MockTestRundownProtectionContext
{
    /**
     * @brief   This is the rundown that we'll try to acquire.
     */
    xpf::Optional<xpf::RundownProtection> Rundown;

    /**
     * @brief   This is the signal which is used to signal that
     *          the thread is awake.
     */
    xpf::Optional<xpf::Signal> IsThreadAwake;

    /**
     * @brief   This is a boolean saying whether the rundown protection
     *          was successfully ran down -- all outstanding references
     *          has been released, or not.
     */
    bool IsRunDownReleased = false;
};

/**
 * @brief       This is a mock callback used for testing rundown protection.
 *              This is the callback where a rundown protection thread
 *              will call wait for release, blocking all further access.
 * 
 * @param[in] Context - a MockTestRundownProtectionContext pointer.
 */
static void XPF_API
MockRundownProtectionCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockTestRundownProtectionContext*>(Context);
    EXPECT_TRUE(mockContext != nullptr);

    if (nullptr != mockContext)
    {
        EXPECT_TRUE(mockContext->Rundown.HasValue());
        EXPECT_TRUE(mockContext->IsThreadAwake.HasValue());

        //
        // We're up. Signal this.
        //
        (*mockContext->IsThreadAwake).Set();

        //
        // This call will block until all outstanding references are removed.
        //
        (*mockContext->Rundown).WaitForRelease();

        //
        // At this point we successfully ran down the object.
        //
        mockContext->IsRunDownReleased = true;
    }
}

/**
 * @brief       This tests the creation of the rundown protection.
 */
TEST(TestRundownProtection, Create)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::RundownProtection> rundown;

    status = xpf::RundownProtection::Create(&rundown);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    EXPECT_TRUE(rundown.HasValue());
}

/**
 * @brief       This tests the acquisition of rundown.
 *              The rundown can be acquired recursively.
 *              This is not a lock!
 */
TEST(TestRundownProtection, AcquireRecursive)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::RundownProtection> rundown;

    status = xpf::RundownProtection::Create(&rundown);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_TRUE((*rundown).Acquire());
    }

    for (size_t i = 0; i < 100; ++i)
    {
        (*rundown).Release();
    }
}

/**
 * @brief       This tests the acquisition of rundown.
 *              The rundown can be acquired recursively via rundownguard.
 *              This is not a lock!
 */
TEST(TestRundownProtection, AcquireRecursiveRundownGuard)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::RundownProtection> rundown;

    status = xpf::RundownProtection::Create(&rundown);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    xpf::RundownGuard guard1{ (*rundown) };
    EXPECT_TRUE(guard1.IsRundownAcquired());

    xpf::RundownGuard guard2{ (*rundown) };
    EXPECT_TRUE(guard2.IsRundownAcquired());
}

/**
 * @brief       This tests the acquisition of rundown can happen
 *              until the WaitForRelease() is called.
 */
TEST(TestRundownProtection, WaitForReleaseBlocksAcquisitions)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MockTestRundownProtectionContext rundownContext;
    xpf::thread::Thread rundownThread;

    status = xpf::RundownProtection::Create(&rundownContext.Rundown);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    status = xpf::Signal::Create(&rundownContext.IsThreadAwake, true);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    //
    // Acquire the rundown 100 times.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_TRUE((*rundownContext.Rundown).Acquire());
    }

    //
    // Now start a thread to block the rundown.
    //
    status = rundownThread.Run(MockRundownProtectionCallback,
                               &rundownContext);
    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    //
    // Wait for thread to come up.
    //
    bool isThreadUp = (*rundownContext.IsThreadAwake).Wait();
    EXPECT_TRUE(isThreadUp);

    //
    // Further acquistions should be blocked.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_FALSE((*rundownContext.Rundown).Acquire());
    }

    //
    // Release the previously 100 acquired rundowns.
    // And wait for thread to terminate.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_FALSE(rundownContext.IsRunDownReleased);
        (*rundownContext.Rundown).Release();
    }
    rundownThread.Join();

    //
    // The rundown should be released.
    //
    EXPECT_TRUE(rundownContext.IsRunDownReleased);
}

/**
 * @brief       This tests the WaitForRelease() without having any acquisitions.
 */
TEST(TestRundownProtection, WaitForRelease)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::RundownProtection> rundown;

    status = xpf::RundownProtection::Create(&rundown);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    (*rundown).WaitForRelease();

    //
    // No acquisition should happen.
    //
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_FALSE((*rundown).Acquire());
    }
}

/**
 * @brief       This tests the Release will panic if there was
 *              no previous acquisition.
 */
TEST(TestRundownProtection, ReleaseNoAcquire)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::RundownProtection> rundown;

    status = xpf::RundownProtection::Create(&rundown);

    EXPECT_TRUE(NT_SUCCESS(status));
    _Analysis_assume_(NT_SUCCESS(status));

    EXPECT_DEATH((*rundown).Release(), ".*");
}
