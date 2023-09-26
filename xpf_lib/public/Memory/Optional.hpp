/**
 * @file        xpf_lib/public/Memory/Optional.hpp
 *
 * @brief       C++ -like optional class.
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
#include "xpf_lib/public/Memory/MemoryAllocator.hpp"


namespace xpf
{

//
// ************************************************************************************************
// This is the section containing the optional class
// ************************************************************************************************
//

/**
 * @brief This is the class to mimic std::optional.
 *        It is used to ensure that invalid objects can't be created.
 */
template <class Type>
class Optional
{
 public:
/**
 * @brief Default constructor.
 */
Optional(
    void
) noexcept(true) : m_Dummy{uint8_t{0}},
                   m_HasValue{false}
{
    XPF_NOTHING();
}

/**
 * @brief Default destructor.
 */
~Optional(
    void
) noexcept(true)
{
    this->Reset();
}

/**
 * @brief Default copy constructor.
 * 
 * @param[in] Other - The other object to construct from.
 */
Optional(
    _In_ _Const_ const Optional& Other
) noexcept(true)
{
    if (Other.HasValue())
    {
        this->Emplace(Other.m_Value);
        this->m_HasValue = true;
    }
    else
    {
        this->m_Dummy = 0;
        this->m_HasValue = false;
    }
}

/**
 * @brief Default move constructor.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Optional(
    _Inout_ Optional&& Other
) noexcept(true)
{
    if (Other.HasValue())
    {
        this->Emplace(xpf::Move(Other.m_Value));
        this->m_HasValue = true;

        xpf::MemoryAllocator::Destruct(&Other.m_Value);
        Other.m_HasValue = false;
        Other.m_Dummy = 0;
    }
    else
    {
        this->m_Dummy = 0;
        this->m_HasValue = false;
    }
}

/**
 * @brief Default copy assignment.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
Optional&
operator=(
    _In_ _Const_ const Optional& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Reset();

        if (Other.HasValue())
        {
            this->Emplace(Other.m_Value);
            this->m_HasValue = true;
        }
    }
    return *this;
}

/**
 * @brief Default move assignment.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
Optional&
operator=(
    _Inout_ Optional&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        this->Reset();

        if (Other.HasValue())
        {
            this->Emplace(xpf::Move(Other.m_Value));
            this->m_HasValue = true;

            xpf::MemoryAllocator::Destruct(&Other.m_Value);
            Other.m_HasValue = false;
            Other.m_Dummy = 0;
        }
    }
    return *this;
}



/**
 * @brief Checks if we have an underlying object.
 *
 * @return true if underlying object is assigned , false otherwise.
 */
inline bool
HasValue(
    void
) const noexcept(true)
{
    return this->m_HasValue;
}

/**
 * @brief Returns the underlying object.
 *
 * @return A non const reference to Type.
 */
inline Type&
operator*(
    void
) noexcept(true)
{
    if (!this->HasValue())
    {
        XPF_DEATH_ON_FAILURE(this->HasValue());
    }

    return this->m_Value;
}

/**
 * @brief Returns the underlying object.
 *
 * @return A const reference to Type.
 */
inline const Type&
operator*(
    void
) const noexcept(true)
{
    if (!this->HasValue())
    {
        XPF_DEATH_ON_FAILURE(this->HasValue());
    }

    return this->m_Value;
}

/**
 * @brief Destroys the underlying object if any.
 */
inline void
Reset(
    void
) noexcept(true)
{
    if (this->m_HasValue)
    {
        xpf::MemoryAllocator::Destruct(&this->m_Value);
    }

    this->m_Dummy = 0;
    this->m_HasValue = false;
}

/**
 * @brief In-Place constructs the object given the provided arguments.
 *
 * @param[in,out] ConstructorArguments - To be provided to the object.
 */
template <typename... Arguments>
inline void
Emplace(
    Arguments&& ...ConstructorArguments
) noexcept(true)
{
    this->Reset();

    xpf::MemoryAllocator::Construct(&this->m_Value,
                                    xpf::Forward<Arguments>(ConstructorArguments)...);
    this->m_HasValue = true;
}

 private:
     /**
      * @brief This will allocate an aligned large-enough storage to hold an object of type Type
      *        without actually calling the constructor for it.
      */
    union
    {
        uint8_t m_Dummy;
        Type m_Value;
    };
    bool m_HasValue = false;
};  // class Optional
};  // namespace xpf
