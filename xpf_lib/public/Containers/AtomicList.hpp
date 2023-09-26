/**
 * @file        xpf_lib/public/Containers/AtomicList.hpp
 *
 * @brief       C-ish like list which provides atomic functionality (by using Atomic operations)
 *              to operations like insert head/ and flush
 *              It's more like C because it's not like the c++ list class.
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
#include "xpf_lib/public/Memory/CompressedPair.hpp"



namespace xpf
{
//
// ************************************************************************************************
// This is the section containing Atomic List implementation.
// ************************************************************************************************
//

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
 * @brief This class does not maintain the element state.
 *        The caller is responsible for allocation and freeing these elements.
 *        This list is intended to be a light-weight way of "atomically" enqueuing elements.
 *        It should be ok as the operations in this class are just pointer assignments.
 *        We can come back to it later if we notice any problems.
 * 
 *        If you need to support dequeue, please use TwoLockQueue implementation.
 *        That should provide a fast algorithm for accessing the elements safely.
 * 
 * @note See the unit tests for the usage of this list.
 */
class AtomicList final
{
 public:
/**
 * @brief Default constructor
 */
AtomicList(
    void
) noexcept(true) = default;

/**
 * @brief Default destructor.
 *        It is the caller responsibility to free the elements in this list.
 *        Assert here on debug.
 */
~AtomicList(
    void
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(this->m_ListHead == nullptr);
}

/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
AtomicList(
    _In_ _Const_ const AtomicList& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
AtomicList(
    _Inout_ AtomicList&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
AtomicList&
operator=(
    _In_ _Const_ const AtomicList& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
AtomicList&
operator=(
    _Inout_ AtomicList&& Other
) noexcept(true) = delete;

/**
 * @brief Checks if the underlying list size is 0.
 *
 * @return true if the buffer size is 0,
 *         false otherwise.
 */
inline bool
IsEmpty(
    void
) const noexcept(true)
{
    return (this->m_ListHead == nullptr);
}

/**
 * @brief This method is used to insert a node.
 *        No operation is performed if no more items can be inserted.
 * 
 *        We don't support the unlink method as that requires head->Next touching.
 *        We can't guarantee that the head was not erased and we are touching garbage memory.
 * 
 *        We only support insert + flush for now. We might want to switch to a busy lock at some point
 *        And extend the functionality if we notice the need. For now it is ok.
 *
 * @param[in, out] Node - the element to be inserted, the links are updated.
 * 
 * @note It is the caller responsibility to ensure that the node is not in the list already.
 */
inline void
Insert(
    _Inout_opt_ XPF_SINGLE_LIST_ENTRY* Node
) noexcept(true)
{
    //
    // Check the invariants - we can't insert a null node
    //
    if (nullptr == Node)
    {
        return;
    }

    while (true)
    {
        //
        // Node will be the new head. Properly set the link.
        //
        auto currentHead = this->Head();
        Node->Next = currentHead;

        //
        // Set the new list head.
        //
        auto actualHead = xpf::ApiAtomicCompareExchangePointer(reinterpret_cast<void* volatile*>(&this->m_ListHead),
                                                               reinterpret_cast<void*>(Node),
                                                               reinterpret_cast<void*>(currentHead));
        //
        // On success, we stop - xpf::ApiAtomicCompareExchangePointer retrieves the original value.
        // If that is what we used as "current", we stop.
        //
        if (actualHead == currentHead)
        {
            break;
        }
    }
}

/**
 * @brief This method is used to flush the list.
 *        The list will become empty after this.
 *
 * @param[out] Elements** - A pointer which will receive the list head.
 */
inline void
Flush(
    _Out_opt_ XPF_SINGLE_LIST_ENTRY** Elements
) noexcept(true)
{
    XPF_SINGLE_LIST_ENTRY* currentHeadNode = nullptr;

    while (true)
    {
        //
        // Set the new head on nullptr.
        //
        currentHeadNode = this->Head();
        auto actualHead = xpf::ApiAtomicCompareExchangePointer(reinterpret_cast<void* volatile*>(&this->m_ListHead),
                                                               nullptr,
                                                               reinterpret_cast<void*>(currentHeadNode));
        //
        // On success, we stop - xpf::ApiAtomicCompareExchangePointer retrieves the original value.
        // If that is what we used as "current", we stop.
        //
        if (actualHead == currentHeadNode)
        {
            break;
        }
    }

    if (nullptr != Elements)
    {
        *Elements = currentHeadNode;
    }
}

/**
 * @brief This method is used to retrieve the head.
 *        Can be changed so use with caution!
 *
 * @return a pointer to the list head.
 * 
 * @note This method is intended for letting the caller walk the list
 *       without actually flushing it. It is then the caller responsibility
 *       to ensure the list won't be modified!
 */
inline
XPF_SINGLE_LIST_ENTRY*
Head(
    void
) noexcept(true)
{
    auto crtHead = xpf::ApiAtomicCompareExchangePointer(reinterpret_cast<void* volatile*>(&this->m_ListHead),
                                                        nullptr,
                                                        nullptr);
    return reinterpret_cast<XPF_SINGLE_LIST_ENTRY*>(crtHead);
}

 private:
     XPF_SINGLE_LIST_ENTRY* m_ListHead = nullptr;
};  // namespace AtomicList
};  // namespace xpf
