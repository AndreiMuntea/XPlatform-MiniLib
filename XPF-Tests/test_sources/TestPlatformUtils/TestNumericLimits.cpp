#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    template <class T>
    class LibApiNumericLimitsFixture : public ::testing::Test {};

    using LibApiNumericLimitsTypes = ::testing::Types<xp_uint8_t,  xp_int8_t,
                                                      xp_uint16_t, xp_int16_t,
                                                      xp_uint32_t, xp_int32_t,
                                                      xp_uint64_t, xp_int64_t>;
    TYPED_TEST_SUITE(LibApiNumericLimitsFixture, LibApiNumericLimitsTypes);


    TYPED_TEST(LibApiNumericLimitsFixture, NumericLimits)
    {
        constexpr auto stlNumericMax = std::numeric_limits<TypeParam>::max();
        constexpr auto stlNumericMin = std::numeric_limits<TypeParam>::min();

        constexpr auto xPlatformNumericMax = XPF::NumericLimits<TypeParam>::MaxValue;
        constexpr auto xPlatformNumericMin = XPF::NumericLimits<TypeParam>::MinValue;

        EXPECT_EQ(stlNumericMax, xPlatformNumericMax);
        EXPECT_EQ(stlNumericMin, xPlatformNumericMin);
    }
}