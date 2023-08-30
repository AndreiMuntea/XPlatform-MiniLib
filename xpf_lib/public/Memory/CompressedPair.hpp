/**
 * @file        xpf_lib/public/Memory/CompressedPair.hpp
 *
 * @brief       This is the class to mimic std::_Compressed_pair.
 *              This class is inteded to be used internally for EBCO.
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



namespace xpf
{

//
// ************************************************************************************************
// This is the section containing the compressed pair class.
// ************************************************************************************************
//


/**
 * @brief This class is inteded to be used internally for EBCO.
 *        This is the class to mimic std::_Compressed_pair.
 *        The first variant is when Type1 is non-empty and non-final.
 *        So we can inherit it and benefit from EBCO (empty base class optimization).
 */
template <class Type1,
          class Type2,
          bool = (xpf::IsTypeEmpty<Type1>() && !xpf::IsTypeFinal<Type1>())>
class CompressedPair final : private Type1
{
 public:
/**
 * @brief CompressedPair constructor provided no argument.
 *        Other constructors can be added when needed.
 */
CompressedPair(
    void
) noexcept(true) : Type1{},
                   m_SecondValue{}
{
    XPF_NOTHING();
}

/**
 * @brief Default destructor.
 */
~CompressedPair(
    void
) noexcept(true) = default;


/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
CompressedPair(
    _In_ _Const_ const CompressedPair& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
CompressedPair(
    _Inout_ CompressedPair&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
CompressedPair&
operator=(
    _In_ _Const_ const CompressedPair& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
CompressedPair&
operator=(
    _Inout_ CompressedPair&& Other
) noexcept(true) = delete;

/**
 * @brief Retrieves reference to Type1. Because we inherit, we return *this.
 * 
 * @return A reference to *this.
 */
inline Type1&
First(
    void
) noexcept(true)
{
    return *this;
}

/**
 * @brief Retrieves const reference to Type1. Because we inherit, we return *this.
 * 
 * @return A const reference to *this.
 */
inline const Type1&
First(
    void
) const noexcept(true)
{
    return *this;
}

/**
 * @brief Retrieves reference to Type2.
 * 
 * @return A reference to m_SecondValue.
 */
inline Type2&
Second(
    void
) noexcept(true)
{
    return this->m_SecondValue;
}

/**
 * @brief Retrieves const reference to Type2.
 * 
 * @return A const reference to m_SecondValue.
 */
inline const Type2&
Second(
    void
) const noexcept(true)
{
    return this->m_SecondValue;
}

 private:
    Type2 m_SecondValue{};
};  // class CompressedPair


/**
 * @brief This class is inteded to be used internally for EBCO.
 *        This is the class to mimic std::_Compressed_pair.
 *        The second variant is when we can't benefit from EBCO.
 *        Store both first and second as members.
 */
template <class Type1,
          class Type2>
class CompressedPair<Type1, Type2, false> final
{
 public:
/**
 * @brief CompressedPair constructor provided no argument.
 *        Other constructors can be added when needed.
 */
CompressedPair(
    void
) noexcept(true) : m_FirstValue{},
                   m_SecondValue{}
{
    XPF_NOTHING();
}

/**
 * @brief Default destructor.
 */
~CompressedPair(
    void
) noexcept(true) = default;


/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
CompressedPair(
    _In_ _Const_ const CompressedPair& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
CompressedPair(
    _Inout_ CompressedPair&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
CompressedPair&
operator=(
    _In_ _Const_ const CompressedPair& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
CompressedPair&
operator=(
    _Inout_ CompressedPair&& Other
) noexcept(true) = delete;

/**
 * @brief Retrieves reference to Type1.
 * 
 * @return A reference to m_FirstValue.
 */
inline Type1&
First(
    void
) noexcept(true)
{
    return this->m_FirstValue;
}

/**
 * @brief Retrieves const reference to Type1.
 * 
 * @return A const reference to m_FirstValue.
 */
inline const Type1&
First(
    void
) const noexcept(true)
{
    return this->m_FirstValue;
}

/**
 * @brief Retrieves reference to Type2.
 * 
 * @return A reference to m_SecondValue.
 */
inline Type2&
Second(
    void
) noexcept(true)
{
    return this->m_SecondValue;
}

/**
 * @brief Retrieves const reference to Type2.
 * 
 * @return A const reference to m_SecondValue.
 */
inline const Type2&
Second(
    void
) const noexcept(true)
{
    return this->m_SecondValue;
}

 private:
    Type1 m_FirstValue{};
    Type2 m_SecondValue{};
};  // class CompressedPair
};  // namespace xpf
