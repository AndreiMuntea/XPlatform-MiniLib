/**
 * @file        xpf_tests/tests/Containers/TestStream.cpp
 *
 * @brief       This contains tests for read write stream.
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
 * @brief       This tests the read write stream by writing numbers.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, Numbers)
{
    xpf::Buffer dataBuffer;

    NTSTATUS status = dataBuffer.Resize(sizeof(uint64_t));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(0x1122334455667788));

    xpf::StreamReader reader(dataBuffer);
    uint64_t value = 0;

    // Peek shouldn't update the cursor. We can read the same value.
    // Over and over again.
    value = 0;
    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(value, true));
    XPF_TEST_EXPECT_TRUE(0x1122334455667788 == value);

    value = 0;
    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(value, true));
    XPF_TEST_EXPECT_TRUE(0x1122334455667788 == value);

    // Now we won't peek, so we won't be able to do that.
    value = 0;
    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(value));
    XPF_TEST_EXPECT_TRUE(0x1122334455667788 == value);

    XPF_TEST_EXPECT_TRUE(!reader.ReadNumber(value, true));
}


/**
 * @brief       This tests the read write stream by writing strings.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, Strings)
{
    static char gDummy[] = "Some Dummy Value";

    xpf::Buffer dataBuffer;

    NTSTATUS status = dataBuffer.Resize(sizeof(gDummy));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);

    // Write byte by byte.
    for (size_t i = 0; i < sizeof(gDummy) - sizeof('\0'); ++i)
    {
        void* gDummyByteAddress = xpf::AlgoAddToPointer(xpf::AddressOf(gDummy), i);
        XPF_TEST_EXPECT_TRUE(writer.WriteBytes(1, static_cast<const uint8_t*>(gDummyByteAddress)));
    }

    xpf::StreamReader reader(dataBuffer);

    // Read byte by byte.
    for (size_t i = 0; i < sizeof(gDummy) - sizeof('\0'); ++i)
    {
        char readByte = '\0';
        void* readByteAddress = xpf::AddressOf(readByte);

        XPF_TEST_EXPECT_TRUE(reader.ReadBytes(1, static_cast<uint8_t*>(readByteAddress)));
        XPF_TEST_EXPECT_TRUE(readByte == gDummy[i]);
    }
}
