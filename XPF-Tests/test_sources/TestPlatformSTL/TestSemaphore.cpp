#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestSemaphoreFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    struct TestSemaphoreContext
    {
        XPF::Semaphore* semaphore = nullptr;
        volatile xp_int32_t* target = nullptr;
    };

    void TestSemaphoreThreadCallback(
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr != Context);
        if (nullptr != Context)
        {
            auto context = reinterpret_cast<volatile TestSemaphoreContext*>(Context);
            
            // Wait for semaphore to be signaled
            context->semaphore->Wait();
            (*context->target) += 1;
        }
    }

    TEST_F(TestSemaphoreFixture, TestSemaphoreDefaultConstructor)
    {
        XPF::Semaphore s;
        EXPECT_TRUE(s.Initialize(5));
        s.Uninitialize();
    }

    TEST_F(TestSemaphoreFixture, TestSemaphoreReleaseAboveLimit)
    {
        XPF::Semaphore s;
        EXPECT_TRUE(s.Initialize(5));

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            s.Release();
        }
        s.Uninitialize();
    }

    TEST_F(TestSemaphoreFixture, TestSemaphoreWaitRelease)
    {
        XPF::Thread threads[50];
        XPF::Semaphore s;
        xp_int32_t number = 0;

        TestSemaphoreContext context;
        context.semaphore = &s;
        context.target = &number;


        EXPECT_TRUE(s.Initialize(5));

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            EXPECT_TRUE(threads[i].Run(TestSemaphoreThreadCallback, &context));
        }

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            // Take number snapshot
            auto initialNumber = number;

            // Release semaphore
            s.Release();

            // Wait for number to be modified
            while (initialNumber == number && initialNumber != 50)
            {
                XPLATFORM_YIELD_PROCESSOR();
            }
        }

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            threads[i].Join();
        }

        EXPECT_TRUE(number == 50);
        s.Uninitialize();
    }
}