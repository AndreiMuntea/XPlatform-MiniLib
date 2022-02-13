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
        _Must_inspect_result_
        bool Run(ThreadCallback Callback, void* Context) noexcept
        {
            XPLATFORM_ASSERT(nullptr == this->callback);

            this->callback = Callback;
            this->context = Context;

            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                this->thread = CreateThread(nullptr, 
                                            0, 
                                            ThreadStartRoutine, 
                                            reinterpret_cast<void*>(this), 
                                            0, 
                                            nullptr);
                return (this->thread != NULL && this->thread != INVALID_HANDLE_VALUE);
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
                return (NT_SUCCESS(status));
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                auto error = pthread_create(&thread, nullptr, ThreadStartRoutine, reinterpret_cast<void*>(this));
                if (error != 0)
                {
                    XPLATFORM_ASSERT((error != 0));
                    XPF::ApiZeroMemory(&this->thread, sizeof(this->thread));
                }
                return (error == 0);
            #else
                #error Unsupported Platform
            #endif
                
        }

        //
        // Waits for the thread to finish.
        // DO NOT call this method if Run was not called or it failed.
        // DO NOT call this method twice!
        // This method MUST be manually called before the thread is actually destroyed.
        // It will NOT be called by the destructor.
        //
        _Must_inspect_result_
        bool Join(void) noexcept
        {
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
                if (thread->callback != nullptr)
                {
                    thread->callback(thread->context);
                }
                return 0;
            }
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            static void ThreadStartRoutine(void* Argument) noexcept
            {
                auto thread = reinterpret_cast<XPF::Thread*>(Argument);
                if (thread->callback != nullptr)
                {
                    thread->callback(thread->context);
                }
            }
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            static void* ThreadStartRoutine(void* Argument) noexcept
            {
                auto thread = reinterpret_cast<XPF::Thread*>(Argument);
                if (thread->callback != nullptr)
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
        // This method is invoked in destructor to ensure all resources are properly disposed.
        //
        void Dispose(void) noexcept
        {
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                if (NULL != this->thread && INVALID_HANDLE_VALUE != this->thread)
                {
                    (void)CloseHandle(this->thread);
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
        ThreadCallback callback = nullptr;
        void* context = nullptr;

        #if defined (XPLATFORM_WINDOWS_USER_MODE)
            HANDLE thread = INVALID_HANDLE_VALUE;
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            HANDLE thread = INVALID_HANDLE_VALUE;
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            pthread_t thread = {0};
        #else
            #error Unsupported Platform
        #endif
    };
}


#endif // __XPLATFORM_THREAD_HPP__