/**
  * @file        xpf_lib/public/core/Algorithm.hpp
  *
  * @brief       Basic header-only algorithms.
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


namespace xpf
{

/**
 * @brief Checks if a number is a power of 2.
 * 
 * @param[in] Number - to check if it is power or 2 or not.
 * 
 * @return True if the Number is a power of 2,
 *         False otherwise.
 * 
 * @note 0 and 1 are considered valid powers of 2.
 * 
 */
template <class Type>
constexpr inline bool
AlgoIsNumberPowerOf2(
    _In_ _Const_ const Type& Number
) noexcept(true)
{
    return ((Number & (Number - 1)) == 0);
}

/**
 * @brief Checks if a number is aligned to a specified alignment.
 * 
 * @param[in] Number - to check if it is aligned.
 * 
 * @param[in] Alignment - Alignment to check against.
 * 
 * @return True if the Number is aligned to the given alignment,
 *         False otherwise.
 * 
 * @note Alignment is considered a valid number if it is a power of 2
 *       and it is greater than 0
 * 
 */
template <class Type>
constexpr inline bool
AlgoIsNumberAligned(
    _In_ _Const_ const Type& Number,
    _In_ _Const_ const Type& Alignment
) noexcept(true)
{
    if ((Alignment == 0) || (!xpf::AlgoIsNumberPowerOf2(Alignment)))
    {
        return false;
    }
    return (Number % Alignment == 0);
}

/**
 * @brief Aligns a number to a given boundary.
 * 
 * @param[in] Value - Value to be aligned.
 * 
 * @param[in] Alignment - Boundary to be aligned to.
 * 
 * @return Aligned value if the value can be aligned,
 *         original value otherwise. 
 * 
 * @note Please use AlgoIsNumberAligned after to check if
 *       the value was properly aligned.
 * 
 */
template <class Type>
constexpr inline Type
AlgoAlignValueUp(
    _In_ _Const_ const Type& Value,
    _In_ _Const_ const Type& Alignment
) noexcept(true)
{
    if ((Alignment == 0) || (!xpf::AlgoIsNumberPowerOf2(Alignment)))
    {
        return Value;
    }

    if (xpf::AlgoIsNumberAligned(Value, Alignment))
    {
        return Value;
    }

    const Type remainder = Alignment - (Value % Alignment);
    const Type alignedValue = Value + remainder;

    return (alignedValue < Value) ? Value
                                  : alignedValue;
}

/**
 * @brief Converts a pointer to its value.
 * 
 * @param[in] Pointer - Pointer to be converted.
 * 
 * @return Pointer as a number.
 * 
 */
template <class PtrType>
constexpr inline auto
AlgoPointerToValue(
    _In_opt_ _Const_ const PtrType* const Pointer
) noexcept(true)
{
    if constexpr (sizeof(PtrType*) == sizeof(uint8_t))
    {
        return reinterpret_cast<uint8_t>(Pointer);
    }
    else if constexpr (sizeof(PtrType*) == sizeof(uint16_t))
    {
        return reinterpret_cast<uint16_t>(Pointer);
    }
    else if constexpr (sizeof(PtrType*) == sizeof(uint32_t))
    {
        return reinterpret_cast<uint32_t>(Pointer);
    }
    else if constexpr (sizeof(PtrType*) == sizeof(uint64_t))
    {
        return reinterpret_cast<uint64_t>(Pointer);
    }
    else
    {
        static_assert(sizeof(PtrType*) == sizeof(uint8_t)  ||
                      sizeof(PtrType*) == sizeof(uint16_t) ||
                      sizeof(PtrType*) == sizeof(uint32_t) ||
                      sizeof(PtrType*) == sizeof(uint64_t),
                      "Unsupported Pointer type!");
    }
}

};  // namespace xpf
