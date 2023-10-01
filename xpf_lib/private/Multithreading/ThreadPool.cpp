/**
 * @file        xpf_lib/private/Multithreading/ThreadPool.cpp
 *
 * @brief       This is the class to provide basic threadpool functionality.
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
 */
XPF_SECTION_DEFAULT;


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ThreadPool::CreateWorkItem(
    _Inout_ xpf::SharedPointer<xpf::ThreadPoolThreadContext, xpf::CriticalMemoryAllocator>& ThreadContext,
    _In_ xpf::thread::Callback UserCallback,
    _In_ xpf::thread::Callback NotProcessedCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true)
{
    //
    // This can be called from DISPATCH_LEVEL.
    //
    XPF_MAX_DISPATCH_LEVEL();

    ThreadPoolWorkItem* workItem = nullptr;

    //
    // Arguments checking.
    //
    if ((ThreadContext.IsEmpty()) || (nullptr == UserCallback) || (nullptr == NotProcessedCallback))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Work items will be stored in non-paged memory. They are critical allocations.
    //
    workItem = reinterpret_cast<ThreadPoolWorkItem*>(this->m_WorkItemAllocator.AllocateMemory(sizeof(*workItem)));
    if (nullptr == workItem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::MemoryAllocator::Construct(workItem);

    //
    // Now initialize the work item properly.
    //
    workItem->ThreadCallback = UserCallback;
    workItem->ThreadCallbackArgument = UserCallbackArgument;
    workItem->ThreadRundownCallback = NotProcessedCallback;

    //
    // Now add it to thread's queue.
    //
    TlqPush((*ThreadContext).WorkQueue, &workItem->WorkItemListEntry);
    (*(*ThreadContext).WakeUpSignal).Set();

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::ThreadPool::DestroyWorkItem(
    _Inout_ void** WorkItem
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Sanity check that we have a valid object.
    //
    if ((nullptr == WorkItem) || (nullptr == (*WorkItem)))
    {
        return;
    }

    //
    // Let's get a more familiar form.
    //
    ThreadPoolWorkItem* workItem = reinterpret_cast<ThreadPoolWorkItem*>(*WorkItem);

    //
    // Destroy the work item.
    //
    xpf::MemoryAllocator::Destruct(workItem);
    this->m_WorkItemAllocator.FreeMemory(reinterpret_cast<void**>(&workItem));

    //
    // Don't leave invalid memory...
    //
    *WorkItem = nullptr;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ThreadPool::Enqueue(
    _In_ xpf::thread::Callback UserCallback,
    _In_ xpf::thread::Callback NotProcessedCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Sanity check for arguments.
    //
    if ((nullptr == UserCallback) || (nullptr == NotProcessedCallback))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now we try to acquire the rundown - this will prevent shutdown until we are finished.
    //
    xpf::RundownGuard guard{ this->m_ThreadpoolRundown };
    if (!guard.IsRundownAcquired())
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Now that we are here, the threadpool won't shutdown until we finish.
    // So we go into our round-robin algorithm. Don't guard access to m_RoundRobinIndex
    // as the only guarantee we need is to be a valid thread.
    //
    auto currentRoundRobinIndex = this->m_RoundRobinIndex;
    if (currentRoundRobinIndex >= this->m_Threads.Size())
    {
        currentRoundRobinIndex = 0;
    }

    //
    // If we reached the end, we go to the beginning.
    // We do that here before doing too many operations.
    //
    this->m_RoundRobinIndex = (this->m_RoundRobinIndex + 1) % this->m_Threads.Size();

    //
    // The lock will have minimal scope - just grab a reference to the selected thread.
    //
    xpf::SharedPointer<xpf::ThreadPoolThreadContext, xpf::CriticalMemoryAllocator> currentThread;
    {
        xpf::SharedLockGuard threadGuard{ this->m_ThreadsLock };
        if (currentRoundRobinIndex < this->m_Threads.Size())
        {
            currentThread = this->m_Threads[currentRoundRobinIndex];
        }
    }

    //
    // Now we have our thread context - enqueue the item in its list.
    //
    return this->CreateWorkItem(currentThread,
                                UserCallback,
                                NotProcessedCallback,
                                UserCallbackArgument);
}

/**
 * @brief   Threadpool related callback code can be paged out.
 *          Our threads are running at passive anyway.
 */
XPF_SECTION_PAGED;


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ThreadPool::Create(
    _Inout_ xpf::Optional<xpf::ThreadPool>* ThreadPoolToCreate
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // We will not initialize over an already initialized threadpool.
    // Assert here and bail early.
    //
    if ((nullptr == ThreadPoolToCreate) || (ThreadPoolToCreate->HasValue()))
    {
        XPF_DEATH_ON_FAILURE(false);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Start by creating a new threadpool. This will be an empty one.
    // It will be initialized below.
    //
    ThreadPoolToCreate->Emplace();

    //
    // We failed to create an event. It shouldn't happen.
    // Assert here and bail early.
    //
    if (!ThreadPoolToCreate->HasValue())
    {
        XPF_DEATH_ON_FAILURE(false);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Grab a reference to the newly created threadpool. Makes the code easier to read.
    // It will be optimized away on release anyway.
    //
    xpf::ThreadPool& newThreadpool = (*(*ThreadPoolToCreate));

    //
    // Then the initial number of threads.
    //
    static_assert(xpf::ThreadPool::INITIAL_THREAD_QUOTA > 0 && 
                  xpf::ThreadPool::INITIAL_THREAD_QUOTA <= xpf::ThreadPool::MAX_THREAD_QUOTA,
                  "Invalid Initial Thread Quota!");

    for (size_t i = 0; i < xpf::ThreadPool::INITIAL_THREAD_QUOTA; ++i)
    {
        status = newThreadpool.CreateThreadContext();
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    //
    // Now we set the round robin index to the first element in the list.
    // All good. Signal success.
    //
    newThreadpool.m_RoundRobinIndex = 0;
    status = STATUS_SUCCESS;

Exit:
    if (!NT_SUCCESS(status))
    {
        ThreadPoolToCreate->Reset();
        XPF_DEATH_ON_FAILURE(!ThreadPoolToCreate->HasValue());
    }
    else
    {
        XPF_DEATH_ON_FAILURE(ThreadPoolToCreate->HasValue());
    }
    return status;
}

void
XPF_API
xpf::ThreadPool::Rundown(
    void
) noexcept(true)
{
    XPF_MAX_APC_LEVEL();

    //
    // Block further inserts and further thread creations.
    //
    this->m_ThreadpoolRundown.WaitForRelease();

    //
    // Further threads won't be allowed as we marked the thread pool for run down.
    //
    {
        xpf::ExclusiveLockGuard guard{ this->m_ThreadsLock };
        for (size_t i = 0; i < this->m_Threads.Size(); ++i)
        {
            DestroyThreadContext(this->m_Threads[i]);
        }
        this->m_Threads.Clear();
    }

    this->m_RoundRobinIndex = 0;
}

_Must_inspect_result_
NTSTATUS
xpf::ThreadPool::CreateThreadContext(
    void
) noexcept(true)
{
    //
    // This should be called only at passive level.
    // Should be called either from Create() flow, either from a threadpool thread.
    //
    XPF_MAX_PASSIVE_LEVEL();

    xpf::SharedPointer<xpf::ThreadPoolThreadContext, xpf::CriticalMemoryAllocator> threadContextSharedPtr;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // If we can't acquire the rundown guard, it means we should bail
    // as shutdown is in progress. While we are creating the thread, we will hold this guard.
    // This is not a lock, so not as expensive.
    //
    xpf::RundownGuard rundownGuard{ this->m_ThreadpoolRundown };
    if (!rundownGuard.IsRundownAcquired())
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // First check if we can create a new thread.
    //
    if (this->m_Threads.Size() >= this->MAX_THREAD_QUOTA)
    {
        return STATUS_QUOTA_EXCEEDED;
    }

    //
    // Threads will be stored in non-paged memory. They are critical allocations.
    //
    threadContextSharedPtr = xpf::MakeShared<xpf::ThreadPoolThreadContext, xpf::CriticalMemoryAllocator>();
    if (threadContextSharedPtr.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::ThreadPoolThreadContext& threadContext = *threadContextSharedPtr;

    //
    // Now to properly initialize the threadContext we need WakeUpSignal.
    // This will be an auto-reset event (so manual reset = false).
    // It will wake the thread multiple times when it has a work item.
    //
    status = xpf::Signal::Create(&threadContext.WakeUpSignal, false);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now set the owner threadpool accordingly.
    // This must be done before running the callback routine.
    //
    threadContext.OwnerThreadPool = this;

    //
    // And now start the thread.
    //
    status = threadContext.CurrentThread.Run(ThreadPoolMainCallback, xpf::AddressOf(threadContext));
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now attempt to add the thread to the main threads list.
    // Check again for m_NumberOfThreads.
    //
    {
        xpf::ExclusiveLockGuard guard{ this->m_ThreadsLock };
        if (this->m_Threads.Size() >= this->MAX_THREAD_QUOTA)
        {
            status = STATUS_QUOTA_EXCEEDED;
        }
        else
        {
            status = this->m_Threads.Emplace(threadContextSharedPtr);
        }
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        this->DestroyThreadContext(threadContextSharedPtr);
    }
    return status;
}

void
XPF_API
xpf::ThreadPool::DestroyThreadContext(
    _Inout_ xpf::SharedPointer<xpf::ThreadPoolThreadContext, xpf::CriticalMemoryAllocator>& ThreadContext
) noexcept(true)
{
    //
    // Can't destroy empty thread context.
    //
    if (ThreadContext.IsEmpty())
    {
        return;
    }

    //
    // This will ease the access.
    //
    xpf::ThreadPoolThreadContext& threadContext = *ThreadContext;

    //
    // Signal the thread that we are shutting down.
    //
    threadContext.IsShutdownSignaled = true;

    //
    // If we have the event we wake up the thread.
    //
    if (threadContext.WakeUpSignal.HasValue())
    {
        (*threadContext.WakeUpSignal).Set();
    }

    //
    // Now wait to finish - it should wake up by the previously set event.
    //
    if (threadContext.CurrentThread.IsJoinable())
    {
        threadContext.CurrentThread.Join();
    }

    //
    // And now process everything that was enqueued and wasn't processed.
    // Don't care how many there were in that list.
    //
    ThreadPoolProcessWorkItems(&threadContext, nullptr);

    //
    // Then we can safely release resources.
    //
    ThreadContext.Reset();
}


void
XPF_API
xpf::ThreadPool::ThreadPoolMainCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    ThreadPoolThreadContext* threadContext = reinterpret_cast<ThreadPoolThreadContext*>(Context);

    //
    // We always need a valid thread context - this is an internal callback.
    // We control it and we always pass it.
    //
    if (nullptr == threadContext)
    {
        XPF_DEATH_ON_FAILURE(false);
        return;
    }

    //
    // Loop until we need to stop.
    //
    while (!threadContext->IsShutdownSignaled)
    {
        //
        // Wait for some work.
        //
        bool waitSatisfied = (*threadContext->WakeUpSignal).Wait();
        if (!waitSatisfied)
        {
            continue;
        }

        //
        // We're up. Now let's do some processing.
        //
        size_t numberOfItemsProcessed = 0;
        ThreadPoolProcessWorkItems(threadContext, &numberOfItemsProcessed);

        //
        // Might need to spawn a thread if we got a huge workload.
        //
        if (!threadContext->IsShutdownSignaled)
        {
            if (numberOfItemsProcessed >= threadContext->OwnerThreadPool->MAX_WORKLOAD_SIZE)
            {
                (void)threadContext->OwnerThreadPool->CreateThreadContext();
            }
        }
    }

    //
    // Got shutdown. Empty the process list again before exiting.
    //
    ThreadPoolProcessWorkItems(threadContext, nullptr);
}

void
XPF_API
xpf::ThreadPool::ThreadPoolProcessWorkItems(
    _In_ void* ThreadPoolContext,
    _Out_opt_ size_t* WorkItemsProcessed
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    size_t workItemsProcessed = 0;
    XPF_SINGLE_LIST_ENTRY* crtEntry = nullptr;
    ThreadPoolThreadContext* threadContext = reinterpret_cast<ThreadPoolThreadContext*>(ThreadPoolContext);

    //
    // We always need a valid thread context - this is an internal callback.
    // We control it and we always pass it.
    //
    if (nullptr == threadContext)
    {
        XPF_DEATH_ON_FAILURE(false);
        goto CleanUp;
    }

    //
    // Flush the list of work items.
    //
    crtEntry = TlqFlush(threadContext->WorkQueue);

    //
    // Iterate them one by one and process them.
    //
    while (crtEntry != nullptr)
    {
        //
        // Grab the work item and move to the next one.
        //
        ThreadPoolWorkItem* workItem = XPF_CONTAINING_RECORD(crtEntry, ThreadPoolWorkItem, WorkItemListEntry);
        crtEntry = crtEntry->Next;

        //
        // Now process the work item.
        //
        if (nullptr != workItem)
        {
            //
            // If shutdown is signaled we execute the rundown callback, otherwise the ThreadCallback.
            //
            xpf::thread::Callback callbackToRun = (threadContext->IsShutdownSignaled) ? workItem->ThreadRundownCallback
                                                                                      : workItem->ThreadCallback;
            if (nullptr != callbackToRun)
            {
                callbackToRun(workItem->ThreadCallbackArgument);
                workItemsProcessed++;
            }

            //
            // Now let's clean the allocated resources for the work item.
            //
            threadContext->OwnerThreadPool->DestroyWorkItem(reinterpret_cast<void**>(&workItem));
        }
    }

CleanUp:
    if (nullptr != WorkItemsProcessed)
    {
        *WorkItemsProcessed = workItemsProcessed;
    }
}
