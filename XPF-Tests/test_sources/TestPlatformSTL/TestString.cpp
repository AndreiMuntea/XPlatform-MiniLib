#include "../../XPF-Tests.h"

namespace XPlatformTest
{
#define INITIALIZE_TYPED_BUFFER(String, Buffer)                             \
        if constexpr (XPF::IsSameType<TypeParam, xp_char8_t>)               \
        {                                                                   \
            String = Buffer;                                                \
        }                                                                   \
        else if constexpr (XPF::IsSameType<TypeParam, xp_char16_t>)         \
        {                                                                   \
            String = u##Buffer;                                             \
        }                                                                   \
        else if constexpr (XPF::IsSameType<TypeParam, xp_char32_t>)         \
        {                                                                   \
            String = U##Buffer;                                             \
        }                                                                   \
        else                                                                \
        {                                                                   \
            static_assert(XPF::IsSameType<TypeParam, xp_char32_t>, "Fail"); \
        }

    template <class T>
    class TestStringFixture : public  testing::Test
    {
    protected:
        void SetUp() override { memoryLeakChecker = new TestMemoryLeak(); EXPECT_TRUE(nullptr != memoryLeakChecker); }
        void TearDown() override { delete memoryLeakChecker; memoryLeakChecker = nullptr; }

        TestMemoryLeak* memoryLeakChecker = nullptr;
    };

    using TestStringTypes = ::testing::Types<xp_char8_t, xp_char16_t, xp_char32_t>;
    TYPED_TEST_SUITE(TestStringFixture, TestStringTypes);


    TYPED_TEST(TestStringFixture, StringViewDefaultConstructor)
    {
        XPF::StringView<TypeParam> view;

        EXPECT_TRUE(view.Size() == 0);
        EXPECT_TRUE(view.RawBuffer() == nullptr);
        EXPECT_TRUE(view.IsEmpty());
        EXPECT_TRUE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferConstructor)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::StringView<TypeParam> view(string);

        EXPECT_TRUE(view.Size() == 11);
        EXPECT_TRUE(view.RawBuffer() != nullptr);
        EXPECT_FALSE(view.IsEmpty());
        EXPECT_FALSE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferConstructorNullBuffer)
    {
        XPF::StringView<TypeParam> view(nullptr);

        EXPECT_TRUE(view.Size() == 0);
        EXPECT_TRUE(view.RawBuffer() == nullptr);
        EXPECT_TRUE(view.IsEmpty());
        EXPECT_TRUE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferWithLengthConstructor)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::StringView<TypeParam> view(string, 3);

        EXPECT_TRUE(view.Size() == 3);
        EXPECT_TRUE(view.RawBuffer() != nullptr);
        EXPECT_FALSE(view.IsEmpty());
        EXPECT_FALSE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferWithLengthConstructorNullString)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::StringView<TypeParam> view(nullptr, 3);

        EXPECT_TRUE(view.Size() == 0);
        EXPECT_TRUE(view.RawBuffer() == nullptr);
        EXPECT_TRUE(view.IsEmpty());
        EXPECT_TRUE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferWithLengthConstructor0Length)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::StringView<TypeParam> view(string, 0);

        EXPECT_TRUE(view.Size() == 0);
        EXPECT_TRUE(view.RawBuffer() == nullptr);
        EXPECT_TRUE(view.IsEmpty());
        EXPECT_TRUE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferWithStringConstructor)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::String<TypeParam> str;
        EXPECT_TRUE(str.Replace(str));

        XPF::StringView<TypeParam> view(str);

        EXPECT_TRUE(view.Size() == 11);
        EXPECT_TRUE(view.RawBuffer() != nullptr);
        EXPECT_FALSE(view.IsEmpty());
        EXPECT_FALSE(view.begin() == view.end());
    }

}