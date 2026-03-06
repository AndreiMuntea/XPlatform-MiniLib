/**
 * @file        xpf_tests/tests/Containers/TestStream.cpp
 *
 * @brief       This contains tests for read write stream.
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

/**
 * @brief       This tests that WriteBytes with 0 bytes returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, WriteZeroBytes)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(16);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);
    uint8_t dummy = 0xAB;

    XPF_TEST_EXPECT_TRUE(!writer.WriteBytes(0, &dummy));
    XPF_TEST_EXPECT_TRUE(0 == writer.StreamSize());
}

/**
 * @brief       This tests that ReadBytes with 0 bytes returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, ReadZeroBytes)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(16);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamReader reader(dataBuffer);
    uint8_t dummy = 0;

    XPF_TEST_EXPECT_TRUE(!reader.ReadBytes(0, &dummy));
}

/**
 * @brief       This tests that WriteBytes with nullptr returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, WriteNullBytes)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(16);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);

    XPF_TEST_EXPECT_TRUE(!writer.WriteBytes(4, nullptr));
    XPF_TEST_EXPECT_TRUE(0 == writer.StreamSize());
}

/**
 * @brief       This tests that ReadBytes with nullptr returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, ReadNullBytes)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(16);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamReader reader(dataBuffer);

    XPF_TEST_EXPECT_TRUE(!reader.ReadBytes(4, nullptr));
}

/**
 * @brief       This tests that writing past the buffer end returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, WritePastBufferEnd)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(4);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);

    //
    // Fill the buffer completely.
    //
    uint32_t value = 0x12345678;
    XPF_TEST_EXPECT_TRUE(writer.WriteBytes(4, reinterpret_cast<const uint8_t*>(&value)));

    //
    // Writing 1 more byte should fail.
    //
    uint8_t extra = 0xFF;
    XPF_TEST_EXPECT_TRUE(!writer.WriteBytes(1, &extra));
}

/**
 * @brief       This tests that reading past the buffer end returns false.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, ReadPastBufferEnd)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(4);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Write 4 bytes so the buffer has valid content.
    //
    xpf::StreamWriter writer(dataBuffer);
    uint32_t value = 0x12345678;
    XPF_TEST_EXPECT_TRUE(writer.WriteBytes(4, reinterpret_cast<const uint8_t*>(&value)));

    //
    // Read all 4 bytes.
    //
    xpf::StreamReader reader(dataBuffer);
    uint32_t readValue = 0;
    XPF_TEST_EXPECT_TRUE(reader.ReadBytes(4, reinterpret_cast<uint8_t*>(&readValue)));

    //
    // Reading 1 more byte should fail.
    //
    uint8_t extra = 0;
    XPF_TEST_EXPECT_TRUE(!reader.ReadBytes(1, &extra));
}

/**
 * @brief       This tests that StreamSize tracks cumulative writes.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, StreamSizeTracking)
{
    xpf::Buffer dataBuffer;
    NTSTATUS status = dataBuffer.Resize(16);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);

    //
    // Write 4 bytes, verify size.
    //
    uint32_t val32 = 0x11223344;
    XPF_TEST_EXPECT_TRUE(writer.WriteBytes(4, reinterpret_cast<const uint8_t*>(&val32)));
    XPF_TEST_EXPECT_TRUE(4 == writer.StreamSize());

    //
    // Write 4 more bytes, verify size.
    //
    XPF_TEST_EXPECT_TRUE(writer.WriteBytes(4, reinterpret_cast<const uint8_t*>(&val32)));
    XPF_TEST_EXPECT_TRUE(8 == writer.StreamSize());
}

/**
 * @brief       This tests writing and reading multiple number types sequentially.
 */
XPF_TEST_SCENARIO(TestReadWriteStream, MultipleNumberTypes)
{
    xpf::Buffer dataBuffer;
    size_t totalSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint64_t);
    NTSTATUS status = dataBuffer.Resize(totalSize);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StreamWriter writer(dataBuffer);
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(uint8_t{ 0x11 }));
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(uint16_t{ 0x2233 }));
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(uint32_t{ 0x44556677 }));
    XPF_TEST_EXPECT_TRUE(writer.WriteNumber(uint64_t{ 0x8899AABBCCDDEEFF }));

    xpf::StreamReader reader(dataBuffer);
    uint8_t v8 = 0;
    uint16_t v16 = 0;
    uint32_t v32 = 0;
    uint64_t v64 = 0;

    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(v8));
    XPF_TEST_EXPECT_TRUE(uint8_t{ 0x11 } == v8);

    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(v16));
    XPF_TEST_EXPECT_TRUE(uint16_t{ 0x2233 } == v16);

    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(v32));
    XPF_TEST_EXPECT_TRUE(uint32_t{ 0x44556677 } == v32);

    XPF_TEST_EXPECT_TRUE(reader.ReadNumber(v64));
    XPF_TEST_EXPECT_TRUE(uint64_t{ 0x8899AABBCCDDEEFF } == v64);
}
