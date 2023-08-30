/**
 * @file        xpf_lib/public/Multithreading/ThreadPool.hpp
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


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/PlatformApi.hpp"

#include "xpf_lib/public/Multithreading/Thread.hpp"
#include "xpf_lib/public/Multithreading/Signal.hpp"
#include "xpf_lib/public/Multithreading/RundownProtection.hpp"

#include "xpf_lib/public/Locks/ReadWriteLock.hpp"
#include "xpf_lib/public/Containers/AtomicList.hpp"
#include "xpf_lib/public/Memory/LookasideListAllocator.hpp"


namespace xpf
{

/**
 * @brief   Serves as a container for threads.
 *          It dynamically grows depending on the workload.
 */
class ThreadPool final
{
 private:
/**
 * @brief ThreadPool constructor - default.
 */
ThreadPool(
    void
) noexcept(true) = default;

 public:
/**
 * @brief Default destructor.
 */
~ThreadPool(
    void
) noexcept(true)
{
    this->Rundown();
}

/**
 * @brief Copy constructor - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 */
ThreadPool(
    _In_ _Const_ const ThreadPool& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
ThreadPool(
    _Inout_ ThreadPool&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - deleted.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
ThreadPool&
operator=(
    _In_ _Const_ const ThreadPool& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - deleted.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
ThreadPool&
operator=(
    _Inout_ ThreadPool&& Other
) noexcept(true) = delete;


/**
 * @brief Enqueues a work item to be executed on the threadpool.
 *
 * @param[in] UserCallback - Callback to be run.
 *
 * @param[in] NotProcessedCallback - When Rundown() is called, we won't call UserCallback,
 *                                   but instead we'll use NotProcessedCallback for fast exit.
 *                                   This should contain as little logic as possible, and only
 *                                   cleanup the resources for UserCallbackArgument.
 *
 * @param[in] UserCallbackArgument - Argument to be provided to the callback.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Enqueue(
    _In_ xpf::thread::Callback UserCallback,
    _In_ xpf::thread::Callback NotProcessedCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true);

/**
 * @brief Waits until all items in threadpool are executed.
 *        Blocks further items from being scheduled.
 * 
 * @return None.
 */
void
XPF_API
Rundown(
    void
) noexcept(true);

/**
 * @brief Create and initialize a ThreadPool. This must be used instead of constructor.
 *        It ensures the ThreadPool is not partially initialized.
 *        This is a middle ground for not using exceptions and not calling ApiPanic() on fail.
 *        We allow a gracefully failure handling.
 *
 * @param[in, out] ThreadPoolToCreate - the ThreadPool to be created. On input it will be empty.
 *                                      On output it will contain a fully initialized ThreadPool
 *                                      or an empty one on fail.
 *
 * @return A proper NTSTATUS error code on fail, or STATUS_SUCCESS if everything went good.
 * 
 * @note The function has strong guarantees that on success ThreadPoolToCreate has a value
 *       and on fail ThreadPoolToCreate does not have a value.
 */
_Must_inspect_result_
static NTSTATUS
XPF_API
Create(
    _Inout_ xpf::Optional<xpf::ThreadPool>* ThreadPoolToCreate
) noexcept(true);

 private:
/**
 * @brief This will create a new threadpool thread.
 *        This will be enqueued in m_ThreadsList.
 * 
 * @return a proper NTSTATUS value.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
CreateThreadContext(
    void
) noexcept(true);

/**
 * @brief This is used to destroy a thread context.
 *        Frees all resources allocated for it.
 *
 * @param[in,out] ThreadContext - to be destroyed.
 */
void
XPF_API
DestroyThreadContext(
    _Inout_ void** ThreadContext
) noexcept(true);


/**
 * @brief This will create a new threadpool work item.
 *        This is what will actually be processed.
 * 
 * @param[in] ThreadContext - The context of a thread where this item  was selected to run.
 * 
 * @param[in] UserCallback - Callback to be run.
 *
 * @param[in] NotProcessedCallback - When Rundown() is called, we won't call UserCallback,
 *                                   but instead we'll use NotProcessedCallback for fast exit.
 *                                   This should contain as little logic as possible, and only
 *                                   cleanup the resources for UserCallbackArgument.
 *
 * @param[in] UserCallbackArgument - Argument to be provided to the callback.
 *
 * @return STATUS_SUCCESS if everything went well,
 *         a proper NTSTATUS error code if not.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
CreateWorkItem(
    _In_ void* ThreadContext,
    _In_ xpf::thread::Callback UserCallback,
    _In_ xpf::thread::Callback NotProcessedCallback,
    _In_opt_ xpf::thread::CallbackArgument UserCallbackArgument
) noexcept(true);

/**
 * @brief This is used to destroy a work item.
 *        Frees all resources allocated for it.
 *
 * @param[in,out] WorkItem - to be destroyed.
 */
void
XPF_API
DestroyWorkItem(
    _Inout_ void** WorkItem
) noexcept(true);


/**
 * @brief       This is the main threadpool callback.
 *              This is executed on each thread.
 *
 * @param[in] Context - a ThreadPoolThreadContext pointer.
 */
static void
XPF_API
ThreadPoolMainCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true);

/**
 * @brief       This callback is used to process a list of work items.
 *
 * @param[in]   ThreadPoolContext - a ThreadPoolThreadContext pointer.
 *                                  This contains the work queue to be flushed.
 *
 * @param[out]  WorkItemsProcessed - an optional parameter which receives
 *                                   the number of work items that has been processed
 *                                   in the current iteration.
 */
static void
XPF_API
ThreadPoolProcessWorkItems(
    _In_ void* ThreadPoolContext,
    _Out_opt_ size_t* WorkItemsProcessed
) noexcept(true);

 private:
     /**
      * @brief   This is used to block the creation of all new threads.
      *          And the scheduling of new items.
      */
     xpf::Optional<xpf::RundownProtection> m_ThreadpoolRundown;
    /**
     * @brief   This will guard the list of threads and will
     *          ensure that it remains consistent.
     */
     xpf::Optional<xpf::ReadWriteLock> m_ThreadpoolLock;
    /**
     * @brief   This will store the underlying threads.
     *          It will grow dynamically depending on the workload.
     * 
     * @note    This is guarded by m_ThreadpoolLock.
     */
     xpf::AtomicList m_ThreadsList;
    /**
     * @brief   This will hold the current number of threads in m_ThreadsList.
     *          It will change when new threads are spawned.
     * 
     * @note    This is guarded by m_ThreadpoolLock.
     */
     size_t m_NumberOfThreads = 0;
    /**
     * @brief   We'll use a lookaside list memory allocator to construct the work items.
     *          They all have the same size and they are the perfect match.
     */
     xpf::Optional<xpf::LookasideListAllocator> m_WorkItemAllocator;


    /**
     * @brief   We will enqueue in a round-robin manner.
     *          If we have n threads, then we will enqueue the first item to the first thread,
     *          the second to the second thread, and so on.
     *
     * @note    The access to this variable is not synchronized (don't really need to).
     *          This is because we don't care if more work items go to one thread or another.
     *          We just want to ensure a minimalist load balancing distributed among threads.
     *          So this index will always be in [0, m_NumberOfThreads - 1].
     */
     XPF_SINGLE_LIST_ENTRY* m_RoundRobinIndex = nullptr;


    /**
     * @brief   This controls when should we spawn a new thread.
     *          If a thread has more than a certain number items at once
     *          it will spawn a new thread to consume them.
     */
     static constexpr size_t MAX_WORKLOAD_SIZE = 64;
    /**
     * @brief   This controls how many threads we can spawn. We won't exceed this number.
     *          Something smarter could be done (depending on current cores).
     *          We can come back to this later.
     */
     static constexpr size_t MAX_THREAD_QUOTA = 16;
    /**
     * @brief   This controls how many threads we spawn during creation of threadpool.
     *          We can't start with a single one as a flood of work items can be enqueued
     *          at the beginning - puting a lot of work on a single threads.
     *          We can come back to this later.
     */
     static constexpr size_t INITIAL_THREAD_QUOTA = 2;


    /**
     * @brief   Default MemoryAllocator is our friend as it requires access to the private
     *          default constructor. It is used in the Create() method to ensure that
     *          no partially constructed objects are created but instead they will be
     *          all fully initialized.
     */

     friend class xpf::MemoryAllocator;
};  // class ThreadPool
};  // namespace xpf
