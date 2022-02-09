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
        EXPECT_TRUE(view.RawBuffer() == string);
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
        EXPECT_TRUE(view.RawBuffer() == string);
        EXPECT_FALSE(view.IsEmpty());
        EXPECT_FALSE(view.begin() == view.end());
    }

    TYPED_TEST(TestStringFixture, StringViewBufferWithLengthConstructorNullString)
    {
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
        EXPECT_TRUE(view.RawBuffer() != string);
        EXPECT_TRUE(view.RawBuffer() == XPF::AddressOf(str[0]));
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

    TYPED_TEST(TestStringFixture, StringViewEquals)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "My123STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);
        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 14);
        XPF::StringView<TypeParam> view3(string3);
        EXPECT_TRUE(view3.Size() == 7);
        XPF::StringView<TypeParam> view4(string4);
        EXPECT_TRUE(view4.Size() == 0);

        EXPECT_TRUE(view1.Equals(view1, true));
        EXPECT_TRUE(view1.Equals(view1, false));

        EXPECT_TRUE(view1.Equals(view2,  true));
        EXPECT_FALSE(view1.Equals(view2, false));

        EXPECT_FALSE(view1.Equals(view3, true));
        EXPECT_FALSE(view1.Equals(view3, false));

        EXPECT_FALSE(view1.Equals(view4, true));
        EXPECT_FALSE(view1.Equals(view4, false));
    }

    TYPED_TEST(TestStringFixture, StringViewStartsWith)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "My123STRING");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);
        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 11);
        XPF::StringView<TypeParam> view3(string3);
        EXPECT_TRUE(view3.Size() == 7);
        XPF::StringView<TypeParam> view4(string4);
        EXPECT_TRUE(view4.Size() == 0);

        EXPECT_TRUE(view1.StartsWith(view1, true));
        EXPECT_TRUE(view1.StartsWith(view1, false));

        EXPECT_TRUE(view1.StartsWith(view2,  true));
        EXPECT_FALSE(view1.StartsWith(view2, false));

        EXPECT_FALSE(view1.StartsWith(view3, true));
        EXPECT_FALSE(view1.StartsWith(view3, false));

        EXPECT_FALSE(view1.StartsWith(view4, true));
        EXPECT_FALSE(view1.StartsWith(view4, false));
    }

    TYPED_TEST(TestStringFixture, StringViewEndsWith)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "3STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);
        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 10);
        XPF::StringView<TypeParam> view3(string3);
        EXPECT_TRUE(view3.Size() == 7);
        XPF::StringView<TypeParam> view4(string4);
        EXPECT_TRUE(view4.Size() == 0);

        EXPECT_TRUE(view1.EndsWith(view1, true));
        EXPECT_TRUE(view1.EndsWith(view1, false));

        EXPECT_TRUE(view1.EndsWith(view2, true));
        EXPECT_FALSE(view1.EndsWith(view2, false));

        EXPECT_FALSE(view1.EndsWith(view3, true));
        EXPECT_FALSE(view1.EndsWith(view3, false));

        EXPECT_FALSE(view1.EndsWith(view4, true));
        EXPECT_FALSE(view1.EndsWith(view4, false));
    }

    TYPED_TEST(TestStringFixture, StringViewContains)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "3STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);
        XPF::StringView<TypeParam> view2(string2);
        EXPECT_TRUE(view2.Size() == 10);
        XPF::StringView<TypeParam> view3(string3);
        EXPECT_TRUE(view3.Size() == 7);
        XPF::StringView<TypeParam> view4(string4);
        EXPECT_TRUE(view4.Size() == 0);

        size_t pos = 0;
        EXPECT_TRUE(view1.Contains(view1, true, pos));
        EXPECT_TRUE(pos == 0);
        EXPECT_TRUE(view1.Contains(view1, false, pos));
        EXPECT_TRUE(pos == 0);

        EXPECT_TRUE(view1.Contains(view2, true, pos));
        EXPECT_TRUE(pos == 4);
        EXPECT_FALSE(view1.Contains(view2, false, pos));

        EXPECT_FALSE(view1.Contains(view3, true, pos));
        EXPECT_FALSE(view1.Contains(view3, false, pos));

        EXPECT_FALSE(view1.Contains(view4, true, pos));
        EXPECT_FALSE(view1.Contains(view4, false, pos));

        EXPECT_FALSE(view4.Contains(view4, true, pos));
    }

    TYPED_TEST(TestStringFixture, StringViewIterator)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::StringView<TypeParam> view1(string1);
        EXPECT_TRUE(view1.Size() == 14);

        size_t pos = 0;
        for (const auto& c : view1)
        {
            EXPECT_EQ(c, string1[pos++]);
        }

        // View should be immutable. This should not compile
        // *view1.begin() = 'a';
    }

    TYPED_TEST(TestStringFixture, StringDefaultConstructor)
    {
        XPF::String<TypeParam> str;

        EXPECT_TRUE(str.Size() == 0);
        EXPECT_TRUE(str.IsEmpty());
        EXPECT_TRUE(str.begin() == str.end());
    }

    TYPED_TEST(TestStringFixture, StringReplaceWithNullptr)
    {
        XPF::String<TypeParam> str;
        
        EXPECT_TRUE(str.Replace(nullptr));

        EXPECT_TRUE(str.Size() == 0);
        EXPECT_TRUE(str.IsEmpty());
        EXPECT_TRUE(str.begin() == str.end());
    }

    TYPED_TEST(TestStringFixture, StringReplace)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::String<TypeParam> str;
        EXPECT_TRUE(str.Replace(string));

        EXPECT_TRUE(str.Size() == 11);
        EXPECT_FALSE(str.IsEmpty());
        EXPECT_FALSE(str.begin() == str.end());
    }

    TYPED_TEST(TestStringFixture, StringReplaceExisting)
    {
        static const TypeParam* string = nullptr;
        INITIALIZE_TYPED_BUFFER(string, "My123String");

        XPF::String<TypeParam> str;
        EXPECT_TRUE(str.Replace(string));

        EXPECT_TRUE(str.Size() == 11);
        EXPECT_FALSE(str.IsEmpty());
        EXPECT_FALSE(str.begin() == str.end());

        EXPECT_TRUE(str.Replace(nullptr));

        EXPECT_TRUE(str.Size() == 0);
        EXPECT_TRUE(str.IsEmpty());
        EXPECT_TRUE(str.begin() == str.end());
    }

    TYPED_TEST(TestStringFixture, StringMoveConstructor)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));

        XPF::String<TypeParam> str2(XPF::Move(str1));
        EXPECT_TRUE(str1.Size() == 0);
        EXPECT_TRUE(str2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringMoveAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "1234");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);

        XPF::String<TypeParam> str2;
        EXPECT_TRUE(str2.Replace(string2));
        EXPECT_TRUE(str2.Size() == 4);

        str2 = XPF::Move(str1);
        EXPECT_TRUE(str1.Size() == 0);
        EXPECT_TRUE(str2.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringSelfMoveAssignment)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);

        str1 = XPF::Move(str1);
        EXPECT_TRUE(str1.Size() == 14);
    }

    TYPED_TEST(TestStringFixture, StringEquals)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "My123STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);
        XPF::String<TypeParam> str2;
        EXPECT_TRUE(str2.Replace(string2));
        EXPECT_TRUE(str2.Size() == 14);
        XPF::String<TypeParam> str3;
        EXPECT_TRUE(str3.Replace(string3));
        EXPECT_TRUE(str3.Size() == 7);
        XPF::String<TypeParam> str4;
        EXPECT_TRUE(str4.Replace(string4));
        EXPECT_TRUE(str4.Size() == 0);

        EXPECT_TRUE(str1.Equals(str1, true));
        EXPECT_TRUE(str1.Equals(str1, false));

        EXPECT_TRUE(str1.Equals(str2, true));
        EXPECT_FALSE(str1.Equals(str2, false));

        EXPECT_FALSE(str1.Equals(str3, true));
        EXPECT_FALSE(str1.Equals(str3, false));

        EXPECT_FALSE(str1.Equals(str4, true));
        EXPECT_FALSE(str1.Equals(str4, false));
    }

    TYPED_TEST(TestStringFixture, StringStartsWith)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "My123STRING");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);
        XPF::String<TypeParam> str2;
        EXPECT_TRUE(str2.Replace(string2));
        EXPECT_TRUE(str2.Size() == 11);
        XPF::String<TypeParam> str3;
        EXPECT_TRUE(str3.Replace(string3));
        EXPECT_TRUE(str3.Size() == 7);
        XPF::String<TypeParam> str4;
        EXPECT_TRUE(str4.Replace(string4));
        EXPECT_TRUE(str4.Size() == 0);

        EXPECT_TRUE(str1.StartsWith(str1, true));
        EXPECT_TRUE(str1.StartsWith(str1, false));

        EXPECT_TRUE(str1.StartsWith(str2, true));
        EXPECT_FALSE(str1.StartsWith(str2, false));

        EXPECT_FALSE(str1.StartsWith(str3, true));
        EXPECT_FALSE(str1.StartsWith(str3, false));

        EXPECT_FALSE(str1.StartsWith(str4, true));
        EXPECT_FALSE(str1.StartsWith(str4, false));
    }

    TYPED_TEST(TestStringFixture, StringEndsWith)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "3STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);
        XPF::String<TypeParam> str2;
        EXPECT_TRUE(str2.Replace(string2));
        EXPECT_TRUE(str2.Size() == 10);
        XPF::String<TypeParam> str3;
        EXPECT_TRUE(str3.Replace(string3));
        EXPECT_TRUE(str3.Size() == 7);
        XPF::String<TypeParam> str4;
        EXPECT_TRUE(str4.Replace(string4));
        EXPECT_TRUE(str4.Size() == 0);

        EXPECT_TRUE(str1.EndsWith(str1, true));
        EXPECT_TRUE(str1.EndsWith(str1, false));

        EXPECT_TRUE(str1.EndsWith(str2, true));
        EXPECT_FALSE(str1.EndsWith(str2, false));

        EXPECT_FALSE(str1.EndsWith(str3, true));
        EXPECT_FALSE(str1.EndsWith(str3, false));

        EXPECT_FALSE(str1.EndsWith(str4, true));
        EXPECT_FALSE(str1.EndsWith(str4, false));
    }

    TYPED_TEST(TestStringFixture, StringContains)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "3STRING123");

        static const TypeParam* string3 = nullptr;
        INITIALIZE_TYPED_BUFFER(string3, "My12323");

        static const TypeParam* string4 = nullptr;

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Replace(string1));
        EXPECT_TRUE(str1.Size() == 14);
        XPF::String<TypeParam> str2;
        EXPECT_TRUE(str2.Replace(string2));
        EXPECT_TRUE(str2.Size() == 10);
        XPF::String<TypeParam> str3;
        EXPECT_TRUE(str3.Replace(string3));
        EXPECT_TRUE(str3.Size() == 7);
        XPF::String<TypeParam> str4;
        EXPECT_TRUE(str4.Replace(string4));
        EXPECT_TRUE(str4.Size() == 0);

        size_t pos = 0;
        EXPECT_TRUE(str1.Contains(str1, true, pos));
        EXPECT_TRUE(pos == 0);
        EXPECT_TRUE(str1.Contains(str1, false, pos));
        EXPECT_TRUE(pos == 0);

        EXPECT_TRUE(str1.Contains(str2, true, pos));
        EXPECT_TRUE(pos == 4);
        EXPECT_FALSE(str1.Contains(str2, false, pos));

        EXPECT_FALSE(str1.Contains(str3, true, pos));
        EXPECT_FALSE(str1.Contains(str3, false, pos));

        EXPECT_FALSE(str1.Contains(str4, true, pos));
        EXPECT_FALSE(str1.Contains(str4, false, pos));

        EXPECT_FALSE(str4.Contains(str4, true, pos));
    }

    TYPED_TEST(TestStringFixture, StringAppend)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "My123String123");

        static const TypeParam* result = nullptr;
        INITIALIZE_TYPED_BUFFER(result, "My123String123My123String123");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Append(string1));
        EXPECT_TRUE(str1.Size() == 14);

        EXPECT_TRUE(str1.Append(nullptr));
        EXPECT_TRUE(str1.Size() == 14);

        EXPECT_TRUE(str1.Append(string1));
        EXPECT_TRUE(str1.Size() == 28);

        EXPECT_TRUE(str1.Equals(result, true));
    }

    TYPED_TEST(TestStringFixture, StringToUpper)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "aaaAAAAaaaaBBBB9999");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "AAAAAAAAAAABBBB9999");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Append(string1));
        EXPECT_TRUE(str1.Size() == 19);

        EXPECT_TRUE(str1.Equals(string1, false));
        EXPECT_FALSE(str1.Equals(string2, false));

        str1.ToUpper();

        EXPECT_FALSE(str1.Equals(string1, false));
        EXPECT_TRUE(str1.Equals(string2, false));
    }

    TYPED_TEST(TestStringFixture, StringToLower)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "aaaaaaaaaaabbbb9999");

        static const TypeParam* string2 = nullptr;
        INITIALIZE_TYPED_BUFFER(string2, "AAAAAAAAAAABBBB9999");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Append(string2));
        EXPECT_TRUE(str1.Size() == 19);

        EXPECT_FALSE(str1.Equals(string1, false));
        EXPECT_TRUE(str1.Equals(string2, false));

        str1.ToLower();

        EXPECT_TRUE(str1.Equals(string1, false));
        EXPECT_FALSE(str1.Equals(string2, false));
    }

    TYPED_TEST(TestStringFixture, StringUpdate)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "123");

        static const TypeParam* result = nullptr;
        INITIALIZE_TYPED_BUFFER(result, "XYQ");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Append(string1));

        EXPECT_TRUE(str1.Equals(string1, false));
        EXPECT_FALSE(str1.Equals(result, false));

        str1[0] = static_cast<TypeParam>('X');
        str1[1] = static_cast<TypeParam>('Y');
        str1[2] = static_cast<TypeParam>('Q');


        EXPECT_FALSE(str1.Equals(string1, false));
        EXPECT_TRUE(str1.Equals(result, false));
    }

    TYPED_TEST(TestStringFixture, StringIteratorUpdate)
    {
        static const TypeParam* string1 = nullptr;
        INITIALIZE_TYPED_BUFFER(string1, "123");

        static const TypeParam* result = nullptr;
        INITIALIZE_TYPED_BUFFER(result, "ppp");

        XPF::String<TypeParam> str1;
        EXPECT_TRUE(str1.Append(string1));

        EXPECT_TRUE(str1.Equals(string1, false));
        EXPECT_FALSE(str1.Equals(result, false));

        for (auto& c : str1)
        {
            c = static_cast<TypeParam>('p');
        }

        *str1.begin() = static_cast<TypeParam>('p');

        EXPECT_FALSE(str1.Equals(string1, false));
        EXPECT_TRUE(str1.Equals(result, false));
    }
}