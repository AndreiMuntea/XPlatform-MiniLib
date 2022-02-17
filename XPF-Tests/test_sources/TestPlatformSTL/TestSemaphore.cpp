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
        XPF::Semaphore s{ 5 };
        EXPECT_TRUE(s.SemaphoreLimit() == 5);
    }

    TEST_F(TestSemaphoreFixture, TestSemaphoreReleaseAboveLimit)
    {
        XPF::Semaphore s{ 5 };
        EXPECT_TRUE(s.SemaphoreLimit() == 5);

        for (xp_uint8_t i = 0; i < s.SemaphoreLimit() + s.SemaphoreLimit(); ++i)
        {
            s.Release();
        }
    }

    TEST_F(TestSemaphoreFixture, TestSemaphoreWaitRelease)
    {
        XPF::Thread threads[50];
        XPF::Semaphore s{ 5 };
        xp_int32_t number = 0;

        TestSemaphoreContext context;
        context.semaphore = &s;
        context.target = &number;


        EXPECT_TRUE(s.SemaphoreLimit() == 5);

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            EXPECT_TRUE(threads[i].Run(TestSemaphoreThreadCallback, &context));
        }

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            // Release one thread
            s.Release();

            // Wait for number to be increased
            while (number != i + 1)
            {
                YieldProcessor();
            }
        }

        for (xp_uint8_t i = 0; i < 50; ++i)
        {
            threads[i].Join();
        }
    }
}