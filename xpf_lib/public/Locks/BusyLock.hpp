/**
 * @file        xpf_lib/public/Locks/BusyLock.hpp
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


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/Locks/Lock.hpp"


namespace xpf
{
/**
 * @brief The spinlock implementation will allow shared access or exclusive access to a resource.
 *        It uses an 16-bit value to allow read/write access.
 * 
 *        Because only one thread is allowed to have exclusive access, the most significat bit will be reserved:
 *        |W|RRRRRRR RRRRRRRR
 * 
 *        This means that there can be up to 2 ^ 15 readers at the same time.
 */
class BusyLock final : public virtual SharedLock
{
 public:
/**
 * @brief BusyLock constructor - default.
 */
BusyLock(
    void
) noexcept(true) = default;

/**
 * @brief Default destructor.
 */
virtual ~BusyLock(
    void
) noexcept(true)
{
    //
    // The lock must be freed. So lock here exclusive and release to ensure.
    // This is a good way to catch hanging threads.
    //
    this->LockExclusive();
    this->UnLockExclusive();
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(BusyLock, delete);

/**
 * @brief Acquires the lock exclusive granting read-write access.
 *
 * @return None.
 * 
 * @note If the lock is already taken exclusively,
 *       this will spin until it can be acquired exclusively.
 */
void
XPF_API
LockExclusive(
    void
) noexcept(true) override;

/**
 * @brief Unlocks the previously acquired exclusive lock.
 *
 * @return None.
 */
void
XPF_API
UnLockExclusive(
    void
) noexcept(true) override;

/**
 * @brief Acquires the lock shared granting read access.
 *
 * @note If the lock is already taken exclusively,
 *       this will spin until it can be acquired shared.
 *
 * @note If someone requested an exclusive access,
 *       this will spin until that request can be satisfied and lock will be released again.
 *
 * @return None.
 *
 * @note Max concurrent readers are 2^15. If this number is reached, this will spin until someone releases the lock.
 */
void
XPF_API
LockShared(
    void
) noexcept(true) override;

/**
 * @brief Unlocks the previously acquired shared lock.
 *
 * @return None.
 */
void
XPF_API
UnLockShared(
    void
) noexcept(true) override;

 private:
    //
    // Because we are using atomic intrinsics, this value need to be properly aligned.
    //
    alignas(uint16_t) volatile uint16_t m_Lock = 0;
};  // class BusyLock
};  // namespace xpf
