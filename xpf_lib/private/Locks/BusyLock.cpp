/**
 * @file        xpf_lib/private/Locks/BusyLock.cpp
 *
 * @brief       This is a custom simple spinlock implementation
 *              which will allow shared access or exclusive access to a resource.
 *              It uses an 16-bit value to allow read/write access.
 *              Its purpose is to be used internally to ensure corectness in data races.
 *              The paths where this is used are critical paths and SHOULD NOT be under
 *              heavy contention.
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
 *          The busy lock can be called at any IRQL. We don't impose
 *          restriction on it.
 */
XPF_SECTION_DEFAULT;


void
XPF_API
xpf::BusyLock::LockExclusive(
    void
) noexcept(true)
{
    while (true)
    {
        //
        // Read the m_Lock by removing the writer bit.
        // This is important as this is the comparison value where we are doing the atomic op below.
        // If a writer already has the lock, we won't be able to acquire it.
        //
        const uint16_t oldLock = (this->m_Lock & 0x7fff);

        //
        // Set the writer bit and try to acquire the lock similar with "read" access.
        //
        const uint16_t newLock = (oldLock | 0x8000);
        if (oldLock != xpf::ApiAtomicCompareExchange(&this->m_Lock, newLock, oldLock))
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // We successfully acquired the lock shared, so now we simply wait until all readers are done.
        // Then we will be the only one with access. Setting the writer bit will prevent any other
        // reader or writer to acquire the lock in the meantime.
        //
        while ((this->m_Lock & 0x7fff) != 0)
        {
            xpf::ApiYieldProcesor();
        }

        //
        // Got exclusive access.
        //
        break;
    }
}

void
XPF_API
xpf::BusyLock::UnLockExclusive(
    void
) noexcept(true)
{
    while (true)
    {
        //
        // Another value than 0x8000 means the lock is not taken exclusively.
        // Assert here then go into an infinite cycle as this is an invalid usage.
        // It is not safe to recover.
        //
        const uint16_t oldLock = this->m_Lock;
        if (oldLock != 0x8000)
        {
            xpf::ApiPanic(STATUS_MUTANT_NOT_OWNED);
            break;
        }

        //
        // Set the lock to 0. We should be the only owner, and another access is prohibited.
        // If not, assert here and investigate what happened.
        //
        const uint16_t newLock = 0;
        if (oldLock != xpf::ApiAtomicCompareExchange(&this->m_Lock, newLock, oldLock))
        {
            xpf::ApiPanic(STATUS_MUTANT_NOT_OWNED);
            break;
        }

        //
        // We successfully released the lock exclusive.
        //
        break;
    }
}

void
XPF_API
xpf::BusyLock::LockShared(
    void
) noexcept(true)
{
    while (true)
    {
        //
        // Read the m_Lock by removing the writer bit
        // (this is important as we don't want to acquire the lock shared
        // if it was already acquired exclusive).
        //
        // A corner case is when we have max concurrent readers.
        // Should never happen but spin if we reach 2 ^ 15 concurrent readers.
        //
        const uint16_t oldLock = (this->m_Lock & 0x7fff);
        if (oldLock == 0x7fff)
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // Increment the number of shared readers and update the lock.
        // If someone managed to change the lock value, spin and try again.
        //
        const uint16_t newLock = (oldLock + 1);
        if (oldLock != xpf::ApiAtomicCompareExchange(&this->m_Lock, newLock, oldLock))
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // We successfully acquired the lock shared.
        //
        break;
    }
}

void
XPF_API
xpf::BusyLock::UnLockShared(
    void
) noexcept(true)
{
    while (true)
    {
        //
        // 0 value means the lock is not taken. This is an invalid usage.
        //
        const uint16_t oldLock = this->m_Lock;
        if (oldLock == 0)
        {
            xpf::ApiPanic(STATUS_MUTANT_NOT_OWNED);
            break;
        }

        //
        // Decrement the number of shared readers and update the lock.
        // If someone managed to change the lock value, spin and try again.
        //
        const uint16_t newLock = (oldLock - 1);
        if (oldLock != xpf::ApiAtomicCompareExchange(&this->m_Lock, newLock, oldLock))
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // We successfully released the lock shared.
        //
        break;
    }
}
