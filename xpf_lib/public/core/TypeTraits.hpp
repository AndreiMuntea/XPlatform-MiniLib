/**
 * @file        xpf_lib/public/core/TypeTraits.hpp
 *
 * @brief       C++ - like type traits header
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

//
// ************************************************************************************************
// This is the section containing required type traits for C++ move/copy semantics.
// ************************************************************************************************
//

/**
 * @brief Definition for std::is_lvalue_reference.
 *        Determine whether type argument is an lvalue reference.
 *        Default to false.
 */
template <class>
constexpr inline bool IsLValueReference = false;

/**
 * @brief Definition for std::is_lvalue_reference.
 *        Determine whether type argument is an lvalue reference.
 *        For T& we consider them lvalue references.
 */
template <class T>
constexpr inline bool IsLValueReference<T&> = true;

/**
 * @brief Definition for std::is_rvalue_reference.
 *        Determine whether type argument is an rvalue reference.
 *        Default to false.
 */
template <class>
constexpr inline bool IsRValueReference = false;

/**
 * @brief Definition for std::is_rvalue_reference.
 *        Determine whether type argument is an rvalue reference.
 *        For T&& we consider them rvalue references.
 */
template <class T>
constexpr inline bool IsRValueReference<T&&> = true;

/**
 * @brief Definition for std::is_trivially_destructible.
 *        Determins whether type can be trivially destructible.
 *        Uses a compiler intrinsic __is_trivially_destructible.
 *
 * @return true if the Type is trivially destructible,
 *         false otherwise.
 */
template <class Type>
constexpr inline bool IsTriviallyDestructible(void) noexcept(true)
{
    return __is_trivially_destructible(Type);
}

/**
 * @brief Definition for std::is_empty.
 *        This template contains single parameter Type (Trait class)
 *        that identifies whether Type is a empty type.
 *        Uses a compiler intrinsic __is_empty.
 *
 * @return true if the Type is considered to be empty,
 *         false otherwise.
 */
template <class Type>
constexpr inline bool IsTypeEmpty(void) noexcept(true)
{
    return __is_empty(Type);
}

/**
 * @brief Definition for std::is_final.
 *        If Type is a final class (that is, a class declared with the final specifier),
 *        provides the member constant value equal true. For any other type, value is false.
 *        Uses a compiler intrinsic __is_final.
 *
 * @return true if the Type is considered to be final,
 *         false otherwise.
 */
template <class Type>
constexpr inline bool IsTypeFinal(void) noexcept(true)
{
    return __is_final(Type);
}

/**
 * @brief Definition for std::is_base_of.
 *        Determines whether Derived inherits from Base or is the same type as Base
 *        Uses a compiler intrinsic __is_base_of.
 *
 * @return true if the Derived inherits from Base (or is the same type),
 *         false otherwise.
 */
template <class Base, class Derived>
constexpr inline bool IsTypeBaseOf(void) noexcept(true)
{
    return __is_base_of(Base, Derived);
}

/**
 * @brief Definition for std::is_same.
 *        Default to false.
 */
template <class, class>
inline constexpr bool IsSameType = false;

/**
 * @brief Definition for std::is_same.
 *        When same type is provided, it is true.
 */
template <class T>
constexpr inline bool IsSameType<T, T> = true;

/**
 * @brief Definition for std::remove_reference.
 *        This is the base case.
 */
template <class T>
struct RemoveReference
{
    /**
     * @brief Will be the underlying T type.
     */
    using Type = T;
};

/**
 * @brief Definition for std::remove_reference.
 *        This is the lvalue reference case.
 */
template <class T>
struct RemoveReference<T&>
{
    /**
     * @brief Will be the underlying T type.
     */
    using Type = T;
};

/**
 * @brief Definition for std::remove_reference.
 *        This is the rvalue reference case.
 */
template <class T>
struct RemoveReference<T&&>
{
    /**
     * @brief Will be the underlying T type.
     */
    using Type = T;
};

/**
 * @brief Helper wrapper to access the underlying real type of an object.
 */
