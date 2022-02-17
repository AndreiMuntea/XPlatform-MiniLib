#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestThreadFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    void TestThreadCallback(
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr != Context);
        if (nullptr != Context)
        {
            auto integer = reinterpret_cast<volatile xp_int32_t*>(Context);
            (void) XPF::ApiAtomicIncrement(integer);
        }
    }


    TEST_F(TestThreadFixture, ThreadDefaultConstructorDestructor)
    {
        XPF::Thread thread;
    }

    TEST_F(TestThreadFixture, ThreadRunJoinNull)
    {
        XPF::Thread thread;

        EXPECT_TRUE(thread.Run(nullptr, nullptr));
        thread.Join();
    }


    TEST_F(TestThreadFixture, ThreadRunSingleThread)
    {
        xp_int32_t number = 0;
        XPF::Thread thread;

        EXPECT_TRUE(thread.Run(TestThreadCallback, &number));
        thread.Join();

        EXPECT_TRUE(number == 1);
    }

    TEST_F(TestThreadFixture, ThreadRunJoin10x)
    {
        xp_int32_t number = 0;
        for (int i = 0; i < 10; ++i)
        {
            XPF::Thread thread;
            EXPECT_TRUE(thread.Run(TestThreadCallback, &number));
            thread.Join();
        }


        EXPECT_TRUE(number == 10);
    }

    TEST_F(TestThreadFixture, ThreadRun10xJoin)
    {
        xp_int32_t number = 0;
        XPF::Thread thread[10];

        for (int i = 0; i < 10; ++i)
        {
            EXPECT_TRUE(thread[i].Run(TestThreadCallback, &number));
        }
        for (int i = 0; i < 10; ++i)
        {
            thread[i].Join();
        }

        EXPECT_TRUE(number == 10);
    }

    TEST_F(TestThreadFixture, ThreadStress)
    {
        xp_int32_t number = 0;
        
        for (int iter = 0; iter < 1000; ++iter)
        {
            XPF::Thread thread[4];
            for (int i = 0; i < 4; ++i)
            {
                EXPECT_TRUE(thread[i].Run(TestThreadCallback, &number));
            }
            for (int i = 0; i < 4; ++i)
            {
                thread[i].Join();
            }
        }

        EXPECT_TRUE(number == 4000);
    }
}