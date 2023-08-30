/**
 * @file        xpf_tests/tests/Containers/TestString.cpp
 *
 * @brief       This contains tests for string view and for string.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"
#include "xpf_tests/Mocks/TestMocks.hpp"


/**
 * @brief       This tests the default constructor of string view.
 */
TEST(TestStringView, DefaultConstructorDestructor)
{
    constexpr const xpf::StringView<char> stringView;
    EXPECT_TRUE(stringView.IsEmpty());
    EXPECT_TRUE(nullptr == stringView.Buffer());
    EXPECT_EQ(size_t{ 0 }, stringView.BufferSize());

    constexpr const xpf::StringView<wchar_t> wstringView;
    EXPECT_TRUE(wstringView.IsEmpty());
    EXPECT_TRUE(nullptr == wstringView.Buffer());
    EXPECT_EQ(size_t{ 0 }, wstringView.BufferSize());
}

/**
 * @brief       This tests the constructor with only a buffer provided.
 */
TEST(TestStringView, BufferConstructor)
{
    //
    // Non-empty buffer.
    //
    constexpr const xpf::StringView stringView("1234");
    EXPECT_FALSE(stringView.IsEmpty());
    EXPECT_FALSE(nullptr == stringView.Buffer());
    EXPECT_EQ(size_t{ 4 }, stringView.BufferSize());

    constexpr const xpf::StringView wstringView(L"1234");
    EXPECT_FALSE(wstringView.IsEmpty());
    EXPECT_FALSE(nullptr == wstringView.Buffer());
    EXPECT_EQ(size_t{ 4 }, wstringView.BufferSize());

    //
    // Empty buffer.
    //
    constexpr const xpf::StringView stringViewEmpty("");
    EXPECT_TRUE(stringViewEmpty.IsEmpty());
    EXPECT_TRUE(nullptr == stringViewEmpty.Buffer());
    EXPECT_EQ(size_t{ 0 }, stringViewEmpty.BufferSize());

    constexpr const xpf::StringView wstringViewEmpty(L"");
    EXPECT_TRUE(wstringViewEmpty.IsEmpty());
    EXPECT_TRUE(nullptr == wstringViewEmpty.Buffer());
    EXPECT_EQ(size_t{ 0 }, wstringViewEmpty.BufferSize());
}

/**
 * @brief       This tests the constructor with a buffer and also size.
 */
TEST(TestStringView, BufferSizeConstructor)
{
    //
    // Non-empty buffer.
    //
    constexpr const xpf::StringView stringView("1234", 4);
    EXPECT_FALSE(stringView.IsEmpty());
    EXPECT_FALSE(nullptr == stringView.Buffer());
    EXPECT_EQ(size_t{ 4 }, stringView.BufferSize());

    constexpr const xpf::StringView wstringView(L"1234", 4);
    EXPECT_FALSE(wstringView.IsEmpty());
    EXPECT_FALSE(nullptr == wstringView.Buffer());
    EXPECT_EQ(size_t{ 4 }, wstringView.BufferSize());

    //
    // Empty buffer with 0 size.
    //
    constexpr const xpf::StringView stringViewEmpty("", 0);
    EXPECT_TRUE(stringViewEmpty.IsEmpty());
    EXPECT_TRUE(nullptr == stringViewEmpty.Buffer());
    EXPECT_EQ(size_t{ 0 }, stringViewEmpty.BufferSize());

    constexpr const xpf::StringView wstringViewEmpty(L"", 0);
    EXPECT_TRUE(wstringViewEmpty.IsEmpty());
    EXPECT_TRUE(nullptr == wstringViewEmpty.Buffer());
    EXPECT_EQ(size_t{ 0 }, wstringViewEmpty.BufferSize());

    //
    // Non-Null Empty buffer with non-0 size.
    //
    constexpr const xpf::StringView stringViewEmpty0Size("", 5);
    EXPECT_FALSE(stringViewEmpty0Size.IsEmpty());
    EXPECT_FALSE(nullptr == stringViewEmpty0Size.Buffer());
    EXPECT_EQ(size_t{ 5 }, stringViewEmpty0Size.BufferSize());

    constexpr const xpf::StringView wstringViewC16Non0Size(L"", 5);
    EXPECT_FALSE(wstringViewC16Non0Size.IsEmpty());
    EXPECT_FALSE(nullptr == wstringViewC16Non0Size.Buffer());
    EXPECT_EQ(size_t{ 5 }, wstringViewC16Non0Size.BufferSize());

    //
    // Non-Empty buffer with 0 size.
    //
    constexpr const xpf::StringView stringViewZeroSize("1234", 0);
    EXPECT_TRUE(stringViewZeroSize.IsEmpty());
    EXPECT_TRUE(nullptr == stringViewZeroSize.Buffer());
    EXPECT_EQ(size_t{ 0 }, stringViewZeroSize.BufferSize());

    constexpr const xpf::StringView wstringViewZeroSize(L"1234", 0);
    EXPECT_TRUE(wstringViewZeroSize.IsEmpty());
    EXPECT_TRUE(nullptr == wstringViewZeroSize.Buffer());
    EXPECT_EQ(size_t{ 0 }, wstringViewZeroSize.BufferSize());
}

