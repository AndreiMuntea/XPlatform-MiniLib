/**
 * @file        xpf_lib/public/Containers/Stream.hpp
 *
 * @brief       This contains an implementation of StreamReader and StreamWriter
 *              classes. These ease the access for serializers when needed to
 *              work with binary-blobs of data.
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
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/PlatformApi.hpp"

#include "xpf_lib/public/Memory/MemoryAllocator.hpp"
#include "xpf_lib/public/Memory/CompressedPair.hpp"

#include "xpf_lib/public/Containers/Vector.hpp"


namespace xpf
{
//
// ************************************************************************************************
// This is the section containing the interfaces
// ************************************************************************************************
//

/**
 * @brief   This is a stream reader interface. This allows decoupling from the implementation.
 *          The default one is provided below in this file. It is especially useful when
 *          deserializing data.
 */
class IStreamReader
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IStreamReader, default);

 public:
/**
 * @brief IStreamReader constructor - default.
 */
IStreamReader(
    void
) noexcept(true) = default;

/**
 * @brief IStreamReader destructor - default
 */
virtual ~IStreamReader(
    void
) noexcept(true) = default;

/**
 * @brief Reads a number of bytes from the underlying stream.
 *
 * @param[in] NumberOfBytes - The number of bytes to read from the stream.
 *
 * @param[in,out] Bytes - The read bytes.
 *
 * @param[in] Peek - If this is true, the cursor will not be adjusted,
 *                   that's it - if subsequent reads will be attempted,
 *                   the same values will be read, as the cursor won't be adjusted.
 *
 * @return true if the operation was successful, false otherwise.
 */
virtual bool
XPF_API
ReadBytes(
    _In_ size_t NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _In_ bool Peek = false
) noexcept(true) = 0;
};  // IStreamReader

/**
 * @brief   This is a stream writer interface. This allows decoupling from the implementation.
 *          The default one is provided below in this file. It is especially useful when
 *          serializing data.
 */
class IStreamWriter
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IStreamWriter, default);

 public:
/**
 * @brief IStreamWriter constructor - default.
 */
IStreamWriter(
    void
) noexcept(true) = default;

/**
 * @brief IStreamWriter destructor - default
 */
virtual ~IStreamWriter(
    void
) noexcept(true) = default;

/**
 * @brief Writes a number of bytes to the underlying stream.
 *
 * @param[in] NumberOfBytes - The number of bytes to write from the stream.
 *
 * @param[in] Bytes - The bytes to be written.
 *
 * @return true if the operation was successful, false otherwise.
 */
virtual bool
XPF_API
WriteBytes(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true) = 0;

/**
 * @brief   Retrieves the number of bytes serialized so far.
 *
 * @return  A number indicating how many bytes have been serialized so far.
 */
virtual size_t
XPF_API
StreamSize(
    void
) const noexcept(true) = 0;

};  // IStreamWriter

//
// ************************************************************************************************
// This is the section containing a stream reader implementation.
// ************************************************************************************************
//

/**
 * @brief   This is a stream reader class which allows easy peeking into a data buffer.
 *          Espeacially useful for deserializing data.
 */
class StreamReader final : public virtual xpf::IStreamReader
{
 public:
/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(StreamReader, delete);

/**
 * @brief StreamReader constructor - default.
 *
 * @param[in] DataBuffer - The binary blob of data from where this stream will read.
 *                         This won't be modified by the StreamReader class - it is read only.
 */
StreamReader(
    _In_ const xpf::Buffer& DataBuffer
) noexcept(true): xpf::IStreamReader(),
                  m_Buffer{ DataBuffer }
{
    XPF_NOTHING();
}

/**
 * @brief StreamReader destructor - default
 */
virtual ~StreamReader(
    void
) noexcept(true) = default;

/**
 * @brief Reads a number from the underlying stream
 *
 * @param[out] Number - The read value.
 *
 * @param[in] Peek - If this is true, the cursor will not be adjusted,
 *                   that's it - if subsequent reads will be attempted,
 *                   the same value will be read, as the cursor won't be adjusted.
 *
 * @return true if the operation was successful, false otherwise.
 */
template <class Type>
inline bool
XPF_API
ReadNumber(
    _Out_ Type& Number,         // NOLINT(runtime/references)
    _In_ bool Peek = false
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    //
    // Grab the address of number. We'll use that to reinterpret as a byte array.
    //
    void* numberAddress = xpf::AddressOf(Number);

    return this->ReadBytes(sizeof(Type),
                           static_cast<uint8_t*>(numberAddress),
                           Peek);
}

/**
 * @brief Reads a number of bytes from the underlying stream.
 *
 * @param[in] NumberOfBytes - The number of bytes to read from the stream.
 *
 * @param[in,out] Bytes - The read bytes.
 *
 * @param[in] Peek - If this is true, the cursor will not be adjusted,
 *                   that's it - if subsequent reads will be attempted,
 *                   the same values will be read, as the cursor won't be adjusted.
 *
 * @return true if the operation was successful, false otherwise.
 */
bool
XPF_API
ReadBytes(
    _In_ size_t NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _In_ bool Peek = false
) noexcept(true) override
{
    //
    // Validate the input parameters.
    // We can't read 0 - bytes or access null pointers.
    //
    if ((0 == NumberOfBytes) || (nullptr == Bytes))
    {
        return false;
    }

    //
    // Validate that we have enough bytes left in buffer.
    // On overflow we stop.
    //
    size_t cursorFinalPosition = 0;
    if (!xpf::ApiNumbersSafeAdd(this->m_Cursor, NumberOfBytes, &cursorFinalPosition))
    {
        return false;
    }
    if (cursorFinalPosition > this->m_Buffer.GetSize())
    {
        return false;
    }

    //
    // Copy the buffer into destination.
    //
    const uint8_t* buffer = static_cast<const uint8_t*>(this->m_Buffer.GetBuffer());
    xpf::ApiCopyMemory(Bytes,
                       &buffer[this->m_Cursor],
                       NumberOfBytes);

    //
    // If we're not peeking, we're adjusting the cursor as well.
    //
    if (!Peek)
    {
        this->m_Cursor = cursorFinalPosition;
    }

    return true;
}

