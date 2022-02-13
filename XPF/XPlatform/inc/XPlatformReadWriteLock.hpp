/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_READ_WRITE_LOCK_HPP__
#define __XPLATFORM_READ_WRITE_LOCK_HPP__

//
// This file contains Read-Write Lock implementation
//

namespace XPF
{
    class ReadWriteLock : public SharedLock
    {
        ReadWriteLock() noexcept : SharedLock()
        {
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                InitializeSRWLock(&this->mutex);
                this->state = LockState::Unlocked;
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                if(NT_SUCCESS(ExInitializeResourceLite(&this->mutex)))
                {
                    this->state = LockState::Unlocked;
                }
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                if(0 == pthread_rwlock_init(&this->mutex, nullptr))
                {
                    this->state = LockState::Unlocked;
                }
            #else
                #error Unsupported Platform
            #endif
        }
        virtual ~ReadWriteLock() noexcept
        {
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            if(this->state != LockState::Uninitialized)
            {
                ///
                /// This helps a lot during debugging
                /// Tries to esnure that this lock was properly released from all threads
                /// 
                LockExclusive();
                UnlockExclusive();

                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    this->mutex = SRWLOCK_INIT;
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    (void) ExDeleteResourceLite(&this->mutex);
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    (void) pthread_rwlock_destroy(&this->mutex);
                #else
                    #error Unsupported Platform
                #endif

                XPF::ApiZeroMemory(XPF::AddressOf(this->mutex), sizeof(this->mutex));
                this->state = LockState::Uninitialized;
            }
        }

        // Copy semantics -- deleted (We can't copy a lock)
        ReadWriteLock(const ReadWriteLock& Other) noexcept = delete;
        ReadWriteLock& operator=(const ReadWriteLock& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ReadWriteLock(ReadWriteLock&& Other) noexcept = delete;
        ReadWriteLock& operator=(ReadWriteLock&& Other) noexcept = delete;

        _Acquires_exclusive_lock_(this->mutex)
        virtual void LockExclusive(void) noexcept override
        {
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            // Don't try to acquire an uninitialized lock
            if (this->state != LockState::Uninitialized)
            {
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    AcquireSRWLockExclusive(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // ExAcquireResourceExclusiveLite returns TRUE if the resource is acquired.
                    // This routine returns FALSE if the input Wait is FALSE and exclusive access cannot be granted immediately.
                    // Safe to ignore return value.
                    //
                    KeEnterCriticalRegion();
                    (void)ExAcquireResourceExclusiveLite(&this->mutex, TRUE);

                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    //
                    // The read-write lock could not be acquired for writing because it was already locked for reading or writing.
                    // We spin here until we can get this lock.
                    // Possible return values:
                    //      * [EBUSY]      The read - write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                    //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                    //      * [EDEADLK]    The current thread already owns the read - write lock for writing or reading.
                    //
                    while(0 != pthread_rwlock_wrlock(&this->mutex));
                #else
                    #error Unsupported Platform
                #endif

                this->state = LockState::AcquiredExclusive;
            }
        }

        _Requires_exclusive_lock_held_(this->mutex)
        _Releases_exclusive_lock_(this->mutex)
        virtual void UnlockExclusive(void) noexcept override
        {
            // Extra safety to early catch AcquireShared() -> ReleaseExclusive() bugs
            XPLATFORM_ASSERT(this->state == LockState::AcquiredExclusive);

            // Don't try to release an uninitialized lock
            if (this->state != LockState::Uninitialized)
            { 
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    ReleaseSRWLockExclusive(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    ExReleaseResourceLite(&this->mutex);
                    KeLeaveCriticalRegion();
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    //
                    // Safe to ignore errors here
                    // Possible return values:
                    //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                    //      * [EPERM]      The current thread does not own the read-write lock.
                    //
                    (void) pthread_rwlock_unlock(&this->mutex);
                #else
                    #error Unsupported Platform
                #endif

                this->state = LockState::Unlocked;
            }
        }

        _Acquires_shared_lock_(this->mutex)
        virtual void LockShared(void) noexcept override
        {
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            // Don't try to lock an uninitialized lock
            if (this->state != LockState::Uninitialized)
            {
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    AcquireSRWLockShared(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // ExAcquireResourceSharedLite returns TRUE if the resource is acquired.
                    // This routine returns FALSE if the input Wait is FALSE shared exclusive access cannot be granted immediately.
                    // Safe to ignore return value.
                    //
                    KeEnterCriticalRegion();
                    (void) ExAcquireResourceSharedLite(&this->mutex, TRUE);
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    //
                    // The read-write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                    // We spin here until we can get this lock.
                    // Possible return values:
                    //      * [EBUSY]      The read - write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                    //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                    //      * [EDEADLK]    The current thread already owns the read - write lock for writing or reading.
                    //
                    while (0 != pthread_rwlock_rdlock(&this->mutex));
                #else
                    #error Unsupported Platform
                #endif
            }
        }

        _Requires_shared_lock_held_(this->mutex)
        _Releases_shared_lock_(this->mutex)
        virtual void UnlockShared(void) noexcept override
        {
            // Extra safety to early catch AcquireExclusive() -> ReleaseShared() bugs
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);
            XPLATFORM_ASSERT(this->state != LockState::AcquiredExclusive);

            // Don't try to unlock an uninitialized lock
            if (this->state != LockState::Uninitialized)
            {
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    ReleaseSRWLockShared(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    ExReleaseResourceLite(&this->mutex);
                    KeLeaveCriticalRegion();
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    //
                    // Safe to ignore errors here
                    // Possible return values:
                    //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                    //      * [EPERM]      The current thread does not own the read-write lock.
                    //
                    (void) pthread_rwlock_unlock(&this->mutex);
                #else
                    #error Unsupported Platform
                #endif
            }
        }

    private:
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_BEGIN
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) SRWLOCK mutex = { 0 };
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) ERESOURCE mutex = { 0 };
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) pthread_rwlock_t mutex{ };
            #else
                #error Unsupported Platform
            #endif
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_END
    };
}


#endif // __XPLATFORM_READ_WRITE_LOCK_HPP__
