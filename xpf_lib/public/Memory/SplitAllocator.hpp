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
 * @brief   SplitAllocator requires some extra initialization.
 *          It is the caller responsibility to ensure this is called
 *          before using the split allocator.
 *
 * @return  A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS XPF_API
SplitAllocatorInitializeSupport(
    void
) noexcept(true);

/**
 * @brief   SplitAllocator requires some extra steps to properly clean up.
 *          It is the caller responsibility to ensure this is called
 *          at the end.
 *
 * @return  Nothing.
 */
void XPF_API
SplitAllocatorDeinitializeSupport(
    void
) noexcept(true);

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
template <bool IsCritical>
class SplitAllocatorTemplate
{
 public:
/**
 * @brief   Default constructor.
 */
SplitAllocatorTemplate(void) noexcept(true) = default;

/**
 * @brief   Default destructor.
 */
~SplitAllocatorTemplate(void) noexcept(true) = default;

/**
 * @brief   This class can be moved and copied.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SplitAllocatorTemplate, default);

/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize - The requsted block size.
 * 
 * @return A block of memory with the required size, or null on failure.
 */
_Check_return_
_Ret_maybenull_
inline void*
AllocateMemory(
    _In_ size_t BlockSize
) noexcept(true)
{
    return xpf::SplitAllocatorAllocate(BlockSize,
                                       IsCritical);
}

/**
* @brief Frees a block of memory.
* 
* @param[in,out] MemoryBlock - To be freed.
* 
*/
inline void
FreeMemory(
    _Inout_ void* MemoryBlock
) noexcept(true)
{
    xpf::SplitAllocatorFree(MemoryBlock,
                            IsCritical);
}
};  // class SplitAllocatorTemlate

/**
 * @brief   Ease of life for using the non-critical split allocator.
 */
using SplitAllocator = xpf::SplitAllocatorTemplate<false>;

/**
 * @brief   Ease of life for using the critical split allocator.
 */
using SplitAllocatorCritical = xpf::SplitAllocatorTemplate<true>;
};  // namespace xpf