/**
 * @brief       This tests the move constructor.
 */
TEST(TestStringView, MoveConstructor)
{
    //
    // Move from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2(xpf::Move(u8StringView1));

    EXPECT_TRUE(u8StringView1.IsEmpty());
    EXPECT_TRUE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 0 }, u8StringView1.BufferSize());

    EXPECT_FALSE(u8StringView2.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView2.BufferSize());
}

/**
 * @brief       This tests the move assignment.
 */
TEST(TestStringView, MoveAssignment)
{
    //
    // Move from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2("ab");

    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView1.BufferSize());

    EXPECT_FALSE(u8StringView2.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 2 }, u8StringView2.BufferSize());

    //
    // Self-Move scenario.
    //
    u8StringView1 = xpf::Move(u8StringView1);
    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView1.BufferSize());

    //
    // Legit move scenario.
    //
    u8StringView1 = xpf::Move(u8StringView2);

    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 2 }, u8StringView1.BufferSize());

    EXPECT_TRUE(u8StringView2.IsEmpty());
    EXPECT_TRUE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 0 }, u8StringView2.BufferSize());

    //
    // Now move empty string.
    //
    u8StringView1 = xpf::Move(u8StringView2);

    EXPECT_TRUE(u8StringView1.IsEmpty());
    EXPECT_TRUE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 0 }, u8StringView1.BufferSize());

    EXPECT_TRUE(u8StringView2.IsEmpty());
    EXPECT_TRUE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 0 }, u8StringView2.BufferSize());
}

/**
 * @brief       This tests the copy constructor.
 */
TEST(TestStringView, CopyConstructor)
{
    //
    // Copy from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2(u8StringView1);

    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView1.BufferSize());

    EXPECT_FALSE(u8StringView2.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView2.BufferSize());
}

/**
 * @brief       This tests the copy assignment.
 */
TEST(TestStringView, CopyAssignment)
{
    //
    // Copy from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2("ab");

    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView1.BufferSize());

    EXPECT_FALSE(u8StringView2.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 2 }, u8StringView2.BufferSize());

    //
    // Self-Copy scenario.
    //
    u8StringView1 = u8StringView1;
    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 4 }, u8StringView1.BufferSize());

    //
    // Legit copy scenario.
    //
    u8StringView1 = u8StringView2;

    EXPECT_FALSE(u8StringView1.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView1.Buffer());
    EXPECT_EQ(size_t{ 2 }, u8StringView1.BufferSize());

    EXPECT_FALSE(u8StringView2.IsEmpty());
    EXPECT_FALSE(nullptr == u8StringView2.Buffer());
    EXPECT_EQ(size_t{ 2 }, u8StringView2.BufferSize());
}

/**
 * @brief       This tests the Equals method.
 */
