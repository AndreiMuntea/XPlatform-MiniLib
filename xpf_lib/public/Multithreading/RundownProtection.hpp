/**
 * @file        xpf_lib/public/Multithreading/RundownProtection.hpp
 *
 * @brief       This implements a rundown protection object.
 *              An object is said to be run down if all outstanding accesses of the object are finished
 *              and no new requests to access the object will be granted.
 *              For example, a shared object might need to be run down so that it can be deleted and
 *              replaced with a new object.
 *              See MSDN for EX_RUNDOWN_REF - as this is inspired on that.
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
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/PlatformApi.hpp"

#include "xpf_lib/public/Memory/Optional.hpp"
#include "xpf_lib/public/Multithreading/Signal.hpp"


namespace xpf
{
/**
 * @brief   Serves as an implementation over RUNDOWN_PROTECTION.
 */
class RundownProtection
{
 private:
/**
 * @brief Default constructor. This generates a partially constructed object.
 *        Create() should be used instead to ensure the signal is properly initialized.
 *        Thus we mark this as private.
 */
RundownProtection(
    void
) noexcept(true) = default;

 public:
/**
 * @brief Default destructor.
 */
~RundownProtection(
    void
) noexcept(true)
{
    this->WaitForRelease();
}

/**
 * @brief Copy constructor - deleted
 * 
 * @param[in] Other - The other object to construct from.
 */
RundownProtection(
    _In_ _Const_ const RundownProtection& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
RundownProtection(
    _Inout_ RundownProtection&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
RundownProtection&
operator=(
    _In_ _Const_ const RundownProtection& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
RundownProtection&
operator=(
    _Inout_ RundownProtection&& Other
) noexcept(true) = delete;

/**
 * @brief Create and initialize a RundownProtection. This must be used instead of constructor.
 *        It ensures the rundown is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] RundownProtectionToCreate - the rundown to be created. On input it will be empty.
 *                                             On output it will contain a fully initialized RundownProtection
 *                                             or an empty one on fail.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success RundownProtectionToCreate has a value
 *       and on fail RundownProtectionToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::RundownProtection>* RundownProtectionToCreate
) noexcept(true);

/**
 * @brief Tries to acquire run-down protection on a shared object so the caller can safely access the object.
 * 
 * @return true if the routine successfully acquires run-down protection for the caller. Otherwise, it returns false.
 */
_Check_return_
bool
XPF_API
Acquire(
    void
) noexcept(true);

/**
 * @brief Releases one reference from rundown.
 * 
 * @return None.
 */
void
XPF_API
Release(
    void
) noexcept(true);

/**
 * @brief Waits until all drivers that have already been granted run-down protection
 *        complete their accesses of the shared object. 
 *        No further requests for run-down protection from drivers that are trying to access the shared object.
 * 
 * @return None.
 */
void
XPF_API
WaitForRelease(
    void
) noexcept(true);

 private:
    /**
     * @brief    Because we are using atomic intrinsics, this value need to be properly aligned.
     *           The rightmost bit will be used to mark that the rundown is active => no further access is allowed.
     *           The other 63 bits will be used to count the access.
     */
    alignas(uint64_t) volatile uint64_t m_Rundown = 0;
    /**
     * @brief   This will be set when the rundown is finished so we don't spin indefinitely.
     */
    xpf::Optional<xpf::Signal> m_RundownSignal;


    /**
     * @brief   The parity bit is the rundown bit. Reserved.
     *          Do not change this value without first validating the implementation.
     */
    static constexpr const uint64_t RUNDOWN_ACTIVE = 1;
    /**
     * @brief   Increment with an increment of two to avoid touching the ACTIVE bit.
     *          Do not change this value without first validating the implementation.
     */
    static constexpr const uint64_t RUNDOWN_INCREMENT = 2;


    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class RundownProtection


/**
 * @brief   Serves as an implementation to facilitate rundown protection access.
 *          Similar with lockguards. On constructor it acquires the rundown,
 *          on destruct it releases it - (if it was successfully acquired).
 *          It is the caller responsibility to check if the rundown was acquired or not
 *          using IsRundownAcquired() method.
 */
class RundownGuard
{
 public:
/**
 * @brief RundownGuard constructor.
 *        Attempts to acquire the given rundown reference.
 *
 * @param[in] RundownReference - A reference to the rundown to be acquired.
 *                               Must remain valid until RundownGuard is destroyed!
 */
RundownGuard(
    _Inout_ xpf::RundownProtection& RundownReference
) noexcept(true) : m_RundownReference{RundownReference}
{
    //
    // First we enter the critical region (if needed).
    //
    #if defined XPF__PLATFORM_WIN_KM
        if (::KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
            ::KeEnterCriticalRegion();
            this->m_IsInsideCriticalRegion = true;
        }
    #endif  // XPF_PLATFORM_WIN_KM

    //
    // Now attempt to acquire the rundown.
    //
    this->m_IsRundownAcquired = this->m_RundownReference.Acquire();
}

/**
 * @brief Default destructor.
 */
~RundownGuard(
    void
) noexcept(true)
{
    //
    // If the rundown was acquired, we release it.
    //
    if (this->m_IsRundownAcquired)
    {
        this->m_RundownReference.Release();
        this->m_IsRundownAcquired = false;
    }

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
 * @brief Copy constructor - deleted
 * 
 * @param[in] Other - The other object to construct from.
 */
RundownGuard(
    _In_ _Const_ const RundownGuard& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
RundownGuard(
    _Inout_ RundownGuard&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
RundownGuard&
operator=(
    _In_ _Const_ const RundownGuard& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
RundownGuard&
operator=(
    _Inout_ RundownGuard&& Other
) noexcept(true) = delete;

/**
 * @brief Checks if the rundown was successfully acquired.
 *
 * @return true if rundown was acquired, false otherwise.
 */
inline bool
IsRundownAcquired(
    void
) const noexcept(true)
{
    return this->m_IsRundownAcquired;
}

 private:
     xpf::RundownProtection& m_RundownReference;
     bool m_IsRundownAcquired = false;

    /**
     * @brief   On windows kernel, we'll also enter a critical region during rundown acquisition.
     */
    #if defined XPF_PLATFORM_WIN_KM
        bool m_IsInsideCriticalRegion = false;
    #endif  // XPF_PLATFORM_WIN_KM
};  // class RundownGuard
};  // namespace xpf
