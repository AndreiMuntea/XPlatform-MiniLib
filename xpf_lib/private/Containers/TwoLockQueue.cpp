/**
 * @file        xpf_lib/private/Containers/TwoLockQueue.cpp
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

#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into default section.
 */
XPF_SECTION_DEFAULT;

void
XPF_API
xpf::TlqPush(
    _Inout_ TwoLockQueue& Queue,
    _Inout_ XPF_SINGLE_LIST_ENTRY* Element
) noexcept(true)
{
    //
    // We can't insert a null element.
    //
    if (nullptr == Element)
    {
        return;
    }
    Element->Next = nullptr;

    //
    // We usually get away with the tail lock only.
    // Except when the queue is empty (case handled below).
    //
    {
        xpf::ExclusiveLockGuard tailGuard{ Queue.TailLock };
        if (nullptr != Queue.Tail)
        {
            Queue.Tail->Next = Element;
            Queue.Tail = Element;
            return;
        }
    }

    //
    // This is the first inserted element,
    // so we need to modify the head as well.
    //
    {
        xpf::ExclusiveLockGuard headGuard{ Queue.HeadLock };
        xpf::ExclusiveLockGuard tailGuard{ Queue.TailLock };

        //
        // Might have raced so check again.
        //
        if (nullptr != Queue.Tail)
        {
            XPF_ASSERT(nullptr != Queue.Head);

            Queue.Tail->Next = Element;
            Queue.Tail = Element;
        }
        else
        {
            XPF_ASSERT(nullptr == Queue.Head);

            Queue.Head = Element;
            Queue.Tail = Element;
        }
    }
}

xpf::XPF_SINGLE_LIST_ENTRY*
XPF_API
xpf::TlqPop(
    _Inout_ TwoLockQueue& Queue
) noexcept(true)
{
    //
    // We usually get away with the head lock only.
    // Except when the last element is removed (checked below).
    //
    {
        xpf::ExclusiveLockGuard headGuard{ Queue.HeadLock };
        if (nullptr == Queue.Head)
        {
            return nullptr;
        }
        else if (Queue.Head->Next != nullptr)
        {
            xpf::XPF_SINGLE_LIST_ENTRY* removedElement = Queue.Head;
            Queue.Head = Queue.Head->Next;

            removedElement->Next = nullptr;
            return removedElement;
        }
    }

    //
    // The last element is removed, so we need to update tail as well.
    //
    {
        xpf::ExclusiveLockGuard headGuard{ Queue.HeadLock };
        xpf::ExclusiveLockGuard tailGuard{ Queue.TailLock };

        //
        // Might have raced so check again.
        //
        if (nullptr == Queue.Head)
        {
            return nullptr;
        }
        else
        {
            xpf::XPF_SINGLE_LIST_ENTRY* removedElement = Queue.Head;

            if (nullptr == Queue.Head->Next)
            {
                XPF_ASSERT(Queue.Head == Queue.Tail);

                Queue.Tail = nullptr;
                Queue.Head = nullptr;
            }
            else
            {
                Queue.Head = Queue.Head->Next;
            }

            removedElement->Next = nullptr;
            return removedElement;
        }
    }
}

xpf::XPF_SINGLE_LIST_ENTRY*
XPF_API
xpf::TlqFlush(
    _Inout_ TwoLockQueue& Queue
) noexcept(true)
{
    //
    // On flush, we need to grab both locks always.
    //
    xpf::ExclusiveLockGuard headGuard{ Queue.HeadLock };
    xpf::ExclusiveLockGuard tailGuard{ Queue.TailLock };

    xpf::XPF_SINGLE_LIST_ENTRY* head = Queue.Head;

    Queue.Head = nullptr;
    Queue.Tail = nullptr;

    return head;
}
