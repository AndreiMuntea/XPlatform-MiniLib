/**
  * @file        xpf_tests/Mocks/TestMocks.hpp
  *
  * @brief       This contains mocks definitions for tests.
  *
  * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
  *
  * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
  *              All rights reserved.
  *
  * @license     See top-level directory LICENSE file.
  */


#pragma once


#include "xpf_tests/XPF-TestIncludes.hpp"


namespace xpf
{
namespace mocks
{

/**
 * @brief This is a dummy non-trivially destructible class.
 */
class Base
{
 public:
/**
 * @brief Default constructor.
 * 
 * @param[in] BufferSize - the size of the buffer to be allocated.
 */
Base(
    _In_ size_t BufferSize
) noexcept(true)
{
    m_BufferSize = BufferSize;
    m_Buffer = xpf::ApiAllocateMemory(m_BufferSize);
    EXPECT_TRUE(m_Buffer != nullptr);
}

/**
 * @brief Destructor.
 */
virtual ~Base(
    void
) noexcept(true)
{
    xpf::ApiFreeMemory(&m_Buffer);
    EXPECT_TRUE(m_Buffer == nullptr);

    m_BufferSize = 0;
}

/**
 * @brief Copy constructor - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 */
Base(
    _In_ _Const_ const Base& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Base(
    _Inout_ Base&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
Base&
operator=(
    _In_ _Const_ const Base& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
Base&
operator=(
    _Inout_ Base&& Other
) noexcept(true) = delete;


/**
 * @brief Gets the underlying buffer.
 * 
 * @return the underlying buffer.
 */
void*
Buffer(
    void
) noexcept(true)
{
    return m_Buffer;
}

/**
 * @brief Gets the underlying buffer.
 * 
 * @return a const reference to the underlying buffer.
 */
const void*
Buffer(
    void
) const noexcept(true)
{
    return m_Buffer;
}

 private:
     size_t m_BufferSize = 0;
     void* m_Buffer = nullptr;
};

/**
 * @brief This is a dummy class that inherits base.
 */
class Derived : public Base
{
 public:
/**
 * @brief Default constructor.
 * 
 * @param[in] BufferSize - the size of the buffer to be allocated.
 */
Derived(
    _In_ size_t BufferSize
) noexcept(true) : Base(BufferSize)
{
    XPF_NOTHING();
}

/**
 * @brief Destructor.
 */
virtual ~Derived(
    void
) noexcept(true) = default;

/**
 * @brief Copy constructor - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 */
Derived(
    _In_ _Const_ const Derived& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
Derived(
    _Inout_ Derived&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - can be implemented when needed.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
Derived&
operator=(
    _In_ _Const_ const Derived& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - can be implemented when needed.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
Derived&
operator=(
    _Inout_ Derived&& Other
) noexcept(true) = delete;
};

};  // namespace mocks
};  // namespace xpf
