/**
 * @file        xpf_lib/private/Multithreading/Thread.cpp
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

#include "xpf_lib/xpf.hpp"


/**
 * @brief   By default all code in here goes into paged section.
 *          We can't create threads only at passive level.
 *          So it is ok for this to be paged-out.
 */
XPF_SECTION_PAGED;


/**
 * @brief   Here we'll define a system-specific callback.
 *          This is what we'll actually run.
 */
#if defined XPF_PLATFORM_WIN_UM
/**
 * @brief   Windows User-Mode callback that will be used as stub.
 * 
 * @param[in] Parameter - This will be an InternalContext.
 */
static DWORD
WINAPI
XpfInternalThreadRunCallback(
    _In_ PVOID Parameter
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    auto context = reinterpret_cast<xpf::thread::InternalContext*>(Parameter);
    if (nullptr != context)
    {
        if (context->UserCallback)
        {
            context->UserCallback(context->UserCallbackArgument);
        }
    }
    return STATUS_SUCCESS;
}

#elif defined XPF_PLATFORM_WIN_KM

/**
 * @brief   Windows Kernel-Mode callback that will be used as stub.
 * 
 * @param[in] Parameter - This will be an InternalContext.
 */
static void
NTAPI
XpfInternalThreadRunCallback(
    _In_ PVOID Parameter
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    auto context = reinterpret_cast<xpf::thread::InternalContext*>(Parameter);
    if (nullptr == context)
    {
        return;
    }

    //
    // The newly created system thread runs at PASSIVE_LEVEL inside a critical region.
    // Bail early if this invariant is invalidated.
    //
    if (PASSIVE_LEVEL != ::KeGetCurrentIrql())
    {
        XPF_ASSERT(false);
        return;
    }

    //
    // Critical region => APCs are disabled but special kernel APCs are enabled.
    // If APCs are not disabled we will not run and we will bail early.
    // If special kernel APCs are disabled, again, we will not run.
    //
    if ((FALSE == ::KeAreApcsDisabled()) ||
        (FALSE != ::KeAreAllApcsDisabled()))
    {
        XPF_ASSERT(false);
        return;
    }

    if (context->UserCallback)
    {
        context->UserCallback(context->UserCallbackArgument);
    }

    //
    // Validate the invariants. We should still be at passive level.
    // APCs should be disabled and special kernel APCs enabled.
    // This will help on debug.
    //
    XPF_ASSERT(PASSIVE_LEVEL == ::KeGetCurrentIrql());
    XPF_ASSERT(FALSE != ::KeAreApcsDisabled);
    XPF_ASSERT(FALSE == ::KeAreAllApcsDisabled);
}

#elif defined XPF_PLATFORM_LINUX_UM

/**
 * @brief   Linux UserMode callback that will be used as stub.
 * 
 * @param[in] Parameter - This will be an InternalContext.
 */
static void*
XPF_PLATFORM_CONVENTION
XpfInternalThreadRunCallback(
    _In_ void* Parameter
) noexcept(true)
{
    auto context = reinterpret_cast<xpf::thread::InternalContext*>(Parameter);
    if (nullptr != context)
    {
        if (context->UserCallback)
        {
            context->UserCallback(context->UserCallbackArgument);
        }
    }
    return nullptr;
}

#else
    #error Unrecognized Platform
