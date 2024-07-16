/**
 * @file        xpf_lib/private/Memory/LookasideListAllocator.cpp
 *
 * @brief       C++ -like memory allocator. This uses multiple lookaside lists
 *              so it will be friendlier with the memory, it will not abuse it
 *              and make it grow due to multiple smaller allocations.
 *              The logic is straightforward: have a lookaside list allocator
 *              for multiple blocks: 64b, 512b, 4096b, 32768b, 262144b
 *              when memory is requested, it finds the smallest allocator which
 *              can satisfy the request. If it's bigger than the largest one,
 *              it goes directly to the default pool. This will ease the
 *              pressure on the system.
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


namespace xpf
{
namespace SplitAlloc
{
/**
 * @brief   We need to know the size of the allocation when we free it
 *          so we can properly select the allocator from the list.
 *
 * @note    The memory layout will be [AllocationBlock][Allocation].
 */
struct AllocationBlock
{
    /**
     * @brief   Will store the original allocation size.
     */
    size_t AllocationSize = 0;

    /**
     * @brief   We need to keep allocations aligned.
     *          So we need to add some extra bytes.
     */
    size_t Padding = 0;
};

/**
 * @brief   This is the value that we are using to initialize the padding member.
 */
#define PADDING_VALUE       7893094

/**
 * @brief   This class holds the list of lookaside list allocators.
 */
class SplitLookasideGroup
{
 public:
/**
 * @brief       Constructor.
 *
 * @param[in]   IsCriticalAllocator - a boolean indicating that the allocators.
 *
 * @note        This can be improved when the need arise. For now this suffice.
 */
SplitLookasideGroup(
    _In_ bool IsCriticalAllocator
) noexcept(true) : m_IsCriticalAllocator{ IsCriticalAllocator },
                    m_Allocator64b{      size_t{ 64 }      + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator },
                    m_Allocator512b{     size_t{ 512 }     + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator },
                    m_Allocator4096b{    size_t{ 4096 }    + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator },
                    m_Allocator32768b{   size_t{ 32768 }   + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator },
                    m_Allocator262144b{  size_t{ 262144 }  + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator },
                    m_Allocator2097152b{ size_t{ 2097152 } + sizeof(xpf::SplitAlloc::AllocationBlock), IsCriticalAllocator }
{
    XPF_MAX_DISPATCH_LEVEL();
}

/**
 * @brief       Default destructor.
 */
~SplitLookasideGroup(void) noexcept(true) = default;

/**
 * @brief This class can not be moved or copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SplitLookasideGroup, delete);

/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize             - The requsted block size.
 *
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
inline void* XPF_API
Allocate(
    _In_ size_t BlockSize
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    /* We need to add a header to the allocation. */
    size_t requiredBytes = 0;
    if (!xpf::ApiNumbersSafeAdd(BlockSize, sizeof(xpf::SplitAlloc::AllocationBlock), &requiredBytes))
    {
        return nullptr;
    }

    /* Allocate based on size. */
    void* block = nullptr;
    if (BlockSize <= 64)
    {
        block = this->m_Allocator64b.AllocateMemory(requiredBytes);
    }
    else if (BlockSize <= 512)
    {
        block = this->m_Allocator512b.AllocateMemory(requiredBytes);
    }
    else if (BlockSize <= 4096)
    {
        block = this->m_Allocator4096b.AllocateMemory(requiredBytes);
    }
    else if (BlockSize <= 32768)
    {
        block = this->m_Allocator32768b.AllocateMemory(requiredBytes);
    }
    else if (BlockSize <= 262144)
    {
        block = this->m_Allocator262144b.AllocateMemory(requiredBytes);
    }
    else if (BlockSize <= 2097152)
    {
        block = this->m_Allocator2097152b.AllocateMemory(requiredBytes);
    }
    else
    {
        block = (this->m_IsCriticalAllocator) ? xpf::CriticalMemoryAllocator::AllocateMemory(requiredBytes)
                                              : xpf::MemoryAllocator::AllocateMemory(requiredBytes);
    }

