/**
 * @file        xpf_tests/tests/Utility/TestProtobufSerializer.cpp
 *
 * @brief       This contains tests for protobuf serialization
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
 * @brief       This tests the serialization of an unsigned integers
 *              which will ocupy multiple bytes.
 */
XPF_TEST_SCENARIO(TestProtobufSerializer, UnsignedValues)
{
    xpf::Buffer dataBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(dataBuffer.Resize(0x1000)));

    xpf::StreamWriter streamWriter(dataBuffer);
    xpf::StreamReader streamReader(dataBuffer);

    xpf::Protobuf protobuf;
    uint8_t serializedBytes[10] = { 0 };
    uint64_t deserializedValue = 0;

    /* 1 byte - 77 == 0b01001101 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(77, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(1, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b01001101);     /* 01001101 will be 01001101 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 77);

    /* 2 bytes - 300 == 0000010 0101100 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(300, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(2, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b10101100);     /* 0101100 will be 10101100 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b00000010);     /* 0000010 will be 00000010 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 300);

    /* 3 bytes - 23656 == 0000001 0111000 1101000 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(23656, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(3, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11101000);     /* 1101000 will be 11101000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b10111000);     /* 0111000 will be 10111000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b00000001);     /* 0000001 will be 00000001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 23656);

    /* 4 bytes - 83886080 == 0101000 0000000 0000000 0000000 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(83886080, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(4, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b10000000);     /* 0000000 will be 10000000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b10000000);     /* 0000000 will be 10000000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b10000000);     /* 0000000 will be 10000000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b00101000);     /* 0101000 will be 00101000 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 83886080);

    /* 5 bytes - 6864118579 ==  0011001 1001001 0001000 1000110 0110011 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(6864118579, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(5, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b10110011);     /* 0110011 will be 10110011 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11000110);     /* 1000110 will be 11000110 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b10001000);     /* 0001000 will be 10001000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11001001);     /* 1001001 will be 11001001 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b00011001);     /* 0011001 will be 00011001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 6864118579);

    /* 6 bytes - 686411857923 == 0010011 1111101 0001010 1010111 0000000 0000011 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(686411857923, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(6, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b10000011);     /* 0000011 will be 10000011 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b10000000);     /* 0000000 will be 10000000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b11010111);     /* 1010111 will be 11010111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b10001010);     /* 0001010 will be 10001010 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b11111101);     /* 1111101 will be 11111101 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[5] == 0b00010011);     /* 0010011 will be 00010011 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 686411857923);

    /* 7 bytes - 78472798192221 == 0010001 1101011 1101101 1111010 0001011 0000100 1011101 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(78472798192221, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(7, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11011101);     /* 1011101 will be 11011101 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b10000100);     /* 0000100 will be 10000100 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b10001011);     /* 0001011 will be 10001011 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11111010);     /* 1111010 will be 11111010 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b11101101);     /* 1101101 will be 11101101 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[5] == 0b11101011);     /* 1101011 will be 11101011 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[6] == 0b00010001);     /* 0010001 will be 00010001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 78472798192221);

    /* 8 bytes - 9999999999999999 == 0010001 1100001 1011110 0100110 1111110 0000011 1111111 1111111 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(9999999999999999, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(8, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b10000011);     /* 0000011 will be 10000011 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11111110);     /* 1111110 will be 11111110 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b10100110);     /* 0100110 will be 10100110 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[5] == 0b11011110);     /* 1011110 will be 11011110 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[6] == 0b11100001);     /* 1100001 will be 11100001 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[7] == 0b00010001);     /* 0010001 will be 00010001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 9999999999999999);

    /* 9 bytes - 2459565876494606882 == 0100010 0010001 0001000 1000100 0100010 0010001 0001000 1000100 0100010 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(2459565876494606882, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(9, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b10100010);     /* 0100010 will be 10100010 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11000100);     /* 1000100 will be 11000100 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b10001000);     /* 0001000 will be 10001000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b10010001);     /* 0010001 will be 10010001 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b10100010);     /* 0100010 will be 10100010 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[5] == 0b11000100);     /* 1000100 will be 11000100 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[6] == 0b10001000);     /* 0001000 will be 10001000 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[7] == 0b10010001);     /* 0010001 will be 10010001 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[8] == 0b00100010);     /* 0100010 will be 00100010 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 2459565876494606882);

    /* 10 bytes - 18446744073709551615 == 0000001 1111111 1111111 1111111 1111111 1111111 1111111 1111111 1111111 1111111 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeUI64(18446744073709551615, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(10, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[5] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[6] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[7] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[8] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[9] == 0b00000001);     /* 0000001 will be 00000001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeUI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 18446744073709551615);
}

/**
 * @brief       This tests the serialization of an signed integers
 *              which will ocupy multiple bytes.
 */
