/**
 * @file        xpf_tests/tests/core/TestAlgorithm.cpp
 *
 * @brief       This contains tests for Algorithm APIs.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"

/**
 * @brief       This tests IsNumberPowerOf2 for various values.
 */
XPF_TEST_SCENARIO(TestAlgorithm, IsNumberPowerOf2)
{
    //
    // 0 and 1 are considered valid powers of 2.
    //
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 1 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 2 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 8 }));

    //
    // Non-powers of 2.
    //
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint32_t{ 3 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint32_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint32_t{ 6 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint32_t{ 7 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint32_t{ 100 }));

    //
    // Large powers of 2.
    //
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 1u << 16 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint32_t{ 1u << 30 }));

    //
    // Also test uint64_t.
    //
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint64_t{ 1 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberPowerOf2(uint64_t{ uint64_t{1} << 40 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberPowerOf2(uint64_t{ 100 }));
}

/**
 * @brief       This tests IsNumberAligned for various values.
 */
XPF_TEST_SCENARIO(TestAlgorithm, IsNumberAligned)
{
    //
    // Alignment 0 should return false.
    //
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 0 }, uint32_t{ 0 }));

    //
    // Non-power-of-2 alignment should return false.
    //
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 6 }, uint32_t{ 3 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 10 }, uint32_t{ 5 }));

    //
    // 0 is aligned to any valid alignment.
    //
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 0 }, uint32_t{ 1 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 0 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 0 }, uint32_t{ 16 }));

    //
    // Aligned values.
    //
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 8 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 16 }, uint32_t{ 8 }));
    XPF_TEST_EXPECT_TRUE(xpf::AlgoIsNumberAligned(uint32_t{ 256 }, uint32_t{ 16 }));

    //
    // Misaligned values.
    //
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 5 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 7 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(!xpf::AlgoIsNumberAligned(uint32_t{ 9 }, uint32_t{ 8 }));
}

/**
 * @brief       This tests AlignValueUp for various values.
 */
XPF_TEST_SCENARIO(TestAlgorithm, AlignValueUp)
{
    //
    // Invalid alignment returns original value.
    //
    XPF_TEST_EXPECT_TRUE(uint32_t{ 5 } == xpf::AlgoAlignValueUp(uint32_t{ 5 }, uint32_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 5 } == xpf::AlgoAlignValueUp(uint32_t{ 5 }, uint32_t{ 3 }));

    //
    // Already aligned value stays the same.
    //
    XPF_TEST_EXPECT_TRUE(uint32_t{ 8 } == xpf::AlgoAlignValueUp(uint32_t{ 8 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 16 } == xpf::AlgoAlignValueUp(uint32_t{ 16 }, uint32_t{ 16 }));

    //
    // Unaligned rounds up.
    //
    XPF_TEST_EXPECT_TRUE(uint32_t{ 8 } == xpf::AlgoAlignValueUp(uint32_t{ 5 }, uint32_t{ 4 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 8 } == xpf::AlgoAlignValueUp(uint32_t{ 7 }, uint32_t{ 8 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 16 } == xpf::AlgoAlignValueUp(uint32_t{ 9 }, uint32_t{ 8 }));

    //
    // Overflow guard returns original value.
    // UINT32_MAX - 1 aligned to 4 would overflow, so original is returned.
    //
    uint32_t overflowValue = static_cast<uint32_t>(0xFFFFFFFE);
    uint32_t result = xpf::AlgoAlignValueUp(overflowValue, uint32_t{ 4 });
    XPF_TEST_EXPECT_TRUE(overflowValue == result);
}

/**
 * @brief       This tests PointerToValue and ValueToPointer round-trip.
 */
XPF_TEST_SCENARIO(TestAlgorithm, PointerToValueAndBack)
{
    //
    // nullptr should convert to 0.
    //
    int* nullPtr = nullptr;
    auto nullValue = xpf::AlgoPointerToValue(nullPtr);
    XPF_TEST_EXPECT_TRUE(0 == nullValue);

    //
    // Round-trip: pointer -> value -> pointer.
    //
    int dummyVariable = 42;
    int* originalPtr = &dummyVariable;

    auto value = xpf::AlgoPointerToValue(originalPtr);
    void* recoveredPtr = xpf::AlgoValueToPointer(static_cast<uint64_t>(value));
    XPF_TEST_EXPECT_TRUE(recoveredPtr == originalPtr);
}

/**
 * @brief       This tests AddToPointer functionality.
 */
XPF_TEST_SCENARIO(TestAlgorithm, AddToPointer)
{
    uint8_t array[16] = { 0 };

    //
    // Offset into array verified.
    //
    void* base = &array[0];
    void* result = xpf::AlgoAddToPointer(base, 4);
    XPF_TEST_EXPECT_TRUE(result == &array[4]);

    //
    // Offset 0 returns same pointer.
    //
    result = xpf::AlgoAddToPointer(base, 0);
    XPF_TEST_EXPECT_TRUE(result == base);

    //
    // Const overload.
    //
    const void* constBase = &array[0];
    const void* constResult = xpf::AlgoAddToPointer(constBase, 8);
    XPF_TEST_EXPECT_TRUE(constResult == &array[8]);

    //
    // null + 0.
    //
    result = xpf::AlgoAddToPointer(static_cast<void*>(nullptr), 0);
    XPF_TEST_EXPECT_TRUE(result == nullptr);
}

/**
 * @brief       This tests AlignValueUp edge cases.
 */
XPF_TEST_SCENARIO(TestAlgorithm, AlignValueUpEdgeCases)
{
    //
    // Alignment of 1 -- every value is already aligned.
    //
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == xpf::AlgoAlignValueUp(uint32_t{ 0 }, uint32_t{ 1 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 1 } == xpf::AlgoAlignValueUp(uint32_t{ 1 }, uint32_t{ 1 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 99 } == xpf::AlgoAlignValueUp(uint32_t{ 99 }, uint32_t{ 1 }));

    //
    // Large alignment (1 << 20) with small value.
    //
    uint32_t largeAlignment = uint32_t{ 1u << 20 };
    XPF_TEST_EXPECT_TRUE(largeAlignment == xpf::AlgoAlignValueUp(uint32_t{ 1 }, largeAlignment));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == xpf::AlgoAlignValueUp(uint32_t{ 0 }, largeAlignment));
}
