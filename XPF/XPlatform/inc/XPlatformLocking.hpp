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

#ifndef __XPLATFORM_LOCKING_HPP__
#define __XPLATFORM_LOCKING_HPP__


//
// This file contains base class for locking.
//

namespace XPF
{
    //
    // All exclusive locks must derive from this class.
    // An InitializeLock method is provided as some locks may fail during init.
    //
    class ExclusiveLock
    {
    public:
        ExclusiveLock() noexcept = default;
        virtual ~ExclusiveLock() noexcept = default;

        // Copy semantics -- deleted (We can't copy a lock)
        ExclusiveLock(const ExclusiveLock& Other) noexcept = delete;
        ExclusiveLock& operator=(const ExclusiveLock& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ExclusiveLock(ExclusiveLock&& Other) noexcept = delete;
        ExclusiveLock& operator=(ExclusiveLock&& Other) noexcept = delete;

        virtual void LockExclusive(void) noexcept = 0;
        virtual void UnlockExclusive(void) noexcept = 0;
    };

    //
    // All shared locks must derive from this class
    //
    class SharedLock : public ExclusiveLock
    {
    public:
        SharedLock() noexcept = default;
        virtual ~SharedLock() noexcept = default;

        // Copy semantics -- deleted (We can't copy a lock)
        SharedLock(const SharedLock& Other) noexcept = delete;
        SharedLock& operator=(const SharedLock& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        SharedLock(SharedLock&& Other) noexcept = delete;
        SharedLock& operator=(SharedLock&& Other) noexcept = delete;

        virtual void LockShared(void) noexcept = 0;
        virtual void UnlockShared(void) noexcept = 0;
    };


    //
    // Class which can be used when working with exclusive locks.
    // Acquires a lock exclusive on constructor and releases it on destructor.
    //
    class ExclusiveLockGuard
    {
    public:
        ExclusiveLockGuard(ExclusiveLock& Lock) noexcept : lock{ Lock }
        {
            this->lock.LockExclusive();
        }
        ~ExclusiveLockGuard() noexcept
        {
            this->lock.UnlockExclusive();
        }

        // Copy semantics -- deleted (We can't copy a lockguard)
        ExclusiveLockGuard(const ExclusiveLockGuard& Other) noexcept = delete;
        ExclusiveLockGuard& operator=(const ExclusiveLockGuard& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        ExclusiveLockGuard(ExclusiveLockGuard&& Other) noexcept = delete;
        ExclusiveLockGuard& operator=(ExclusiveLockGuard&& Other) noexcept = delete;

    private:
        ExclusiveLock& lock;
    };

    //
    // Class which can be used when working with shared locks.
    // Acquires a lock shared on constructor and releases it on destructor.
    //
    class SharedLockGuard
    {
    public:
        SharedLockGuard(SharedLock& Lock) noexcept : lock{ Lock }
        {
            this->lock.LockShared();
        }
        ~SharedLockGuard() noexcept
        {
            this->lock.UnlockShared();
        }

        // Copy semantics -- deleted (We can't copy a lockguard)
        SharedLockGuard(const SharedLockGuard& Other) noexcept = delete;
        SharedLockGuard& operator=(const SharedLockGuard& Other) noexcept = delete;

        // Move semantics -- deleted (Nor we can move it)
        SharedLockGuard(SharedLockGuard&& Other) noexcept = delete;
        SharedLockGuard& operator=(SharedLockGuard&& Other) noexcept = delete;

    private:
        SharedLock& lock;
    };
}

#endif // __XPLATFORM_LOCKING_HPP__