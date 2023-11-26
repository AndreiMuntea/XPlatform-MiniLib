/**
 * @file        xpf_tests/tests/core/TestEndianess.cpp
 *
 * @brief       This contains tests for endianess APIs.
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
 * @brief        This tests the conversions between the same format.
 *
 */
XPF_TEST_SCENARIO(TestEndianess, SameEndianess)
{
    const uint64_t value = 0x1122334455667788;
    uint64_t convertedValue = 0;

    convertedValue = xpf::EndianessConvertBetweenRepresentations(value,
                                                                 xpf::Endianess::Little,
                                                                 xpf::Endianess::Little);
    XPF_TEST_EXPECT_TRUE(value == convertedValue);

    convertedValue = xpf::EndianessConvertBetweenRepresentations(value,
                                                                 xpf::Endianess::Big,
                                                                 xpf::Endianess::Big);
    XPF_TEST_EXPECT_TRUE(value == convertedValue);
}

/**
 * @brief        This tests the unsupported conversions.
 *
 */
XPF_TEST_SCENARIO(TestEndianess, InvalidConversions)
{
    const uint64_t value = 0x1122334455667788;

    XPF_TEST_EXPECT_DEATH(xpf::EndianessConvertBetweenRepresentations(value,
                                                                      xpf::Endianess::Unknown,
                                                                      xpf::Endianess::Little));
    XPF_TEST_EXPECT_DEATH(xpf::EndianessConvertBetweenRepresentations(value,
                                                                      xpf::Endianess::MAX,
                                                                      xpf::Endianess::Little));
    XPF_TEST_EXPECT_DEATH(xpf::EndianessConvertBetweenRepresentations(value,
                                                                      xpf::Endianess::Big,
                                                                      xpf::Endianess::Unknown));
    XPF_TEST_EXPECT_DEATH(xpf::EndianessConvertBetweenRepresentations(value,
                                                                      xpf::Endianess::Big,
                                                                      xpf::Endianess::MAX));
}

/**
 * @brief        This tests the conversions from local machine.
 *
 */
XPF_TEST_SCENARIO(TestEndianess, ToAndFromHost)
{
    const uint64_t value = 0x1122334455667788;
    uint64_t convertedValue = 0;

    convertedValue = xpf::EndianessHostToBig(value);
    void* convertedValueAddress = xpf::AddressOf(convertedValue);

    uint8_t firstByteBig = *(static_cast<uint8_t*>(convertedValueAddress));
    XPF_TEST_EXPECT_TRUE(firstByteBig == 0x11);
    XPF_TEST_EXPECT_TRUE(value == xpf::EndianessBigToHost(convertedValue));

    convertedValue = xpf::EndianessHostToLittle(value);

    uint8_t firstByteLittle = *(static_cast<uint8_t*>(convertedValueAddress));
    XPF_TEST_EXPECT_TRUE(firstByteLittle == 0x88);
    XPF_TEST_EXPECT_TRUE(value == xpf::EndianessLittleToHost(convertedValue));
}
