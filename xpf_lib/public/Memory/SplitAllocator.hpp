/**
 * @file        xpf_lib/public/Memory/SplitAllocator.hpp
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
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2024.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#pragma once

#include "xpf_lib/public/Memory/LookasideListAllocator.hpp"

namespace xpf
{
/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize             - The requsted block size.
 * @param[in] CriticalAllocation    - A boolean indicating whether the
 *                                    allocation is considered critical or not.
 *
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
void* XPF_API
SplitAllocatorAllocate(
    _In_ size_t BlockSize,
    _In_ bool CriticalAllocation
) noexcept(true);

/**
 * @brief Frees a block of memory.
 *
 * @param[in,out] MemoryBlock           - To be freed.
 * @param[in]     CriticalAllocation    - A boolean indicating whether the
 *                                        allocation is considered critical or not.
 *
 * @return Nothing.
 */
void XPF_API
SplitAllocatorFree(
    _Inout_ void* MemoryBlock,
    _In_ bool CriticalAllocation
) noexcept(true);

/**
 * @brief   A convenience class which wraps C-APIs
 */
class SplitAllocator
{
 public:
/**
 * @brief   Default constructor.
 */
     SplitAllocator(void) noexcept(true) = default;

/**
 * @brief   Default destructor.
 */
~SplitAllocator(void) noexcept(true) = default;

/**
 * @brief   This class can be moved and copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SplitAllocator, default);

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
    return xpf::SplitAllocatorAllocate(BlockSize,
                                       false);
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
    xpf::SplitAllocatorFree(MemoryBlock,
                            false);
}
};  // class SplitAllocator


/**
 * @brief   A convenience class which wraps C-APIs
 *          for critical allocations
 */
class SplitAllocatorCritical
{
 public:
/**
 * @brief   Default constructor.
 */
SplitAllocatorCritical(void) noexcept(true) = default;

/**
 * @brief   Default destructor.
 */
~SplitAllocatorCritical(void) noexcept(true) = default;

/**
 * @brief   This class can be moved and copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SplitAllocatorCritical, default);

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
    return xpf::SplitAllocatorAllocate(BlockSize,
                                       true);
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
    xpf::SplitAllocatorFree(MemoryBlock,
                            true);
}
};  // class SplitAllocator
};  // namespace xpf
