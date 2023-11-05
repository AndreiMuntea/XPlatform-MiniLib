/**
 * @file        xpf_lib/public/Utility/ISerializable.hpp
 *
 * @brief       This provides the interface for a class that needs to be
 *              serializable. It also specifies the ISerializer interface
 *              that a specific serializer must respect in order to be compatible.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/Containers/String.hpp"
#include "xpf_lib/public/Memory/SharedPointer.hpp"
#include "xpf_lib/public/Containers/Stream.hpp"

namespace xpf
{
/**
 * @brief   This is the interface for the Serializer class.
 *          All other serializers must inherit this one.
 */
class ISerializer
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ISerializer, default);

 public:
/**
 * @brief ISerializer constructor - default.
 */
ISerializer(
    void
) noexcept(true) = default;

/**
 * @brief ISerializer destructor - default.
 */
virtual ~ISerializer(
    void
) noexcept(true) = default;

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
virtual bool
XPF_API
SerializeI64(
    _In_ _Const_ const int64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) = 0;

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
virtual bool
XPF_API
SerializeUI64(
    _In_ _Const_ const uint64_t& Number,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) = 0;

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
virtual bool
XPF_API
SerializeBinaryBlob(
    _In_ _Const_ const xpf::StringView<char>& Buffer,
    _Inout_ xpf::IStreamWriter& Stream
) noexcept(true) = 0;

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
virtual bool
XPF_API
DeserializeI64(
    _Out_ int64_t& Number,              // NOLINT(runtime/references)
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true) = 0;

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
virtual bool
XPF_API
DeserializeUI64(
    _Out_ uint64_t& Number,             // NOLINT(runtime/references)
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true) = 0;

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
virtual bool
XPF_API
DeserializeBinaryBlob(
    _Out_ xpf::Vector<uint8_t>& Buffer,     // NOLINT(runtime/references)
    _Inout_ xpf::IStreamReader& Stream
) noexcept(true) = 0;
};  // class ISerializer

/**
 * @brief   This is the interface for a serializable class.
 *          For a class to be serializable must inherit this interface.
 */
class ISerializable
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ISerializable, default);

 public:
/**
 * @brief ISerializable constructor - default.
 */
ISerializable(
    void
) noexcept(true) = default;

/**
 * @brief ISerializable destructor - default.
 */
virtual ~ISerializable(
    void
) noexcept(true) = default;


/**
 * @brief This method is used to serialize to a specific serializer.
 *
 * @param[in, out] Serializer - The serializer to serialize to.
 *
 * @param[in,out] Stream - The stream where the given value will be serialized.
 *
 * @return true if this class object was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeTo(
    _Inout_ xpf::ISerializer& Serializer,       // NOLINT(runtime/references)
    _Inout_ xpf::IStreamWriter& Stream
) const noexcept(true) = 0;

/**
 * @brief This method is used to deserialize from a specific serializer.
 *
 * @param[in, out] Serializer - The serializer to deserialize from.
 *
 * @param[in,out] Stream - The stream from where the given value will be deserialized.
 *
 * @return A shared pointer containing a new instance of the ISerializable object.
 *         Will be empty on failure.
 */
virtual xpf::SharedPointer<ISerializable>
XPF_API
DeserializeFrom(
    _Inout_ xpf::ISerializer& Serializer,       // NOLINT(runtime/references)
    _Inout_ xpf::IStreamReader& Stream
) const noexcept(true) = 0;
};  // class ISerializable
};  // namespace xpf
