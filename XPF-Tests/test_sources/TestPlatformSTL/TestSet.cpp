#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestSetFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_F(TestSetFixture, TestSetDefaultConstructor)
    {
        XPF::Set<int> set;

        EXPECT_TRUE(set.IsEmpty());
        EXPECT_TRUE(set.Size() == 0);
        EXPECT_TRUE(set.begin() == set.end());
    }

    TEST_F(TestSetFixture, TestSetInsert)
    {
        XPF::Set<int> set;

        EXPECT_TRUE(set.Emplace(100));

        EXPECT_FALSE(set.IsEmpty());
        EXPECT_TRUE(set.Size() == 1);
        EXPECT_TRUE(*set.begin() == 100);
    }

    TEST_F(TestSetFixture, TestSetInsertSameElementTwice)
    {
        XPF::Set<int> set;

        EXPECT_TRUE(set.Emplace(100));
        EXPECT_FALSE(set.Emplace(100));

        EXPECT_FALSE(set.IsEmpty());
        EXPECT_TRUE(set.Size() == 1);
        EXPECT_TRUE(*set.begin() == 100);
    }

    TEST_F(TestSetFixture, TestSetInsertFindErase)
    {
        XPF::Set<int> set;

        for (int i = 0; i < 100; ++i)
        {
            auto itBefore = set.Find(i);
            EXPECT_TRUE(itBefore == set.end());
            EXPECT_FALSE(set.Erase(itBefore));

            EXPECT_TRUE(set.Emplace(i));
            auto itAfter = set.Find(i);

            EXPECT_TRUE(itAfter != set.end());
            EXPECT_TRUE(*itAfter == i);
        }

        for (int i = 99; i >= 0; --i)
        {
            auto it = set.Find(i);
            EXPECT_TRUE(set.Erase(it));
        }
    }

    TEST_F(TestSetFixture, TestSetMoveConstructor)
    {
        XPF::Set<int> set1;
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(set1.Emplace(i));
        }
        EXPECT_TRUE(set1.Size() == 100);


        XPF::Set<int> set2{ XPF::Move(set1) };

        EXPECT_TRUE(set1.Size() == 0);
        EXPECT_TRUE(set2.Size() == 100);
    }

    TEST_F(TestSetFixture, TestSetMoveAssignment)
    {
        XPF::Set<int> set1;
        XPF::Set<int> set2;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(set1.Emplace(i));
        }
        EXPECT_TRUE(set1.Size() == 100);

        for (int i = 0; i < 1200; ++i)
        {
            EXPECT_TRUE(set2.Emplace(i));
        }
        EXPECT_TRUE(set2.Size() == 1200);

        set2 = XPF::Move(set1);

        EXPECT_TRUE(set1.Size() == 0);
        EXPECT_TRUE(set2.Size() == 100);
    }

    TEST_F(TestSetFixture, TestSetSelfMoveAssignment)
    {
        XPF::Set<int> set;

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(set.Emplace(i));
        }
        EXPECT_TRUE(set.Size() == 100);

        set = XPF::Move(set);
        EXPECT_TRUE(set.Size() == 100);
    }

    TEST_F(TestSetFixture, TestSetIterator)
    {
        XPF::Set<int> set;

        for (int i = 100; i < 200; ++i)
        {
            EXPECT_TRUE(set.Emplace(i));
        }

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(set.Emplace(i));
        }

        for (int i = 300; i >= 200; --i)
        {
            EXPECT_TRUE(set.Emplace(i));
        }

        int i = 0;
        for (const auto& e : set)
        {
            EXPECT_EQ(i++, e);
        }
    }
}