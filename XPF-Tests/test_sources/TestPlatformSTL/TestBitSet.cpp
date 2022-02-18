#include "../../XPF-Tests.h"

namespace XPlatformTest
{

    class TestBitSetFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    TEST_F(TestBitSetFixture, TestBitSetDefaultConstructor)
    {
        XPF::BitSet bitset;

        EXPECT_TRUE(bitset.BitsCount() == 0);
    }

    TEST_F(TestBitSetFixture, TestBitSetExtend)
    {
        XPF::BitSet bitset;

        EXPECT_TRUE(bitset.BitsCount() == 0);

        EXPECT_TRUE(bitset.Extend(0));
        EXPECT_TRUE(bitset.BitsCount() == 0);

        EXPECT_TRUE(bitset.Extend(100));
        EXPECT_TRUE(bitset.BitsCount() == 104);

        for (size_t i = 0; i < bitset.BitsCount(); ++i)
        {
            EXPECT_FALSE(bitset.IsBitSet(i));
        }
    }

    TEST_F(TestBitSetFixture, TestBitSetClearAndSet)
    {
        XPF::BitSet bitset;

        EXPECT_TRUE(bitset.BitsCount() == 0);

        EXPECT_TRUE(bitset.Extend(100));
        EXPECT_TRUE(bitset.BitsCount() == 104);

        for (size_t i = 0; i < bitset.BitsCount(); ++i)
        {
            EXPECT_FALSE(bitset.IsBitSet(i));
            bitset.SetBit(i);

            EXPECT_TRUE(bitset.IsBitSet(i));

            for (size_t j = 0; j < bitset.BitsCount(); ++j)
            {
                if (j <= i)
                {
                    EXPECT_TRUE(bitset.IsBitSet(j));
                }
                else
                {
                    EXPECT_FALSE(bitset.IsBitSet(j));
                }
            }
            EXPECT_TRUE(bitset.BitsCount() == 104);
        }

        for (size_t i = 0; i < bitset.BitsCount(); ++i)
        {
            EXPECT_TRUE(bitset.IsBitSet(i));
            bitset.ClearBit(i);

            EXPECT_FALSE(bitset.IsBitSet(i));

            for (size_t j = 0; j < bitset.BitsCount(); ++j)
            {
                if (j <= i)
                {
                    EXPECT_FALSE(bitset.IsBitSet(j));
                }
                else
                {
                    EXPECT_TRUE(bitset.IsBitSet(j));
                }
            }
        }
    }

    TEST_F(TestBitSetFixture, TestBitSetClearAllSetAll)
    {
        XPF::BitSet bitset;
        EXPECT_TRUE(bitset.Extend(100));
        EXPECT_TRUE(bitset.BitsCount() == 104);

        bitset.SetAll();
        for (size_t i = 0; i < bitset.BitsCount(); ++i)
        {
            EXPECT_TRUE(bitset.IsBitSet(i));
        }
        EXPECT_TRUE(bitset.BitsCount() == 104);

        bitset.ClearAll();
        for (size_t i = 0; i < bitset.BitsCount(); ++i)
        {
            EXPECT_FALSE(bitset.IsBitSet(i));
        }
        EXPECT_TRUE(bitset.BitsCount() == 104);
    }
}