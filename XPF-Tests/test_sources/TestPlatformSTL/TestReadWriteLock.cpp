#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestReadWriteLockFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    struct RwLockTestSharedContext
    {
        XPF::ReadWriteLock* lock = nullptr;
        volatile xp_int32_t* acquired = nullptr;
        xp_int32_t target = 0;
    };

    struct RwLockTestExclusiveContext
    {
        XPF::ReadWriteLock* lock = nullptr;
        xp_int32_t number = 0;
        xp_int32_t* sum = nullptr;
    };

    void TestSharedLocking(
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr != Context);
        if (nullptr != Context)
        {
            auto context = reinterpret_cast<volatile RwLockTestSharedContext*>(Context);
            {
                XPF::SharedLockGuard guard{ *context->lock };
                (void)XPF::ApiAtomicIncrement(context->acquired);
                while (*context->acquired != context->target)
                {
                    XPLATFORM_YIELD_PROCESSOR();
                }
            }
            delete context;
        }
    }

    void TestExclusiveLocking(
        _Inout_opt_ void* Context
    ) noexcept
    {
        EXPECT_TRUE(nullptr != Context);
        if (nullptr != Context)
        {
            auto context = reinterpret_cast<volatile RwLockTestExclusiveContext*>(Context);
            {
                XPF::ExclusiveLockGuard guard{ *context->lock };
                (*context->sum) += context->number;
            }
            delete context;
        }
    }

    TEST_F(TestReadWriteLockFixture, TestReadWriteLockDefaultConstructor)
    {
        XPF::ReadWriteLock rwLock;
        EXPECT_TRUE(rwLock.Initialize());
        rwLock.Uninitialize();
    }

    TEST_F(TestReadWriteLockFixture, TestReadWriteLockAcquireReleaseShared)
    {
        XPF::ReadWriteLock rwLock;
        EXPECT_TRUE(rwLock.Initialize());

        rwLock.LockShared();
        rwLock.UnlockShared();
        rwLock.Uninitialize();
    }

    TEST_F(TestReadWriteLockFixture, TestReadWriteLockAcquireReleaseExclusive)
    {
        XPF::ReadWriteLock rwLock;
        EXPECT_TRUE(rwLock.Initialize());

        rwLock.LockExclusive();
        rwLock.UnlockExclusive();
        rwLock.Uninitialize();
    }

    TEST_F(TestReadWriteLockFixture, TestReadWriteLockSharedLocking)
    {
        XPF::ReadWriteLock rwLock;
        xp_int32_t acquired = 0;
        EXPECT_TRUE(rwLock.Initialize());

        XPF::Thread threads[20];
        xp_int32_t threadsCount = sizeof(threads) / sizeof(threads[0]);

        for (xp_int32_t i = 0; i < threadsCount; ++i)
        {
            auto ctx = new RwLockTestSharedContext();
            EXPECT_TRUE(ctx != nullptr);

            ctx->lock = &rwLock;
            ctx->acquired = &acquired;
            ctx->target = threadsCount;

            EXPECT_TRUE(threads[i].Run(TestSharedLocking, ctx));
        }

        for (xp_int32_t i = 0; i < threadsCount; ++i)
        {
            threads[i].Join();
        }

        rwLock.Uninitialize();
    }

    TEST_F(TestReadWriteLockFixture, TestReadWriteLockExclusiveLocking)
    {
        XPF::ReadWriteLock rwLock;
        xp_int32_t sum = 0;
        EXPECT_TRUE(rwLock.Initialize());

        XPF::Thread threads[20];
        xp_int32_t threadsCount = sizeof(threads) / sizeof(threads[0]);

        for (xp_int32_t i = 0; i < threadsCount; ++i)
        {
            auto ctx = new RwLockTestExclusiveContext();
            EXPECT_TRUE(ctx != nullptr);

            ctx->lock = &rwLock;
            ctx->number = i;
            ctx->sum = &sum;

            EXPECT_TRUE(threads[i].Run(TestExclusiveLocking, ctx));
        }

        for (xp_int32_t i = 0; i < threadsCount; ++i)
        {
            threads[i].Join();
        }

        EXPECT_TRUE(sum == ((threadsCount - 1) * threadsCount) / 2);

        rwLock.Uninitialize();
    }
}