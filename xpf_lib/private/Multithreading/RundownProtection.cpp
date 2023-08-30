/**
 * @file        xpf_lib/private/Multithreading/RundownProtection.cpp
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

#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into default section.
 *          We don't want to page out rundowns.
 */
XPF_SECTION_DEFAULT;


_Check_return_
bool
XPF_API
xpf::RundownProtection::Acquire(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    while (true)
    {
        //
        // Grab the current value of rundown.
        //
        const uint64_t currentValue = this->m_Rundown;

        //
        // The parity bit is used to check if the Rundown is active => no further access is allowed.
        //
        if ((currentValue & this->RUNDOWN_ACTIVE) != 0)
        {
            return false;
        }

        //
        // We shouldn't reach this limit, but if we do, spin and retry.
        // Avoid setting the rundown bit via an increment.
        //
        if ((xpf::NumericLimits<uint64_t>::MaxValue() - this->RUNDOWN_INCREMENT) <= currentValue)
        {
            xpf::ApiYieldProcesor();
            continue;
        }

        //
        // Avoid setting the parity bit with additions.
        //
        const uint64_t newValue = currentValue + this->RUNDOWN_INCREMENT;
        XPF_ASSERT((newValue & this->RUNDOWN_ACTIVE) == 0);

        //
        // Atomic swap - ApiAtomicCompareExchange returns the original value.
        // If we managed to acquire the rundown we are done, otherwise we retry.
        //
        if (currentValue == xpf::ApiAtomicCompareExchange(&this->m_Rundown, newValue, currentValue))
        {
            return true;
        }

        //
        // Somebody else changed the rundown.
        // No biggie, try again.
        //
        continue;
    }
}

void
XPF_API
xpf::RundownProtection::Release(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    while (true)
    {
        //
        // Grab the current value of rundown.
        //
        const uint64_t currentValue = this->m_Rundown;

        //
        // We increment with an increment of 2.
        // We shouldn't be below 2. This is an invalid usage.
        //
        if (currentValue < this->RUNDOWN_INCREMENT)
        {
            xpf::ApiPanic(STATUS_INVALID_STATE_TRANSITION);
            return;
        }

        //
        // Decrement with the rundown decrement.
        //
        const uint64_t newValue = currentValue - this->RUNDOWN_INCREMENT;

        //
        // Atomic swap - ApiAtomicCompareExchange returns the original value.
        // If we managed to acquire the rundown we are done, otherwise we retry.
        //
        if (currentValue == xpf::ApiAtomicCompareExchange(&this->m_Rundown, newValue, currentValue))
        {
            //
            // Now we have a special case - if the rundown is active and we were the last
            // to touch it, we need to set the event.
            //
            // This is equivalent with the new value being 1 (RUNDOWN_ACTIVE).
            //
            if (newValue == this->RUNDOWN_ACTIVE)
            {
                (*this->m_RundownSignal).Set();
            }

            //
            // All good - we released the rundown.
            //
            return;
        }

        //
        // Somebody else changed the rundown.
        // No biggie, try again.
        //
        continue;
    }
}


/**
 * @brief   The code below can be paged-out. We can't use it at DISPATCH level.
 */
XPF_SECTION_PAGED;

void
XPF_API
xpf::RundownProtection::WaitForRelease(
    void
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    while (true)
    {
        //
        // Grab the current value of rundown.
        //
        const uint64_t currentValue = this->m_Rundown;

        //
        // If the rundown bit is set, we wait for rundown to be released.
        // the rundown is released when the value is 1 (only the active bit is set).
        // We did not set the bit, so somebody else did. We'll wait for the signal.
        //
        if ((currentValue & this->RUNDOWN_ACTIVE) != 0)
        {
            while (this->m_Rundown != this->RUNDOWN_ACTIVE)
            {
                (*this->m_RundownSignal).Wait();
            }
            break;
        }

        //
        // We need to set the rundown bit to prevent further access.
        //
        const uint64_t newValue = currentValue | this->RUNDOWN_ACTIVE;

        //
        // Atomic swap - ApiAtomicCompareExchange returns the original value.
        // If we managed to acquire the rundown we are done, otherwise we retry.
        //
        if (currentValue == xpf::ApiAtomicCompareExchange(&this->m_Rundown, newValue, currentValue))
        {
            //
            // Now we have a special case - if the rundown is active but there were no
            // acquisitions - we need to set the event as we were the ones which marked
            // the rundown as active.
            //
            if (newValue == this->RUNDOWN_ACTIVE)
            {
                (*this->m_RundownSignal).Set();
            }

            //
            // Don't exit here as we need to wait for the rundown signal.
            // Spin again, as we'll enter the check above.
            // ...
            //
        }

        //
        // Somebody else changed the rundown.
        // No biggie, try again.
        //
        continue;
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::RundownProtection::Create(
    _Inout_ xpf::Optional<xpf::RundownProtection>* RundownProtectionToCreate
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized rundown.
    // Assert here and bail early.
    //
    if ((nullptr == RundownProtectionToCreate) || (RundownProtectionToCreate->HasValue()))
    {
        XPF_ASSERT(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new rundown. This will be an empty one.
    // It will be initialized below.
    //
    RundownProtectionToCreate->Emplace();

    //
    // We failed to create a rundown. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!RundownProtectionToCreate->HasValue())
    {
        XPF_ASSERT(false);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Grab a reference to the new rundown. Makes the code more easier.
    // On release, this is optimized away.
    //
    xpf::RundownProtection& newRundown = (*(*RundownProtectionToCreate));

    //
    // We need to initialize the rundown event.
    // This will be a manual reset event (once it is signaled it stays signaled.
    //
    status = xpf::Signal::Create(&newRundown.m_RundownSignal, true);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // All good - created the rundown.
    //
    status = STATUS_SUCCESS;

Exit:
    if (!NT_SUCCESS(status))
    {
        RundownProtectionToCreate->Reset();
        XPF_ASSERT(!RundownProtectionToCreate->HasValue());
    }
    else
    {
        XPF_ASSERT(RundownProtectionToCreate->HasValue());
    }
    return status;
}