XPF_TEST_SCENARIO(TestProtobufSerializer, SignedValues)
{
    xpf::Buffer dataBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(dataBuffer.Resize(0x1000)));

    xpf::StreamWriter streamWriter(dataBuffer);
    xpf::StreamReader streamReader(dataBuffer);

    xpf::Protobuf protobuf;
    uint8_t serializedBytes[10] = { 0 };
    int64_t deserializedValue = 0;

    /* 0 should be zig-zag encoded as 0000000 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(0, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(1, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b0000000);     /* 0000000 will be 00000000 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 0);

    /* -1 should be zig-zag encoded as 1 so 0000001*/
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(-1, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(1, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b00000001);     /* 0000001 will be 00000001 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == -1);

    /* 1 should be zig-zag encoded as 2 so 0000010*/
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(1, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(1, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b0000010);     /* 0000010 will be 00000010 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 1);

    /* -2 should be zig-zag encoded as 3 so 0000011*/
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(-2, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(1, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b0000011);     /* 0000011 will be 00000011 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == -2);

    /* 2147483647 should be zig-zag encoded as 4294967294 so 0001111 1111111 1111111 1111111 1111110 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(2147483647, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(5, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11111110);     /* 1111110 will be 11111110 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b00001111);     /* 0001111 will be 00001111 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == 2147483647);

    /* -2147483648 should be zig-zag encoded as 4294967295 so 0001111 1111111 1111111 1111111 1111111 */
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeI64(-2147483648, streamWriter));
    XPF_TEST_EXPECT_TRUE(streamReader.ReadBytes(5, &serializedBytes[0], true));
    XPF_TEST_EXPECT_TRUE(serializedBytes[0] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[1] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[2] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[3] == 0b11111111);     /* 1111111 will be 11111111 */
    XPF_TEST_EXPECT_TRUE(serializedBytes[4] == 0b00001111);     /* 0001111 will be 00001111 */

    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeI64(deserializedValue, streamReader));
    XPF_TEST_EXPECT_TRUE(deserializedValue == -2147483648);
}


/**
 * @brief       This tests the serialization of binary blob data.
 */
XPF_TEST_SCENARIO(TestProtobufSerializer, BinaryBlobs)
{
    xpf::Buffer dataBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(dataBuffer.Resize(0x1000)));

    xpf::StreamWriter streamWriter(dataBuffer);
    xpf::StreamReader streamReader(dataBuffer);

    xpf::Protobuf protobuf;
    xpf::Vector<uint8_t> binaryData;

    xpf::StringView<char> emptyData;
    XPF_TEST_EXPECT_TRUE(false == protobuf.SerializeBinaryBlob(emptyData, streamWriter));

    xpf::StringView<char> someData = "Some Random Dummy String! And also some random value: 0x1234!";
    XPF_TEST_EXPECT_TRUE(protobuf.SerializeBinaryBlob(someData, streamWriter));
    XPF_TEST_EXPECT_TRUE(protobuf.DeserializeBinaryBlob(binaryData, streamReader));

    xpf::StringView<char> resultedData(reinterpret_cast<char*>(&binaryData[0]),
                                       binaryData.Size());
    XPF_TEST_EXPECT_TRUE(someData.Equals(resultedData, true));
}