 private:
     const xpf::Buffer& m_Buffer;
     size_t m_Cursor = 0;
};  // StreamReader


//
// ************************************************************************************************
// This is the section containing a stream writer implementation.
// ************************************************************************************************
//

/**
 * @brief   This is a stream reader class which allows easy writing into a data buffer.
 *          Espeacially useful for serializing data.
 */
class StreamWriter final : public virtual xpf::IStreamWriter
{
 public:
/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(StreamWriter, delete);

/**
 * @brief StreamWriter constructor - default.
 *
 * @param[in,out] DataBuffer - The binary blob of data from where this stream will write to.
 */
StreamWriter(
    _Inout_ xpf::Buffer& DataBuffer
) noexcept(true): xpf::IStreamWriter(),
                  m_Buffer{ DataBuffer }
{
    XPF_NOTHING();
}

/**
 * @brief StreamWriter destructor - default
 */
virtual ~StreamWriter(
    void
) noexcept(true) = default;

/**
 * @brief Writes a number from the underlying stream
 *
 * @param[in] Number - The value to be written.
 *
 * @return true if the operation was successful, false otherwise.
 */
template <class Type>
inline bool
XPF_API
WriteNumber(
    _In_ const Type& Number
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    //
    // Grab the address of number. We'll use that to reinterpret as a byte array.
    //
    const void* numberAddress = xpf::AddressOf(Number);

    return this->WriteBytes(sizeof(Type),
                            static_cast<const uint8_t*>(numberAddress));
}

/**
 * @brief Writes a number of bytes to the underlying stream.
 *
 * @param[in] NumberOfBytes - The number of bytes to write from the stream.
 *
 * @param[in] Bytes - The bytes to be written.
 *
 * @return true if the operation was successful, false otherwise.
 */
bool
XPF_API
WriteBytes(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true) override
{
    //
    // Validate the input parameters.
    // We can't write 0 - bytes or access null pointers.
    //
    if ((0 == NumberOfBytes) || (nullptr == Bytes))
    {
        return false;
    }

    //
    // Validate that we have enough bytes left in buffer.
    // On overflow we stop.
    //
    size_t cursorFinalPosition = 0;
    if (!xpf::ApiNumbersSafeAdd(this->m_Cursor, NumberOfBytes, &cursorFinalPosition))
    {
        return false;
    }
    if (cursorFinalPosition > this->m_Buffer.GetSize())
    {
        return false;
    }

    //
    // Copy the buffer into destination.
    //
    auto buffer = static_cast<uint8_t*>(this->m_Buffer.GetBuffer());
    xpf::ApiCopyMemory(&buffer[this->m_Cursor],
                       Bytes,
                       NumberOfBytes);
    this->m_Cursor = cursorFinalPosition;

    return true;
}

/**
 * @brief   Retrieves the number of bytes serialized so far.
 *
 * @return  A number indicating how many bytes have been serialized so far.
 */
size_t
XPF_API
StreamSize(
    void
) const noexcept(true) override
{
    return this->m_Cursor;
}

 private:
     xpf::Buffer& m_Buffer;
     size_t m_Cursor = 0;
};  // StreamWriter
};  // namespace xpf
