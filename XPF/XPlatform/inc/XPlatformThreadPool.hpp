/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_THREADPOOL_HPP__
#define __XPLATFORM_THREADPOOL_HPP__

//
// This file contains a threadpool implementation.
// It has 2 methods:
//      > CallbackRoutine
//          --> this is called under normal circumstances. Shoud contain the processing logic.
//      > NotProcessedRoutine
//          --> this is called when the threadpool exit was signaled. Must contain only cleanup logic
//

namespace XPF
{
    struct ThreadPoolWorkitem
    {
    public:
        ThreadPoolWorkitem(
            _In_opt_ XPF::ThreadCallback CallbackRoutine, 
            _In_opt_ XPF::ThreadCallback NotProcessedRoutine,
            _In_opt_ void* WorkItem
        ) noexcept: callbackRoutine{ CallbackRoutine },
                    notProcessedRoutine{ NotProcessedRoutine },
                    workItem{ WorkItem } 
        { }
        ~ThreadPoolWorkitem() noexcept = default;

        // Copy semantics -- deleted (We can't copy the work item)
        ThreadPoolWorkitem(const ThreadPoolWorkitem&) noexcept = delete;
        ThreadPoolWorkitem& operator=(const ThreadPoolWorkitem&) noexcept = delete;

        // Move semantics -- deleted (nor we can move it)
        ThreadPoolWorkitem(ThreadPoolWorkitem&&) noexcept = delete;
        ThreadPoolWorkitem& operator=(ThreadPoolWorkitem&&) noexcept = delete;

    public:
        XPF::ThreadCallback callbackRoutine = nullptr;
        XPF::ThreadCallback notProcessedRoutine = nullptr;
        void* workItem = nullptr;
    };

    enum class ThreadPoolState
    {
        Uninitialized = 0,
        Initialized = 1,
        Running = 2,
    };

    template < xp_int8_t NoThreads, 
               class LockType  = ReadWriteLock, 
               class Allocator = MemoryAllocator<ThreadPoolWorkitem>>
    class ThreadPool
    {
        static_assert(__is_base_of(MemoryAllocator<ThreadPoolWorkitem>, Allocator), "Allocators should derive from MemoryAllocator");
        static_assert(__is_base_of(ExclusiveLock, LockType), "Locks must derive from ExclusiveLock class");
        static_assert(2 <= NoThreads && NoThreads <= 4, "Invalid Number of Threads");

    public:
        ThreadPool() noexcept : semaphore(NoThreads) { }
        ~ThreadPool() noexcept { XPLATFORM_ASSERT(this->state == ThreadPoolState::Uninitialized); }

        // Copy semantics -- deleted (We can't copy the threadpool)
        ThreadPool(const ThreadPool&) noexcept = delete;
        ThreadPool& operator=(const ThreadPool&) noexcept = delete;

        // Move semantics -- deleted (nor we can move it)
        ThreadPool(ThreadPool&&) noexcept = delete;
        ThreadPool& operator=(ThreadPool&&) noexcept = delete;

        //
        // Enqueue a work item to the threadpool.
        //
        _Must_inspect_result_
        bool 
        SubmitWork(
            _In_ XPF::ThreadCallback CallbackRoutine,
            _In_ XPF::ThreadCallback NotProcessedRoutine,
            _In_opt_ void* WorkItem
        ) noexcept
        {
            //
            // Both routines must be provided.
            // A work item is not necessarily required.
            //
            XPLATFORM_ASSERT(CallbackRoutine != nullptr);
            XPLATFORM_ASSERT(NotProcessedRoutine != nullptr);

            if (nullptr == CallbackRoutine || nullptr == NotProcessedRoutine)
            {
                return false;
            }

            //
            // Threadpool was stopped. We first check without having the lock taken.
            // 
            if (this->state != ThreadPoolState::Running)
            {
                return false;
            }

            //
            // Now recheck the state with the lock taken to avoid races.
            //
            XPF::ExclusiveLockGuard guard{ this->workQueueLock };
            if (this->state != ThreadPoolState::Running)
            {
                return false;
            }

            //
            // If we can't insert to the queue, we exit.
            //
            if (!this->workQueue.InsertTail(CallbackRoutine, NotProcessedRoutine, WorkItem))
            {
                return false;
            }

            //
            // Signal workers to pick up the work item
            //
            this->semaphore.Release();
            return true;
        }

