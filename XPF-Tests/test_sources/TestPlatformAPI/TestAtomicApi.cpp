#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    template <class T>
    class LibApiIncrementDecrementFixture : public ::testing::Test {};

    using LibApiIncrementDecrementTypes = ::testing::Types<xp_uint8_t,  xp_int8_t,
                                                           xp_uint16_t, xp_int16_t,
                                                           xp_uint32_t, xp_int32_t,
                                                           xp_uint64_t, xp_int64_t>;
    TYPED_TEST_SUITE(LibApiIncrementDecrementFixture, LibApiIncrementDecrementTypes);


    TYPED_TEST(LibApiIncrementDecrementFixture, IncrementNumber)
    {
        alignas(sizeof(TypeParam)) TypeParam number{ 5 };
        EXPECT_TRUE(6 == XPF::ApiAtomicIncrement(&number));
        EXPECT_TRUE(6 == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, IncrementMin)
    {
        constexpr auto minValue = XPF::NumericLimits<TypeParam>::MinValue;
        alignas(sizeof(TypeParam)) TypeParam number = minValue;
        EXPECT_TRUE(minValue + 1 == XPF::ApiAtomicIncrement(&number));
        EXPECT_TRUE(minValue + 1 == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, IncrementMax)
    {
        constexpr auto maxValue = XPF::NumericLimits<TypeParam>::MaxValue;
        constexpr auto minValue = XPF::NumericLimits<TypeParam>::MinValue;
        alignas(sizeof(TypeParam)) TypeParam number = maxValue;
        EXPECT_TRUE(minValue == XPF::ApiAtomicIncrement(&number));
        EXPECT_TRUE(minValue == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, DecrementNumber)
    {
        alignas(sizeof(TypeParam)) TypeParam number{ 10 };
        EXPECT_TRUE(9 == XPF::ApiAtomicDecrement(&number));
        EXPECT_TRUE(9 == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, DecrementMin)
    {
        constexpr auto maxValue = XPF::NumericLimits<TypeParam>::MaxValue;
        constexpr auto minValue = XPF::NumericLimits<TypeParam>::MinValue;
        alignas(sizeof(TypeParam)) TypeParam number = minValue;
        EXPECT_TRUE(maxValue == XPF::ApiAtomicDecrement(&number));
        EXPECT_TRUE(maxValue == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, DecrementMax)
    {
        constexpr auto maxValue = XPF::NumericLimits<TypeParam>::MaxValue;
        alignas(sizeof(TypeParam)) TypeParam number = maxValue;
        EXPECT_TRUE(maxValue - 1 == XPF::ApiAtomicDecrement(&number));
        EXPECT_TRUE(maxValue - 1 == number);
    }

    TYPED_TEST(LibApiIncrementDecrementFixture, Exchange)
    {
        constexpr auto maxValue = XPF::NumericLimits<TypeParam>::MaxValue;
        constexpr auto minValue = XPF::NumericLimits<TypeParam>::MinValue;

        alignas(sizeof(TypeParam)) TypeParam number = 0;

        EXPECT_TRUE(0 == XPF::ApiAtomicExchange(&number, maxValue));
        EXPECT_TRUE(maxValue == number);

        EXPECT_TRUE(maxValue == XPF::ApiAtomicExchange(&number, minValue));
        EXPECT_TRUE(minValue == number);

        EXPECT_TRUE(minValue == XPF::ApiAtomicExchange(&number, TypeParam{ 5 }));
        EXPECT_TRUE(5 == number);
    }
}