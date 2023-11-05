/**
 * @file        xpf_lib/public/Multithreading/Signal.hpp
 *
 * @brief       This is a wrapper class over a signal.
 *              This is similar with Windows event (see CreateEvent API)
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


namespace xpf
{

/**
 * @brief This is storage for platform specific SignalHandle.
 *        It is represented differently on our platforms.
 *        We need this in the header as we can return it to the caller
 *        for better access
 */
struct SignalHandle
{
    /**
     * @brief This stores details whether this is a manual rest
     *        signal or not.
     */
    bool IsManualResetEvent = false;

    #if defined XPF_PLATFORM_WIN_UM
        /**
         * @brief On windows user mode this is a handle to the actual event.
         *        Created using CreateEventW API.
         */
        HANDLE Handle = nullptr;

    #elif defined XPF_PLATFORM_WIN_KM
        /**
         * @brief On windows kernel mode this is a KEVENT object.
         *        This is opaque for us.
         */
        KEVENT* Handle = nullptr;

    #elif defined XPF_PLATFORM_LINUX_UM
        /**
         * @brief On linux we have a little bit more legwork to do.
         *        We'll use a condition variable to broadcast waiting threads.
         */
        pthread_cond_t ConditionVariable;
        /**
         * @brief This will be used in combo with ConditionVariable above.
         */
        pthread_mutex_t ContitionMutex;
        /**
         * @brief This stores details about the current state (signaled / not signaled).
         */
        bool IsSignaled = false;
        /**
         * @brief This stores details about the initialization state.
         *        It is true only if both condition variable and the mutex are properly initialized.
         */
        bool IsProperlyInitialized = false;
    #else
        #error Unrecognized Platform
    #endif
};

/**
 * @brief This is a Signal class that can be signaled or resetted.
 *        Or threads can wait for it.
 */
class Signal final
{
 private:
/**
 * @brief Default constructor. This generates a partially constructed object.
 *        Create() should be used instead to ensure the signal is properly initialized.
 *        Thus we mark this as private.
 */
Signal(
    void
) noexcept(true) = default;

 public:
/**
 * @brief Default destructor.
 */
~Signal(
    void
) noexcept(true)
{
    this->Destroy();
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Signal, delete);

/**
 * @brief Sets the specified signal object to the signaled state.
 *        If the Signal was created with manual reset = true,
 *        it stays signaled until Reset() is called,
 *        otherwise it stays signaled until it satisfies one wait.
 * 
 * @note If the previous state was signaled, it remains signaled.
 *       If the previous state was non-signaled, it goes signaled,
 *       but if there are waiters and the event was created with
 *       manual reset = false, then it satisfies one wait
 *       then it goes back to unsignaled.
 * 
 * @return None.
 */
void
XPF_API
Set(
    void
) noexcept(true);

/**
 * @brief Sets the specified signal object to the non-signaled state.
 *
 * @note If the previous state was not signaled, it remains unsignaled.
 *       Waiters are not affected.
 * 
 * @return None.
 */
void
XPF_API
Reset(
    void
) noexcept(true);

/**
 * @brief Waits for the signal to be signaled. It waits for the specified amount of time.
 * 
 * @param[in] TimeoutInMilliSeconds - Number of milliseconds to wait for the signal.
 * 
 * @return true if the wait was satisfied (the signal was signaled),
 *         false otherwise (the timeout was reached or the event was not signaled).
 */
bool
XPF_API
Wait(
    _In_ uint32_t TimeoutInMilliSeconds = xpf::NumericLimits<uint32_t>::MaxValue()
) noexcept(true);

/**
 * @brief Retrieves the underlying signal handle.
 *        This contains platform specific signal identifier.
 * 
 * @return xpf::SignalHandle* representing the signal handle.
 * 
 * @note  It shouldn't be normally used, but it may be required,
 *        for some more-advanced computations.
 *        It is the caller responsibility to ensure it remains valid.
 */
const xpf::SignalHandle*
XPF_API
SignalHandle(
    void
) const noexcept(true);

/**
 * @brief Create and initialize a Signal. This must be used instead of constructor.
 *        It ensures the signal is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] SignalToCreate - the signal to be created. On input it will be empty.
 *                                  On output it will contain a fully initialized signal
 *                                  or an empty one on fail.
 *
 * @param[in] ManualReset - a boolean specifying whether the signal will be manually reseted
 *                          or it will be auto-resetted after satisfying one wait.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success SignalToCreate has a value
 *       and on fail SignalToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::Signal>* SignalToCreate,
    _In_ bool ManualReset
) noexcept(true);

 private:
/**
 * @brief Clears the underlying resources allocated for signal.
 *        This is called only by the destructor to ensure we will not
 *        have any waiters while we're destroying the signal.
 *
 */
void
XPF_API
Destroy(
    void
) noexcept(true);

 private:
    /**
     * @brief   This is the signal identifier wrapped in its own struct..
     *          As it may vary between platforms.
     * 
     * @note    On windows user mode this is a HANDLE as returned by CreateEvent method.
     *          On windows kernel mode this is the KEVENT object.
     *          See the structure for other platforms.
     * 
     *          Getter for this method is available in the event class
     *          If the caller wants to do some operations with it.
     */
     xpf::SignalHandle m_SignalHandle;

    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class Event
};  // namespace xpf
