#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    template <class T>
    class LibApiSafeArithmeticFixture : public ::testing::Test {};

    using LibApiSafeArithmeticTypes = ::testing::Types<xp_uint8_t, xp_uint16_t, xp_uint32_t, xp_uint64_t>;
    TYPED_TEST_SUITE(LibApiSafeArithmeticFixture, LibApiSafeArithmeticTypes);


    TYPED_TEST(LibApiSafeArithmeticFixture, UintAddOk)
    {
        TypeParam number1{ 5 };
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_TRUE(XPF::ApiUIntAdd(number1, number2, &result));
        EXPECT_EQ(number1 + number2, result);
    }

    TYPED_TEST(LibApiSafeArithmeticFixture, UintAddOverflow)
    {
        TypeParam number1 = XPF::NumericLimits<TypeParam>::MaxValue;
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_FALSE(XPF::ApiUIntAdd(number1, number2, &result));
    }

    TYPED_TEST(LibApiSafeArithmeticFixture, UintSubOk)
    {
        TypeParam number1{ 10 };
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_TRUE(XPF::ApiUIntSub(number1, number2, &result));
        EXPECT_EQ(number1 - number2, result);
    }

    TYPED_TEST(LibApiSafeArithmeticFixture, UintSubUnderflow)
    {
        TypeParam number1 = XPF::NumericLimits<TypeParam>::MinValue;
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_FALSE(XPF::ApiUIntSub(number1, number2, &result));
    }

    TYPED_TEST(LibApiSafeArithmeticFixture, UintMultOk)
    {
        TypeParam number1{ 2 };
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_TRUE(XPF::ApiUIntMult(number1, number2, &result));
        EXPECT_EQ(number1 * number2, result);
    }

    TYPED_TEST(LibApiSafeArithmeticFixture, UintMultOverflow)
    {
        TypeParam number1 = XPF::NumericLimits<TypeParam>::MaxValue;
        TypeParam number2{ 9 };
        TypeParam result{ 0 };
        EXPECT_FALSE(XPF::ApiUIntMult(number1, number2, &result));
    }

}