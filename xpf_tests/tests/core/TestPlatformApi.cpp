/**
  * @file        xpf_tests/tests/core/TestPlatformApi.cpp
  *
  * @brief       This contains tests for platform API set.
  *
  * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
  *
  * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
  *              All rights reserved.
  *
  * @license     See top-level directory LICENSE file.
  */


#include "xpf_tests/XPF-TestIncludes.hpp"


 /**
  * @brief       This tests the alloc and free routines.
  *
  */
XPF_TEST_SCENARIO(TestPlatformApi, AllocAndFree)
{
    //
    // Non-zero size alloc.
    //
    void* ptr = xpf::ApiAllocateMemory(100);
    XPF_TEST_EXPECT_TRUE(nullptr != ptr);

    xpf::ApiFreeMemory(&ptr);
    XPF_TEST_EXPECT_TRUE(nullptr == ptr);

    //
    // Zero size alloc.
    //
    ptr = xpf::ApiAllocateMemory(0);
    XPF_TEST_EXPECT_TRUE(nullptr != ptr);

    xpf::ApiFreeMemory(&ptr);
    XPF_TEST_EXPECT_TRUE(nullptr == ptr);
}


/**
 * @brief       This tests the to lowercase method.
 *
 */
XPF_TEST_SCENARIO(TestPlatformApi, ToLower)
{
    //
    // Test for char.
    //
    XPF_TEST_EXPECT_TRUE('a' == xpf::ApiCharToLower('A'));
    XPF_TEST_EXPECT_TRUE('a' == xpf::ApiCharToLower('a'));
    XPF_TEST_EXPECT_TRUE('9' == xpf::ApiCharToLower('9'));

    //
    // Test for wchar_t.
    //
    XPF_TEST_EXPECT_TRUE(L'a' == xpf::ApiCharToLower(L'A'));
    XPF_TEST_EXPECT_TRUE(L'a' == xpf::ApiCharToLower(L'a'));
    XPF_TEST_EXPECT_TRUE(L'9' == xpf::ApiCharToLower(L'9'));
}

/**
 * @brief       This tests the to uppercase method.
 *
 */
XPF_TEST_SCENARIO(TestPlatformApi, ToUpper)
{
    //
    // Test for char.
    //
    XPF_TEST_EXPECT_TRUE('A' == xpf::ApiCharToUpper('A'));
    XPF_TEST_EXPECT_TRUE('A' == xpf::ApiCharToUpper('a'));
    XPF_TEST_EXPECT_TRUE('9' == xpf::ApiCharToUpper('9'));

    //
    // Test for wchar_t.
    //
    XPF_TEST_EXPECT_TRUE(L'A' == xpf::ApiCharToUpper(L'A'));
    XPF_TEST_EXPECT_TRUE(L'A' == xpf::ApiCharToUpper(L'a'));
    XPF_TEST_EXPECT_TRUE(L'9' == xpf::ApiCharToUpper(L'9'));
}

/**
 * @brief       This tests the atomic increment method
 *
 */