template <class T>
using RemoveReferenceType = typename RemoveReference<T>::Type;

/**
 * @brief Definition for std::forward<T>.
 *        Forwards lvalues as either lvalues or as rvalues, depending on T.
 * 
 * @param[in] Argument - the object to be forwarded.
 * 
 * @return static_cast<T&&>(Argument);
 */
template <class T>
constexpr inline T&& Forward(
    _In_ RemoveReferenceType<T>& Argument
) noexcept(true)
{
    //
    // Forward an lvalue as either an lvalue or an rvalue
    //
    return static_cast<T&&>(Argument);
}

/**
 * @brief Definition for std::forward<T>.
 *        Forwards lvalues as either lvalues or as rvalues, depending on T.
 * 
 * @param[in] Argument - the object to be forwarded.
 * 
 * @return static_cast<T&&>(Argument);
 */
template <class T>
constexpr inline T&& Forward(
    _In_ RemoveReferenceType<T>&& Argument
) noexcept(true)
{
    //
    // Forward an rvalue as an rvalue
    //
    static_assert(!xpf::IsLValueReference<T>,
                  "Bad forward call!");

    return static_cast<T&&>(Argument);
}

/**
 * @brief Definition for std::move<T>.
 *        std::move is used to indicate that an object t may be "moved from",
 *        i.e. allowing the efficient transfer of resources from t to another object.
 * 
 * @param[in] Argument - the object to be moved.
 * 
 * @return static_cast<typename std::remove_reference<T>::type&&>(t)
 */
template <class T>
constexpr inline RemoveReferenceType<T>&& Move(
    _In_ T&& Argument
) noexcept(true)
{
    //
    // Forward Argument as movable.
    //
    return static_cast<xpf::RemoveReferenceType<T>&&>(Argument);
}

/**
 * @brief Definition for std::addressof<T>.
 *        Obtains the actual address of the object or function arg,
 *        even in presence of overloaded operator&.
 *
 * @param[in] Argument - lvalue object or function.
 *
 * @return Pointer to Argument.
 */
template <class T>
constexpr inline T*
AddressOf(
    _In_ T& Argument
) noexcept(true)
{
    return __builtin_addressof(Argument);
}

/**
 * @brief Definition for std::addressof<T>.
 *        Rvalue overload is deleted to prevent taking the address of const rvalues.
 *
 * @param[in] Argument - UNUSED RValue Parameter.
 *
 * @return Pointer to Argument.
 */
template <class T>
constexpr inline const T*
AddressOf(
    _In_ _Const_ const T&& Argument
) noexcept(true) = delete;



//
// ************************************************************************************************
// This is the section containing numeric limits for the types.
// ************************************************************************************************
//

/**
 * @brief Definition for (some) of the std::numeric_limits.
 */
template <class T>
struct NumericLimits
{
    /**
     * @brief Minimum value for a given type. Must be specialized!
     * 
     * @return The minimum value that T type can store.
     */
    static T MinValue();

    /**
     * @brief Maximum value for a given type. Must be specialized!
     * 
     * @return The maximum value that T type can store.
     */
    static T MaxValue();
};

/**
 * @brief Definition for numeric limits of uint8_t.
 */
template <>
struct NumericLimits<uint8_t>
{
    /**
     * @brief Minimum value for an uint8_t data type: 0.
     * 
     * @return constexpr value 0.
     */
    static constexpr inline uint8_t MinValue() { return uint8_t{ 0 }; }

    /**
     * @brief Maximum value for an uint8_t data type: 0xFF.
     * 
     * @return constexpr value 0xFF.
     */
    static constexpr inline  uint8_t MaxValue() { return uint8_t{ 0xFF }; }
};

/**
 * @brief Definition for numeric limits of int8_t.
 */
template <>
struct NumericLimits<int8_t>
{
    /**
     * @brief Minimum value for an int8_t data type: -128.
     * 
     * @return constexpr value -128.
     */
    static constexpr inline int8_t MinValue() { return  int8_t{ -127 } - int8_t{ 1 }; }

