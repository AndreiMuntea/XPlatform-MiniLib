/**
  * @file        xpf_lib/public/core/Endianess.hpp
  *
  * @brief       This contains helper methods to convert from-and-to supported endians.
  *              It is very useful when you need to send data from one machine to another.
  *              Correctly identifies the current host endianess, and convert to a known format.
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
#include "xpf_lib/public/core/Algorithm.hpp"

namespace xpf
{
enum class Endianess : uint32_t
{
    /**
     * Unknown endianees - this should never be the case.
     * It is useful only to initialize an Endianess instance.
     * If this is returned, something has gone very wrong.
     */
    Unknown = 0,

    /**
     * Little endian format - little end is first.
     * For example 0x12345678 will be stored as 78 56 34 12
     */
    Little = 1,

    /**
     * Big endian format - big end is first.
     * For example 0x12345678 will be stored as 12 34 56 78
     */
    Big = 2,

    /**
     * Don't add anything after this value.
     * Everything should be before it.
     */ 
    MAX
};  // enum class Endianess


/**
 * @brief Computes the endianess on the local machine.
 *        See the enum class Endianess above for more details.
 *
 *
 * @return The local machine endianess.
 */
inline xpf::Endianess
EndianessOnLocalMachine(
    void
) noexcept(true)
{
    /* This occupies 4 bytes in memory. Depending on the first byte, we can deduce endianess. */
    const uint32_t endianess = 0xAB;
    
    if ((*reinterpret_cast<const uint8_t*>(&endianess)) == 0xAB)
    {
        return xpf::Endianess::Little;
    }
    else
    {
        return xpf::Endianess::Big;
    }
}

/**
 * @brief Inverts the byte order of a number. This is done regardless of endianess.
 *
 * @param[in] Value - The value to be inverted.
 *
 * @return The value with the bytes inverted.
 *
 * @note If Value is 0x12345678 the returned value is 0x78563412.
 */
template <class Type>
inline Type
EndianessInvertByteOrder(
    _In_ Type Value
) noexcept(true)
{
    Type newValue = 0;

    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");
    //
    // Invert the byte order.
    //
    for (size_t i = 0; i < sizeof(Type); ++i)
    {
        newValue = newValue << 8;
        newValue = newValue | (Value & 0xFF);
        Value = Value >> 8;
    }

    return newValue;
}

/**
 * @brief Converts a value from a given endianess representation to another one.
 *
 *
 * @param[in] Value - The value to be converted.
 *
 * @param[in] From  - The representation in which the Value is stored.
 *
 * @param[in] To    - The representation in which to convert the value.
 *
 *
 * @return The representation of Value in the "To" representation.
 */
template <class Type>
inline Type
EndianessConvertBetweenRepresentations(
    _In_ _Const_ const Type& Value,
    _In_ _Const_ const xpf::Endianess& From,
    _In_ _Const_ const xpf::Endianess& To
) noexcept(true)
{
    /* Restrict this operation to integers only. */
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    /* Little -> Little and Big -> Big requires no conversion */
    if (((From == xpf::Endianess::Little) && (To == xpf::Endianess::Little)) || 
        ((From == xpf::Endianess::Big)    && (To == xpf::Endianess::Big)))
    {
        return Value;
    }

    /* Little -> Big and Big -> Little requires inversion of byte order. */
    if (((From == xpf::Endianess::Little) && (To == xpf::Endianess::Big)) || 
        ((From == xpf::Endianess::Big)    && (To == xpf::Endianess::Little)))
    {
        return EndianessInvertByteOrder(Value);
    }

    /* Others are not yet supported. Should be extended when the need arise. */
    XPF_DEATH_ON_FAILURE(false);
    return Value;
}

/**
 * @brief Converts a numerical value in big endian.
 *
 * @param[in] Value - The value to be converted in big endian.
 *                    This value is stored in the native (local machine) representation.
 *
 * @return The value in big endian format.
 */
template <class Type>
inline Type
EndianessHostToBig(
    _In_ _Const_ const Type& Value
) noexcept(true)
{
    return EndianessConvertBetweenRepresentations(Value,
                                                  xpf::EndianessOnLocalMachine(),
                                                  xpf::Endianess::Big);
}

/**
 * @brief Converts a numerical value from big endian format in the local machine representation.
 *
 * @param[in] Value - The value to be converted in the native (local machine) representation
 *                    This value is stored in the big endian representation.
 *
 * @return The value in the local machine representation.
 */
template <class Type>
inline Type
EndianessBigToHost(
    _In_ _Const_ const Type& Value
) noexcept(true)
{
    return EndianessConvertBetweenRepresentations(Value,
                                                  xpf::Endianess::Big,
                                                  xpf::EndianessOnLocalMachine());
}

/**
 * @brief Converts a numerical value in little endian.
 *
 * @param[in] Value - The value to be converted in little endian.
 *                    This value is stored in the native (local machine) representation.
 *
 * @return The value in little endian format.
 */
template <class Type>
inline Type
EndianessHostToLittle(
    _In_ _Const_ const Type& Value
) noexcept(true)
{
    return EndianessConvertBetweenRepresentations(Value,
                                                  xpf::EndianessOnLocalMachine(),
                                                  xpf::Endianess::Little);
}

/**
 * @brief Converts a numerical value from little endian format in the local machine representation.
 *
 * @param[in] Value - The value to be converted in the native (local machine) representation
 *                    This value is stored in the little endian representation.
 *
 * @return The value in the local machine representation.
 */
template <class Type>
inline Type
EndianessLittleToHost(
    _In_ _Const_ const Type& Value
) noexcept(true)
{
    return EndianessConvertBetweenRepresentations(Value,
                                                  xpf::Endianess::Little,
                                                  xpf::EndianessOnLocalMachine());
}
};  // namespace xpf