XPF_TEST_SCENARIO(TestPlatformApi, AtomicIncrement)
{
    //
    // Test for uint8_t.
    //
    alignas(uint8_t) uint8_t u8Value = 0;
    XPF_TEST_EXPECT_TRUE(uint8_t{ 1 } == xpf::ApiAtomicIncrement(&u8Value));
    XPF_TEST_EXPECT_TRUE(uint8_t{ 1 } == u8Value);

    u8Value = xpf::NumericLimits<uint8_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(uint8_t{ 0 } == xpf::ApiAtomicIncrement(&u8Value));
    XPF_TEST_EXPECT_TRUE(uint8_t{ 0 } == u8Value);

    //
    // Test for uint16_t.
    //
    alignas(uint16_t) uint16_t u16Value = 0;
    XPF_TEST_EXPECT_TRUE(uint16_t{ 1 } == xpf::ApiAtomicIncrement(&u16Value));
    XPF_TEST_EXPECT_TRUE(uint16_t{ 1 } == u16Value);

    u16Value = xpf::NumericLimits<uint16_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(uint16_t{ 0 } == xpf::ApiAtomicIncrement(&u16Value));
    XPF_TEST_EXPECT_TRUE(uint16_t{ 0 } == u16Value);

    //
    // Test for uint32_t.
    //
    alignas(uint32_t) uint32_t u32Value = 0;
    XPF_TEST_EXPECT_TRUE(uint32_t{ 1 } == xpf::ApiAtomicIncrement(&u32Value));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 1 } == u32Value);

    u32Value = xpf::NumericLimits<uint32_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == xpf::ApiAtomicIncrement(&u32Value));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == u32Value);

    //
    // Test for uint64_t.
    //
    alignas(uint64_t) uint64_t u64Value = 0;
    XPF_TEST_EXPECT_TRUE(uint64_t{ 1 } == xpf::ApiAtomicIncrement(&u64Value));
    XPF_TEST_EXPECT_TRUE(uint64_t{ 1 } == u64Value);

    u64Value = xpf::NumericLimits<uint64_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(uint64_t{ 0 } == xpf::ApiAtomicIncrement(&u64Value));
    XPF_TEST_EXPECT_TRUE(uint64_t{ 0 } == u64Value);

    //
    // Test for int8_t.
    //
    alignas(int8_t) int8_t i8Value = -5;
    XPF_TEST_EXPECT_TRUE(int8_t{ -4 } == xpf::ApiAtomicIncrement(&i8Value));
    XPF_TEST_EXPECT_TRUE(int8_t{ -4 } == i8Value);

    i8Value = xpf::NumericLimits<int8_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int8_t>::MinValue() == xpf::ApiAtomicIncrement(&i8Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int8_t>::MinValue() == i8Value);

    //
    // Test for int16_t.
    //
    alignas(int16_t) int16_t i16Value = -5;
    XPF_TEST_EXPECT_TRUE(int16_t{ -4 } == xpf::ApiAtomicIncrement(&i16Value));
    XPF_TEST_EXPECT_TRUE(int16_t{ -4 } == i16Value);

    i16Value = xpf::NumericLimits<int16_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int16_t>::MinValue() == xpf::ApiAtomicIncrement(&i16Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int16_t>::MinValue() == i16Value);

    //
    // Test for int32_t.
    //
    alignas(int32_t) int32_t i32Value = -5;
    XPF_TEST_EXPECT_TRUE(int32_t{ -4 } == xpf::ApiAtomicIncrement(&i32Value));
    XPF_TEST_EXPECT_TRUE(int32_t{ -4 } == i32Value);

    i32Value = xpf::NumericLimits<int32_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int32_t>::MinValue() == xpf::ApiAtomicIncrement(&i32Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int32_t>::MinValue() == i32Value);

    //
    // Test for int64_t.
    //
    alignas(int64_t) int64_t i64Value = -5;
    XPF_TEST_EXPECT_TRUE(int64_t{ -4 } == xpf::ApiAtomicIncrement(&i64Value));
    XPF_TEST_EXPECT_TRUE(int64_t{ -4 } == i64Value);

    i64Value = xpf::NumericLimits<int64_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int64_t>::MinValue() == xpf::ApiAtomicIncrement(&i64Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int64_t>::MinValue() == i64Value);
}

/**
 * @brief       This tests the atomic decrement method
 *
 */