TEST(TestStringView, Equals)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer equals empty buffer.
    //
    stringView.Assign(L"");
    EXPECT_TRUE(stringView.Equals(L"", true));
    EXPECT_TRUE(stringView.Equals(L"", false));
    EXPECT_TRUE(stringView.Equals(nullptr, true));
    EXPECT_TRUE(stringView.Equals(nullptr, false));

    //
    // Tests null buffer equals empty buffer.
    //
    stringView.Assign(nullptr);
    EXPECT_TRUE(stringView.Equals(L"", true));
    EXPECT_TRUE(stringView.Equals(L"", false));
    EXPECT_TRUE(stringView.Equals(nullptr, true));
    EXPECT_TRUE(stringView.Equals(nullptr, false));

    //
    // Tests same buffer same case equals.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.Equals(L"aBCd", true));
    EXPECT_TRUE(stringView.Equals(L"aBCd", false));

    //
    // Tests same buffer different case equals only case insensitive.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Equals(L"AbcD", true));
    EXPECT_TRUE(stringView.Equals(L"aBCd",  false));

    //
    // Tests smaller buffer is not equal.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Equals(L"aBC", true));
    EXPECT_FALSE(stringView.Equals(L"aBC", false));

    //
    // Tests bigger buffer is not equal.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Equals(L"aBCdD", true));
    EXPECT_FALSE(stringView.Equals(L"aBCdD", false));
}

/**
 * @brief       This tests the StartsWith method.
 */
TEST(TestStringView, StartsWith)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer startsWith empty buffer.
    //
    stringView.Assign(L"");
    EXPECT_TRUE(stringView.StartsWith(L"", true));
    EXPECT_TRUE(stringView.StartsWith(L"", false));
    EXPECT_TRUE(stringView.StartsWith(nullptr, true));
    EXPECT_TRUE(stringView.StartsWith(nullptr, false));

    //
    // Tests null buffer starts with empty buffer.
    //
    stringView.Assign(nullptr);
    EXPECT_TRUE(stringView.StartsWith(L"", true));
    EXPECT_TRUE(stringView.StartsWith(L"", false));
    EXPECT_TRUE(stringView.StartsWith(nullptr, true));
    EXPECT_TRUE(stringView.StartsWith(nullptr, false));

    //
    // Tests same buffer starts with same buffer.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.StartsWith(L"aBCd", true));
    EXPECT_TRUE(stringView.StartsWith(L"aBCd", false));

    //
    // Tests same buffer different case starts with only case insensitive.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.StartsWith(L"AbcD", true));
    EXPECT_TRUE(stringView.StartsWith(L"aBCd",  false));

    //
    // Tests smaller buffer starts with.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.StartsWith(L"aBC", true));
    EXPECT_TRUE(stringView.StartsWith(L"aBC", false));

    //
    // Tests smaller buffer does not starts with.
    //
    stringView.Assign(L"aaBCd");
    EXPECT_FALSE(stringView.StartsWith(L"aBC", true));
    EXPECT_FALSE(stringView.StartsWith(L"aBC", false));

    //
    // Tests bigger buffer does not starts with.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.StartsWith(L"aBCdD", true));
    EXPECT_FALSE(stringView.StartsWith(L"aBCdD", false));
}

/**
 * @brief       This tests the EndsWith method.
 */
TEST(TestStringView, EndsWith)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer ends with empty buffer.
    //
    stringView.Assign(L"");
    EXPECT_TRUE(stringView.EndsWith(L"", true));
    EXPECT_TRUE(stringView.EndsWith(L"", false));
    EXPECT_TRUE(stringView.EndsWith(nullptr, true));
    EXPECT_TRUE(stringView.EndsWith(nullptr, false));

    //
    // Tests null buffer ends with empty buffer.
    //
    stringView.Assign(nullptr);
    EXPECT_TRUE(stringView.EndsWith(L"", true));
    EXPECT_TRUE(stringView.EndsWith(L"", false));
    EXPECT_TRUE(stringView.EndsWith(nullptr, true));
    EXPECT_TRUE(stringView.EndsWith(nullptr, false));

    //
    // Tests same buffer ends with same buffer.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.EndsWith(L"aBCd", true));
    EXPECT_TRUE(stringView.EndsWith(L"aBCd", false));

    //
    // Tests same buffer different case ends with only case insensitive.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.EndsWith(L"AbcD", true));
    EXPECT_TRUE(stringView.EndsWith(L"aBCd", false));

    //
    // Tests smaller buffer ends with.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.EndsWith(L"BCd", true));
    EXPECT_TRUE(stringView.EndsWith(L"BCd", false));

    //
    // Tests smaller buffer does not ends with.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.EndsWith(L"aBC", true));
    EXPECT_FALSE(stringView.EndsWith(L"aBC", false));

    //
    // Tests bigger buffer does not ends with.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.EndsWith(L"aBCdD", true));
    EXPECT_FALSE(stringView.EndsWith(L"aBCdD", false));
}

