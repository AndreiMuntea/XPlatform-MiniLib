#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    class LibApiUtilsApiSetFixture : public ::testing::Test {};


    TEST_F(LibApiUtilsApiSetFixture, Min)
    {
        int a = 10;
        int b = 100;

        EXPECT_EQ(XPF::Min(a, b), a);
        EXPECT_EQ(XPF::Min(a, a), a);
        EXPECT_EQ(XPF::Min(b, b), b);
    }

    TEST_F(LibApiUtilsApiSetFixture, Max)
    {
        int a = 10;
        int b = 100;

        EXPECT_EQ(XPF::Max(a, b), b);
        EXPECT_EQ(XPF::Max(a, a), a);
        EXPECT_EQ(XPF::Max(b, b), b);
    }

    TEST_F(LibApiUtilsApiSetFixture, ArePointersEqual)
    {
        int a = 10;
        int b = 100;

        EXPECT_TRUE(XPF::ArePointersEqual(&a, &a));
        EXPECT_TRUE(XPF::ArePointersEqual(&b, &b));
        EXPECT_FALSE(XPF::ArePointersEqual(&a, &b));
    }

    TEST_F(LibApiUtilsApiSetFixture, Swap)
    {
        int a = 10;
        int b = 100;

        XPF::Swap(a, a);
        EXPECT_EQ(a, 10);

        XPF::Swap(b, b);
        EXPECT_EQ(b, 100);

        XPF::Swap(a, b);
        EXPECT_EQ(b, 10);
        EXPECT_EQ(a, 100);
    }

    TEST_F(LibApiUtilsApiSetFixture, IsPowerOf2)
    {
        EXPECT_TRUE(XPF::IsPowerOf2(0));
        EXPECT_TRUE(XPF::IsPowerOf2(1));
        EXPECT_TRUE(XPF::IsPowerOf2(2));
        EXPECT_TRUE(XPF::IsPowerOf2(4));
        EXPECT_TRUE(XPF::IsPowerOf2(4096));
        EXPECT_TRUE(XPF::IsPowerOf2(8192));

        EXPECT_FALSE(XPF::IsPowerOf2(3));
        EXPECT_FALSE(XPF::IsPowerOf2(11));
        EXPECT_FALSE(XPF::IsPowerOf2(6));
        EXPECT_FALSE(XPF::IsPowerOf2(992));
        EXPECT_FALSE(XPF::IsPowerOf2(8191));
    }

    TEST_F(LibApiUtilsApiSetFixture, IsAligned)
    {
        EXPECT_TRUE(XPF::IsAligned(2, 2));
        EXPECT_TRUE(XPF::IsAligned(64, 8));
        EXPECT_TRUE(XPF::IsAligned(128, 64));
        EXPECT_TRUE(XPF::IsAligned(4, 2));
        EXPECT_TRUE(XPF::IsAligned(16, 8));

        EXPECT_FALSE(XPF::IsAligned(2, 0));
        EXPECT_FALSE(XPF::IsAligned(2, 0));
        EXPECT_FALSE(XPF::IsAligned(64, 0));
        EXPECT_FALSE(XPF::IsAligned(128, 0));

        EXPECT_FALSE(XPF::IsAligned(2, 3));
        EXPECT_FALSE(XPF::IsAligned(2, 9));
        EXPECT_FALSE(XPF::IsAligned(64, 34));
        EXPECT_FALSE(XPF::IsAligned(128, 642));

        EXPECT_FALSE(XPF::IsAligned(2, 8));
        EXPECT_FALSE(XPF::IsAligned(4, 8));
        EXPECT_FALSE(XPF::IsAligned(8, 16));
        EXPECT_FALSE(XPF::IsAligned(32, 64));
        EXPECT_FALSE(XPF::IsAligned(64, 128));
    }

    TEST_F(LibApiUtilsApiSetFixture, AlignUp)
    {
        // Safety checks
        EXPECT_EQ(XPF::AlignUp(0, 128), 0);
        EXPECT_EQ(XPF::AlignUp(128, 0), 128);
        EXPECT_EQ(XPF::AlignUp(555, 16384), 555);

        // Alignment should be a power of 2
        EXPECT_EQ(XPF::AlignUp(7, 129), 7);
        EXPECT_EQ(XPF::AlignUp(9, 3), 9);
        EXPECT_EQ(XPF::AlignUp(88, 17), 88);

        // Overflow check
        EXPECT_EQ(XPF::AlignUp(XPF::NumericLimits<size_t>::MaxValue, 128), XPF::NumericLimits<size_t>::MaxValue);

        EXPECT_EQ(XPF::AlignUp(7, 128), 128);
        EXPECT_EQ(XPF::AlignUp(9, 16), 16);
        EXPECT_EQ(XPF::AlignUp(16, 4), 16);
    }
}