XPF_TEST_SCENARIO(TestPlatformApi, AtomicDecrement)
{
    //
    // Test for uint8_t.
    //
    alignas(uint8_t) uint8_t u8Value = 0;
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint8_t>::MaxValue() == xpf::ApiAtomicDecrement(&u8Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint8_t>::MaxValue() == u8Value);

    u8Value = xpf::NumericLimits<uint8_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint8_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&u8Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint8_t>::MaxValue() - 1 == u8Value);

    //
    // Test for uint16_t.
    //
    alignas(uint16_t) uint16_t u16Value = 0;
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint16_t>::MaxValue() == xpf::ApiAtomicDecrement(&u16Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint16_t>::MaxValue() == u16Value);

    u16Value = xpf::NumericLimits<uint16_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint16_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&u16Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint16_t>::MaxValue() - 1 == u16Value);

    //
    // Test for uint32_t.
    //
    alignas(uint32_t) uint32_t u32Value = 0;
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint32_t>::MaxValue() == xpf::ApiAtomicDecrement(&u32Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint32_t>::MaxValue() == u32Value);

    u32Value = xpf::NumericLimits<uint32_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint32_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&u32Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint32_t>::MaxValue() - 1 == u32Value);

    //
    // Test for uint64_t.
    //
    alignas(uint64_t) uint64_t u64Value = 0;
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint64_t>::MaxValue() == xpf::ApiAtomicDecrement(&u64Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint64_t>::MaxValue() == u64Value);

    u64Value = xpf::NumericLimits<uint64_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint64_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&u64Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<uint64_t>::MaxValue() - 1 == u64Value);

    //
    // Test for int8_t.
    //
    alignas(int8_t) int8_t i8Value = -5;
    XPF_TEST_EXPECT_TRUE(int8_t{-6} == xpf::ApiAtomicDecrement(&i8Value));
    XPF_TEST_EXPECT_TRUE(int8_t{-6} == i8Value);

    i8Value = xpf::NumericLimits<int8_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int8_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&i8Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int8_t>::MaxValue() - 1 == i8Value);

    //
    // Test for int16_t.
    //
    alignas(int16_t) int16_t i16Value = -5;
    XPF_TEST_EXPECT_TRUE(int16_t{ -6 } == xpf::ApiAtomicDecrement(&i16Value));
    XPF_TEST_EXPECT_TRUE(int16_t{ -6 } == i16Value);

    i16Value = xpf::NumericLimits<int16_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int16_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&i16Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int16_t>::MaxValue() - 1 == i16Value);

    //
    // Test for int32_t.
    //
    alignas(int32_t) int32_t i32Value = -5;
    XPF_TEST_EXPECT_TRUE(int32_t{ -6 } == xpf::ApiAtomicDecrement(&i32Value));
    XPF_TEST_EXPECT_TRUE(int32_t{ -6 } == i32Value);

    i32Value = xpf::NumericLimits<int32_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int32_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&i32Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int32_t>::MaxValue() - 1 == i32Value);

    //
    // Test for int64_t.
    //
    alignas(int64_t) int64_t i64Value = -5;
    XPF_TEST_EXPECT_TRUE(int64_t{ -6 } == xpf::ApiAtomicDecrement(&i64Value));
    XPF_TEST_EXPECT_TRUE(int64_t{ -6 } == i64Value);

    i64Value = xpf::NumericLimits<int64_t>::MaxValue();
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int64_t>::MaxValue() - 1 == xpf::ApiAtomicDecrement(&i64Value));
    XPF_TEST_EXPECT_TRUE(xpf::NumericLimits<int64_t>::MaxValue() - 1 == i64Value);
}

/**
 * @brief       This tests the atomic compare exchange method
 *
 */
