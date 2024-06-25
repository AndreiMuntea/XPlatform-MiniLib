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
        const uint64_t currentValue = xpf::ApiAtomicCompareExchange(&this->m_Rundown, uint64_t{0}, uint64_t{ 0 });

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
        XPF_DEATH_ON_FAILURE((newValue & this->RUNDOWN_ACTIVE) == 0);

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
        const uint64_t currentValue = xpf::ApiAtomicCompareExchange(&this->m_Rundown, uint64_t{0}, uint64_t{ 0 });

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
        const uint64_t currentValue = xpf::ApiAtomicCompareExchange(&this->m_Rundown, uint64_t{0}, uint64_t{ 0 });

        //
        // If the rundown bit is set, we wait for rundown to be released.
        // the rundown is released when the value is 1 (only the active bit is set).
        // We did not set the bit, so somebody else did. We'll wait here.
        //
        if ((currentValue & this->RUNDOWN_ACTIVE) != 0)
        {
            while (this->m_Rundown != this->RUNDOWN_ACTIVE)
            {
                //
                // Relinquish the resources to allow another thread to run.
                // This value is small enough so we allow fast response.
                //
                xpf::ApiSleep(100);
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
            // Spin again, as we'll enter the check above.
            //
            XPF_NOTHING();
        }

        //
        // Somebody else changed the rundown.
        // No biggie, try again.
        //
        continue;
    }
}
