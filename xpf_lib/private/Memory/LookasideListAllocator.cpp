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

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::LookasideListAllocator::Create(
    _Inout_ xpf::Optional<xpf::LookasideListAllocator>* LookasideAllocatorToCreate,
    _In_ size_t ElementSize,
    _In_ bool IsCriticalAllocator
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized lookaside list allocator.
    // Assert here and bail early.
    //
    if ((nullptr == LookasideAllocatorToCreate) || (LookasideAllocatorToCreate->HasValue()))
    {
        XPF_ASSERT(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new allocator. This will be an empty one.
    // It will be initialized below.
    //
    LookasideAllocatorToCreate->Emplace();

    //
    // We failed to create an event. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!LookasideAllocatorToCreate->HasValue())
    {
        XPF_ASSERT(false);
        return STATUS_NO_DATA_DETECTED;
    }
    xpf::LookasideListAllocator& newLookasideAllocator = (*(*LookasideAllocatorToCreate));

    //
    // The element size needs to be at least the size of the atomic list.
    // As we will reuse the memory for blocks.
    //
    newLookasideAllocator.m_ElementSize = (ElementSize < sizeof(XPF_SINGLE_LIST_ENTRY)) ? sizeof(XPF_SINGLE_LIST_ENTRY)
                                                                                        : ElementSize;
    newLookasideAllocator.m_IsCriticalAllocator = IsCriticalAllocator;

    //
    // Now let's see how many elements we can store. Compute this based on element size.
    // We don't want to exceed approximatively 1 mb in one allocator.
    // If the allocation is somehow bigger than this limit, we'll store 5 elements in our lookaside list.
    // These numbers can be changed if we notice we have a problem.
    //
    const size_t maxElements = size_t{ 1048576 } / newLookasideAllocator.m_ElementSize;
    newLookasideAllocator.m_MaxElements = (maxElements < 5) ? 5
                                                            : maxElements;
    newLookasideAllocator.m_CurrentElements = 0;

    //
    // Now we need the sentinel node - the initial allocation.
    // If this fails, we stop.
    //
    XPF_SINGLE_LIST_ENTRY* sentinel = reinterpret_cast<XPF_SINGLE_LIST_ENTRY*>(newLookasideAllocator.NewMemoryBlock());
    if (nullptr == sentinel)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    sentinel->Next = nullptr;

    //
    // Now set the head and tail to point to the sentinel.
    //
    newLookasideAllocator.m_ListHead = sentinel;
    newLookasideAllocator.m_ListTail = sentinel;

    //
    // All good. We're done.
    //
    status = STATUS_SUCCESS;
Exit:
    if (!NT_SUCCESS(status))
    {
        LookasideAllocatorToCreate->Reset();
        XPF_ASSERT(!LookasideAllocatorToCreate->HasValue());
    }
    else
    {
        XPF_ASSERT(LookasideAllocatorToCreate->HasValue());
    }
    return status;
}

void
XPF_API
xpf::LookasideListAllocator::Destroy(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Acquire both locks. Prevent further operations.
    // This is done only on destructor.
    //
    xpf::ExclusiveLockGuard headGuard(this->m_HeadLock);
    xpf::ExclusiveLockGuard tailGuard(this->m_TailLock);

    //
    // Walk the list and free everything.
    //
    XPF_SINGLE_LIST_ENTRY* crtEntry = this->m_ListHead;
    while (nullptr != crtEntry)
    {
        void* blockToBeDestroyed = reinterpret_cast<void*>(crtEntry);
        crtEntry = crtEntry->Next;

        this->DeleteMemoryBlock(&blockToBeDestroyed);
    }

    //
    // Don't leave garbage.
    //
    this->m_ListHead = nullptr;
    this->m_ListTail = nullptr;
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
    // This is likely an invalid usage. Assert here.
    //
    if (BlockSize > this->m_ElementSize)
    {
        XPF_ASSERT(false);
        return nullptr;
    }

    //
    // Now we start - grab the head lock. We always dequeue at head.
    // If we have a memory block in list, we return it.
    //
    {
        xpf::ExclusiveLockGuard headGuard{ this->m_HeadLock };

        //
        // A null head means we are already destructed.
        // Shouldn't happen. Assert here and bail.
        //
        if (nullptr == this->m_ListHead)
        {
            xpf::ApiPanic(STATUS_INVALID_STATE_TRANSITION);
            return nullptr;
        }

        //
        // We keep the sentinel in list.
        // So if next is null, we can't allocate.
        //
        if (nullptr != this->m_ListHead->Next)
        {
            memoryBlock = this->m_ListHead;
            this->m_ListHead = this->m_ListHead->Next;

            //
            // Remove one element from the list.
            //
            xpf::ApiAtomicDecrement(&this->m_CurrentElements);
        }
    }

    //
    // If we didn't have a memory block we take the long route.
    //
    if (nullptr == memoryBlock)
    {
        memoryBlock = reinterpret_cast<XPF_SINGLE_LIST_ENTRY*>(this->NewMemoryBlock());
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
        this->FreeMemory(reinterpret_cast<void**>(&newEntry));
        return;
    }

    //
    // Now we attempt to free the block - this means, enqueue to list.
    // Always enqueue at tail.
    //
    {
        xpf::ExclusiveLockGuard tailGuard{ this->m_TailLock };

        //
        // A null tail means we are already destructed.
        // Shouldn't happen. Assert here and bail.
        // Leak the memory block instead.
        //
        if (nullptr == this->m_ListTail)
        {
            xpf::ApiPanic(STATUS_INVALID_STATE_TRANSITION);
            return;
        }

        //
        // Adjust the tail to point to the new element.
        //
        this->m_ListTail->Next = newEntry;
        this->m_ListTail = newEntry;

        //
        // We emplaced one element in the list.
        //
        xpf::ApiAtomicIncrement(&this->m_CurrentElements);
    }
}
