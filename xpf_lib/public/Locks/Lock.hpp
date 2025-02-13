/**
 * @file        xpf_lib/public/Locks/Lock.hpp
 *
 * @brief       Generic lock and lockguards.
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


namespace xpf
{

//
// ************************************************************************************************
// This is the section containing ExclusiveLock base class.
// ************************************************************************************************
//

/**
 * @brief This is the base class for an exclusive lock.
 *        All other lock classes must inherit this one.
 */
class ExclusiveLock
{
 public:
/**
 * @brief ExclusiveLock constructor - default.
 */
ExclusiveLock(
    void
) noexcept(true) = default;

/**
 * @brief Default destructor.
 */
virtual ~ExclusiveLock(
    void
) noexcept(true) = default;

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ExclusiveLock, delete);

/**
 * @brief Acquires the lock exclusive granting read-write access.
 *
 * @return None.
 */
virtual void
XPF_API
LockExclusive(
    void
) noexcept(true) = 0;

/**
 * @brief Unlocks the previously acquired exclusive lock.
 * 
 * @return None.
 */
virtual void
XPF_API
UnLockExclusive(
    void
) noexcept(true) = 0;
};  // class ExclusiveLock

//
// ************************************************************************************************
// This is the section containing SharedLock base class.
// ************************************************************************************************
//

/**
 * @brief This is the base class for a shared lock.
 *        All other shared lock classes must inherit this one.
 */
class SharedLock : public virtual ExclusiveLock
{
 public:
/**
 * @brief SharedLock constructor - default.
 */
SharedLock(
    void
) noexcept(true) = default;

/**
 * @brief Default destructor.
 */
virtual ~SharedLock(
    void
) noexcept(true)
{
    XPF_NOTHING();
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SharedLock, delete);

/**
 * @brief Acquires the lock shared granting read access.
 * 
 * @return None.
 */
virtual void
XPF_API
LockShared(
    void
) noexcept(true) = 0;

/**
 * @brief Unlocks the previously acquired shared lock.
 * 
 * @return None.
 */
virtual void
XPF_API
UnLockShared(
    void
) noexcept(true) = 0;
};  // class SharedLock

//
// ************************************************************************************************
// This is the section containing Exclusive Lock Guard class.
// ************************************************************************************************
//

/**
 * @brief This is the base class for an exclusive lock guard.
 *        On constructor this class acquires the lock exclusively.
 *        On destructor this will release the lock.
 */
class ExclusiveLockGuard final
{
 public:
/**
 * @brief ExclusiveLockGuard constructor.
 *        Acquires the provided lock.
 * 
 * @param[in] Lock - A reference to the lock to be acquired exclusively.
 *                   Must remain valid until ExclusiveLockGuard is destroyed!
 */
ExclusiveLockGuard(
    _Inout_ ExclusiveLock& Lock
) noexcept(true) : m_ExclusiveLock{Lock}
{
    //
    // First we enter the critical region (if needed).
    //
    #if defined XPF_PLATFORM_WIN_KM
        if (::KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
            ::KeEnterCriticalRegion();
            this->m_IsInsideCriticalRegion = true;
        }
    #endif  // XPF_PLATFORM_WIN_KM

    m_ExclusiveLock.LockExclusive();
}

/**
 * @brief ExclusiveLockGuard destructor.
 *        Releses the previously taken lock.
 */
~ExclusiveLockGuard(
    void
) noexcept(true)
{
    m_ExclusiveLock.UnLockExclusive();

    //
    // And now exit the critical region.
    //
    #if defined XPF_PLATFORM_WIN_KM
        if (this->m_IsInsideCriticalRegion)
        {
            ::KeLeaveCriticalRegion();
            this->m_IsInsideCriticalRegion = false;
        }
    #endif
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ExclusiveLockGuard, delete);

 private:
     ExclusiveLock& m_ExclusiveLock;

    /**
     * @brief   On windows kernel, we'll also enter a critical region during lock acquisition.
     */
    #if defined XPF_PLATFORM_WIN_KM
        bool m_IsInsideCriticalRegion = false;
    #endif  // XPF_PLATFORM_WIN_KM
};  // class ExclusiveLockGuard

//
// ************************************************************************************************
// This is the section containing Shared Lock Guard class.
// ************************************************************************************************
//

/**
 * @brief This is the base class for a shared lock guard.
 *        On constructor this class acquires the lock shared.
 *        On destructor this will release the lock.
 */
class SharedLockGuard final
{
 public:
/**
 * @brief SharedLockGuard constructor.
 *        Acquires the provided lock.
 * 
 * @param[in] Lock - A reference to the lock to be acquired shared.
 *                   Must remain valid until SharedLockGuard is destroyed!
 */
SharedLockGuard(
    _Inout_ SharedLock& Lock
) noexcept(true) : m_SharedLock{Lock}
{
    //
    // First we enter the critical region (if needed).
    //
    #if defined XPF_PLATFORM_WIN_KM
        if (::KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
            ::KeEnterCriticalRegion();
            this->m_IsInsideCriticalRegion = true;
        }
    #endif  // XPF_PLATFORM_WIN_KM

    m_SharedLock.LockShared();
}

/**
 * @brief SharedLockGuard destructor.
 *        Releses the previously taken lock.
 */
~SharedLockGuard(
    void
) noexcept(true)
{
    m_SharedLock.UnLockShared();

    //
    // And now exit the critical region.
    //
    #if defined XPF_PLATFORM_WIN_KM
        if (this->m_IsInsideCriticalRegion)
        {
            ::KeLeaveCriticalRegion();
            this->m_IsInsideCriticalRegion = false;
        }
    #endif
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(SharedLockGuard, delete);

 private:
     SharedLock& m_SharedLock;

    /**
     * @brief   On windows kernel, we'll also enter a critical region during lock acquisition.
     */
    #if defined XPF_PLATFORM_WIN_KM
        bool m_IsInsideCriticalRegion = false;
    #endif  // XPF_PLATFORM_WIN_KM
};  // class SharedLockGuard
};  // namespace xpf
