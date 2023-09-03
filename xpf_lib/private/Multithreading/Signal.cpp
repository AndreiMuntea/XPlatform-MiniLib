/**
 * @file        xpf_lib/private/Multithreading/Signal.cpp
 *
 * @brief       This is a wrapper class over a signal.
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
  *          Signals can be used at any IRQL. So we don't page out this code.
  */
XPF_SECTION_DEFAULT;


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::Signal::Create(
    _Inout_ xpf::Optional<xpf::Signal>* SignalToCreate,
    _In_ bool ManualReset
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized event.
    // Assert here and bail early.
    //
    if ((nullptr == SignalToCreate) || (SignalToCreate->HasValue()))
    {
        XPF_ASSERT(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new event. This will be an empty one.
    // It will be initialized below.
    //
    SignalToCreate->Emplace();

    //
    // We failed to create an event. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!SignalToCreate->HasValue())
    {
        XPF_ASSERT(false);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Grab a reference - makes the code easier to read.
    // On release, this goes away anyway.
    //
    xpf::Signal& newSignal = (*(*SignalToCreate));

    //
    // First we fill the details available on all platforms.
    //
    newSignal.m_SignalHandle.IsManualResetEvent = ManualReset;

    //
    // Now create a signal using platform-specific API.
    //
    #if defined XPF_PLATFORM_WIN_UM
        //
        // On Windows UM we'll use an Event object to represent a signal.
        // The m_SignalHandle will be the event handle returned by CreateEventW.
        //
        HANDLE eventHandle = ::CreateEventW(NULL,
                                            (ManualReset) ? TRUE : FALSE,
                                            FALSE,
                                            NULL);
        if ((NULL == eventHandle) || (INVALID_HANDLE_VALUE == eventHandle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }

        newSignal.m_SignalHandle.Handle = eventHandle;
        status = STATUS_SUCCESS;

    #elif defined XPF_PLATFORM_WIN_KM
        //
        // On Windows KM we'll use the KEVENT to store the event.
        // This will be allocated from nonpaged pool => Critical allocation.
        //
        // It is required to be allocated from nonpaged pool as we need it to
        // function at DISPATCH level as well so it can't be paged out.
        //
        // Storage for an event object must be resident - see msdn for KeInitializeEvent.
        //
        void* eventObject = xpf::CriticalMemoryAllocator::AllocateMemory(sizeof(KEVENT));
        if (nullptr == eventObject)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        ::KeInitializeEvent(reinterpret_cast<PRKEVENT>(eventObject),
                            (ManualReset) ? NotificationEvent : SynchronizationEvent,
                            FALSE);

        newSignal.m_SignalHandle.Handle = reinterpret_cast<KEVENT*>(eventObject);
        status = STATUS_SUCCESS;

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // Preinitialize the condition variable and the mutex.
        //
        xpf::ApiZeroMemory(&newSignal.m_SignalHandle.ConditionVariable, sizeof(pthread_cond_t));
        xpf::ApiZeroMemory(&newSignal.m_SignalHandle.ContitionMutex, sizeof(pthread_mutex_t));

        //
        // Start by initializing the condition variable.
        //
        int error = pthread_cond_init(&newSignal.m_SignalHandle.ConditionVariable,
                                      NULL);
        if (0 != error)
        {
            status = NTSTATUS_FROM_PLATFORM_ERROR(error);
            goto Exit;
        }

        //
        // And now the mutex.
        //
        error = pthread_mutex_init(&newSignal.m_SignalHandle.ContitionMutex,
                                   NULL);
        if (0 != error)
        {
            const int destroyStatus = pthread_cond_destroy(&newSignal.m_SignalHandle.ConditionVariable);
            XPF_VERIFY(0 == destroyStatus);

            status = NTSTATUS_FROM_PLATFORM_ERROR(error);
            goto Exit;
        }

        //
        // All good. Mark this as properly initialized.
        //
        newSignal.m_SignalHandle.IsProperlyInitialized = true;
        status = STATUS_SUCCESS;

    #else
        #error Unrecognized Platform
    #endif


Exit:
    if (!NT_SUCCESS(status))
    {
        SignalToCreate->Reset();
        XPF_ASSERT(!SignalToCreate->HasValue());
    }
    else
    {
        XPF_ASSERT(SignalToCreate->HasValue());
    }
    return status;
}

void
XPF_API
xpf::Signal::Destroy(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_UM
        //
        // Close the opened handle.
        //
        if (nullptr != this->m_SignalHandle.Handle)
        {
            const BOOL closeStatus = ::CloseHandle(this->m_SignalHandle.Handle);
            XPF_VERIFY(FALSE != closeStatus);
            this->m_SignalHandle.Handle = nullptr;
        }

    #elif defined XPF_PLATFORM_WIN_KM
        //
        // On Windows KM we'll free resources allocated for the event.
        //
        if (nullptr != this->m_SignalHandle.Handle)
        {
            xpf::CriticalMemoryAllocator::FreeMemory(
                reinterpret_cast<void**>(&this->m_SignalHandle.Handle));
            this->m_SignalHandle.Handle = nullptr;
        }

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // On Linux UM we'll destroy the mutex and the condition variable.
        //
        if (this->m_SignalHandle.IsProperlyInitialized)
        {
            const int destroyStatusCond = pthread_cond_destroy(&this->m_SignalHandle.ConditionVariable);
            XPF_VERIFY(0 == destroyStatusCond);

            const int destroyStatusMutex = pthread_mutex_destroy(&this->m_SignalHandle.ContitionMutex);
            XPF_VERIFY(0 == destroyStatusMutex);

            this->m_SignalHandle.IsProperlyInitialized = false;
        }

        //
        // And we won't leave garbage there.
        //
        xpf::ApiZeroMemory(&this->m_SignalHandle.ConditionVariable, sizeof(pthread_cond_t));
        xpf::ApiZeroMemory(&this->m_SignalHandle.ContitionMutex, sizeof(pthread_mutex_t));

    #else
        #error Unrecognized Platform
    #endif
}

void
XPF_API
xpf::Signal::Set(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();


    #if defined XPF_PLATFORM_WIN_UM
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        (VOID) ::SetEvent(this->m_SignalHandle.Handle);

    #elif defined XPF_PLATFORM_WIN_KM
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        (VOID) ::KeSetEvent(this->m_SignalHandle.Handle,
                            IO_NO_INCREMENT,
                            FALSE);
    #elif defined XPF_PLATFORM_LINUX_UM
        XPF_ASSERT(this->m_SignalHandle.IsProperlyInitialized);

        //
        // Start by acquiring the condition mutex.
        //
        const int mutexAcquireStatus = pthread_mutex_lock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == mutexAcquireStatus);

        //
        // Mark the state as signaled - this variable is guarded by the mutex.
        // So its access is safe here.
        //
        this->m_SignalHandle.IsSignaled = true;

        if (this->m_SignalHandle.IsManualResetEvent)
        {
            //
            // On manual reset, we signal ALL threads.
            //
            const int signalStatus = pthread_cond_broadcast(&this->m_SignalHandle.ConditionVariable);
            XPF_VERIFY(0 == signalStatus);
        }
        else
        {
            //
            // On auto reset, we signal AT LEAST one thread.
            // We have a bit more legwork to do to ensure only one thread is signaled.
            // This is handled in Wait().
            //
            const int signalStatus = pthread_cond_signal(&this->m_SignalHandle.ConditionVariable);
            XPF_VERIFY(0 == signalStatus);
        }

        //
        // And now release the mutex.
        //
        const int releaseStatus = pthread_mutex_unlock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == releaseStatus);

    #else
        #error Unrecognized Platform
    #endif
}

void
XPF_API
xpf::Signal::Reset(
    void
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_UM
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        (VOID) ::ResetEvent(this->m_SignalHandle.Handle);

    #elif defined XPF_PLATFORM_WIN_KM
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        (VOID) ::KeResetEvent(this->m_SignalHandle.Handle);

    #elif defined XPF_PLATFORM_LINUX_UM
        XPF_ASSERT(this->m_SignalHandle.IsProperlyInitialized);

        //
        // Start by acquiring the condition mutex.
        //
        const int mutexAcquireStatus = pthread_mutex_lock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == mutexAcquireStatus);

        //
        // Mark the state as non signaled - this variable is guarded by the mutex.
        // So its access is safe here.
        //
        this->m_SignalHandle.IsSignaled = false;

        //
        // And now release the condition mutex.
        //
        const int releaseStatus = pthread_mutex_unlock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == releaseStatus);

    #else
        #error Unrecognized Platform
    #endif
}

