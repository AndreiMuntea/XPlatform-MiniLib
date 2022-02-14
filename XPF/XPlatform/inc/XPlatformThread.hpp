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

#ifndef __XPLATFORM_THREAD_HPP__
#define __XPLATFORM_THREAD_HPP__

//
// This file contains basic thread functionality.
//

namespace XPF
{
    //
    // Basic method to pass to a thread
    //
    typedef void(*ThreadCallback)(
        _Inout_opt_ void* Context
    ) noexcept;

    class Thread
    {
    public:
        Thread() noexcept = default;
        ~Thread() noexcept { Dispose(); };

        // Copy semantics -- deleted (We can't copy the thread)
        Thread(const Thread&) noexcept = delete;
        Thread& operator=(const Thread&) noexcept = delete;

        // Move semantics -- deleted (nor we can move it)
        Thread(Thread&&) noexcept = delete;
        Thread& operator=(Thread&&) noexcept = delete;

        //
        // Schedules the thread to run.
        // Do not run this method twice - it will lead to undefined behavior.
        //
        _No_competing_thread_
        _Must_inspect_result_
        bool Run(ThreadCallback Callback, void* Context) noexcept
        {
            //
            // Can't run the same thread object twice.
            // It may lead to memory leaks. Try to guard as much as possible.
            //
            if (this->wasCreated)
            {
                XPLATFORM_ASSERT(false);
                return false;
            }

            //
            // Initialize callback and the context.
            // These will be run on a separate thread.
            // 
            this->callback = Callback;
            this->context = Context;

            //
            // Platform specific API to create a thread.
            // It may fail in case of low system resources.
            // The result of this function must always be inspected.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                this->thread = CreateThread(nullptr, 
                                            0, 
                                            ThreadStartRoutine, 
                                            reinterpret_cast<void*>(this), 
                                            0, 
                                            nullptr);
                this->wasCreated = (this->thread != NULL && this->thread != INVALID_HANDLE_VALUE);
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                OBJECT_ATTRIBUTES objectAttributes = { 0 };
                InitializeObjectAttributes(&objectAttributes, 
                                           nullptr, 
                                           OBJ_KERNEL_HANDLE, 
                                           nullptr, 
                                           nullptr);

                auto status = PsCreateSystemThread(&this->thread, 
                                                   THREAD_ALL_ACCESS, 
                                                   &objectAttributes, 
                                                   nullptr, 
                                                   nullptr, 
                                                   ThreadStartRoutine, 
                                                   reinterpret_cast<void*>(this));
                if (!NT_SUCCESS(status))
                {
                    XPLATFORM_ASSERT(NT_SUCCESS(status));
                    this->thread = INVALID_HANDLE_VALUE;
                }
                this->wasCreated = (NT_SUCCESS(status));
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                auto error = pthread_create(&thread, nullptr, ThreadStartRoutine, reinterpret_cast<void*>(this));
                if (error != 0)
                {
                    XPLATFORM_ASSERT((error != 0));
                }
                this->wasCreated = (error == 0);
            #else
                #error Unsupported Platform
            #endif
            
            return this->wasCreated;
        }

        //
        // Waits for the thread to finish.
        // DO NOT call this method if Run was not called or it failed.
        // This method CAN be manually called before the thread is actually destroyed.
        //
        _Must_inspect_result_
        bool Join(void) noexcept
        {
            //
            // Can't join a thread that was not created.
            // Join should not be called in such scenarios.
            // Assert here and bail out quickly.
            //
            if (!this->wasCreated)
            {
                XPLATFORM_ASSERT(false);
                return false;
            }

            //
            // Join should not be called twice.
            // Return true as thread was already joined.
            //
            if (XPF::ApiAtomicExchange(&this->wasJoined, xp_int8_t{ 1 }) == 1)
            {
                return true;
            }

            //
            // Platform specific API to wait for the thread.
            // It will wait indefinitely. Similar with std::thread behavior.
            // Won't attempt to kill the thread as that may lead to leaving locks acquired.
            // It is better to have a hanging thread as it will be easier to investigate the problem as well.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                return (WAIT_OBJECT_0 == WaitForSingleObject(this->thread, INFINITE));
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                return NT_SUCCESS(ZwWaitForSingleObject(this->thread, FALSE, nullptr));
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                return (0 == pthread_join(this->thread, nullptr));
            #else
                #error Unsupported Platform
            #endif
        }

    private:
        //
        // The actual platform specific thread routine.
        // It is platform-dependant so we need a glue method to be called.
        // In this method we simply call the actual callback.
        //
        #if defined (XPLATFORM_WINDOWS_USER_MODE)
            static DWORD WINAPI ThreadStartRoutine(void* Argument) noexcept
            {
                auto thread = reinterpret_cast<XPF::Thread*>(Argument);
                if (thread != nullptr && thread->callback != nullptr)
                {
                    thread->callback(thread->context);
                }
                return 0;
            }
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            static void ThreadStartRoutine(void* Argument) noexcept
            {
                auto thread = reinterpret_cast<XPF::Thread*>(Argument);
                if (thread != nullptr && thread->callback != nullptr)
                {
                    thread->callback(thread->context);
                }
            }
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            static void* ThreadStartRoutine(void* Argument) noexcept
            {
                auto thread = reinterpret_cast<XPF::Thread*>(Argument);
                if (thread != nullptr && thread->callback != nullptr)
                {
                    thread->callback(thread->context);
                }
                return nullptr;
            }
        #else
            #error Unsupported Platform
        #endif

        //
        // Some platforms require extra cleanup after a thread ended.
        // It ensures the thread was stopped
        // This method is invoked in destructor to ensure all resources are properly disposed.
        //
        void Dispose(void) noexcept
        {
            //
            // Nothing left to do if thread was not properly initialized.
            // Bail out quickly in this case without attempting anything.
            //
            if (this->wasCreated == false)
            {
                return;
            }
            
            //
            // Wait for the thread to finish. If it fails, we can't really do anything. Just cleanup the resources.
            // Also add an assert to signal this case on debugging.
            //
            if (!Join())
            {
                XPLATFORM_ASSERT(false);
            }

            //
            // Platform specific API to dispose the needed resources that are created with the thread.
            // We can't really do anything if these method fails.
            // Simply ignore the result.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                if (NULL != this->thread && INVALID_HANDLE_VALUE != this->thread)
                {
                    (void) CloseHandle(this->thread);
                    this->thread = INVALID_HANDLE_VALUE;
                }
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                if (NULL != this->thread && INVALID_HANDLE_VALUE != this->thread)
                {
                    (void) ZwClose(this->thread);
                    this->thread = INVALID_HANDLE_VALUE;
                }
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                return;
            #else
                #error Unsupported Platform
            #endif
        }

    private:
        //
        // Callback and context that are user-defined.
        // These will be called from the separate thread.
        //
        ThreadCallback callback = nullptr;
        void* context = nullptr;

        //
        // These are used to signal the thread state.
        // Try to prevent invalid transitions as much as possible.
        //
        volatile xp_int8_t wasJoined = 0;
        volatile bool wasCreated = false;

        //
        // Platform-Specific thread resource.
        // On Windows - use HANDLE
        // On Linux - a pthread_t
        //
        #if defined (XPLATFORM_WINDOWS_USER_MODE)
            HANDLE thread = INVALID_HANDLE_VALUE;
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            HANDLE thread = INVALID_HANDLE_VALUE;
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            pthread_t thread{ };
        #else
            #error Unsupported Platform
        #endif
    };
}


#endif // __XPLATFORM_THREAD_HPP__