/**
 * @file        xpf_lib/private/Memory/LookasideListAllocator.cpp
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

#include "xpf_lib/xpf.hpp"


 /**
  * @brief   By default all code in here goes into default section.
  *          We don't want to page out the allocator.
  */
XPF_SECTION_DEFAULT;

void
XPF_API
xpf::LookasideListAllocator::Destroy(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Walk the list and free everything.
    //
    XPF_SINGLE_LIST_ENTRY* crtEntry = xpf::TlqFlush(this->m_TwoLockQueue);
    while (nullptr != crtEntry)
    {
        void* blockToBeDestroyed = reinterpret_cast<void*>(crtEntry);
        crtEntry = crtEntry->Next;

        this->DeleteMemoryBlock(&blockToBeDestroyed);
    }

    //
    // Don't leave garbage.
    //
    this->m_ElementSize = 0;
    this->m_CurrentElements = 0;
    this->m_MaxElements = 0;
}

_Ret_maybenull_
void*
XPF_API
xpf::LookasideListAllocator::NewMemoryBlock(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    return (this->m_IsCriticalAllocator) ? xpf::CriticalMemoryAllocator::AllocateMemory(this->m_ElementSize)
                                         : xpf::MemoryAllocator::AllocateMemory(this->m_ElementSize);
}

void
XPF_API
xpf::LookasideListAllocator::DeleteMemoryBlock(
    _Inout_ void** MemoryBlock
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    (this->m_IsCriticalAllocator) ? xpf::CriticalMemoryAllocator::FreeMemory(MemoryBlock)
                                  : xpf::MemoryAllocator::FreeMemory(MemoryBlock);
}

_Check_return_
_Ret_maybenull_
void*
XPF_API
xpf::LookasideListAllocator::AllocateMemory(
    _In_ size_t BlockSize
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    XPF_SINGLE_LIST_ENTRY* memoryBlock = nullptr;

    //
    // If the block size is greater than the element size, we can't satisfy the allocation.
    // This is likely an invalid usage.
    //
    if (BlockSize > this->m_ElementSize)
    {
        return nullptr;
    }

    //
    // If we have a memory block in list, we return it.
    //
    memoryBlock = xpf::TlqPop(this->m_TwoLockQueue);

    //
    // If we didn't have a memory block we take the long route.
    //
    if (nullptr == memoryBlock)
    {
        memoryBlock = reinterpret_cast<XPF_SINGLE_LIST_ENTRY*>(this->NewMemoryBlock());
    }
    else
    {
        xpf::ApiAtomicDecrement(&this->m_CurrentElements);
    }

    //
    // Ensure we don't send garbage back.
    // It is especially useful as the memory block might contain a valid "next" pointer.
    // We don't want the caller to play with it :)
    //
    if (nullptr != memoryBlock)
    {
        memoryBlock->Next = nullptr;
    }
    return memoryBlock;
}

void
XPF_API
xpf::LookasideListAllocator::FreeMemory(
    _Inout_ void** MemoryBlock
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Can't free null block...
    //
    if ((nullptr == MemoryBlock) || (nullptr == (*MemoryBlock)))
    {
        return;
    }

    //
    // Save the memory block as a list entry.
    //
    XPF_SINGLE_LIST_ENTRY* newEntry = reinterpret_cast<XPF_SINGLE_LIST_ENTRY*>(*MemoryBlock);
    newEntry->Next = nullptr;

    //
    // Invalidate the memory block.
    // The caller won't have further access to it.
    //
    *MemoryBlock = nullptr;

    //
    // We might not want to store it into the list if we have too many elements.
    // In such cases we free the memory directly. We don't care if we might race.
    // This value is a best effort to not store too much memory at once.
    // The algorithm works correctly regardless of how many blocks we have.
    //
    if (xpf::ApiAtomicCompareExchange(&this->m_CurrentElements, uint32_t{0}, uint32_t{0}) >= this->m_MaxElements)
    {
        this->DeleteMemoryBlock(reinterpret_cast<void**>(&newEntry));
    }
    else
    {
        xpf::TlqPush(this->m_TwoLockQueue, newEntry);
        xpf::ApiAtomicIncrement(&this->m_CurrentElements);
    }
}
