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

namespace xpf
{
/**
 * @brief   This is the interface for the Serializer class.
 *          All other serializers must inherit this one.
 */
class ISerializer
{
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
 *
 * @param[in] Number - The number to be serialized.
 *
 * @return true if the number was sucessfully serialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeI64(
    _In_ _Const_ const int64_t& Number
) noexcept(true) = 0;

/**
 * @brief This method is used to serialize an unsigned number.
 *
 * @param[in] Number - The number to be serialized.
 *
 * @return true if the number was sucessfully serialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeUI64(
    _In_ _Const_ const uint64_t& Number
) noexcept(true) = 0;

/**
 * @brief This method is used to serialize an ansi buffer.
 *
 * @param[in] Buffer - The buffer to be serialized.
 *
 * @return true if the buffer was sucessfully serialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeAnsiBuffer(
    _In_ _Const_ const xpf::StringView<char>& Buffer
) noexcept(true) = 0;

/**
 * @brief This method is used to serialize a wide buffer.
 *
 * @param[in] Buffer - The buffer to be serialized.
 *
 * @return true if the buffer was sucessfully serialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeWideBuffer(
    _In_ _Const_ const xpf::StringView<wchar_t>& Buffer
) noexcept(true) = 0;

/**
 * @brief This method is used to deserialize a signed number.
 *
 * @param[out] Number - This contains the deserialized value.
 *
 * @return true if the number was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
DeserializeI64(
    _Out_ int64_t& Number
) noexcept(true) = 0;

/**
 * @brief This method is used to deserialize an unsigned number.
 *
 * @param[out] Number - This contains the deserialized value.
 *
 * @return true if the number was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
DeserializeUI64(
    _Out_ uint64_t& Number
) noexcept(true) = 0;

/**
 * @brief This method is used to deserialize an ansi string.
 *
 * @param[out] Buffer - This contains the deserialized buffer.
 *
 * @return true if the buffer was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
DeserializeAnsiBuffer(
    _Out_ xpf::String<char>& Buffer
) noexcept(true) = 0;

/**
 * @brief This method is used to deserialize a wide string.
 *
 * @param[out] Buffer - This contains the deserialized buffer.
 *
 * @return true if the buffer was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
DeserializeWideBuffer(
    _Out_ xpf::String<wchar_t>& Buffer
) noexcept(true) = 0;

/**
 * @brief This method is used to retrieve serialized data by this serializer.
 *
 * @return A string view representing a byte-array like data.
 *         This will represent the data in a serialized format.
 *         This won't behave like a string, but rather as a byte-array.
 */
virtual xpf::StringView<char>
XPF_API
GetData(
    void
) const noexcept(true) = 0;

/**
 * @brief This method is used to replace the currently available data for a serializer.
 *        It is useful when we are trying to deserialize objects from an already constructed data stream.
 *
 * @param[in] Data - The data to replace the existing byte-array.
 *
 * @param[in] ShouldDeepCopy - A boolean which controls whether the Data should be duplicated (true),
 *                             or it is guaranteed by the to remain valid while the serializer is used (false).
 *
 * 
 * @return true if the buffer data was sucessfully set,
 *         false otherwise.
 */
virtual bool
XPF_API
SetData(
    _In_ _Const_ const xpf::StringView<char>& Data,
    _In_ bool ShouldDeepCopy
) noexcept(true) = 0;

/**
 * @brief Copy constructor - default.
 * 
 * @param[in] Other - The other object to construct from.
 */
ISerializer(
    _In_ _Const_ const ISerializer& Other
) noexcept(true) = default;

/**
 * @brief Move constructor - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
ISerializer(
    _Inout_ ISerializer&& Other
) noexcept(true) = default;

/**
 * @brief Copy assignment - default.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
ISerializer&
operator=(
    _In_ _Const_ const ISerializer& Other
) noexcept(true) = default;

/**
 * @brief Move assignment - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
ISerializer&
operator=(
    _Inout_ ISerializer&& Other
) noexcept(true) = default;
};  // class ISerializer

/**
 * @brief   This is the interface for a serializable class.
 *          For a class to be serializable must inherit this interface.
 */
class ISerializable
{
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
 * @return true if this class object was sucessfully deserialized,
 *         false otherwise.
 */
virtual bool
XPF_API
SerializeTo(
    _Inout_ xpf::ISerializer& Serializer
) const noexcept(true) = 0;

/**
 * @brief This method is used to deserialize from a specific serializer.
 *
 * @param[in, out] Serializer - The serializer to deserialize from.
 *
 * @return A shared pointer containing a new instance of the ISerializable object.
 *         Will be empty on failure.
 */
virtual xpf::SharedPointer<ISerializable>
XPF_API
DeserializeFrom(
    _Inout_ xpf::ISerializer& Serializer
) const noexcept(true) = 0;

/**
 * @brief Copy constructor - default.
 * 
 * @param[in] Other - The other object to construct from.
 */
ISerializable(
    _In_ _Const_ const ISerializable& Other
) noexcept(true) = default;

/**
 * @brief Move constructor - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
ISerializable(
    _Inout_ ISerializable&& Other
) noexcept(true) = default;

/**
 * @brief Copy assignment - default.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
ISerializable&
operator=(
    _In_ _Const_ const ISerializable& Other
) noexcept(true) = default;

/**
 * @brief Move assignment - default.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
ISerializable&
operator=(
    _Inout_ ISerializable && Other
) noexcept(true) = default;
};  // class ISerializable
};  // namespace xpf
