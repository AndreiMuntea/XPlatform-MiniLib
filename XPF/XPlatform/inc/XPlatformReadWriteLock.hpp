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
    public:
        _IRQL_requires_max_(APC_LEVEL)
        ReadWriteLock() noexcept : SharedLock()
        {
            ///
            /// Platform specific API to initialize a read-write lock.
            /// If somethin will go bad, the lock state will not be Unlocked, it will remain Uninitialized.
            /// One should always inspect the lock state before using it.
            /// 
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                InitializeSRWLock(&this->mutex);
                this->state = LockState::Unlocked;
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
                auto status = ExInitializeResourceLite(&this->mutex);
                XPLATFORM_ASSERT(NT_SUCCESS(status));
                if (NT_SUCCESS(status))
                {
                    this->state = LockState::Unlocked;
                }
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                auto error = pthread_rwlock_init(&this->mutex, nullptr);
                XPLATFORM_ASSERT(0 == error);
                if (0 == error)
                {
                    this->state = LockState::Unlocked;
                }
            #else
                #error Unsupported Platform
            #endif

            //
            // We usually don't expect lock initialization to fail.
            // Assert here to signal it in debug builds.
            // It may happen when the system has not enough resources.
            // It is always a good idea to use the .State() method from base class to inspect the state.
            // The lock will not be acquired if it has not been initialized!
            //
            XPLATFORM_ASSERT(this->state == LockState::Unlocked);
        }

        _IRQL_requires_max_(APC_LEVEL)
        virtual ~ReadWriteLock() noexcept
        {
            //
            // We usually don't expect lock to not be initialized.
            // However we assert here to catch such rare scenarios on debug builds.
            //
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            //
            // Do not try to destroy an uninitialized object.
            //
            if(this->state != LockState::Uninitialized)
            {
                ///
                /// This helps a lot during debugging.
                /// Tries to esnure that this lock was properly released from all threads.
                /// If there is a hanging lock, it will not be incorrectly destroyed leading to memory corruptions.
                /// 
                LockExclusive();
                UnlockExclusive();

                //
                // We don't expect this lock to be reused.
                // And this variable is actually used in every subsequent function.
                // If we have an use after free, some assert might trigger and help us find it.
                //
                this->state = LockState::Uninitialized;

                //
                // Platform-Specific API to release all resources allocated internally by the OS.
                // We can't really do anything if the API fails.
                // Just ignore the result and mark the state as uninitialized to ensure the lock is not reused.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    this->mutex = SRWLOCK_INIT;
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
                    auto status = ExDeleteResourceLite(&this->mutex);
                    if (!NT_SUCCESS(status))
                    {
                        XPLATFORM_ASSERT(NT_SUCCESS(status));
                    }
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    auto error = pthread_rwlock_destroy(&this->mutex);
                    if (0 != error)
                    {
                        XPLATFORM_ASSERT(0 == error);
                    }
                #else
                    #error Unsupported Platform
                #endif
            }
        }

        // Copy semantics -- deleted (We can't copy a lock)
        ReadWriteLock(const ReadWriteLock& Other) noexcept = delete;
        ReadWriteLock& operator=(const ReadWriteLock& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ReadWriteLock(ReadWriteLock&& Other) noexcept = delete;
        ReadWriteLock& operator=(ReadWriteLock&& Other) noexcept = delete;

        _IRQL_requires_max_(APC_LEVEL)
        _Acquires_exclusive_lock_(this->mutex)
        virtual void LockExclusive(void) noexcept override
        {
            //
            // We usually don't expect lock to not be initialized.
            // However we assert here to catch such rare scenarios on debug builds.
            //
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            //
            // Don't try to acquire an uninitialized lock
            //
            if (this->state != LockState::Uninitialized)
            {
                //
                // Platform-Specific API to acquire the lock exclusively.
                // It guarantees that no failures will take place and the lock will be acquired exclusively after this method returns.
                // Or it will be stucked into a while() loop signaling a deadlock (LINUX only)
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    AcquireSRWLockExclusive(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // ExAcquireResourceExclusiveLite returns TRUE if the resource is acquired.
                    // This routine returns FALSE if the input Wait is FALSE and exclusive access cannot be granted immediately.
                    // Safe to ignore return value.
                    //
                    XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
                    KeEnterCriticalRegion();
                    (void) ExAcquireResourceExclusiveLite(&this->mutex, TRUE);

                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    //
                    // The read-write lock could not be acquired for writing because it was already locked for reading or writing.
                    // We spin here until we can get this lock.
                    // Possible return values:
                    //      * [EBUSY]      The read - write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                    //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                    //      * [EDEADLK]    The current thread already owns the read - write lock for writing or reading.
                    //
                    while(0 != pthread_rwlock_wrlock(&this->mutex))
                    {
                        //
                        // Don't busy wait. Try to relinquish the CPU by moving the thread to the end of the queue.
                        //
                        XPLATFORM_YIELD_PROCESSOR();
                    }
                #else
                    #error Unsupported Platform
                #endif

                //
                // Mark the lock as acquired exclusively. 
                // To be used in unlock functions to not release shared an exclusive lock or vice-versa.
                // This variable should be protected by the lock itself.
                // It is safe to write it here because the lock is taken.
                //
                this->state = LockState::AcquiredExclusive;
            }
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Requires_exclusive_lock_held_(this->mutex)
        _Releases_exclusive_lock_(this->mutex)
        virtual void UnlockExclusive(void) noexcept override
        {
            //
            // Extra safety to early catch AcquireShared() -> ReleaseExclusive() bugs
            //
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);
            XPLATFORM_ASSERT(this->state == LockState::AcquiredExclusive);

            //
            // Don't try to release an uninitialized lock
            //
            if (this->state != LockState::Uninitialized)
            { 
                //
                // Mark the lock state as unlocked before actually releasing the lock.
                // It is ok to do so because this state is rather informative, used only for asserts.
                // And if doing it with the lock held, we ensure no data races while writing this variable.
                // 
                this->state = LockState::Unlocked;

                //
                // Platform-Specific API to release the lock exclusively.
                // If the lock was not acquired exclusively, it will lead to undefined behavior by releasing the lock.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    ReleaseSRWLockExclusive(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
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

        _IRQL_requires_max_(APC_LEVEL)
        _Acquires_shared_lock_(this->mutex)
        virtual void LockShared(void) noexcept override
        {
            //
            // We usually don't expect lock to not be initialized.
            // However we assert here to catch such rare scenarios on debug builds.
            //
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);

            //
            // Don't try to lock an uninitialized lock
            //
            if (this->state != LockState::Uninitialized)
            {
                //
                // Platform-Specific API to acquire the lock shared.
                // It guarantees that no failures will take place and the lock will be acquired shared after this method returns.
                // Or it will be stucked into a while() loop signaling a deadlock (LINUX only)
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    AcquireSRWLockShared(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // ExAcquireResourceSharedLite returns TRUE if the resource is acquired.
                    // This routine returns FALSE if the input Wait is FALSE shared exclusive access cannot be granted immediately.
                    // Safe to ignore return value.
                    //
                    XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
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
                    while (0 != pthread_rwlock_rdlock(&this->mutex))
                    {
                        //
                        // Don't busy wait. Try to relinquish the CPU by moving the thread to the end of the queue.
                        //
                        XPLATFORM_YIELD_PROCESSOR();
                    }
                #else
                    #error Unsupported Platform
                #endif
            }
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Requires_shared_lock_held_(this->mutex)
        _Releases_shared_lock_(this->mutex)
        virtual void UnlockShared(void) noexcept override
        {
            //
            // Extra safety to early catch AcquireExclusive() -> ReleaseShared() bugs
            // 
            XPLATFORM_ASSERT(this->state != LockState::Uninitialized);
            XPLATFORM_ASSERT(this->state != LockState::AcquiredExclusive);

            //
            // Don't try to unlock an uninitialized lock
            //
            if (this->state != LockState::Uninitialized)
            {
                //
                // Platform-Specific API to release the lock exclusively.
                // If the lock was not acquired exclusively, it will lead to undefined behavior by releasing the lock.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    ReleaseSRWLockShared(&this->mutex);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    XPLATFORM_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
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
        //
        // Platform specific definition for a read-write lock.
        // For windows user mode we use the SRWLOCK for that.
        // For windows kernel mode we use ERESOURCE (as it copes well with verifier and !locks)
        // For linux we use the pthread_rwlock_t
        //
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_BEGIN
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) SRWLOCK mutex{ };
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) ERESOURCE mutex{ };
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                alignas(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT) pthread_rwlock_t mutex{ };
            #else
                #error Unsupported Platform
            #endif
        XPLATFORM_SUPPRESS_ALIGNMENT_WARNING_END
    };
}


#endif // __XPLATFORM_READ_WRITE_LOCK_HPP__
