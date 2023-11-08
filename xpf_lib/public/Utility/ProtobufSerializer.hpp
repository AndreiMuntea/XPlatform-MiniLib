/**
 * @file        xpf_lib/public/Utility/ProtobufSerializer.hpp
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


#pragma once


#include "xpf_lib/public/Utility/ISerializable.hpp"

namespace xpf
{
/**
 * @brief This class implements the ISerializer interface.
 *        It follows a minimalistic implementation for protobuf protocol.
 *            - Unsigned int values are serialized as varint128.
 *            - Signed values are zig zag encoded and then serialized as varint 128.
 *            - The binary blob data is serialized byte by byte.
 */
class Protobuf final : public virtual ISerializer
{
 public:
/**
 * @brief Protobuf constructor - default.
 */
Protobuf(
    void
) noexcept(true) = default;

/**
 * @brief Protobuf destructor - default
 */
virtual ~Protobuf(
    void
) noexcept(true)
{
    XPF_NOTHING();
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Protobuf, delete);

/**
 * @brief This method is used to serialize a signed number.
 *        The value will be serialized into the provided stream.
 *
 * @param[in] Number - The number to be serialized.
 *
 * @param[in,out] Stream - The stream where the given value will be serialized.
 *
 * @return true if the number was sucessfully serialized,
 *         false otherwise.
 */
bool
XPF_API
SerializeI64(
    _In_ _Const_ const int64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) override;

/**
 * @brief This method is used to serialize an unsigned number.
 *        The value will be serialized into the provided stream.
 *
 * @param[in] Number - The number to be serialized.
 *
 * @param[in,out] Stream - The stream where the given value will be serialized.
 *
 * @return true if the number was sucessfully serialized,
 *         false otherwise.
 */
bool
XPF_API
SerializeUI64(
    _In_ _Const_ const uint64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) override;

/**
 * @brief This method is used to serialize a binary blob.
 *        Can be used for character buffers, or simply plain binary data.
 *        The value will be serialized into the provided stream.
 *
 * @param[in] Buffer - The binary blob to be serialized.
 *
 * @param[in,out] Stream - The stream where the given value will be serialized.
 *
 * @return true if the buffer was sucessfully serialized,
 *         false otherwise.
 */
bool
XPF_API
SerializeBinaryBlob(
    _In_ _Const_ const xpf::StringView<char>& Buffer,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) override;

/**
 * @brief This method is used to deserialize a signed number.
 *        The value will be serialized from the provided stream.
 *
 * @param[out] Number - This contains the deserialized value.
 *
 * @param[in,out] Stream - The stream from where the given value will be deserialized.
 *
 * @return true if the number was sucessfully deserialized,
 *         false otherwise.
 */
bool
XPF_API
DeserializeI64(
    _Out_ int64_t& Number,
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true) override;

/**
 * @brief This method is used to deserialize an unsigned number.
 *        The value will be serialized from the provided stream.
 *
 * @param[out] Number - This contains the deserialized value.
 *
 * @param[in,out] Stream - The stream from where the given value will be deserialized.
 *
 * @return true if the number was sucessfully deserialized,
 *         false otherwise.
 */
bool
XPF_API
DeserializeUI64(
    _Out_ uint64_t& Number,
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true) override;

/**
 * @brief This method is used to deserialize a binary blob.
 *        Can be used for character buffers, or simply plain binary data.
 *        The value will be deserialized from the provided stream.
 *
 * @param[out] Buffer - This contains the deserialized buffer.
 *
 * @param[in,out] Stream - The stream from where the given value will be deserialized.
 *
 * @return true if the buffer was sucessfully deserialized,
 *         false otherwise.
 */
bool
XPF_API
DeserializeBinaryBlob(
    _Out_ xpf::Vector<uint8_t>& Buffer,
    _Inout_ xpf::IStreamReader & Stream
) noexcept(true) override;

 private:
/**
 * @brief Zigzag encode a signed integer.
 *
 * @param[in] Value The signed integer to be encoded.
 *
 * @return The zigzag-encoded value.
 *
 * @details This function performs zigzag encoding on a signed integer to represent it
 *          as a non-negative integer, suitable for serialization or transmission.
 *
 *          Positive integers p are encoded as 2 * p (the even numbers),
 *          while negative integers n are encoded as 2 * |n| - 1 (the odd numbers).
 *          The encoding thus “zig-zags” between positive and negative numbers. For example:
 *
 *          | Signed Original | Encoded As |
 *             0                 0
 *            -1                 1
 *             1                 2
 *            -2                 3
 *
 *          See https://protobuf.dev/programming-guides/encoding/
 */
uint64_t
XPF_API
ZigZagEncode(
    _In_ _Const_ const int64_t& Value
) noexcept(true);

/**
 * @brief Zigzag decode a zigzag-encoded integer.
 *
 * @param[in] Value The zigzag-encoded value to be decoded.
 *
 * @return The decoded signed integer.
 */
int64_t
XPF_API
ZigZagDecode(
    _In_ _Const_ const uint64_t& Value
) noexcept(true);
};  // namespace Protobuf
};  // namespace xpf