#endif


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::thread::Thread::Run(
    _In_ xpf::thread::Callback UserCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // This shouldn't cause contention unless Run is called from
    // multiple threads on the same object - which is an incorrect behavior.
    //
    // But we don't want undefined behavior throughout the project.
    // So we guard against it as good as we can.
    //
    xpf::ExclusiveLockGuard guard{ this->m_ContextLock };

    //
    // Ensure we don't have a thread handle - nothing is running.
    //
    if (nullptr != this->m_Context.ThreadHandle)
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    //
    // Prepare to run the thread.
    //
    this->m_Context.UserCallback = UserCallback;
    this->m_Context.UserCallbackArgument = UserCallbackArgument;

    //
    // Now create a thread using platform-specific API.
    //
    #if defined XPF_PLATFORM_WIN_UM
        this->m_Context.ThreadHandle = ::CreateThread(NULL,
                                                      0,
                                                      reinterpret_cast<LPTHREAD_START_ROUTINE>(&XpfInternalThreadRunCallback),
                                                      reinterpret_cast<LPVOID>(&this->m_Context),
                                                      0,
                                                      NULL);
        if ((NULL == this->m_Context.ThreadHandle) || (INVALID_HANDLE_VALUE == this->m_Context.ThreadHandle))
        {
            this->m_Context.ThreadHandle = nullptr;
            return STATUS_INVALID_HANDLE;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        HANDLE threadHandle = NULL;

        OBJECT_ATTRIBUTES attributes = {0};
        InitializeObjectAttributes(&attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

        //
        // Thread creation can be safely done at PASSIVE_LEVEL.
        //
        if (PASSIVE_LEVEL != ::KeGetCurrentIrql())
        {
            this->m_Context.ThreadHandle = nullptr;
            return STATUS_INVALID_STATE_TRANSITION;
        }

        //
        // Create the thread in the context of system process.
        //
        NTSTATUS status = ::PsCreateSystemThread(&threadHandle,
                                                 THREAD_ALL_ACCESS,
                                                 &attributes,
                                                 NULL,
                                                 NULL,
                                                 reinterpret_cast<PKSTART_ROUTINE>(&XpfInternalThreadRunCallback),
                                                 reinterpret_cast<PVOID>(&this->m_Context));
        if (!NT_SUCCESS(status))
        {
            this->m_Context.ThreadHandle = nullptr;
            return status;
        }

        //
        // And now get a reference to the thread object. We can close the handle afterwards.
        //
        status = ::ObReferenceObjectByHandle(threadHandle,
                                             THREAD_ALL_ACCESS,
                                             *PsThreadType,
                                             KernelMode,
                                             &this->m_Context.ThreadHandle,
                                             NULL);
        XPF_VERIFY(NT_SUCCESS(::ZwClose(threadHandle)));
        if (!NT_SUCCESS(status))
        {
            this->m_Context.ThreadHandle = nullptr;
            return status;
        }

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // Allocate memory for pthread
        //
        this->m_Context.ThreadHandle = xpf::CriticalMemoryAllocator::AllocateMemory(sizeof(pthread_t));
        if (nullptr == this->m_Context.ThreadHandle)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        xpf::ApiZeroMemory(this->m_Context.ThreadHandle, sizeof(pthread_t));

        //
        // And now create the thread.
        //
        const int error = pthread_create(reinterpret_cast<pthread_t*>(this->m_Context.ThreadHandle),
                                         NULL,
                                         XpfInternalThreadRunCallback,
                                         &this->m_Context);
        if (0 != error)
        {
            xpf::CriticalMemoryAllocator::FreeMemory(&this->m_Context.ThreadHandle);
            this->m_Context.ThreadHandle = nullptr;
            return NTSTATUS_FROM_PLATFORM_ERROR(error);
        }

    #else
        #error Unrecognized Platform
    #endif

    //
    // Now we are running.
    //
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::thread::Thread::Join(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // This shouldn't cause contention unless Join is called from
    // multiple threads on the same object - which is an incorrect behavior.
    //
    // But we don't want undefined behavior throughout the project.
    // So we guard against it as good as we can.
    //
    xpf::ExclusiveLockGuard guard{ this->m_ContextLock };

    //
    // If we don't have a thread handle, we are done.
    //
    if (nullptr == this->m_Context.ThreadHandle)
    {
        return;
    }

    //
    // Now wait for the thread - wait for infinite, as it is not safe
    // to terminate a thread. It's a logic error if it can't be finished gracefully.
    //
    #if defined XPF_PLATFORM_WIN_UM
        const DWORD waitResult = ::WaitForSingleObject(this->m_Context.ThreadHandle,
                                                       INFINITE);
        XPF_VERIFY(WAIT_OBJECT_0 == waitResult);

        const BOOL closeResult = ::CloseHandle(this->m_Context.ThreadHandle);
        XPF_VERIFY(FALSE != closeResult);

    #elif defined XPF_PLATFORM_WIN_KM
        const NTSTATUS status = ::KeWaitForSingleObject(this->m_Context.ThreadHandle,
                                                        Executive,
                                                        KernelMode,
                                                        FALSE,
                                                        NULL);
        XPF_VERIFY(NT_SUCCESS(status));

        ObDereferenceObject(this->m_Context.ThreadHandle);

    #elif defined XPF_PLATFORM_LINUX_UM
        const int error = pthread_join(*(reinterpret_cast<pthread_t*>(this->m_Context.ThreadHandle)),
                                       NULL);
        XPF_VERIFY(0 == error);

        xpf::CriticalMemoryAllocator::FreeMemory(&this->m_Context.ThreadHandle);
    #else
        #error Unrecognized Platform
    #endif

    //
    // All good. set the thread handle on null to allow other callback.
    //
    this->m_Context.ThreadHandle = nullptr;
    this->m_Context.UserCallback = nullptr;
    this->m_Context.UserCallbackArgument = nullptr;
}

bool
XPF_API
xpf::thread::Thread::IsJoinable(
    void
) const noexcept(true)
{
    //
    // The function is in paged code section. Might be paged out.
    // Assert here for a MAX_PASSIVE_LEVEL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    return (this->m_Context.ThreadHandle != nullptr) ? true
                                                     : false;
}

_Ret_maybenull_
void*
XPF_API
xpf::thread::Thread::ThreadHandle(
    void
) const noexcept(true)
{
    //
    // The function is in paged code section. Might be paged out.
    // Assert here for a MAX_PASSIVE_LEVEL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    return this->m_Context.ThreadHandle;
}