    /* Prepend the header. */
    if (nullptr != block)
    {
        xpf::SplitAlloc::AllocationBlock* header = static_cast<xpf::SplitAlloc::AllocationBlock*>(block);
        header->AllocationSize = BlockSize;
        header->Padding = PADDING_VALUE;

        block = xpf::AlgoAddToPointer(block, sizeof(xpf::SplitAlloc::AllocationBlock));

        /* We should still send aligned memory back to the caller. */
        if (!xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(block), XPF_DEFAULT_ALIGNMENT))
        {
            XPF_DEATH_ON_FAILURE(false);
        }
    }

    return block;
}

/**
 * @brief Frees a block of memory.
 *
 * @param[in,out] MemoryBlock           - To be freed.
 *
 */
inline void XPF_API
Free(
    _Inout_ void* MemoryBlock
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    /* Can't free nulls... */
    if (nullptr == MemoryBlock)
    {
        return;
    }

    /* Grab the header. */
    void* allocationStart = static_cast<uint8_t*>(MemoryBlock) - sizeof(xpf::SplitAlloc::AllocationBlock);
    xpf::SplitAlloc::AllocationBlock* header = static_cast<xpf::SplitAlloc::AllocationBlock*>(allocationStart);

    /* Sanity check. */
    XPF_DEATH_ON_FAILURE(header->Padding == PADDING_VALUE);

    /* Grab the original block size. */
    const size_t blockSize = header->AllocationSize;

    /* Free based on original size. */
    if (blockSize <= 64)
    {
        this->m_Allocator64b.FreeMemory(allocationStart);
    }
    else if (blockSize <= 512)
    {
        this->m_Allocator512b.FreeMemory(allocationStart);
    }
    else if (blockSize <= 4096)
    {
        this->m_Allocator4096b.FreeMemory(allocationStart);
    }
    else if (blockSize <= 32768)
    {
        this->m_Allocator32768b.FreeMemory(allocationStart);
    }
    else if (blockSize <= 262144)
    {
        this->m_Allocator262144b.FreeMemory(allocationStart);
    }
    else if (blockSize <= 2097152)
    {
        this->m_Allocator2097152b.FreeMemory(allocationStart);
    }
    else
    {
        if (this->m_IsCriticalAllocator)
        {
            xpf::CriticalMemoryAllocator::FreeMemory(allocationStart);
        }
        else
        {
            xpf::MemoryAllocator::FreeMemory(allocationStart);
        }
    }
}

 private:
    bool m_IsCriticalAllocator = false;
    xpf::LookasideListAllocator m_Allocator64b;
    xpf::LookasideListAllocator m_Allocator512b;
    xpf::LookasideListAllocator m_Allocator4096b;
    xpf::LookasideListAllocator m_Allocator32768b;
    xpf::LookasideListAllocator m_Allocator262144b;
    xpf::LookasideListAllocator m_Allocator2097152b;
};  // class SplitLookasideGroup
};  // namespace SplitAlloc
};  // namespace xpf

/**
 * @brief   This instance corresponds to non-critical split lookaside allocator.
 *          For Non-CRT environments (such as windows kernel),
 *          see xpf::InitializeCppSupport to see how you can init these.
 */
static xpf::SplitAlloc::SplitLookasideGroup gNonCriticalSplitAllocator{ false };

/**
 * @brief   This instance corresponds to critical split lookaside allocator.
 *          For Non-CRT environments (such as windows kernel),
 *          see xpf::InitializeCppSupport to see how you can init these.
 */
static xpf::SplitAlloc::SplitLookasideGroup gCriticalSplitAllocator{ true };


_Use_decl_annotations_
void* XPF_API
xpf::SplitAllocatorAllocate(
    _In_ size_t BlockSize,
    _In_ bool CriticalAllocation
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    if (CriticalAllocation)
    {
        return gCriticalSplitAllocator.Allocate(BlockSize);
    }
    else
    {
        return gNonCriticalSplitAllocator.Allocate(BlockSize);
    }
}

void XPF_API
xpf::SplitAllocatorFree(
    _Inout_ void* MemoryBlock,
    _In_ bool CriticalAllocation
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    if (CriticalAllocation)
    {
        gCriticalSplitAllocator.Free(MemoryBlock);
    }
    else
    {
        gNonCriticalSplitAllocator.Free(MemoryBlock);
    }
}
