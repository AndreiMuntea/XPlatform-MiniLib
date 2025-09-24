/**
 * @file        xpf_lib/public/Containers/TwoLockQueue.hpp
 *
 * @brief       C-like struct which provides functionality for an atomic container.
 *              It uses busy lock as the only operations are linking / unlinking.
 *              It shouldn't cause contention, as the only race that may happen is during
 *              insertion / deletion of the first element when both locks are taken.
 *              Or during flush which requires both locks as well.
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

#include "xpf_lib/public/Locks/BusyLock.hpp"

namespace xpf
{

/**
 * @brief This structure MUST be present inside all Types
 *        that use the Atomic list. It resembles the LIST_ENTRY with one entry.
 * 
 * @note Because this relies on atomic intrinsics, we need this to be aligned
 *       properly - so we don't cause undefined behavior. Align to XPF_DEFAULT_ALIGNMENT.
 */
typedef struct alignas(XPF_DEFAULT_ALIGNMENT) _XPF_SINGLE_LIST_ENTRY
{
    /**
     * @brief Pointer to the next element in the list.
     */
    alignas(XPF_DEFAULT_ALIGNMENT) struct _XPF_SINGLE_LIST_ENTRY* Next;
}XPF_SINGLE_LIST_ENTRY;

/**
 * @brief The two-lock queue is a Live-lock free structure presented in  "Simple, Fast, and Practical
 *        Non-Blocking and Blocking Concurrent Queue Algorithms" by Maged M. Michael and Michael L. Scott.
 *        (http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)
 *
 * @note  The algorithm is slightly adjusted as the main implementation requires at least a sentinel
 *        to be present in the two lock queue. This will handle this scenario by acquiring both locks.
 *        It brings a bit of overhead, but we won't be limited to having at least one sentinel.
 */
struct TwoLockQueue final
{
    /**
     * @brief This lock is responsible for synchronizing Head access.
     *        We always pop at head. 
     */
    xpf::BusyLock HeadLock;
    /**
     * @brief This points to the first element (the oldest) from queue.
     */
    XPF_SINGLE_LIST_ENTRY* Head = nullptr;
    /**
     * @brief This lock is responsible for synchronizing Tail access.
     *        We always insert at tail. 
     */
    xpf::BusyLock TailLock;
    /**
     * @brief This points to the last element (the newest) from queue.
     */
    XPF_SINGLE_LIST_ENTRY* Tail = nullptr;
};  // struct TwoLockQueue

/**
 * @brief Inserts an element in the given Two Lock Queue.
 *        The element will be inserted at the tail of the queue.
 *
 * @param[in,out] Queue - The queue where the element should be inserted.
 *
 * @param[in,out] Element - The element to be inserted.
 *
 * @return void.
 *
 * @note This function can not fail.
 */
void
XPF_API
TlqPush(
    _Inout_ TwoLockQueue& Queue,                                                // NOLINT(runtime/references)
    _Inout_ XPF_SINGLE_LIST_ENTRY* Element
) noexcept(true);

/**
 * @brief Removes an element in the given Two Lock Queue.
 *        The element will be removed from the head of the queue.
 *
 * @param[in,out] Queue - The queue where we should pop from.
 *
 * @returns The last entry removed from the queue.
 *          NULL if the queue was empty.
 */
XPF_SINGLE_LIST_ENTRY*
XPF_API
TlqPop(
    _Inout_ TwoLockQueue& Queue                                                 // NOLINT(runtime/references)
) noexcept(true);

/**
 * @brief Clears the list and returns a pointer to the first element.
 *        This will make both head and tail be NULL.
 *
 * @param[in,out] Queue - The queue where we should free from.
 *
 * @returns The list structure will be preserved.
 *          This returns a pointer to the old head of queue.
 *          It can be used to walk the list.
 */
XPF_SINGLE_LIST_ENTRY*
XPF_API
TlqFlush(
    _Inout_ TwoLockQueue& Queue                                                 // NOLINT(runtime/references)
) noexcept(true);
};  // namespace xpf
