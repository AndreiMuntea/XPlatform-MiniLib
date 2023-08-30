/**
 * @file        xpf_tests/tests/core/TestCore.cpp
 *
 * @brief       This contains tests for core definitions.
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
 * @brief       This tests that asserts does inded dies on debug.
 *
 */
TEST(TestCore, AssertDeathOnDebug)
{
    #if defined XPF_CONFIGURATION_DEBUG
        EXPECT_DEATH(XPF_ASSERT(false), "");
    #else
        EXPECT_NO_THROW(XPF_ASSERT(false));
    #endif
}


/**
 * @brief       This tests that verify does inded dies on debug.
 *
 */
TEST(TestCore, VerifyDeathOnDebug)
{
    #if defined XPF_CONFIGURATION_DEBUG
        EXPECT_DEATH(XPF_VERIFY(false), "");
    #else
        EXPECT_NO_THROW(XPF_VERIFY(false));
    #endif
}


/**
 * @brief        This tests that asserts are evaluated only on debug.
 *
 */
TEST(TestCore, AssertEvaluateOnDebug)
{
    int value = 0;

    #if defined XPF_CONFIGURATION_DEBUG
        EXPECT_DEATH(XPF_ASSERT(value != 0), "");
    #else
        EXPECT_NO_THROW(XPF_ASSERT(value != 0));
    #endif
}


/**
 * @brief        This tests that verify expressions are evaluated only on debug.
 *
 */
TEST(TestCore, VerifyEvaluateOnDebug)
{
    int value = 0;

    if (XPF_VERIFY(value == 0))
    {
        EXPECT_TRUE(true);
    }
    else
    {
        EXPECT_TRUE(false);
    }
}


/**
 * @brief       This tests the numeric limits.
 *
 */
TEST(TestCore, NumericLimits)
{
    EXPECT_EQ(std::numeric_limits<uint8_t>::max(),  xpf::NumericLimits<uint8_t>::MaxValue());
    EXPECT_EQ(std::numeric_limits<int8_t>::max(),   xpf::NumericLimits<int8_t>::MaxValue());

    EXPECT_EQ(std::numeric_limits<uint16_t>::max(), xpf::NumericLimits<uint16_t>::MaxValue());
    EXPECT_EQ(std::numeric_limits<int16_t>::max(),  xpf::NumericLimits<int16_t>::MaxValue());

    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), xpf::NumericLimits<uint32_t>::MaxValue());
    EXPECT_EQ(std::numeric_limits<int32_t>::max(),  xpf::NumericLimits<int32_t>::MaxValue());

    EXPECT_EQ(std::numeric_limits<uint64_t>::max(), xpf::NumericLimits<uint64_t>::MaxValue());
    EXPECT_EQ(std::numeric_limits<int64_t>::max(),  xpf::NumericLimits<int64_t>::MaxValue());
}

/**
 * @brief       This tests the string length api.
 *
 */
TEST(TestCore, StringLength)
{
    //
    // Test for char8_t.
    //
    const char* nullChar8TString = nullptr;
    EXPECT_EQ(size_t{ 0 }, xpf::ApiStringLength(nullChar8TString));

    const char zeroChar8TString[] = { '\0' };
    EXPECT_EQ(size_t{ 0 }, xpf::ApiStringLength(zeroChar8TString));

    const char randomChar8TString[] = "1234 abcd";
    EXPECT_EQ(size_t{ 9 }, xpf::ApiStringLength(randomChar8TString));

    //
    // Test for wchar_t.
    //
    constexpr const wchar_t* nullChar16TString = nullptr;
    EXPECT_EQ(size_t{ 0 }, xpf::ApiStringLength(nullChar16TString));

    constexpr const wchar_t zeroChar16TString[] = { '\0' };
    EXPECT_EQ(size_t{ 0 }, xpf::ApiStringLength(zeroChar16TString));

    constexpr const wchar_t randomChar16TString[] = L"1234 abcd";
    EXPECT_EQ(size_t{ 9 }, xpf::ApiStringLength(randomChar16TString));
}
