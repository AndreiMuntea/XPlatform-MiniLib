/**
 * @file        xpf_tests/tests/Multithreading/TestSignal.cpp
 *
 * @brief       This contains tests for signal.
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
 * @brief       This is a context used in signal tests.
 */
struct MockTestSignalContext
{
    /**
     * @brief   This will be incremented by each thread.
     */
    alignas(uint64_t) uint64_t Increment = 0;

    /**
     * @brief   This is the signal on which each thread will wait.
     */
    xpf::Optional<xpf::Signal> Signal;
};

/**
 * @brief       This is a mock callback used for testing signal.
 *              It increments the Increment with one unit.
 * 
 * @param[in] Context - a MockTestSignalContext pointer.
 */
static void XPF_API
MockSignalCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockTestSignalContext*>(Context);

    if (nullptr != mockContext)
    {
        (*mockContext->Signal).Wait();
        xpf::ApiAtomicIncrement(&mockContext->Increment);
    }
}

 /**
  * @brief       This tests the creation of the event.
  */
XPF_TEST_SCENARIO(TestSignal, Create)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Optional<xpf::Signal> signal;

    status = xpf::Signal::Create(&signal, false);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(signal.HasValue());
    XPF_TEST_EXPECT_TRUE(nullptr != (*signal).SignalHandle());
}

/**
 * @brief       This tests that multiple threads will be signaled by a manual reset event.
 */
XPF_TEST_SCENARIO(TestSignal, ManualReset)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MockTestSignalContext context;

    status = xpf::Signal::Create(&context.Signal, true);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::thread::Thread threads[10];

    //
    // The event is not signaled. All threads will block.
    //
    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        status = threads[i].Run(MockSignalCallback, &context);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    //
    // Signal the event - this is a manual reset so all threads will get it.
    //
    (*context.Signal).Set();

    //
    // All threads should terminate now.
    //
    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    //
    // This should be true.
    //
    XPF_TEST_EXPECT_TRUE(XPF_ARRAYSIZE(threads) == context.Increment);
}

/**
 * @brief       This tests that only one thread will be signaled by an auto reset event.
 */
XPF_TEST_SCENARIO(TestSignal, AutoReset)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MockTestSignalContext context;

    status = xpf::Signal::Create(&context.Signal, false);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::thread::Thread threads[10];

    (*context.Signal).Reset();

    //
    // The event is not signaled. All threads will block.
    //
    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        status = threads[i].Run(MockSignalCallback, &context);
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    }

    for (size_t i = 1; i <= XPF_ARRAYSIZE(threads); ++i)
    {
        //
        // Signal the event - this is an auto reset so only one thread will get it.
        //
        (*context.Signal).Set();

        //
        // Wait for a single thread.
        //
        while (context.Increment != i)
        {
            xpf::ApiYieldProcesor();
        }

        //
        // Now spin a little more to ensure no other threads got notified.
        //
        for (size_t spin = 0; spin < 100; ++spin)
        {
            XPF_TEST_EXPECT_TRUE(uint64_t{ i } == context.Increment);
            xpf::ApiYieldProcesor();
        }
    }

    //
    // All threads should terminate now.
    //
    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }

    //
    // This should be true.
    //
    XPF_TEST_EXPECT_TRUE(XPF_ARRAYSIZE(threads) == context.Increment);
}