/**
 * @brief       This tests the Substring method.
 */
TEST(TestStringView, Substring)
{
    xpf::StringView<wchar_t> stringView;
    size_t index = 0;

    //
    // Tests empty buffer does not contain buffer.
    //
    stringView.Assign(L"");
    EXPECT_FALSE(stringView.Substring(L"", true, nullptr));
    EXPECT_FALSE(stringView.Substring(L"", false, nullptr));
    EXPECT_FALSE(stringView.Substring(nullptr, true, nullptr));
    EXPECT_FALSE(stringView.Substring(nullptr, false, nullptr));

    //
    // Tests null buffer does not cantain empty buffer.
    //
    stringView.Assign(nullptr);
    EXPECT_FALSE(stringView.Substring(L"", true, nullptr));
    EXPECT_FALSE(stringView.Substring(L"", false, nullptr));
    EXPECT_FALSE(stringView.Substring(nullptr, true, nullptr));
    EXPECT_FALSE(stringView.Substring(nullptr, false, nullptr));

    //
    // Tests same buffer contains the same buffer.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.Substring(L"aBCd", true, &index));
    EXPECT_EQ(size_t{ 0 }, index);

    EXPECT_TRUE(stringView.Substring(L"aBCd", false, &index));
    EXPECT_EQ(size_t{ 0 }, index);

    //
    // Tests same buffer different case contains only case insensitive.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Substring(L"AbcD", true, &index));

    EXPECT_TRUE(stringView.Substring(L"aBCd", false, &index));
    EXPECT_EQ(size_t{ 0 }, index);

    //
    // Tests smaller is contained.
    //
    stringView.Assign(L"aBCd");
    EXPECT_TRUE(stringView.Substring(L"BCd", true, &index));
    EXPECT_EQ(size_t{ 1 }, index);

    EXPECT_TRUE(stringView.Substring(L"BCd", false, &index));
    EXPECT_EQ(size_t{ 1 }, index);

    //
    // Tests smaller buffer is not contained.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Substring(L"axY", true, &index));
    EXPECT_FALSE(stringView.Substring(L"axY", false, &index));

    //
    // Tests bigger buffer is not contained.
    //
    stringView.Assign(L"aBCd");
    EXPECT_FALSE(stringView.Substring(L"aBCdD", true, &index));
    EXPECT_FALSE(stringView.Substring(L"aBCdD", false, &index));
}

/**
 * @brief       This tests the index operator.
 */
TEST(TestStringView, IndexOperator)
{
    xpf::StringView<wchar_t> stringView = L"a1b2";

    EXPECT_EQ(stringView[0], L'a');
    EXPECT_EQ(stringView[1], L'1');
    EXPECT_EQ(stringView[2], L'b');
    EXPECT_EQ(stringView[3], L'2');

    //
    // Now test the out of bounds access of the view.
    //
    EXPECT_DEATH(stringView[1000], ".*");

    xpf::StringView<wchar_t> emptyView = L"";
    EXPECT_DEATH(emptyView[0], ".*");
}

/**
 * @brief       This tests the Remove Prefix
 */
