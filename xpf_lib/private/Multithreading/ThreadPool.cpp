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

/**
 * @brief   This will be enqueued to be executed by the threadpool.
 */
struct ThreadPoolWorkItem
{
    /**
     * @brief   This callback will be ran by the worker thread.
     */
    xpf::thread::Callback ThreadCallback = nullptr;

    /**
     * @brief   This callback will be ran by the worker thread
     *          when Rundown() is called
     */
    xpf::thread::Callback ThreadRundownCallback = nullptr;

    /**
     * @brief   The context to be passed to callback.
     */
    xpf::thread::CallbackArgument ThreadCallbackArgument = nullptr;

    /**
     * @brief   List entry as this structure will be enqueued in a thread list.
     *          Required by the underlying API.
     */
    xpf::XPF_SINGLE_LIST_ENTRY WorkItemListEntry = { 0 };
};

/**
 * @brief   Serves as a container for threads.
 *          This will be passed by threadpool to each thread.
 */
struct ThreadPoolThreadContext
{
    /**
     * @brief   Identifies the current running thread.
     */
    xpf::thread::Thread CurrentThread;

    /**
     * @brief   Serves as a container for threads.
     *          The CurrentThread is part of this threadpool.
     */
    xpf::ThreadPool* OwnerThreadPool = nullptr;

    /**
     * @brief   Signals the thread to wake up - something changed. 
     */
    xpf::Optional<xpf::Signal> WakeUpSignal;

    /**
     * @brief   Signals the thread that it should exit as soon as possible. 
     */
    bool IsShutdownSignaled = false;

    /**
     * @brief   A list of enqueued work items to be processed.
     */
    xpf::AtomicList WorkQueue;

