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
        EXPECT_TRUE(str.Replace(string));

        XPF::StringView<TypeParam> view(str);

        EXPECT_TRUE(view.Size() == 11);
        EXPECT_TRUE(view.RawBuffer() != nullptr);
        EXPECT_FALSE(view.IsEmpty());
        EXPECT_FALSE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewCopyConstructor)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        XPF::StringView<TypeParam> view2(view1);
        EXPECT_TRUE(view1.Size() == 14);
        EXPECT_TRUE(view2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringViewCopyAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "1234");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 4);

        view2 = view1;
        EXPECT_TRUE(view1.Size() == 14);
        EXPECT_TRUE(view2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringViewSelfCopyAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        view1 = view1;
        EXPECT_TRUE(view1.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringViewMoveConstructor)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        XPF::StringView<TypeParam> view2(XPF::Move(view1));
        EXPECT_TRUE(view1.Size() == 0);
        EXPECT_TRUE(view2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringViewMoveAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "1234");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 4);

        view2 = XPF::Move(view1);
        EXPECT_TRUE(view1.Size() == 0);
        EXPECT_TRUE(view2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringViewSelfMoveAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        view1 = XPF::Move(view1);
        EXPECT_TRUE(view1.Size() == 14);
    }

}