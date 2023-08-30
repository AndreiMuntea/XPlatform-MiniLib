/**
 * @file        xpf_lib/private/Locks/ReadWriteLock.cpp
 *
 * @brief       The default read write lock that uses platform-specific APIs.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into paged section.
 *          This lock cannot be used at DISPATCH_LEVEL.
 */
XPF_SECTION_PAGED;


/**
 * @brief   This structure holds the platform specific data for storing a lock.
 *          We use a C-like struct as it stores low-level data.
 */
typedef struct _XPF_RW_LOCK
{
    #if defined XPF_PLATFORM_WIN_UM
        //
        // On windows user mode we use a SRWLOCK.
        //
        SRWLOCK RwLock;
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // On Windows kernel mode we use an ERESOURCE.
        //
        ERESOURCE RwLock;
    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // On Linux user mode we use pthread rwlock.
        //
        pthread_rwlock_t RwLock;
    #else
        #error Unrecognized Platform
    #endif
} XPF_RW_LOCK;


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ReadWriteLock::Create(
    _Inout_ xpf::Optional<xpf::ReadWriteLock>* LockToCreate
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized lock.
    // Assert here and bail early.
    //
    if ((nullptr == LockToCreate) || (LockToCreate->HasValue()))
    {
        XPF_ASSERT(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new lock. This will be an empty one.
    // It will be initialized below.
    //
    LockToCreate->Emplace();

    //
    // We failed to create a lock. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!LockToCreate->HasValue())
    {
        XPF_ASSERT(false);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // First we allocate space for a RW_LOCK
    // which stores an actual lock. This is considered a critical allocation.
    // Locks are considered critical structures. So we do best effort.
    //
    // Plus it makes the code easier as ERESOURCE requires resident storage.
    // So we need to allocate from nonpagedpool.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(
                                xpf::CriticalMemoryAllocator::AllocateMemory(sizeof(XPF_RW_LOCK)));
    if (nullptr == lock)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }
    xpf::ApiZeroMemory(lock, sizeof(XPF_RW_LOCK));

    //
    // And now platform specific initialization.
    //
    #if defined XPF_PLATFORM_WIN_UM
        //
        // Initialize the SRW Lock. This function can't fail.
        //
        ::InitializeSRWLock(&lock->RwLock);
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // Initialize the ERESOURCE structure.
        //
        status = ::ExInitializeResourceLite(&lock->RwLock);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // Initialize the rwlock.
        //
        {
            const int error = pthread_rwlock_init(&lock->RwLock, NULL);
            if (error != 0)
            {
                status = NTSTATUS_FROM_PLATFORM_ERROR(error);
                goto Exit;
            }
        }
    #else
        #error Unrecognized Platform
    #endif

    //
    // The platform specific initialization is done.
    // Now assign it to the lock, and set the lock on nullptr.
    // It will be freed once the LockToCreate is freed.
    //
    (*(*LockToCreate)).m_Lock = reinterpret_cast<XPF_RW_LOCK*>(lock);
    lock = nullptr;

    //
    // Signal the success. Everything went well.
    //
    status = STATUS_SUCCESS;

Exit:
    if (nullptr != lock)
    {
        xpf::CriticalMemoryAllocator::FreeMemory(reinterpret_cast<void**>(&lock));
    }

    if (!NT_SUCCESS(status))
    {
        LockToCreate->Reset();
        XPF_ASSERT(!LockToCreate->HasValue());
    }
    else
    {
        XPF_ASSERT(LockToCreate->HasValue());
    }
    return status;
}

void
XPF_API
xpf::ReadWriteLock::Destroy(
    void
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    //
    // If the underlying lock is not initialized, we are done.
    //
    if (nullptr == this->m_Lock)
    {
        return;
    }

    //
    // We need access to RW_LOCK structure. R-CAST here to ease the access.
    // On release this will be optimized away anyway.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(this->m_Lock);

    //
    // Platform specific cleanup.
    //
    #if defined XPF_PLATFORM_WIN_UM
        //
        // On windows UM no extra cleanup is required for SRWLOCK.
        //
        UNREFERENCED_PARAMETER(lock);
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // On windows KM we destroy the ERESOURCE.
        //
        const NTSTATUS status = ::ExDeleteResourceLite(&lock->RwLock);
        XPF_VERIFY(NT_SUCCESS(status));
    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // On Linux User Mode we destroy the rwlock
        //
        const int error = pthread_rwlock_destroy(&lock->RwLock);
        XPF_VERIFY(0 == error);
    #else
        #error Unrecognized Platform
    #endif

    //
    // And now clean the allocated memory.
    //
    xpf::CriticalMemoryAllocator::FreeMemory(&this->m_Lock);
    this->m_Lock = nullptr;
}

void
XPF_API
xpf::ReadWriteLock::LockExclusive(
    void
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    //
    // This shouldn't be called with a null underlying object.
    // Create should guarantee that the object is not partially constructed.
    // Assert here and investigate.
    //
    XPF_ASSERT(nullptr != this->m_Lock);

    //
    // We need access to RW_LOCK structure. R-CAST here to ease the access.
    // On release this will be optimized away anyway.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(this->m_Lock);

    #if defined XPF_PLATFORM_WIN_UM
        ::AcquireSRWLockExclusive(&lock->RwLock);
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // First we enter a critical region. Will leave it on release.
        // This will prevent the thread to be suspended while it holds the lock.
        //
        ::KeEnterCriticalRegion();

        //
        // Now acquire the resource exclusively.
        // This shouldn't fail as per documentation.
        //
        // ExAcquireResourceExclusiveLite returns TRUE if the resource is acquired.
        // This routine returns FALSE if the input Wait is FALSE
        // and exclusive access cannot be granted immediately.
        //
        const BOOLEAN wasAcquired = ::ExAcquireResourceExclusiveLite(&lock->RwLock,
                                                                     TRUE);
        XPF_VERIFY(FALSE != wasAcquired);

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // On Linux User Mode we retry to lock until we get a 0.
        // Normaly it shouldn't happen to re-enter.
        //
        while (0 != pthread_rwlock_wrlock(&lock->RwLock))
        {
            xpf::ApiYieldProcesor();
        }
    #else
        #error Unrecognized Platform
    #endif
}

void
XPF_API
xpf::ReadWriteLock::UnLockExclusive(
    void
) noexcept(true)
{
    //
    // Even if the resource can be released at dispatch level,
    // we don't allow this to be called at dispatch as it can be acquired at max APC level
    // and it should be released with the original IRQL.
    //
    XPF_MAX_APC_LEVEL();

    //
    // This shouldn't be called with a null underlying object.
    // Create should guarantee that the object is not partially constructed.
    // Assert here and investigate.
    //
    XPF_ASSERT(nullptr != this->m_Lock);

    //
    // We need access to RW_LOCK structure. R-CAST here to ease the access.
    // On release this will be optimized away anyway.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(this->m_Lock);

    #if defined XPF_PLATFORM_WIN_UM
        _Analysis_assume_lock_held_(lock->RwLock);
        ::ReleaseSRWLockExclusive(&lock->RwLock);
    #elif defined XPF_PLATFORM_WIN_KM
        ::ExReleaseResourceLite(&lock->RwLock);
        ::KeLeaveCriticalRegion();
    #elif defined XPF_PLATFORM_LINUX_UM
        const int error = pthread_rwlock_unlock(&lock->RwLock);
        XPF_VERIFY(0 == error);
    #else
        #error Unrecognized Platform
    #endif
}

void
XPF_API
xpf::ReadWriteLock::LockShared(
    void
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    //
    // This shouldn't be called with a null underlying object.
    // Create should guarantee that the object is not partially constructed.
    // Assert here and investigate.
    //
    XPF_ASSERT(nullptr != this->m_Lock);

    //
    // We need access to RW_LOCK structure. R-CAST here to ease the access.
    // On release this will be optimized away anyway.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(this->m_Lock);

    #if defined XPF_PLATFORM_WIN_UM
        ::AcquireSRWLockShared(&lock->RwLock);
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // First we enter a critical region. Will leave it on release.
        // This will prevent the thread to be suspended while it holds the lock.
        //
        ::KeEnterCriticalRegion();

        //
        // Now acquire the resource exclusively.
        // This shouldn't fail as per documentation.
        //
        // ExAcquireResourceSharedLite returns TRUE if the resource is acquired.
        // This routine returns FALSE if the input Wait is FALSE
        // and shared access cannot be granted immediately.
        //
        const BOOLEAN wasAcquired = ::ExAcquireResourceSharedLite(&lock->RwLock,
                                                                  TRUE);
        XPF_VERIFY(FALSE != wasAcquired);
    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // On Linux User Mode we retry to lock until we get a 0.
        // Normaly it shouldn't happen to re-enter.
        //
        while (0 != pthread_rwlock_rdlock(&lock->RwLock))
        {
            xpf::ApiYieldProcesor();
        }
    #else
        #error Unrecognized Platform
    #endif
}

void
XPF_API
xpf::ReadWriteLock::UnLockShared(
    void
) noexcept(true)
{
    //
    // Even if the resource can be released at dispatch level,
    // we don't allow this to be called at dispatch as it can be acquired at max APC level
    // and it should be released with the original IRQL.
    //
    XPF_MAX_APC_LEVEL();

    //
    // This shouldn't be called with a null underlying object.
    // Create should guarantee that the object is not partially constructed.
    // Assert here and investigate.
    //
    XPF_ASSERT(nullptr != this->m_Lock);

    //
    // We need access to RW_LOCK structure. R-CAST here to ease the access.
    // On release this will be optimized away anyway.
    //
    XPF_RW_LOCK* lock = reinterpret_cast<XPF_RW_LOCK*>(this->m_Lock);

    #if defined XPF_PLATFORM_WIN_UM
        ::ReleaseSRWLockShared(&lock->RwLock);
    #elif defined XPF_PLATFORM_WIN_KM
        ::ExReleaseResourceLite(&lock->RwLock);
        ::KeLeaveCriticalRegion();
    #elif defined XPF_PLATFORM_LINUX_UM
        const int error = pthread_rwlock_unlock(&lock->RwLock);
        XPF_VERIFY(0 == error);
    #else
        #error Unrecognized Platform
    #endif
}