TEST(TestStringView, RemovePrefix)
{
    //
    // Remove from empty view.
    //
    xpf::StringView<wchar_t> emptyView = L"";
    EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemovePrefix(0);
    EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemovePrefix(100);
    EXPECT_TRUE(emptyView.IsEmpty());

    //
    // Remove from non-empty view.
    //
    xpf::StringView<wchar_t> nonEmptyView = L"abc";
    EXPECT_EQ(size_t{ 3 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'a', nonEmptyView[0]);
    EXPECT_EQ(L'b', nonEmptyView[1]);
    EXPECT_EQ(L'c', nonEmptyView[2]);

    nonEmptyView.RemovePrefix(0);
    EXPECT_EQ(size_t{ 3 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'a', nonEmptyView[0]);
    EXPECT_EQ(L'b', nonEmptyView[1]);
    EXPECT_EQ(L'c', nonEmptyView[2]);

    nonEmptyView.RemovePrefix(1);
    EXPECT_EQ(size_t{ 2 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'b', nonEmptyView[0]);
    EXPECT_EQ(L'c', nonEmptyView[1]);

    nonEmptyView.RemovePrefix(4);
    EXPECT_EQ(size_t{ 0 }, nonEmptyView.BufferSize());

    //
    // Remove all from non-empty view.
    //
    nonEmptyView.Assign(L"abcdefg");
    nonEmptyView.RemovePrefix(100);
    EXPECT_EQ(size_t{ 0 }, nonEmptyView.BufferSize());
}

/**
 * @brief       This tests the Remove Suffix
 */
TEST(TestStringView, RemoveSuffix)
{
    //
    // Remove from empty view.
    //
    xpf::StringView<wchar_t> emptyView = L"";
    EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemoveSuffix(0);
    EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemoveSuffix(100);
    EXPECT_TRUE(emptyView.IsEmpty());

    //
    // Remove from non-empty view.
    //
    xpf::StringView<wchar_t> nonEmptyView = L"abc";
    EXPECT_EQ(size_t{ 3 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'a', nonEmptyView[0]);
    EXPECT_EQ(L'b', nonEmptyView[1]);
    EXPECT_EQ(L'c', nonEmptyView[2]);

    nonEmptyView.RemoveSuffix(0);
    EXPECT_EQ(size_t{ 3 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'a', nonEmptyView[0]);
    EXPECT_EQ(L'b', nonEmptyView[1]);
    EXPECT_EQ(L'c', nonEmptyView[2]);

    nonEmptyView.RemoveSuffix(1);
    EXPECT_EQ(size_t{ 2 }, nonEmptyView.BufferSize());
    EXPECT_EQ(L'a', nonEmptyView[0]);
    EXPECT_EQ(L'b', nonEmptyView[1]);

    nonEmptyView.RemoveSuffix(2);
    EXPECT_EQ(size_t{ 0 }, nonEmptyView.BufferSize());

    //
    // Remove all from non-empty view.
    //
    nonEmptyView.Assign(L"abcdefg");
    nonEmptyView.RemoveSuffix(100);
    EXPECT_EQ(size_t{ 0 }, nonEmptyView.BufferSize());
}

/**
 * @brief       This tests the default constructor of string.
 */
TEST(TestString, DefaultConstructorDestructor)
{
    //
    // Ansi string.
    //
    xpf::String<char> string;
    EXPECT_TRUE(string.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, string.BufferSize());

    //
    // Should be the same size due to compressed_pair.
    //
    const size_t expectedStringSize = sizeof(void*) + sizeof(size_t);
    EXPECT_EQ(expectedStringSize, static_cast<size_t>(sizeof(string)));

    //
    // Wide string.
    //
    xpf::String<wchar_t> wstring;
    EXPECT_TRUE(wstring.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, wstring.BufferSize());

    //
    // Should be the same size due to compressed_pair.
    //
    const size_t expectedWStringSize = sizeof(void*) + sizeof(size_t);
    EXPECT_EQ(expectedWStringSize, static_cast<size_t>(sizeof(wstring)));
}

/**
 * @brief       This tests the move constructor.
 */
TEST(TestString, MoveConstructor)
{
    //
    // Move from a string to another.
    //
    xpf::String<char> string1;
    EXPECT_TRUE(NT_SUCCESS(string1.Append("1234")));

    xpf::String string2(xpf::Move(string1));

    EXPECT_TRUE(string1.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, string1.BufferSize());

    EXPECT_FALSE(string2.IsEmpty());
    EXPECT_EQ(size_t{ 4 }, string2.BufferSize());
}

/**
 * @brief       This tests the move assignment.
 */
TEST(TestString, MoveAssignment)
{
    //
    // Move from a string to another.
    //
    xpf::String<char> string1;
    EXPECT_TRUE(NT_SUCCESS(string1.Append("1234")));

    xpf::String<char> string2;
    EXPECT_TRUE(NT_SUCCESS(string2.Append("ab")));

    EXPECT_FALSE(string1.IsEmpty());
    EXPECT_EQ(size_t{ 4 }, string1.BufferSize());

    EXPECT_FALSE(string2.IsEmpty());
    EXPECT_EQ(size_t{ 2 }, string2.BufferSize());

    //
    // Self-Move scenario.
    //
    string1 = xpf::Move(string1);
    EXPECT_FALSE(string1.IsEmpty());
    EXPECT_EQ(size_t{ 4 }, string1.BufferSize());

    //
    // Legit move scenario.
    //
    string1 = xpf::Move(string2);

    EXPECT_FALSE(string1.IsEmpty());
    EXPECT_EQ(size_t{ 2 }, string1.BufferSize());

    EXPECT_TRUE(string2.IsEmpty());;
    EXPECT_EQ(size_t{ 0 }, string2.BufferSize());

    //
    // Now move empty string.
    //
    string1 = xpf::Move(string2);

    EXPECT_TRUE(string1.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, string1.BufferSize());

    EXPECT_TRUE(string2.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, string2.BufferSize());
}

/**
 * @brief       This tests the index operator.
 */
TEST(TestString, IndexOperator)
{
    xpf::String<wchar_t> string1;
    EXPECT_TRUE(NT_SUCCESS(string1.Append(L"a1b2")));

    string1[0] = L'X';
    string1[1] = L'Y';
    string1[2] = L'z';
    string1[3] = L'7';

    const auto& constString1 = string1;
    EXPECT_EQ(constString1[0], L'X');
    EXPECT_EQ(constString1[1], L'Y');
    EXPECT_EQ(constString1[2], L'z');
    EXPECT_EQ(constString1[3], L'7');

    //
    // Now test the out of bounds access of the view.
    //
    EXPECT_DEATH(string1[1000], ".*");
}

/**
 * @brief       This tests the append and reset method.
 */
TEST(TestString, AppendReset)
{
    xpf::String<char> string1;

    EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));

    EXPECT_EQ(string1[0], L'a');
    EXPECT_EQ(string1[1], L'1');
    EXPECT_EQ(string1[2], L'b');
    EXPECT_EQ(string1[3], L'2');

    EXPECT_EQ(string1[4], L'a');
    EXPECT_EQ(string1[5], L'1');
    EXPECT_EQ(string1[6], L'b');
    EXPECT_EQ(string1[7], L'2');

    EXPECT_EQ(string1[8], L'a');
    EXPECT_EQ(string1[9], L'1');
    EXPECT_EQ(string1[10], L'b');
    EXPECT_EQ(string1[11], L'2');

    string1.Reset();
    EXPECT_EQ(size_t{ 0 }, string1.BufferSize());

    //
    // Now we test the append with the same view.
    //
    EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    EXPECT_TRUE(NT_SUCCESS(string1.Append(string1.View())));  // a1b2a1b2
    EXPECT_TRUE(NT_SUCCESS(string1.Append(string1.View())));  // a1b2a1b2a1b2a1b2

    EXPECT_TRUE(string1.View().Equals("a1b2a1b2a1b2a1b2", true));
}


/**
 * @brief       This tests the ToLower method.
 */
TEST(TestString, ToLower)
{
    xpf::String<char> string1;

    EXPECT_TRUE(NT_SUCCESS(string1.Append("AAaabb12BBCCs")));
    EXPECT_TRUE(string1.View().Equals("AAaabb12BBCCs", true));

    string1.ToLower();
    EXPECT_TRUE(string1.View().Equals("aaaabb12bbccs", true));

    string1.Reset();
    string1.ToLower();
    EXPECT_TRUE(string1.IsEmpty());
}

/**
 * @brief       This tests the ToUpper method.
 */
TEST(TestString, ToUpper)
{
    xpf::String<char> string1;

    EXPECT_TRUE(NT_SUCCESS(string1.Append("AAaabb12BBCCs")));
    EXPECT_TRUE(string1.View().Equals("AAaabb12BBCCs", true));

    string1.ToUpper();
    EXPECT_TRUE(string1.View().Equals("AAAABB12BBCCS", true));

    string1.Reset();
    string1.ToUpper();
    EXPECT_TRUE(string1.IsEmpty());
}
