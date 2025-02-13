/**
 * @file        xpf_lib/private/Utility/ProtobufSerializer.cpp
 *
 * @brief       In here you'll find a minimalistic implementation of protocol buffer.
 *              It respects the documentation and it's simplistic enough as will
 *              respect the ISerializer interface. We don't want complex stuff.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/xpf.hpp"

 /**
  * @brief   By default all code in here goes into paged section.
  *          We shouldn't attempt serialization at higher IRQLs
  */
XPF_SECTION_PAGED;

bool
XPF_API
xpf::Protobuf::SerializeI64(
    _In_ _Const_ const int64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Signed int64 values are serialized as unsigned int 64 zig-zag encoded values.
    //
    return this->SerializeUI64(this->ZigZagEncode(Number),
                               Stream);
}

bool
XPF_API
xpf::Protobuf::SerializeUI64(
    _In_ _Const_ const uint64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Here we perform Base 128 Varints encoding. https://protobuf.dev/programming-guides/encoding/
    //
    // Variable-width integers, or varints, are at the core of the wire format.
    // They allow encoding unsigned 64-bit integers using anywhere between one and ten bytes,
    // with small values using fewer bytes.
    //
    // Each byte in the varint has a continuation bit that indicates if the byte that follows it is part of the varint.
    // This is the most significant bit (MSB) of the byte (sometimes also called the sign bit).
    // The lower 7 bits are a payload;
    // the resulting integer is built by appending together the 7-bit payloads of its constituent bytes.
    //

    uint64_t value = Number;
    do
    {
        uint8_t lastByte = value & 0b01111111;
        value = value >> 7;

        if (value != 0)
        {
            lastByte = lastByte | 0b10000000;
        }

        if (!Stream.WriteBytes(1, &lastByte))
        {
            return false;
        }
    } while (value != 0);

    return true;
}

bool
XPF_API
xpf::Protobuf::SerializeBinaryBlob(
    _In_ _Const_ const xpf::StringView<char>& Buffer,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Binary blobs are serialized as [n][byte0][byte1]...[byten]
    // where n is the number of bytes.
    //

    const uint64_t numberOfBytes = Buffer.BufferSize();
    if (numberOfBytes > xpf::NumericLimits<uint32_t>::MaxValue())
    {
        return false;
    }

    if (!this->SerializeUI64(numberOfBytes, Stream))
    {
        return false;
    }

    for (size_t i = 0; i < Buffer.BufferSize(); ++i)
    {
        const uint8_t byte = Buffer[i];
        if (!Stream.WriteBytes(1, &byte))
        {
            return false;
        }
    }

    return true;
}

bool
XPF_API
xpf::Protobuf::DeserializeI64(
    _Out_ int64_t& Number,
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    Number = 0;

    //
    // This is serialized as unsigned value, and then we do zigzag decoding.
    //
    uint64_t zigZagEncodedValue = 0;
    if (!this->DeserializeUI64(zigZagEncodedValue, Stream))
    {
        return false;
    }

    Number = this->ZigZagDecode(zigZagEncodedValue);
    return true;
}

bool
XPF_API
xpf::Protobuf::DeserializeUI64(
    _Out_ uint64_t& Number,
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    Number = 0;

    //
    // The bytes are packed in [msb][7bit-payload]
    // When msb is 0, we are done.
    // The payload bytes are in reverse order.
    //
    uint8_t shift = 0;
    uint64_t value = 0;

    while (true)
    {
        uint8_t byte = 0;
        if (!Stream.ReadBytes(1, &byte))
        {
            return false;
        }
        uint8_t payload = byte & 0b01111111;

        value = value | (static_cast<uint64_t>(payload) << shift);
        shift = shift + 7;

        //
        // If MSB is 0, then we are finished.
        //
        if ((byte & 0b10000000) == 0)
        {
            break;
        }

        //
        // We are using at most 10 bytes to encode an integer.
        // So if the MSB is still 1, stream is corrupted.
        //
        if (shift >= 70)
        {
            return false;
        }
    }

    Number = value;
    return true;
}

bool
XPF_API
xpf::Protobuf::DeserializeBinaryBlob(
    _Out_ xpf::Vector<uint8_t>& Buffer,
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    Buffer.Clear();

    //
    // Deserialize the length.
    //
    uint64_t binaryBlobSize64 = 0;
    if (!this->DeserializeUI64(binaryBlobSize64, Stream))
    {
        return false;
    }
    if (binaryBlobSize64 > xpf::NumericLimits<uint32_t>::MaxValue())
    {
        return false;
    }
    const uint32_t binaryBlobSize = static_cast<uint32_t>(binaryBlobSize64);

    //
    // Allocate a large enough buffer to read the data.
    //
    xpf::Vector<uint8_t> binaryBlob{ Buffer.GetAllocator() };
    if (!NT_SUCCESS(binaryBlob.Resize(binaryBlobSize)))
    {
        return false;
    }
    for (uint32_t i = 0; i < binaryBlobSize; ++i)
    {
        if (!NT_SUCCESS(binaryBlob.Emplace(uint8_t{ 0 })))
        {
            return false;
        }
    }

    //
    // Do the actual read.
    //
    if (!Stream.ReadBytes(binaryBlobSize, &binaryBlob[0]))
    {
        return false;
    }

    Buffer = xpf::Move(binaryBlob);
    return true;
}

uint64_t
XPF_API
xpf::Protobuf::ZigZagEncode(
    _In_ _Const_ const int64_t& Value
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Value << 1 will make an even number. Equivalent with Value = Value * 2.
    // Value >> 63 will take the sign bit. As the original value is signed we have 2 case:
    //      0 bit - the original number is positive, meaning the value will remain Value * 2
    //      1 bit - the original number is negative, meaning the value will be + 1.
    //
    return (static_cast<uint64_t>(Value) << 1) ^ (Value >> 63);
}

int64_t
XPF_API
xpf::Protobuf::ZigZagDecode(
    _In_ _Const_ const uint64_t& Value
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Value >> 1 will remove the "sign bit" which was used to make the value even or odd
    // depending on its sign.
    // Value & 1 will extract the original sign bit which is 0 or 1.
    //      Applying minus to 0, it will equal 0
    //      Applying minus to 1, it will equal -1
    //
    return static_cast<int64_t>((Value >> 1) ^ (-static_cast<int64_t>(Value & 1)));
}
