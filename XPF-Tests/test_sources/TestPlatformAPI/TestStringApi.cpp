#include "../../XPF-Tests.h"

namespace XPlatformTest
{
    template <class T>
    class LibApiStringFixture : public ::testing::Test {};

    using LibApiStringTypes = ::testing::Types<xp_char8_t, xp_char16_t, xp_char32_t>;
    TYPED_TEST_SUITE(LibApiStringFixture, LibApiStringTypes);


    TYPED_TEST(LibApiStringFixture, StringLengthNullString)
    {
        TypeParam* nullString = nullptr;
        size_t length{ 0 };

        EXPECT_FALSE(XPF::ApiStringLength(nullString, &length));
    }


    TYPED_TEST(LibApiStringFixture, ApiStringLength)
    {
        TypeParam nullChar = 0;
        size_t length{ 0 };

        EXPECT_TRUE(XPF::ApiStringLength(&nullChar, &length));
        EXPECT_EQ(length, size_t{ 0 });
    }

    TYPED_TEST(LibApiStringFixture, StringLength)
    {
        const TypeParam* string = nullptr;
        size_t length{ 0 };

        if constexpr (XPF::IsSameType<TypeParam, xp_char8_t>)
        {
            string = "My123String";
        }
        else if constexpr (XPF::IsSameType<TypeParam, xp_char16_t>)
        {
            string = u"My123String";
        }
        else if constexpr (XPF::IsSameType<TypeParam, xp_char32_t>)
        {
            string = U"My123String";
        }

        EXPECT_TRUE(XPF::ApiStringLength(string, &length));
        EXPECT_EQ(length, size_t{ 11 });
    }

    TYPED_TEST(LibApiStringFixture, CharToLower)
    {
        EXPECT_EQ(TypeParam{ 'a' }, XPF::ApiCharToLower(TypeParam{ 'A' }));
        EXPECT_EQ(TypeParam{ 'a' }, XPF::ApiCharToLower(TypeParam{ 'a' }));
        EXPECT_EQ(TypeParam{ '9' }, XPF::ApiCharToLower(TypeParam{ '9' }));
    }

    TYPED_TEST(LibApiStringFixture, CharToUpper)
    {
        EXPECT_EQ(TypeParam{ 'A' }, XPF::ApiCharToUpper(TypeParam{ 'A' }));
        EXPECT_EQ(TypeParam{ 'A' }, XPF::ApiCharToUpper(TypeParam{ 'a' }));
        EXPECT_EQ(TypeParam{ '9' }, XPF::ApiCharToUpper(TypeParam{ '9' }));
    }
}