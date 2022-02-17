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

#ifndef __XPLATFORM_SEMAPHORE_HPP__
#define __XPLATFORM_SEMAPHORE_HPP__

//
// This file contains semaphore implementation.
//

namespace XPF
{
    class Semaphore
    {
    public:
        Semaphore(xp_int8_t Limit) noexcept
        {
            this->limit = Limit;

            //
            // Specifies the maximum count value that the semaphore can attain.This value must be positive. 
            // It determines how many waiting threads become eligible for execution when the semaphore is set
            //     to the signaled state and can therefore access the resource that the semaphore protects.
            //
            if (this->limit <= 0)
            {
                XPLATFORM_ASSERT(this->limit <= 0);
                this->limit = 0;
                return;
            }

            //
            // Platform specific API used to create a semaphore.
            // We don't really expect the API to fail.
            // However, if it does, set the semaphore limit to 0 as no thread will be able to acquire the semaphore.
            // SemaphoreLimit() should be used to retrieve the semaphore limit.
            //
            #if defined (XPLATFORM_WINDOWS_USER_MODE)
                this->semaphore = CreateSemaphoreExW(nullptr, 0, this->limit, nullptr, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
                if (this->semaphore == NULL || this->semaphore == INVALID_HANDLE_VALUE)
                {
                    XPLATFORM_ASSERT(false);
                    this->limit = 0;
                }
            #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                KeInitializeSemaphore(&this->semaphore, 0, this->limit);
            #elif defined(XPLATFORM_LINUX_USER_MODE)
                auto error = sem_init(&this->semaphore, 0, this->limit);
                if (0 != error)
                {
                    XPLATFORM_ASSERT(0 == error);
                    this->limit = 0;
                }
            #else
                #error Unsupported Platform
            #endif

            //
            // We don't really expect this API to fail.
            // It is a good practice to invoke SemaphoreLimit() method before using the semaphore 
            //      and ensure that the limit is greater than 0.
            // Assert here to investigate failures on debug builds.
            //
            XPLATFORM_ASSERT(this->limit != 0);
        }
        ~Semaphore() noexcept
        {
            //
            // If limit is 0, the semaphore was not properly initialized.
            // Assert here to signal this use case.
            //
            XPLATFORM_ASSERT(this->limit != 0);

            if (this->limit > 0)
            {
                //
                // Platform specific API used to destropy a semaphore.
                // We don't really expect the API to fail.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                if (this->semaphore != NULL && this->semaphore != INVALID_HANDLE_VALUE)
                {
                    (void) CloseHandle(this->semaphore);
                    this->semaphore = INVALID_HANDLE_VALUE;
                }
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // Nothing left to do.
                    //
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    auto error = sem_destroy(&this->semaphore);
                    if (0 != error)
                    {
                        XPLATFORM_ASSERT(0 == error);
                        this->limit = 0;
                    }
                #else
                    #error Unsupported Platform
                #endif

                //
                // Set the semaphore limit to 0.
                //
                this->limit = 0;
            }
        }

        // Copy semantics -- deleted (We can't copy the semaphore)
        Semaphore(const Semaphore&) noexcept = delete;
        Semaphore& operator=(const Semaphore&) noexcept = delete;

        // Move semantics -- deleted (nor we can move it)
        Semaphore(Semaphore&&) noexcept = delete;
        Semaphore& operator=(Semaphore&&) noexcept = delete;

        //
        // Retrieves the actual semaphoreLimit after initialization.
        // A valid semaphore has a value greater than 0.
        // An invalid semaphore may lead to undefined behavior.
        //
        xp_uint8_t SemaphoreLimit(void) const noexcept { return this->limit; }

        //
        // Increases the count of the specified semaphore by 1.
        //
        void Release(void) noexcept
        {
            //
            // This is an invalid usage. Assert here and investigate.
            //
            XPLATFORM_ASSERT(this->limit != 0);
            if (this->limit != 0)
            {
                //
                // Platform specific API used to release a semaphore.
                // We can't do much if this fails. Just assert here.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    //
                    // Safe to ignore the return value.
                    // If the specified amount would cause the semaphore's count to exceed the maximum count 
                    //      that was specified when the semaphore was created, 
                    //      the count is not changed and the function returns FALSE.
                    // If the function succeeds, the return value is nonzero.
                    //
                    (void) ReleaseSemaphore(this->semaphore, 1, nullptr);
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    //
                    // If the return value is zero, the previous state of the semaphore object is not-signaled.
                    // We can't do much if we couldn't signal the semaphore.
                    // 
                    // Releasing a semaphore object causes the semaphore count to be augmented by 
                    //      the value of the Adjustment parameter. 
                    // If the resulting value is greater than the limit of the semaphore object, 
                    //      the count is not adjusted and an exception, 
                    //      STATUS_SEMAPHORE_LIMIT_EXCEEDED, is raised.
                    // 
                    // In case of exception, we simply ignore it.
                    //
                    __try
                    { 
                        (void) KeReleaseSemaphore(&this->semaphore, 0, 1, FALSE);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        YieldProcessor();
                    }
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    (void) sem_post(&this->semaphore);
                #else
                    #error Unsupported Platform
                #endif
            }
        }

        //
        // Decrease the count of the specified semaphore by 1.
        //
        void Wait(void) noexcept
        {
            //
            // This is an invalid usage. Assert here and investigate.
            //
            XPLATFORM_ASSERT(this->limit != 0);
            if (this->limit != 0)
            {
                
                //
                // Platform specific API used to wait for a semaphore.
                //
                // Safe to ignore the return value.
                // We should not timeout in this case as we have an indefinite sleep.
                // Just assert here, because the thread will be awaken afterwards anyway.
                //
                #if defined (XPLATFORM_WINDOWS_USER_MODE)
                    auto result = WaitForSingleObject(this->semaphore, INFINITE);
                    if (WAIT_OBJECT_0 != result)
                    {
                        XPLATFORM_ASSERT(WAIT_OBJECT_0 == result);
                    }
                #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
                    auto status = KeWaitForSingleObject(this->semaphore, Executive, KernelMode, FALSE, nullptr);
                    if (!NT_SUCCESS(status))
                    {
                        XPLATFORM_ASSERT(NT_SUCCESS(status));
                    }
                #elif defined(XPLATFORM_LINUX_USER_MODE)
                    auto error = sem_wait(&this->semaphore);
                    if (0 != error)
                    {
                        XPLATFORM_ASSERT(0 == error);
                    }
                #else
                    #error Unsupported Platform
                #endif
            }
        }

    private:
        //
        // Platform specific definition for a semaphore.
        // For windows user mode we use the HANDLE for that.
        // For windows kernel mode we use KSEMAPHORE.
        // For linux we use the sem_t.
        //
        #if defined (XPLATFORM_WINDOWS_USER_MODE)
            HANDLE semaphore = INVALID_HANDLE_VALUE;
        #elif defined (XPLATFORM_WINDOWS_KERNEL_MODE)
            KSEMAPHORE semaphore{ };
        #elif defined(XPLATFORM_LINUX_USER_MODE)
            sem_t semaphore{ };
        #else
            #error Unsupported Platform
        #endif

        //
        // These are needed to ensure the semaphore is properly initialized and used correctly.
        // A valid semaphore has a non-zero limit.
        // Initially there are no available tasks. The semaphore must be manually released.
        //
        alignas(xp_int8_t)  volatile xp_int8_t limit = 0;
    };
}


#endif // __XPLATFORM_SEMAPHORE_HPP__