    /**
     * @brief   Threads are also stored in their own list inside threadpool.
     */
    xpf::XPF_SINGLE_LIST_ENTRY ThreadListEntry = { 0 };
};

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ThreadPool::CreateWorkItem(
    _In_ void* ThreadContext,
    _In_ xpf::thread::Callback UserCallback,
    _In_ xpf::thread::Callback NotProcessedCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true)
{
    //
    // This can be called from DISPATCH_LEVEL.
    //
    XPF_MAX_DISPATCH_LEVEL();

    ThreadPoolThreadContext* threadContext = nullptr;
    ThreadPoolWorkItem* workItem = nullptr;

    //
    // Arguments checking.
    //
    if ((nullptr == ThreadContext) || (nullptr == UserCallback) || (nullptr == NotProcessedCallback))
    {
        return STATUS_INVALID_PARAMETER;
    }
    threadContext = reinterpret_cast<ThreadPoolThreadContext*>(ThreadContext);

    //
    // Work items will be stored in non-paged memory. They are critical allocations.
    //
    workItem = reinterpret_cast<ThreadPoolWorkItem*>((*this->m_WorkItemAllocator).AllocateMemory(sizeof(*workItem)));
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
    threadContext->WorkQueue.Insert(&workItem->WorkItemListEntry);
    (*threadContext->WakeUpSignal).Set();

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
    (*this->m_WorkItemAllocator).FreeMemory(reinterpret_cast<void**>(&workItem));

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
    xpf::RundownGuard guard{ (*this->m_ThreadpoolRundown) };
    if (!guard.IsRundownAcquired())
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Now that we are here, the threadpool won't shutdown until we finish.
    // So we go into our round-robin algorithm. Don't guard access to m_RoundRobinIndex
    // as the only guarantee we need is to be a valid thread. And the thread list->next will
    // never be modified (we can't pop from atomic list and we won't flush as the rundown is acquired).
    //
    auto roundRobinIndex = this->m_RoundRobinIndex;
    if (nullptr == roundRobinIndex)
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    //
    // If we reached the end, we go to the head of the list. We do that here before doing too many
    // operations.
    //
    this->m_RoundRobinIndex = (nullptr == roundRobinIndex->Next) ? this->m_ThreadsList.Head()
                                                                 : roundRobinIndex->Next;

    ThreadPoolThreadContext* threadContext = XPF_CONTAINING_RECORD(roundRobinIndex, ThreadPoolThreadContext, ThreadListEntry);
    if (nullptr == threadContext)
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    //
    // Now we have our thread context - enqueue the item in its list.
    //
    return this->CreateWorkItem(threadContext,
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
    // First thing first - we create the rundown.
    //
    status = xpf::RundownProtection::Create(&newThreadpool.m_ThreadpoolRundown);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Then the lock.
    //
    status = xpf::ReadWriteLock::Create(&newThreadpool.m_ThreadpoolLock);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Then the allocator.
    //
    status = xpf::LookasideListAllocator::Create(&newThreadpool.m_WorkItemAllocator,
                                                 sizeof(ThreadPoolWorkItem),
                                                 true);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    //
    // Then the initial number of threads.
    //
    XPF_DEATH_ON_FAILURE(newThreadpool.INITIAL_THREAD_QUOTA != 0);
    for (size_t i = 0; i < newThreadpool.INITIAL_THREAD_QUOTA; ++i)
    {
        status = newThreadpool.CreateThreadContext();
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }
    XPF_DEATH_ON_FAILURE(newThreadpool.m_NumberOfThreads == newThreadpool.INITIAL_THREAD_QUOTA);

    //
    // Now we set the round robin index to the first element in the list.
    //
    newThreadpool.m_RoundRobinIndex = newThreadpool.m_ThreadsList.Head();
    XPF_DEATH_ON_FAILURE(nullptr != newThreadpool.m_RoundRobinIndex);

    //
    // All good. Signal success.
    //
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

    XPF_SINGLE_LIST_ENTRY* crtEntry = nullptr;

    //
    // Block further inserts and further thread creations.
    //
    if (this->m_ThreadpoolRundown.HasValue())
    {
        (*this->m_ThreadpoolRundown).WaitForRelease();
    }

    //
    // If the lock was not initialized, We're done.
    //
    if (!this->m_ThreadpoolLock.HasValue())
    {
        return;
    }

    xpf::ExclusiveLockGuard guard{ (*this->m_ThreadpoolLock) };

    this->m_ThreadsList.Flush(&crtEntry);
    this->m_NumberOfThreads = 0;
    this->m_RoundRobinIndex = nullptr;

    //
    // Further threads won't be allowed as we marked the thread pool for run down.
    //
    while (nullptr != crtEntry)
    {
        //
        // Grab the current thread context and move to the next one.
        //
        ThreadPoolThreadContext* threadContext = XPF_CONTAINING_RECORD(crtEntry, ThreadPoolThreadContext, ThreadListEntry);
        crtEntry = crtEntry->Next;

        //
        // Now Destroy the context.
        //
        this->DestroyThreadContext(reinterpret_cast<void**>(&threadContext));
    }
}

_Must_inspect_result_
NTSTATUS
xpf::ThreadPool::CreateThreadContext(
    void
) noexcept(true)
{
    //
    // This should be called only at passive level.
    // Should be called eiterh from Create() flow, either from a threadpool thread.
    //
    XPF_MAX_PASSIVE_LEVEL();

    ThreadPoolThreadContext* threadContext = nullptr;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // If we can't acquire the rundown guard, it means we should bail
    // as shutdown is in progress. While we are creating the thread, we will hold this guard.
    // This is not a lock, so not as expensive.
    //
    xpf::RundownGuard rundownGuard{ (*this->m_ThreadpoolRundown) };
    if (!rundownGuard.IsRundownAcquired())
    {
        status = STATUS_SHUTDOWN_IN_PROGRESS;
        goto CleanUp;
    }

    //
    // First check if we can create a new thread.
    //
    if (this->m_NumberOfThreads >= this->MAX_THREAD_QUOTA)
    {
        status = STATUS_QUOTA_EXCEEDED;
        goto CleanUp;
    }

    //
    // Threads will be stored in non-paged memory. They are critical allocations.
    //
    threadContext = reinterpret_cast<ThreadPoolThreadContext*>(
                                    xpf::CriticalMemoryAllocator::AllocateMemory(sizeof(*threadContext)));
    if (nullptr == threadContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }
    xpf::MemoryAllocator::Construct(threadContext);

    //
    // Now to properly initialize the threadContext we need WakeUpSignal.
    // This will be an auto-reset event (so manual reset = false).
    // It will wake the thread multiple times when it has a work item.
    //
    status = xpf::Signal::Create(&threadContext->WakeUpSignal, false);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now set the owner threadpool accordingly.
    // This must be done before running the callback routine.
    //
    threadContext->OwnerThreadPool = this;

    //
    // And now start the thread.
    //
    status = threadContext->CurrentThread.Run(ThreadPoolMainCallback, threadContext);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now attempt to add the thread to the main threads list.
    // Check again for m_NumberOfThreads. Hold the lock with minimal scope.
    //
    {
        xpf::ExclusiveLockGuard lockGuard{ (*this->m_ThreadpoolLock) };
        if (this->m_NumberOfThreads >= this->MAX_THREAD_QUOTA)
        {
            status = STATUS_QUOTA_EXCEEDED;
        }
        else
        {
            this->m_ThreadsList.Insert(&threadContext->ThreadListEntry);
            this->m_NumberOfThreads++;
            status = STATUS_SUCCESS;
        }
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        DestroyThreadContext(reinterpret_cast<void**>(&threadContext));
        threadContext = nullptr;
    }
    return status;
}

void
XPF_API
xpf::ThreadPool::DestroyThreadContext(
    _Inout_ void** ThreadContext
) noexcept(true)
{
    //
    // Sanity check that we have a valid object.
    //
    if ((nullptr == ThreadContext) || (nullptr == (*ThreadContext)))
    {
        return;
    }

    //
    // Let's get a more familiar form.
    //
    ThreadPoolThreadContext* threadContext = reinterpret_cast<ThreadPoolThreadContext*>(*ThreadContext);

    //
    // Signal the thread that we are shutting down.
    //
    threadContext->IsShutdownSignaled = true;

    //
    // If we have the event we wake up the thread.
    //
    if (threadContext->WakeUpSignal.HasValue())
    {
        (*threadContext->WakeUpSignal).Set();
    }

    //
    // Now wait to finish - it should wake up by the previously set event.
    //
    if (threadContext->CurrentThread.IsJoinable())
    {
        threadContext->CurrentThread.Join();
    }

    //
    // And now process everything that was enqueued and wasn't processed.
    // Don't care how many there were in that list.
    //
    ThreadPoolProcessWorkItems(threadContext, nullptr);

    //
    // Then we can safely release resources.
    //
    xpf::MemoryAllocator::Destruct(threadContext);
    xpf::CriticalMemoryAllocator::FreeMemory(reinterpret_cast<void**>(&threadContext));

    //
    // Don't leave invalid memory...
    //
    *ThreadContext = nullptr;
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
    threadContext->WorkQueue.Flush(&crtEntry);

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
