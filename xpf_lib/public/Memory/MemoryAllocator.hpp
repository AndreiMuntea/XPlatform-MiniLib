/**
 * @file        xpf_lib/public/Memory/MemoryAllocator.hpp
 *
 * @brief       C++ -like memory allocator.
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
// This is the section containing the default memory allocator class.
// ************************************************************************************************
//

/**
 * @brief This is the default memory allocator for xpf C++ objects.
 *        Can be substituted by custom allocators when needed.
 */
class MemoryAllocator
{
 public:
/**
 * @brief Default constructor.
 */
MemoryAllocator(
    void
) noexcept(true) = default;


/**
 * @brief Default destructor.
 */
~MemoryAllocator(
    void
) noexcept(true) = default;

/**
 * @brief This class can be moved and copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(MemoryAllocator, default);

/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize - The requsted block size.
 * 
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
static inline void*
AllocateMemory(
    _In_ size_t BlockSize
) noexcept(true)
{
    return xpf::ApiAllocateMemory(BlockSize,
                                  false);        // CriticalAllocation = false.
}

/**
 * @brief Frees a block of memory.
 * 
 * @param[in,out] MemoryBlock - To be freed.
 * 
 */
static inline void
FreeMemory(
    _Inout_ void* MemoryBlock
) noexcept(true)
{
    xpf::ApiFreeMemory(MemoryBlock);
}

/**
 * @brief Constructs an object of given type with provided arguments.
 *        Uses placement new to properly create the object.
 * 
 * @param[in,out] Object - A pointer where the object will be constructed.
 * 
 * @param[in,out] ConstructorArguments - To be provided to the object.
 * 
 * @note No need to reimplement Construct method when defining your own allocator.
 *       This is the one that is used anyway. Try to keep this logic in one place
 *       so we don't duplicate it.
 */
template <class Type, typename... Arguments>
static inline void
Construct(
    _Inout_ Type* Object,
    Arguments&& ...ConstructorArguments
) noexcept(true)
{
    static_assert(noexcept(Type(xpf::Forward<Arguments>(ConstructorArguments)...)),
                  "Object can not be safely constructed!");

    ::new (Object) Type(xpf::Forward<Arguments>(ConstructorArguments)...);
}

/**
 * @brief Destructs an object of given type.
 *        This doesn't free the memory.
 * 
 * @param[in,out] Object - A pointer where the object will be destroyed.
 *
 * @note No need to reimplement Destruct method when defining your own allocator.
 *       This is the one that is used anyway. Try to keep this logic in one place
 *       so we don't duplicate it.
 */
template <class Type>
static inline void
Destruct(
    _Inout_ Type* Object
) noexcept(true)
{
    static_assert(noexcept(Object->~Type()),
                  "Object can not be safely destructed!");

    if constexpr (!xpf::IsTriviallyDestructible<Type>())
    {
        Object->~Type();
    }
}
};  // class MemoryAllocator

/**
 * @brief This is a memory allocator which allocates critical memory.
 *        Uses default alloc memory function, with critical=true.
 * 
 * @note  On Windows KM this means the memory is allocated from nonpagedpool.
 */
class CriticalMemoryAllocator
{
 public:
/**
 * @brief Default constructor.
 */
CriticalMemoryAllocator(
    void
) noexcept(true) = default;


/**
 * @brief Default destructor.
 */
~CriticalMemoryAllocator(
    void
) noexcept(true) = default;

/**
 * @brief This class can be moved and copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(CriticalMemoryAllocator, default);

/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize - The requsted block size.
 * 
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
static inline void*
AllocateMemory(
    _In_ size_t BlockSize
) noexcept(true)
{
    return xpf::ApiAllocateMemory(BlockSize,
                                  true);         // CriticalAllocation = true.
}

/**
 * @brief Frees a block of memory.
 * 
 * @param[in,out] MemoryBlock - To be freed.
 * 
 */
static inline void
FreeMemory(
    _Inout_ void* MemoryBlock
) noexcept(true)
{
    xpf::ApiFreeMemory(MemoryBlock);
}
};  // class CriticalMemoryAllocator

/**
 * @brief   Helper for the default template of allocate memory.
 */
using FnMemoryAlloc = decltype(xpf::MemoryAllocator::AllocateMemory)*;

/**
 * @brief   Helper for the default template of freeing memory.
 */
using FnMemoryFree = decltype(xpf::MemoryAllocator::FreeMemory)*;

/**
 * @brief   A simple structure to store alloc and free functions.
 */
struct PolymorphicAllocator
{
    /**
     * @brief   This API is used to allocate memory. 
     */
    FnMemoryAlloc AllocFunction = &xpf::MemoryAllocator::AllocateMemory;

    /**
     * @brief   This API is used to free memory. 
     */
    FnMemoryFree FreeFunction = &xpf::MemoryAllocator::FreeMemory;
};  // struct PolymorphicAllocator

};  // namespace xpf