bool
XPF_API
xpf::Signal::Wait(
    _In_ uint32_t TimeoutInMilliSeconds
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // We'll know if the event is signaled or not (the wait was satisfied or not).
    // And we'll use this variable to return this information to the caller.
    //
    bool waitSatisfied = false;

    #if defined XPF_PLATFORM_WIN_UM
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        const DWORD waitResult = ::WaitForSingleObject(this->m_SignalHandle.Handle,
                                                       TimeoutInMilliSeconds);
        waitSatisfied = (WAIT_OBJECT_0 == waitResult) ? true
                                                      : false;
    #elif defined XPF_PLATFORM_WIN_KM
        //
        // Specifies the absolute or relative time, in units of 100 nanoseconds,
        // for which the wait is to occur. A negative value indicates relative time
        //
        LARGE_INTEGER interval = { 0 };

        //
        // 1 millisecond = 1000000 nanoseconds
        // 1 millisecond = 10000 (100 ns)
        //
        interval.QuadPart = static_cast<LONG64>(-10000) * static_cast<LONG64>(TimeoutInMilliSeconds);

        //
        // Now wait for the event. See msdn for KeWaitForSingleObject.
        // We don't handle indepenedent statuses here. Only care about SUCCESS or not.
        //
        XPF_ASSERT(nullptr != this->m_SignalHandle.Handle);
        const NTSTATUS status = ::KeWaitForSingleObject(this->m_SignalHandle.Handle,
                                                        Executive,
                                                        KernelMode,
                                                        FALSE,
                                                        &interval);
        waitSatisfied = (STATUS_SUCCESS == status) ? true
                                                   : false;

    #elif defined XPF_PLATFORM_LINUX_UM
        XPF_ASSERT(this->m_SignalHandle.IsProperlyInitialized);

        //
        // First we do the legwork of computing the absolute time.
        // We need to grab the current time.
        //
        timeval now;
        xpf::ApiZeroMemory(&now, sizeof(now));

        //
        // The wait is not satisfied if we fail to get the time.
        // Not much we can do, exit here.
        //
        if (0 != gettimeofday(&now, NULL))
        {
            return false;
        }
        const time_t currentTimeInSeconds = (now.tv_sec + (now.tv_usec / 1000000));  // Seconds + (microseconds in seconds)

        //
        // Compute the absolute time. We are good citizens and on the unlikely case of overflow,
        // we'll use the max possible value.
        //
        timespec waitUntil;
        xpf::ApiZeroMemory(&waitUntil, sizeof(waitUntil));
                                                                    //
        waitUntil.tv_sec = ((TimeoutInMilliSeconds / 1000) + 1);    // Add the timeout in milliseconds.
                                                                    // And add one second for loss - eg when we provide 500 ms.
                                                                    //
        waitUntil.tv_sec += currentTimeInSeconds;
        if (waitUntil.tv_sec < currentTimeInSeconds)
        {
            waitUntil.tv_sec = xpf::NumericLimits<time_t>::MaxValue();
        }

        //
        // Start by acquiring the condition mutex.
        //
        const int mutexAcquireStatus = pthread_mutex_lock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == mutexAcquireStatus);

        //
        // Now we wait in a loop as we have a special case to handle
        // when we have an auto reset event.
        //
        while (true)
        {
            //
            // If the state is signaled, we bail early.
            //
            if (this->m_SignalHandle.IsSignaled)
            {
                if (!this->m_SignalHandle.IsManualResetEvent)
                {
                    this->m_SignalHandle.IsSignaled = false;
                }
                waitSatisfied = true;
                break;
            }

            const int condWaitStatus = pthread_cond_timedwait(&this->m_SignalHandle.ConditionVariable,
                                                              &this->m_SignalHandle.ContitionMutex,
                                                              &waitUntil);
            //
            // We timed out. Stop the loop.
            //
            if (0 != condWaitStatus)
            {
                break;
            }
        }

        //
        // And now release the mutex.
        //
        const int releaseStatus = pthread_mutex_unlock(&this->m_SignalHandle.ContitionMutex);
        XPF_VERIFY(0 == releaseStatus);

    #else
        #error Unrecognized Platform
    #endif

    return waitSatisfied;
}

const xpf::SignalHandle*
XPF_API
xpf::Signal::SignalHandle(
    void
) const noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();
    return &this->m_SignalHandle;
}