    /**
     * @brief Maximum value for an int8_t data type: 127.
     * 
     * @return constexpr value 127.
     */
    static constexpr inline int8_t MaxValue() { return int8_t{ 127 }; }
};

/**
 * @brief Definition for numeric limits of uint16_t.
 */
template <>
struct NumericLimits<uint16_t>
{
    /**
     * @brief Minimum value for an uint16_t data type: 0.
     * 
     * @return constexpr value 0.
     */
    static constexpr inline uint16_t MinValue() { return uint16_t{ 0 }; }

    /**
     * @brief Maximum value for an uint16_t data type: 0xFFFF.
     * 
     * @return constexpr value 0xFFFF.
     */
    static constexpr inline uint16_t MaxValue() { return uint16_t{ 0xFFFF }; }
};

/**
 * @brief Definition for numeric limits of int16_t.
 */
template <>
struct NumericLimits<int16_t>
{
    /**
     * @brief Minimum value for an int16_t data type: -32768.
     * 
     * @return constexpr value -32768.
     */
    static constexpr inline int16_t MinValue() { return  int16_t{ -32767 } - int16_t{ 1 }; }

    /**
     * @brief Maximum value for an int16_t data type: 32767.
     * 
     * @return constexpr value 32767.
     */
    static constexpr inline int16_t MaxValue() { return int16_t{ 32767 }; }
};

/**
 * @brief Definition for numeric limits of uint32_t.
 */
template <>
struct NumericLimits<uint32_t>
{
    /**
     * @brief Minimum value for an uint32_t data type: 0.
     * 
     * @return constexpr value 0.
     */
    static constexpr inline uint32_t MinValue() { return uint32_t{ 0 }; }

    /**
     * @brief Maximum value for an uint32_t data type: 0xFFFFFFFF.
     * 
     * @return constexpr value 0xFFFFFFFF.
     */
    static constexpr inline uint32_t MaxValue() { return uint32_t{ 0xFFFFFFFF }; }
};

/**
 * @brief Definition for numeric limits of int32_t.
 */
template <>
struct NumericLimits<int32_t>
{
    /**
     * @brief Minimum value for an int32_t data type: -2147483648.
     * 
     * @return constexpr value -2147483648.
     */
    static constexpr inline int32_t MinValue() { return int32_t{ -2147483647 } - int32_t{ 1 }; }

    /**
     * @brief Maximum value for an int32_t data type: 2147483647.
     * 
     * @return constexpr value 2147483647.
     */
    static constexpr inline int32_t MaxValue() { return int32_t{ 2147483647 }; }
};

/**
 * @brief Definition for numeric limits of uint64_t.
 */
template <>
struct NumericLimits<uint64_t>
{
    /**
     * @brief Minimum value for an uint64_t data type: 0.
     * 
     * @return constexpr value 0.
     */
    static constexpr inline uint64_t MinValue() { return uint64_t{ 0 }; }

    /**
     * @brief Maximum value for an uint64_t data type: 0xFFFFFFFFFFFFFFFF.
     * 
     * @return constexpr value 0xFFFFFFFFFFFFFFFF.
     */
    static constexpr inline uint64_t MaxValue() { return uint64_t{ 0xFFFFFFFFFFFFFFFF }; }
};

/**
 * @brief Definition for numeric limits of int64_t.
 */
template <>
struct NumericLimits<int64_t>
{
    /**
     * @brief Minimum value for an int64_t data type: -9223372036854775808.
     * 
     * @return constexpr value -9223372036854775808.
     */
    static constexpr inline int64_t MinValue() { return int64_t{ -9223372036854775807 } - int64_t{ 1 }; }

    /**
     * @brief Maximum value for an int64_t data type: 9223372036854775807.
     * 
     * @return constexpr value 9223372036854775807.
     */
    static constexpr inline int64_t MaxValue() { return int64_t{ 9223372036854775807}; }
};
};  // namespace xpf
