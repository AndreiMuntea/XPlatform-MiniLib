/**
 * @file        xpf_tests/tests/Memory/TestLookasideListAllocator.cpp
 *
 * @brief       This contains tests for lookaside list allocator.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"

/**
 * @brief       This is a mock context for the lookaside stress callback.
 */
struct MockLookasideStressContext
{
    /**
     * @brief The allocator to be stressed.
     */
    xpf::LookasideListAllocator* Allocator = nullptr;

    /**
     * @brief The block size to allocate.
     */
    size_t BlockSize = 0;
};

/**
 * @brief       This is a mock callback used for stress testing the lookaside allocator.
 *
 * @param[in] Context - A pointer to a MockLookasideStressContext.
 */
static void XPF_API
MockLookasideStressCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    MockLookasideStressContext* mockContext = static_cast<MockLookasideStressContext*>(Context);
    if (nullptr != mockContext && nullptr != mockContext->Allocator)
    {
        for (size_t i = 0; i < 1000; ++i)
        {
            void* block = mockContext->Allocator->AllocateMemory(mockContext->BlockSize);
            if (nullptr != block)
            {
                mockContext->Allocator->FreeMemory(block);
            }
        }
    }
}

/**
 * @brief       This tests the default constructor and destructor of LookasideListAllocator.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, DefaultConstructorDestructor)
{
    //
    // Construct with ElementSize=128, IsCritical=false.
    // Let it destruct. No crash expected.
    //
    xpf::LookasideListAllocator allocator(128, false);
}

/**
 * @brief       This tests basic allocate and free operations.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, AllocateAndFree)
{
    xpf::LookasideListAllocator allocator(256, false);

    //
    // Allocate exact match size.
    //
    void* block1 = allocator.AllocateMemory(256);
    XPF_TEST_EXPECT_TRUE(nullptr != block1);

    //
    // Allocate smaller than element size.
    //
    void* block2 = allocator.AllocateMemory(128);
    XPF_TEST_EXPECT_TRUE(nullptr != block2);

    //
    // Free both blocks.
    //
    allocator.FreeMemory(block1);
    allocator.FreeMemory(block2);
}

/**
 * @brief       This tests that allocating a block larger than ElementSize returns nullptr.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, AllocateTooLargeBlock)
{
    xpf::LookasideListAllocator allocator(64, false);

    //
    // Request size > ElementSize should fail.
    //
    void* tooLarge = allocator.AllocateMemory(65);
    XPF_TEST_EXPECT_TRUE(nullptr == tooLarge);

    //
    // Exact size should succeed.
    //
    void* exactSize = allocator.AllocateMemory(64);
    XPF_TEST_EXPECT_TRUE(nullptr != exactSize);

    allocator.FreeMemory(exactSize);
}

/**
 * @brief       This tests that freed blocks are zero-filled on reuse from cache.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, FreeAndReuseFromCache)
{
    xpf::LookasideListAllocator allocator(128, false);

    //
    // Allocate and write a pattern.
    //
    void* block = allocator.AllocateMemory(128);
    XPF_TEST_EXPECT_TRUE(nullptr != block);
    for (size_t i = 0; i < 128; ++i)
    {
        static_cast<uint8_t*>(block)[i] = 0xAB;
    }

    //
    // Free the block (should go into cache).
    //
    allocator.FreeMemory(block);
    block = nullptr;

    //
    // Allocate again -- the block should be zero-filled.
    //
    void* reusedBlock = allocator.AllocateMemory(128);
    XPF_TEST_EXPECT_TRUE(nullptr != reusedBlock);

    const uint8_t* bytes = static_cast<const uint8_t*>(reusedBlock);
    bool allZero = true;
    for (size_t i = 0; i < 128; ++i)
    {
        if (bytes[i] != 0)
        {
            allZero = false;
            break;
        }
    }
    XPF_TEST_EXPECT_TRUE(allZero);

    allocator.FreeMemory(reusedBlock);
}

/**
 * @brief       This tests that FreeMemory(nullptr) is a no-op and does not crash.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, FreeNullBlock)
{
    xpf::LookasideListAllocator allocator(128, false);
    allocator.FreeMemory(nullptr);
}

/**
 * @brief       This tests the allocator with IsCritical=true.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, CriticalAllocator)
{
    xpf::LookasideListAllocator allocator(128, true);

    //
    // Allocate and write a pattern.
    //
    void* block = allocator.AllocateMemory(128);
    XPF_TEST_EXPECT_TRUE(nullptr != block);
    for (size_t i = 0; i < 128; ++i)
    {
        static_cast<uint8_t*>(block)[i] = 0xCD;
    }

    //
    // Free and reallocate -- should be zero-filled.
    //
    allocator.FreeMemory(block);
    block = nullptr;

    void* reusedBlock = allocator.AllocateMemory(128);
    XPF_TEST_EXPECT_TRUE(nullptr != reusedBlock);

    const uint8_t* bytes = static_cast<const uint8_t*>(reusedBlock);
    bool allZero = true;
    for (size_t i = 0; i < 128; ++i)
    {
        if (bytes[i] != 0)
        {
            allZero = false;
            break;
        }
    }
    XPF_TEST_EXPECT_TRUE(allZero);

    allocator.FreeMemory(reusedBlock);
}

/**
 * @brief       This tests the allocator under multithreaded stress.
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, StressAllocFree)
{
    xpf::LookasideListAllocator allocator(128, false);
    MockLookasideStressContext context;
    context.Allocator = &allocator;
    context.BlockSize = 128;

    xpf::thread::Thread threads[8];

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        XPF_TEST_EXPECT_TRUE(NT_SUCCESS(threads[i].Run(MockLookasideStressCallback, &context)));
    }

    for (size_t i = 0; i < XPF_ARRAYSIZE(threads); ++i)
    {
        threads[i].Join();
    }
}

/**
 * @brief       This tests that a small ElementSize is clamped to sizeof(XPF_SINGLE_LIST_ENTRY).
 */
XPF_TEST_SCENARIO(TestLookasideListAllocator, SmallElementSizeClamped)
{
    //
    // ElementSize=1 should be internally promoted to sizeof(XPF_SINGLE_LIST_ENTRY).
    //
    xpf::LookasideListAllocator allocator(1, false);

    //
    // Allocate and free should succeed with the clamped size.
    //
    void* block = allocator.AllocateMemory(sizeof(xpf::XPF_SINGLE_LIST_ENTRY));
    XPF_TEST_EXPECT_TRUE(nullptr != block);

    allocator.FreeMemory(block);
}
