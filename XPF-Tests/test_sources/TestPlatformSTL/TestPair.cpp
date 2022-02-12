#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class TestPairFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_F(TestPairFixture, TestPairDefaultConstructor)
    {
        XPF::Pair<xp_uint8_t, xp_int32_t> pair;

        EXPECT_TRUE(pair.First == 0);
        EXPECT_TRUE(pair.Second == 0);
    }

    TEST_F(TestPairFixture, TestPairElementCopyConstructor)
    {
        XPF::Pair<xp_uint8_t, xp_int32_t> pair;

        EXPECT_TRUE(pair.First == 0);
        EXPECT_TRUE(pair.Second == 0);
    }

    TEST_F(TestPairFixture, TestPairConstructorWithCopy)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair{ first, second };

        EXPECT_TRUE(pair.First == first);
        EXPECT_TRUE(pair.Second == second);
    }

    TEST_F(TestPairFixture, TestPairConstructorWithMove)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair{ XPF::Move(first), XPF::Move(second) };

        EXPECT_TRUE(pair.First == 100);
        EXPECT_TRUE(pair.Second == -2100);
    }

    TEST_F(TestPairFixture, TestPairCopyConstructor)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };
        XPF::Pair<xp_uint8_t, xp_int32_t> pair2{ pair1 };

        EXPECT_TRUE(pair1.First == 100);
        EXPECT_TRUE(pair1.Second == -2100);

        EXPECT_TRUE(pair2.First == 100);
        EXPECT_TRUE(pair2.Second == -2100);
    }

    TEST_F(TestPairFixture, TestPairMoveConstructor)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };
        XPF::Pair<xp_uint8_t, xp_int32_t> pair2{ XPF::Move(pair1) };

        EXPECT_TRUE(pair1.First == 100);
        EXPECT_TRUE(pair1.Second == -2100);

        EXPECT_TRUE(pair2.First == 100);
        EXPECT_TRUE(pair2.Second == -2100);
    }

    TEST_F(TestPairFixture, TestPairCopyAssignment)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };
        XPF::Pair<xp_uint8_t, xp_int32_t> pair2{200, 300};

        pair1 = pair2;

        EXPECT_TRUE(pair1.First == 200);
        EXPECT_TRUE(pair1.Second == 300);

        EXPECT_TRUE(pair2.First == 200);
        EXPECT_TRUE(pair2.Second == 300);
    }

    TEST_F(TestPairFixture, TestPairSelfCopyAssignment)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };

        pair1 = pair1;

        EXPECT_TRUE(pair1.First == 100);
        EXPECT_TRUE(pair1.Second == -2100);
    }

    TEST_F(TestPairFixture, TestPairMoveAssignment)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };
        XPF::Pair<xp_uint8_t, xp_int32_t> pair2{ 200, 300 };

        pair1 = XPF::Move(pair2);

        EXPECT_TRUE(pair1.First == 200);
        EXPECT_TRUE(pair1.Second == 300);
    }

    TEST_F(TestPairFixture, TestPairSelfMoveAssignment)
    {
        xp_uint8_t first = 100;
        xp_int32_t second = -2100;
        XPF::Pair<xp_uint8_t, xp_int32_t> pair1{ XPF::Move(first), XPF::Move(second) };

        pair1 = XPF::Move(pair1);

        EXPECT_TRUE(pair1.First == 100);
        EXPECT_TRUE(pair1.Second == -2100);
    }
}