        //
        // Starts a threadpool.
        // This method must be called before doing any actual work.
        // This routine should not be called from multiple threads.
        //
        _Must_inspect_result_
        _No_competing_thread_
        bool Start(void) noexcept 
        {
            //
            // The lock is not currently initialized.
            // We fail the start and bail out quickly.
            //
            if (this->workQueueLock.State() == XPF::LockState::Uninitialized)
            {
                this->state = ThreadPoolState::Uninitialized;
                return false;
            }

            //
            // Something went wrong with the semaphore initialization.
            // Bail out quickly.
            //
            if (this->semaphore.SemaphoreLimit() != NoThreads)
            {
                this->state = ThreadPoolState::Uninitialized;
                return false;
            }

            //
            // We do the initialization lock free.
            // This routine is prone to data races.
            // Don't call it twice.
            //
            XPLATFORM_ASSERT(this->state == ThreadPoolState::Initialized);

            //
            // From this point onward, the threadpool is in running state.
            //
            this->state = ThreadPoolState::Running;

            //
            // Try to start the threads independently.
            // If some threads fail to start, we stop and report the failure.
            //
            for (xp_uint8_t i = 0; i < NoThreads; ++i)
            {
                if (!this->threads[i].Run(ThreadPoolRoutine, this))
                {
                    CleanupThreads();
                    return false;
                }
            }
            return true;
        }

        //
        // Stops a threadpool from processing other work items.
        // Waits for all threads to finish execution then block inserts.
        // Don't call this method twice
        //
        void Stop(void) noexcept
        {
            //
            // The lock is not currently initialized.
            // We bail out quickly.
            //
            if (this->workQueueLock.State() == XPF::LockState::Uninitialized)
            {
                this->state = ThreadPoolState::Uninitialized;
                return;
            }
            //
            // Semaphore was not properly initialized.
            // Return from here.
            //
            if (this->semaphore.SemaphoreLimit() != NoThreads)
            {
                this->state = ThreadPoolState::Uninitialized;
                return;
            }

            //
            // We can cleanup the threads.
            //
            CleanupThreads();
        }

    private:
        //
        // This is the worker callback routine.
        // Threads will run inside a while loop until the Stop() is signaled.
        //
        static void ThreadPoolRoutine(_Inout_opt_ void* Context) noexcept
        {
            auto threadpool = reinterpret_cast<ThreadPool*>(Context);
            if (threadpool == nullptr)
            {
                XPLATFORM_ASSERT(false);
                return;
            }

            while (threadpool->state == ThreadPoolState::Running)
            {
                List<ThreadPoolWorkitem, Allocator> processingQueue;

                threadpool->semaphore.Wait();
  
                //
                // Process as much as possible without acquiring the lock.
                // Simply move all elements in a separate processing queue.
                //
                {
                    XPF::ExclusiveLockGuard guard{ threadpool->workQueueLock };
                    XPF::Swap(processingQueue, threadpool->workQueue);
                }
                threadpool->ProcessWorkList(processingQueue);
            }
        }

        //
        // This method is used to process parts of work items.
        // It will do so without any lock taken.
        // If the threadpool stop method is signaled, it will call NotProcessedRoutine to bail out as quickly as possible
        //
        _Requires_lock_not_held_(this->workQueueLock)
        void 
        ProcessWorkList(
            _Inout_ List<ThreadPoolWorkitem, Allocator>& WorkList
        ) noexcept
        {
            for (auto& element : WorkList)
            {
                //
                // If the threadpool has to stop we exit as quickly as possible.
                // Check for each work item independently.
                //
                if (this->state == ThreadPoolState::Running)
                {
                    if (element.callbackRoutine != nullptr)
                    {
                        element.callbackRoutine(element.workItem);
                    }
                }
                else
                {
                    if (element.notProcessedRoutine != nullptr)
                    {
                        element.notProcessedRoutine(element.workItem);
                    }
                }
            }
            WorkList.Clear();
        }

        //
        // This method is called to properly dispose the resources.
        //
        _Requires_lock_not_held_(this->workQueueLock)
        void 
        CleanupThreads(
            void
        ) noexcept
        {

            XPLATFORM_ASSERT(this->state != ThreadPoolState::Uninitialized);

            //
            //  Mark the state as uninitialized to block further inserts 
            //
            workQueueLock.LockExclusive();
            this->state = ThreadPoolState::Uninitialized;
            
            //
            // First release the semaphore to wake all threads
            //
            for (xp_int8_t i = 0; i < NoThreads; ++i)
            {
                this->semaphore.Release();
            }
            workQueueLock.UnlockExclusive();

            //
            // Now wait for each thread to finish
            //
            for (xp_int8_t i = 0; i < NoThreads; ++i)
            {
                if (threads[i].Joinable())
                {
                    threads[i].Join();
                }
            }

            //
            // Clean the rest of the items in queue.
            //
            ProcessWorkList(workQueue);
        }

    private:
        Thread threads[NoThreads];
        Semaphore semaphore;
        LockType workQueueLock;
        List<ThreadPoolWorkitem, Allocator> workQueue;
        volatile ThreadPoolState state = ThreadPoolState::Initialized;
    };
}

#endif // __XPLATFORM_THREADPOOL_HPP__