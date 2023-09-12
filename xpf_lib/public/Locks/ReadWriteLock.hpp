/**
 * @file        xpf_lib/public/Locks/ReadWriteLock.hpp
 *
 * @brief       The default read write lock that uses platform-specific APIs.
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
#include "xpf_lib/public/Memory/Optional.hpp"

namespace xpf
{
/**
 * @brief This is a lock that will allow shared access to a resource.
 *        It is not recursive. So don't attempt to acquire it on the same
 *        thread before releasing it.
 */
class ReadWriteLock final : public SharedLock
{
 private:
/**
 * @brief ReadWriteLock constructor - default.
 */
ReadWriteLock(
    void
) noexcept(true) = default;

 public:
/**
 * @brief Default destructor.
 */
virtual ~ReadWriteLock(
    void
) noexcept(true)
{
    //
    // The lock must be freed. So lock here exclusive and release to ensure.
    // This is a good way to catch hanging threads.
    //
    this->LockExclusive();
    this->UnLockExclusive();

    //
    // And now clear the underlying resources.
    //
    this->Destroy();
}

/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
ReadWriteLock(
    _In_ _Const_ const ReadWriteLock& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
ReadWriteLock(
    _Inout_ ReadWriteLock&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
ReadWriteLock&
operator=(
    _In_ _Const_ const ReadWriteLock& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
ReadWriteLock&
operator=(
    _Inout_ ReadWriteLock&& Other
) noexcept(true) = delete;

/**
 * @brief Acquires the lock exclusive granting read-write access.
 * 
 * @return None.
 * 
 * @note If the lock is already taken exclusively, it will block
 *       until access can be granted.
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
 *        Don't acquire twice on the same thread without first releasing
 *        otherwise a deadlock will occur.
 * 
 * @return None.
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

/**
 * @brief Create and initialize a ReadWriteLock. This must be used instead of constructor.
 *        It ensures the lock is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] LockToCreate - the lock to be created. On input it will be empty.
 *                                On output it will contain a fully initialized ReadWriteLock
 *                                or an empty one on fail.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success LockToCreate has a value
 *       and on fail LockToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::ReadWriteLock>* LockToCreate
) noexcept(true);

 private:
/**
 * @brief Clears the underlying resources allocated for read write lock.
 *        This is called only by the destructor.
 *
 */
void
XPF_API
Destroy(
    void
) noexcept(true);

 private:
    /**
     * @brief   This is the lock identifier stored as a void*.
     *          As it may vary between platforms.
     * 
     * @note    On windows user mode this is a SRWLOCK.
     *          On windows kernel mode this is the ERESOURCE object.
     */
     void* m_Lock = nullptr;

    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class ReadWriteLock
};  // namespace xpf
