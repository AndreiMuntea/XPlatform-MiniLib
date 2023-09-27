/**
 * @file        xpf_lib/public/Memory/LookasideListAllocator.hpp
 *
 * @brief       C++ -like memory allocator. This will use a list to provide quick access
 *              to freed resources. Instead of freeing them to the system, the blocks
 *              will be enqueued in a list and will be returned to the caller.
 *              It uses a two-lock queue like mechanism for managing quick access to blocks.
 *              (http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)
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
#include "xpf_lib/public/Memory/Optional.hpp"

#include "xpf_lib/public/Locks/BusyLock.hpp"
#include "xpf_lib/public/Containers/TwoLockQueue.hpp"


namespace xpf
{


/**
 * @brief This is a memory allocator which allocates memory and keeps it inside a list.
 *        memory allocations are expensive, so this actually caches some allocs.
 * 
 *        It uses a two-lock queue algorithm for synchronization.
 *        The two-lock queue is a Live-lock free structure presented in  "Simple, Fast, and Practical
 *        Non-Blocking and Blocking Concurrent Queue Algorithms" by Maged M. Michael and Michael L. Scott.
 *        (http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)
 */
class LookasideListAllocator final
{
 public:
/**
 * @brief Default constructor.
 *
 * @param[in] ElementSize - The maximum size that the element can have.
 *                          The allocator won't be able to retrieve elements bigger than this size.
 *
 * @param[in] IsCriticalAllocator - If true, it will allocate using the Critical default allocator,
 *                                  if false, it will allocate using the Memory default allocator.
 */
LookasideListAllocator(
    _In_ size_t ElementSize,
    _In_ bool IsCriticalAllocator
) noexcept(true)
{
    //
    // This needs to be at least XPF_SINGLE_LIST_ENTRY as the element will be inserted in the Two Lock Queue.
    //
    this->m_ElementSize = (ElementSize < sizeof(XPF_SINGLE_LIST_ENTRY)) ? sizeof(XPF_SINGLE_LIST_ENTRY)
                                                                        : ElementSize;
    this->m_IsCriticalAllocator = IsCriticalAllocator;

    //
    // Now let's see how many elements we can store. Compute this based on element size.
    // We don't want to exceed approximatively 1 mb in one allocator.
    // If the allocation is somehow bigger than this limit, we'll store 5 elements in our lookaside list.
    // These numbers can be changed if we notice we have a problem.
    //
    const size_t maxElements = size_t{ 1048576 } / this->m_ElementSize;
    this->m_MaxElements = (maxElements < 5) ? 5
                                            : maxElements;
    this->m_CurrentElements = 0;
}

/**
 * @brief Default destructor.
 */
~LookasideListAllocator(
    void
) noexcept(true)
{
    this->Destroy();
}

/**
 * @brief Copy constructor is prohibited.
 * 
 * @param[in] Other - The other object to construct from.
 */
LookasideListAllocator(
    _In_ _Const_ const LookasideListAllocator& Other
) noexcept(true) = delete;

/**
 * @brief Default move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
LookasideListAllocator(
    _Inout_ LookasideListAllocator&& Other
) noexcept(true) = delete;

/**
 * @brief Default copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
LookasideListAllocator&
operator=(
    _In_ _Const_ const LookasideListAllocator& Other
) noexcept(true) = delete;

/**
 * @brief Default move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
LookasideListAllocator&
operator=(
    _Inout_ LookasideListAllocator&& Other
) noexcept(true) = delete;


/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize - The requsted block size.
 * 
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
void*
XPF_API
AllocateMemory(
    _In_ size_t BlockSize
) noexcept(true);

/**
 * @brief Frees a block of memory.
 * 
 * @param[in,out] MemoryBlock - To be freed.
 * 
 * @return None.
 * 
 * @note The memory will actually be pushed inside a lookaside list.
 *       It will be available faster using AllocateMemory.
 * 
 */
void
XPF_API
FreeMemory(
    _Inout_ void** MemoryBlock
) noexcept(true);

 private:
/**
 * @brief Clears the underlying resources allocated.
 *
 */
void
XPF_API
Destroy(
    void
) noexcept(true);

/**
 * @brief Creates a new block of memory. This will actually use
 *        the system APIs to allocate resources.
 *
 * @return A pointer to the newly allocated memory block.
 */
_Ret_maybenull_
void*
XPF_API
NewMemoryBlock(
    void
) noexcept(true);

/**
 * @brief Frees a block of memory.
 *
 * @param[in,out] MemoryBlock - To be freed.
 *
 * @note The memory will actually be deleted here.
 *
 */
void
XPF_API
DeleteMemoryBlock(
    _Inout_ void** MemoryBlock
) noexcept(true);

 private:
    xpf::TwoLockQueue m_TwoLockQueue;
    bool m_IsCriticalAllocator = false;

    /*
     * @brief   We'll compute the number of elements that we can store in the list.
     *          We'll not exceed this number, to not gobble memory forever.
     */
    size_t m_ElementSize = 0;
    size_t m_MaxElements = 0;
    alignas(uint32_t) volatile uint32_t m_CurrentElements = 0;
};  // class CriticalMemoryAllocator
};  // namespace xpf
