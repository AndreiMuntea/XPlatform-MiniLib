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
        ReadWriteLock() noexcept = default;
        virtual ~ReadWriteLock() noexcept { XPLATFORM_ASSERT(this->mutex == nullptr); };

        // Copy semantics -- deleted (We can't copy a lock)
        ReadWriteLock(const ReadWriteLock& Other) noexcept = delete;
        ReadWriteLock& operator=(const ReadWriteLock& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ReadWriteLock(ReadWriteLock&& Other) noexcept = delete;
        ReadWriteLock& operator=(ReadWriteLock&& Other) noexcept = delete;

        //
        // This method must be called before using any acquire / release operations.
        // Do not call it twice for the same object. It may lead to undefined behavior.
        //
        _IRQL_requires_max_(APC_LEVEL)
        virtual bool Initialize(void) noexcept override
        {
            //
            // Do not call this method twice. 
            // We expect the mutex object to be uninitialized.
            //
            XPLATFORM_ASSERT(nullptr == this->mutex);

            ///
            /// Platform specific API to initialize a read-write lock.
            /// If somethin will go bad, the lock state will not be Unlocked, it will remain Uninitialized.
            /// One should always inspect the lock state before using it.
            /// 
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                this->mutex = reinterpret_cast<SRWLOCK*>(XPF::ApiAllocMemory(sizeof(SRWLOCK)));
                if (nullptr == this->mutex)
                {
                    return false;
                }
                InitializeSRWLock(this->mutex);
                return true;
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                this->mutex = reinterpret_cast<ERESOURCE*>(ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), 'csr#'));
                if (nullptr == this->mutex)
                {
                    return false;
                }
                auto status = ExInitializeResourceLite(this->mutex);
                if (!NT_SUCCESS(status))
                {
                    ExFreePoolWithTag(this->mutex, 'csr#');
                    this->mutex = nullptr;
                }
                return NT_SUCCESS(status);
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                this->mutex = reinterpret_cast<pthread_rwlock_t*>(XPF::ApiAllocMemory(sizeof(pthread_rwlock_t)));
                if (nullptr == this->mutex)
                {
                    return false;
                }
                auto error = pthread_rwlock_init(this->mutex, nullptr);
                if (0 != error)
                {
                    XPF::ApiFreeMemory(this->mutex);
                    this->mutex = nullptr;
                }
                return (0 == error);
            #else
                #error Unsupported Platform
            #endif
        }

        //
        // This method must be called before destroying the object.
        // It MUST be called even if initialize method failed.
        //
        _IRQL_requires_max_(APC_LEVEL)
        virtual void Uninitialize(void) noexcept override
        {
            //
            // Mutex object was not properly initialized.
            // Bail out quickly.
            // 
            if (nullptr == this->mutex)
            {
                return;
            }

            //
            // This helps a lot during debugging.
            // If another thread may have forgotten to release the mutex, this will block here.
            //
            LockExclusive();
            UnlockExclusive();

            //
            // Platform-Specific API to release all resources allocated internally by the OS.
            // We can't really do anything if the API fails.
            // Just ignore the result and mark the state as uninitialized to ensure the lock is not reused.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                XPF::ApiFreeMemory(this->mutex);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                auto status = ExDeleteResourceLite(this->mutex);
                if (!NT_SUCCESS(status))
                {
                    XPLATFORM_ASSERT(NT_SUCCESS(status));
                }
                ExFreePoolWithTag(this->mutex, 'csr#');
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                auto error = pthread_rwlock_destroy(this->mutex);
                if (0 != error)
                {
                    XPLATFORM_ASSERT(0 == error);
                }
                XPF::ApiFreeMemory(this->mutex);
            #else
                #error Unsupported Platform
            #endif

            //
            // Mark the mutex as freed.
            //
            this->mutex = nullptr;
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Acquires_exclusive_lock_(*this->mutex)
        virtual void LockExclusive(void) noexcept override
        {
            XPLATFORM_ASSERT(nullptr != this->mutex);

            //
            // Platform-Specific API to acquire the lock exclusively.
            // It guarantees that no failures will take place and the lock will be acquired exclusively after this method returns.
            // Or it will be stucked into a while() loop signaling a deadlock (LINUX only)
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                AcquireSRWLockExclusive(this->mutex);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                //
                // ExAcquireResourceExclusiveLite returns TRUE if the resource is acquired.
                // This routine returns FALSE if the input Wait is FALSE and exclusive access cannot be granted immediately.
                // Safe to ignore return value.
                //
                KeEnterCriticalRegion();
                (void) ExAcquireResourceExclusiveLite(this->mutex, TRUE);

            #elif defined(XPLATFORM_LINUX_USER_MODE)
                //
                // The read-write lock could not be acquired for writing because it was already locked for reading or writing.
                // We spin here until we can get this lock.
                // Possible return values:
                //      * [EBUSY]      The read - write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                //      * [EDEADLK]    The current thread already owns the read - write lock for writing or reading.
                //
                while(0 != pthread_rwlock_wrlock(this->mutex))
                {
                    XPLATFORM_YIELD_PROCESSOR();
                }
            #else
                #error Unsupported Platform
            #endif
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Requires_exclusive_lock_held_(*this->mutex)
        _Releases_exclusive_lock_(*this->mutex)
        virtual void UnlockExclusive(void) noexcept override
        {
            XPLATFORM_ASSERT(nullptr != this->mutex);

            //
            // Platform-Specific API to release the lock exclusively.
            // If the lock was not acquired exclusively, it will lead to undefined behavior by releasing the lock.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                ReleaseSRWLockExclusive(this->mutex);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                ExReleaseResourceLite(this->mutex);
                KeLeaveCriticalRegion();
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                //
                // Safe to ignore errors here
                // Possible return values:
                //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                //      * [EPERM]      The current thread does not own the read-write lock.
                //
                (void) pthread_rwlock_unlock(this->mutex);
            #else
                #error Unsupported Platform
            #endif
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Acquires_shared_lock_(*this->mutex)
        virtual void LockShared(void) noexcept override
        {
            XPLATFORM_ASSERT(nullptr != this->mutex);

            //
            // Platform-Specific API to acquire the lock shared.
            // It guarantees that no failures will take place and the lock will be acquired shared after this method returns.
            // Or it will be stucked into a while() loop signaling a deadlock (LINUX only)
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                AcquireSRWLockShared(this->mutex);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                //
                // ExAcquireResourceSharedLite returns TRUE if the resource is acquired.
                // This routine returns FALSE if the input Wait is FALSE and shared access cannot be granted immediately.
                // Safe to ignore return value.
                //
                KeEnterCriticalRegion();
                (void) ExAcquireResourceSharedLite(this->mutex, TRUE);
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                //
                // The read-write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                // We spin here until we can get this lock.
                // Possible return values:
                //      * [EBUSY]      The read - write lock could not be acquired for reading because a writer holds the lock or was blocked on it.
                //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                //      * [EDEADLK]    The current thread already owns the read - write lock for writing or reading.
                //
                while (0 != pthread_rwlock_rdlock(this->mutex))
                {
                    XPLATFORM_YIELD_PROCESSOR();
                }
            #else
                #error Unsupported Platform
            #endif
        }

        _IRQL_requires_max_(APC_LEVEL)
        _Requires_shared_lock_held_(*this->mutex)
        _Releases_shared_lock_(*this->mutex)
        virtual void UnlockShared(void) noexcept override
        {
            XPLATFORM_ASSERT(nullptr != this->mutex);
            //
            // Platform-Specific API to release the lock exclusively.
            // If the lock was not acquired exclusively, it will lead to undefined behavior by releasing the lock.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                ReleaseSRWLockShared(this->mutex);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                ExReleaseResourceLite(this->mutex);
                KeLeaveCriticalRegion();
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                //
                // Safe to ignore errors here
                // Possible return values:
                //      * [EINVAL]     The value specified by rwlock does not refer to an initialised read - write lock object.
                //      * [EPERM]      The current thread does not own the read-write lock.
                //
                (void) pthread_rwlock_unlock(this->mutex);
            #else
                #error Unsupported Platform
            #endif
        }

    private:
        //
        // Platform specific definition for a read-write lock.
        // For windows user mode we use the SRWLOCK for that.
        // For windows kernel mode we use ERESOURCE (as it copes well with verifier and !locks)
        // For linux we use the pthread_rwlock_t
        //
        #if defined (XPLATFORM_WINDOWS_USER_MODE)
            SRWLOCK* mutex = nullptr;
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            ERESOURCE* mutex = nullptr;
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            pthread_rwlock_t* mutex = nullptr;
        #else
            #error Unsupported Platform
        #endif
    };
}


#endif // __XPLATFORM_READ_WRITE_LOCK_HPP__
