/**
 * @file        xpf_lib/public/Multithreading/Thread.hpp
 *
 * @brief       This is the class to mimic std::thread.
 *              It contains only a small subset of functionality.
 *              More can be added when needed.
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

#include "xpf_lib/public/Locks/BusyLock.hpp"


namespace xpf
{
namespace thread
{

//
// ************************************************************************************************
// This is the section containing the definition for user-exposed thread callback.
// ************************************************************************************************
//

/**
 * @brief   Definition for the argument that the user must specify to the callback.
 */
using CallbackArgument = void*;

/**
 * @brief   Definition for a pointer to a function taking a pointer to void as argument.
 */
typedef void (XPF_API* Callback) (CallbackArgument) noexcept(true);

/**
 * @brief   This struct is internally used by the Thread class and stores platform specific data.
 */
struct InternalContext
{
   /**
    * @brief   The callback to be executed.
    */
    xpf::thread::Callback UserCallback = nullptr;

    /**
     * @brief   The argument to be provided to UserCallback.
     */
    xpf::thread::CallbackArgument UserCallbackArgument = nullptr;

    /**
     * @brief   This is the thread identifier stored as a void*.
     *          As it may vary between platforms.
     * 
     * @note    On windows user mode this is a HANDLE as returned by CreateThread method.
     *          On windows kernel mode this is the KTHREAD object.
     * 
     *          Getter for this method is available in the thread class
     *          If the caller wants to do some operations with it.
     */
    void* ThreadHandle = nullptr;
};  // struct InternalContext


//
// ************************************************************************************************
// This is the section containing the actual thread implementation.
// ************************************************************************************************
//

/**
 * @brief   Serves as a container for running callbacks.
 */
class Thread final
{
 public:
/**
 * @brief Thread constructor - default.
 */
Thread(
    void
) noexcept(true) = default;

/**
 * @brief Default destructor.
 */
~Thread(
    void
) noexcept(true)
{
    this->Join();
}

/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(Thread, delete);

/**
 * @brief Schedules a given callback to run on a thread.
 * 
 * @param[in] UserCallback - Callback to be run.
 * 
 * @param[in] UserCallbackArgument - Argument to be provided to the callback.
 * 
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 * 
 * @note If a callback is already running, a proper status is returned.
 *       Use Join() to wait until the thread finishes before scheduling a new callback.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Run(
    _In_ xpf::thread::Callback UserCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true);

/**
 * @brief Waits for the current thread to finish execution.
 * 
 * @return None.
 */
void
XPF_API
Join(
    void
) noexcept(true);

/**
 * @brief Checks if the current thread is running a callback.
 * 
 * @return true if there is a callback running, false otherwise.
 */
bool
XPF_API
IsJoinable(
    void
) const noexcept(true);

/**
 * @brief Retrieves the underlying thread handle.
 *        This contains platform specific thread identifier.
 * 
 * @return void* representing the thread handle.
 *         See the internal context structure for details on the
 *         platform specific data.
 *         If no callback is currently running, the return is nullptr.
 * 
 * @note  It shouldn't be normally used, but it may be required,
 *        for some more-advanced computations.
 *        It is the caller responsibility to ensure it remains valid
 *        and Join() will not be called in the meantime.
 */
_Ret_maybenull_
void*
XPF_API
ThreadHandle(
    void
) const noexcept(true);

 private:
    /**
     * @brief We'll use a simple BusyLock to prevent invalid usage.
     *        For example calling Run() twice on different threads.
     *        This is a simple lock that should have no effect if
     *        the thread is used correctly, but also helps if it is not.
     */
    xpf::thread::InternalContext m_Context;
    xpf::BusyLock m_ContextLock;
};  // class Thread
};  // namespace thread
};  // namespace xpf
