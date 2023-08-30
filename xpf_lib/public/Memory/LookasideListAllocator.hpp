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
#include "xpf_lib/public/Containers/AtomicList.hpp"


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
 private:
/**
 * @brief Default constructor.
 */
LookasideListAllocator(
    void
) noexcept(true) = default;

 public:
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

/**
 * @brief Create and initialize a LookasideListAllocator. This must be used instead of constructor.
 *        It ensures the LookasideListAllocator is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] LookasideAllocatorToCreate - the allocator to be created. On input it will be empty.
 *                                              On output it will contain a fully initialized allocator
 *                                              or an empty one on fail.
 *
 * @param[in] ElementSize - The maximum size of the element. This allocator will not be able to
 *                          Allocate elements with a bigger size than this one.
 * 
 * @param[in] IsCriticalAllocator - A boolean specifying whether the critical alloc should be used or not.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success LookasideAllocatorToCreate has a value
 *       and on fail LookasideAllocatorToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::LookasideListAllocator>* LookasideAllocatorToCreate,
    _In_ size_t ElementSize,
    _In_ bool IsCriticalAllocator
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
    xpf::BusyLock m_HeadLock;
    XPF_SINGLE_LIST_ENTRY* m_ListHead = nullptr;

    xpf::BusyLock m_TailLock;
    XPF_SINGLE_LIST_ENTRY* m_ListTail = nullptr;

    bool m_IsCriticalAllocator = false;

    /*
     * @brief   We'll compute the number of elements that we can store in the list.
     *          We'll not exceed this number, to not gobble memory forever.
     */
    size_t m_ElementSize = 0;
    size_t m_MaxElements = 0;
    alignas(uint32_t) volatile uint32_t m_CurrentElements = 0;

    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class CriticalMemoryAllocator
};  // namespace xpf
