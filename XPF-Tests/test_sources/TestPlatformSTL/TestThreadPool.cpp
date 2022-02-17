#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestThreadPoolFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    static void TestThreadPoolCallback(
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr != Context);
        if (nullptr != Context)
        {
            auto integer = reinterpret_cast<volatile xp_int32_t*>(Context);
            (void)XPF::ApiAtomicIncrement(integer);
        }
    }

    TEST_F(TestThreadPoolFixture, TestThreadPoolStartStop)
    {
        XPF::ThreadPool<4> threadpool;

        EXPECT_TRUE(threadpool.Start());
        threadpool.Stop();
    }

    TEST_F(TestThreadPoolFixture, TestThreadPoolSubmitWork)
    {
        XPF::ThreadPool<4> threadpool;
        xp_uint32_t sum = 0;

        EXPECT_TRUE(threadpool.Start());

        for (int i = 0; i < 100000; ++i)
        {
            EXPECT_TRUE(threadpool.SubmitWork(TestThreadPoolCallback, TestThreadPoolCallback, reinterpret_cast<void*>(&sum)));
        }

        threadpool.Stop();

        EXPECT_TRUE(sum == 100000);
    }
}