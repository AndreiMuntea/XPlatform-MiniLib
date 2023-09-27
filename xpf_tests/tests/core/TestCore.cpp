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
XPF_TEST_SCENARIO(TestCore, AssertDeathOnDebug)
{
    #if defined XPF_CONFIGURATION_DEBUG
        XPF_TEST_EXPECT_DEATH(XPF_ASSERT(false));
    #else
        XPF_TEST_EXPECT_NO_DEATH(XPF_ASSERT(false));
    #endif
}


/**
 * @brief       This tests that verify does inded dies on debug.
 *
 */
XPF_TEST_SCENARIO(TestCore, VerifyDeathOnDebug)
{
    #if defined XPF_CONFIGURATION_DEBUG
        XPF_TEST_EXPECT_DEATH(XPF_VERIFY(false));
    #else
        XPF_TEST_EXPECT_NO_DEATH(XPF_VERIFY(false));
    #endif
}


/**
 * @brief        This tests that asserts are evaluated only on debug.
 *
 */
XPF_TEST_SCENARIO(TestCore, AssertEvaluateOnDebug)
{
    int value = 0;

    #if defined XPF_CONFIGURATION_DEBUG
        XPF_TEST_EXPECT_DEATH(XPF_ASSERT(value != 0));
    #else
        XPF_UNREFERENCED_PARAMETER(value);
        XPF_TEST_EXPECT_NO_DEATH(XPF_ASSERT(value != 0));
    #endif
}


/**
 * @brief        This tests that verify expressions are evaluated only on debug.
 *
 */
XPF_TEST_SCENARIO(TestCore, VerifyEvaluateOnDebug)
{
    int value = 0;

    if (XPF_VERIFY(value == 0))
    {
        XPF_TEST_EXPECT_TRUE(true);
    }
    else
    {
        XPF_TEST_EXPECT_TRUE(false);
    }
}


/**
 * @brief       This tests the numeric limits.
 *
 */
XPF_TEST_SCENARIO(TestCore, NumericLimits)
{
    XPF_TEST_EXPECT_TRUE(UCHAR_MAX ==  xpf::NumericLimits<uint8_t>::MaxValue());
    XPF_TEST_EXPECT_TRUE(CHAR_MAX  ==  xpf::NumericLimits<int8_t>::MaxValue());

    XPF_TEST_EXPECT_TRUE(UINT16_MAX == xpf::NumericLimits<uint16_t>::MaxValue());
    XPF_TEST_EXPECT_TRUE(INT16_MAX  == xpf::NumericLimits<int16_t>::MaxValue());

    XPF_TEST_EXPECT_TRUE(UINT32_MAX == xpf::NumericLimits<uint32_t>::MaxValue());
    XPF_TEST_EXPECT_TRUE(INT32_MAX  == xpf::NumericLimits<int32_t>::MaxValue());

    XPF_TEST_EXPECT_TRUE(UINT64_MAX == xpf::NumericLimits<uint64_t>::MaxValue());
    XPF_TEST_EXPECT_TRUE(INT64_MAX  == xpf::NumericLimits<int64_t>::MaxValue());
}

/**
 * @brief       This tests the string length api.
 *
 */
XPF_TEST_SCENARIO(TestCore, StringLength)
{
    //
    // Test for char8_t.
    //
    const char* nullChar8TString = nullptr;
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == xpf::ApiStringLength(nullChar8TString));

    const char zeroChar8TString[] = { '\0' };
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == xpf::ApiStringLength(zeroChar8TString));

    const char randomChar8TString[] = "1234 abcd";
    XPF_TEST_EXPECT_TRUE(size_t{ 9 } == xpf::ApiStringLength(randomChar8TString));

    //
    // Test for wchar_t.
    //
    constexpr const wchar_t* nullChar16TString = nullptr;
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == xpf::ApiStringLength(nullChar16TString));

    constexpr const wchar_t zeroChar16TString[] = { '\0' };
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == xpf::ApiStringLength(zeroChar16TString));

    constexpr const wchar_t randomChar16TString[] = L"1234 abcd";
    XPF_TEST_EXPECT_TRUE(size_t{ 9 } == xpf::ApiStringLength(randomChar16TString));
}