XPF_TEST_SCENARIO(TestPlatformApi, AtomicCompareExchange)
{
    //
    // Test for uint8_t.
    //
    alignas(uint8_t) uint8_t u8Value = 0;
    XPF_TEST_EXPECT_TRUE(uint8_t{ 0 } == xpf::ApiAtomicCompareExchange(&u8Value,
                                                                       uint8_t{ 10 },
                                                                       uint8_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(uint8_t{ 0 } == u8Value);

    XPF_TEST_EXPECT_TRUE(uint8_t{ 0 } == xpf::ApiAtomicCompareExchange(&u8Value,
                                                                       uint8_t{ 100 },
                                                                       uint8_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(uint8_t{ 100 } == u8Value);

    //
    // Test for uint16_t.
    //
    alignas(uint16_t) uint16_t u16Value = 0;
    XPF_TEST_EXPECT_TRUE(uint16_t{ 0 } == xpf::ApiAtomicCompareExchange(&u16Value,
                                                                        uint16_t{ 10 },
                                                                        uint16_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(uint16_t{ 0 } == u16Value);

    XPF_TEST_EXPECT_TRUE(uint16_t{ 0 } == xpf::ApiAtomicCompareExchange(&u16Value,
                                                                        uint16_t{ 100 },
                                                                        uint16_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(uint16_t{ 100 } == u16Value);

    //
    // Test for uint32_t.
    //
    alignas(uint32_t) uint32_t u32Value = 0;
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == xpf::ApiAtomicCompareExchange(&u32Value,
                                                                        uint32_t{ 10 },
                                                                        uint32_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == u32Value);

    XPF_TEST_EXPECT_TRUE(uint32_t{ 0 } == xpf::ApiAtomicCompareExchange(&u32Value,
                                                                        uint32_t{ 100 },
                                                                        uint32_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 100 } == u32Value);

    //
    // Test for uint64_t.
    //
    alignas(uint64_t) uint64_t u64Value = 0;
    XPF_TEST_EXPECT_TRUE(uint64_t{ 0 } == xpf::ApiAtomicCompareExchange(&u64Value,
                                                                        uint64_t{ 10 },
                                                                        uint64_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(uint64_t{ 0 } == u64Value);

    XPF_TEST_EXPECT_TRUE(uint64_t{ 0 } == xpf::ApiAtomicCompareExchange(&u64Value,
                                                                        uint64_t{ 100 },
                                                                        uint64_t{ 0 }));
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == u64Value);

    //
    // Test for int8_t.
    //
    alignas(int8_t) int8_t i8Value = -5;
    XPF_TEST_EXPECT_TRUE(int8_t{ -5 } == xpf::ApiAtomicCompareExchange(&i8Value,
                                                                       int8_t{ 10 },
                                                                       int8_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(int8_t{ -5 } == i8Value);

    XPF_TEST_EXPECT_TRUE(int8_t{ -5 } == xpf::ApiAtomicCompareExchange(&i8Value,
                                                                       int8_t{ 100 },
                                                                       int8_t{ -5 }));
    XPF_TEST_EXPECT_TRUE(int8_t{ 100 } == i8Value);

    //
    // Test for int16_t.
    //
    alignas(int16_t) int16_t i16Value = -5;
    XPF_TEST_EXPECT_TRUE(int16_t{ -5 } == xpf::ApiAtomicCompareExchange(&i16Value,
                                                                        int16_t{ 10 },
                                                                        int16_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(int16_t{ -5 } == i16Value);

    XPF_TEST_EXPECT_TRUE(int16_t{ -5 } == xpf::ApiAtomicCompareExchange(&i16Value,
                                                                        int16_t{ 100 },
                                                                        int16_t{ -5 }));
    XPF_TEST_EXPECT_TRUE(int16_t{ 100 } == i16Value);

    //
    // Test for int32_t.
    //
    alignas(int32_t) int32_t i32Value = -5;
    XPF_TEST_EXPECT_TRUE(int32_t{ -5 } == xpf::ApiAtomicCompareExchange(&i32Value,
                                                                        int32_t{ 10 },
                                                                        int32_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(int32_t{ -5 } == i32Value);

    XPF_TEST_EXPECT_TRUE(int32_t{ -5 } == xpf::ApiAtomicCompareExchange(&i32Value,
                                                                        int32_t{ 100 },
                                                                        int32_t{ -5 }));
    XPF_TEST_EXPECT_TRUE(int32_t{ 100 } == i32Value);

    //
    // Test for int64_t.
    //
    alignas(int64_t) int64_t i64Value = -5;
    XPF_TEST_EXPECT_TRUE(int64_t{ -5 } == xpf::ApiAtomicCompareExchange(&i64Value,
                                                                        int64_t{ 10 },
                                                                        int64_t{ 5 }));
    XPF_TEST_EXPECT_TRUE(int64_t{ -5 } == i64Value);

    XPF_TEST_EXPECT_TRUE(int64_t{ -5 } == xpf::ApiAtomicCompareExchange(&i64Value,
                                                                        int64_t{ 100 },
                                                                        int64_t{ -5 }));
    XPF_TEST_EXPECT_TRUE(int64_t{ 100 } == i64Value);
}
