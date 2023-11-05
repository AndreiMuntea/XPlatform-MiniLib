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

    if (nullptr == m_Buffer)
    {
        xpf::ApiPanic(STATUS_INSUFFICIENT_RESOURCES);
    }
}

/**
 * @brief Destructor.
 */
virtual ~Base(
    void
) noexcept(true)
{
    xpf::ApiFreeMemory(&m_Buffer);
    m_BufferSize = 0;

    if (nullptr != m_Buffer)
    {
        xpf::ApiPanic(STATUS_INVALID_BUFFER_SIZE);
    }
}

/**
 * @brief Copy and move are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Base, delete);

/**
 * @brief Gets the underlying buffer.
 * 
 * @return the underlying buffer.
 */
inline void*
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
inline const void*
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
 * @brief Copy and move are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Derived, delete);
};

class VirtualInheritanceDerived : public virtual Derived
{
public:
/**
 * @brief Default constructor.
 * 
 * @param[in] BufferSize - the size of the buffer to be allocated.
 */
VirtualInheritanceDerived(
    _In_ size_t BufferSize
) noexcept(true) : Derived(BufferSize)
{
    XPF_NOTHING();
}

/**
 * @brief Destructor.
 */
virtual ~VirtualInheritanceDerived(
    void
) noexcept(true) = default;

/**
 * @brief Copy and move are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(VirtualInheritanceDerived, delete);
};

};  // namespace mocks
};  // namespace xpf
