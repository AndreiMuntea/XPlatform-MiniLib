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
XPF_TEST_SCENARIO(TestStringView, DefaultConstructorDestructor)
{
    constexpr const xpf::StringView<char> stringView;
    XPF_TEST_EXPECT_TRUE(stringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == stringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == stringView.BufferSize());

    constexpr const xpf::StringView<wchar_t> wstringView;
    XPF_TEST_EXPECT_TRUE(wstringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == wstringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == wstringView.BufferSize());
}

/**
 * @brief       This tests the constructor with only a buffer provided.
 */
XPF_TEST_SCENARIO(TestStringView, BufferConstructor)
{
    //
    // Non-empty buffer.
    //
    constexpr const xpf::StringView stringView("1234");
    XPF_TEST_EXPECT_TRUE(!stringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != stringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == stringView.BufferSize());

    constexpr const xpf::StringView wstringView(L"1234");
    XPF_TEST_EXPECT_TRUE(!wstringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != wstringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == wstringView.BufferSize());

    //
    // Empty buffer.
    //
    constexpr const xpf::StringView stringViewEmpty("");
    XPF_TEST_EXPECT_TRUE(stringViewEmpty.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == stringViewEmpty.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == stringViewEmpty.BufferSize());

    constexpr const xpf::StringView wstringViewEmpty(L"");
    XPF_TEST_EXPECT_TRUE(wstringViewEmpty.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == wstringViewEmpty.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == wstringViewEmpty.BufferSize());
}

/**
 * @brief       This tests the constructor with a buffer and also size.
 */
XPF_TEST_SCENARIO(TestStringView, BufferSizeConstructor)
{
    //
    // Non-empty buffer.
    //
    constexpr const xpf::StringView stringView("1234", 4);
    XPF_TEST_EXPECT_TRUE(!stringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != stringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == stringView.BufferSize());

    constexpr const xpf::StringView wstringView(L"1234", 4);
    XPF_TEST_EXPECT_TRUE(!wstringView.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != wstringView.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == wstringView.BufferSize());

    //
    // Empty buffer with 0 size.
    //
    constexpr const xpf::StringView stringViewEmpty("", 0);
    XPF_TEST_EXPECT_TRUE(stringViewEmpty.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == stringViewEmpty.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == stringViewEmpty.BufferSize());

    constexpr const xpf::StringView wstringViewEmpty(L"", 0);
    XPF_TEST_EXPECT_TRUE(wstringViewEmpty.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == wstringViewEmpty.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == wstringViewEmpty.BufferSize());

    //
    // Non-Null Empty buffer with non-0 size.
    //
    constexpr const xpf::StringView stringViewEmpty0Size("", 5);
    XPF_TEST_EXPECT_TRUE(!stringViewEmpty0Size.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != stringViewEmpty0Size.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 5 } == stringViewEmpty0Size.BufferSize());

    constexpr const xpf::StringView wstringViewC16Non0Size(L"", 5);
    XPF_TEST_EXPECT_TRUE(!wstringViewC16Non0Size.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != wstringViewC16Non0Size.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 5 } == wstringViewC16Non0Size.BufferSize());

    //
    // Non-Empty buffer with 0 size.
    //
    constexpr const xpf::StringView stringViewZeroSize("1234", 0);
    XPF_TEST_EXPECT_TRUE(stringViewZeroSize.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == stringViewZeroSize.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == stringViewZeroSize.BufferSize());

    constexpr const xpf::StringView wstringViewZeroSize(L"1234", 0);
    XPF_TEST_EXPECT_TRUE(wstringViewZeroSize.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == wstringViewZeroSize.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == wstringViewZeroSize.BufferSize());
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestStringView, MoveConstructor)
{
    //
    // Move from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2(xpf::Move(u8StringView1));

    XPF_TEST_EXPECT_TRUE(u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView2.BufferSize());
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestStringView, MoveAssignment)
{
    //
    // Move from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2("ab");

    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == u8StringView2.BufferSize());

    //
    // Self-Move scenario.
    //
    u8StringView1 = xpf::Move(u8StringView1);
    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView1.BufferSize());

    //
    // Legit move scenario.
    //
    u8StringView1 = xpf::Move(u8StringView2);

    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == u8StringView2.BufferSize());

    //
    // Now move empty string.
    //
    u8StringView1 = xpf::Move(u8StringView2);

    XPF_TEST_EXPECT_TRUE(u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == u8StringView2.BufferSize());
}

/**
 * @brief       This tests the copy constructor.
 */
XPF_TEST_SCENARIO(TestStringView, CopyConstructor)
{
    //
    // Copy from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2(u8StringView1);

    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView2.BufferSize());
}

/**
 * @brief       This tests the copy assignment.
 */
XPF_TEST_SCENARIO(TestStringView, CopyAssignment)
{
    //
    // Copy from a string to another.
    //
    xpf::StringView u8StringView1("1234");
    xpf::StringView u8StringView2("ab");

    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == u8StringView2.BufferSize());

    //
    // CLANG will warn about self assignment.
    // But this is intentional here. So we manually disable the warning
    //
    #if defined XPF_COMPILER_CLANG
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wself-assign-overloaded"
    #endif  // XPF_COMPILER_CLANG

        u8StringView1 = u8StringView1;
        XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
        XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
        XPF_TEST_EXPECT_TRUE(size_t{ 4 } == u8StringView1.BufferSize());

    #if defined XPF_COMPILER_CLANG
        #pragma clang diagnostic pop
    #endif  // XPF_COMPILER_CLANG
    //
    // Legit copy scenario.
    //
    u8StringView1 = u8StringView2;

    XPF_TEST_EXPECT_TRUE(!u8StringView1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView1.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == u8StringView1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!u8StringView2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr != u8StringView2.Buffer());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == u8StringView2.BufferSize());
}

/**
 * @brief       This tests the Equals method.
 */
XPF_TEST_SCENARIO(TestStringView, Equals)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer equals empty buffer.
    //
    stringView.Assign(L"");
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(nullptr, false));

    //
    // Tests null buffer equals empty buffer.
    //
    stringView.Assign(nullptr);
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(nullptr, false));

    //
    // Tests same buffer same case equals.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"aBCd", true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"aBCd", false));

    //
    // Tests same buffer different case equals only case insensitive.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Equals(L"AbcD", true));
    XPF_TEST_EXPECT_TRUE(stringView.Equals(L"aBCd",  false));

    //
    // Tests smaller buffer is not equal.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Equals(L"aBC", true));
    XPF_TEST_EXPECT_TRUE(!stringView.Equals(L"aBC", false));

    //
    // Tests bigger buffer is not equal.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Equals(L"aBCdD", true));
    XPF_TEST_EXPECT_TRUE(!stringView.Equals(L"aBCdD", false));
}

/**
 * @brief       This tests the StartsWith method.
 */
XPF_TEST_SCENARIO(TestStringView, StartsWith)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer startsWith empty buffer.
    //
    stringView.Assign(L"");
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(nullptr, false));

    //
    // Tests null buffer starts with empty buffer.
    //
    stringView.Assign(nullptr);
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(nullptr, false));

    //
    // Tests same buffer starts with same buffer.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"aBCd", true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"aBCd", false));

    //
    // Tests same buffer different case starts with only case insensitive.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.StartsWith(L"AbcD", true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"aBCd",  false));

    //
    // Tests smaller buffer starts with.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"aBC", true));
    XPF_TEST_EXPECT_TRUE(stringView.StartsWith(L"aBC", false));

    //
    // Tests smaller buffer does not starts with.
    //
    stringView.Assign(L"aaBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.StartsWith(L"aBC", true));
    XPF_TEST_EXPECT_TRUE(!stringView.StartsWith(L"aBC", false));

    //
    // Tests bigger buffer does not starts with.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.StartsWith(L"aBCdD", true));
    XPF_TEST_EXPECT_TRUE(!stringView.StartsWith(L"aBCdD", false));
}

/**
 * @brief       This tests the EndsWith method.
 */
XPF_TEST_SCENARIO(TestStringView, EndsWith)
{
    xpf::StringView<wchar_t> stringView;

    //
    // Tests empty buffer ends with empty buffer.
    //
    stringView.Assign(L"");
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(nullptr, false));

    //
    // Tests null buffer ends with empty buffer.
    //
    stringView.Assign(nullptr);
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"", true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"", false));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(nullptr, true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(nullptr, false));

    //
    // Tests same buffer ends with same buffer.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"aBCd", true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"aBCd", false));

    //
    // Tests same buffer different case ends with only case insensitive.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.EndsWith(L"AbcD", true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"aBCd", false));

    //
    // Tests smaller buffer ends with.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"BCd", true));
    XPF_TEST_EXPECT_TRUE(stringView.EndsWith(L"BCd", false));

    //
    // Tests smaller buffer does not ends with.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.EndsWith(L"aBC", true));
    XPF_TEST_EXPECT_TRUE(!stringView.EndsWith(L"aBC", false));

    //
    // Tests bigger buffer does not ends with.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.EndsWith(L"aBCdD", true));
    XPF_TEST_EXPECT_TRUE(!stringView.EndsWith(L"aBCdD", false));
}

/**
 * @brief       This tests the Substring method.
 */
XPF_TEST_SCENARIO(TestStringView, Substring)
{
    xpf::StringView<wchar_t> stringView;
    size_t index = 0;

    //
    // Tests empty buffer does not contain buffer.
    //
    stringView.Assign(L"");
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"", true, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"", false, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(nullptr, true, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(nullptr, false, nullptr));

    //
    // Tests null buffer does not cantain empty buffer.
    //
    stringView.Assign(nullptr);
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"", true, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"", false, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(nullptr, true, nullptr));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(nullptr, false, nullptr));

    //
    // Tests same buffer contains the same buffer.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.Substring(L"aBCd", true, &index));
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == index);

    XPF_TEST_EXPECT_TRUE(stringView.Substring(L"aBCd", false, &index));
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == index);

    //
    // Tests same buffer different case contains only case insensitive.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"AbcD", true, &index));

    XPF_TEST_EXPECT_TRUE(stringView.Substring(L"aBCd", false, &index));
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == index);

    //
    // Tests smaller is contained.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(stringView.Substring(L"BCd", true, &index));
    XPF_TEST_EXPECT_TRUE(size_t{ 1 } == index);

    XPF_TEST_EXPECT_TRUE(stringView.Substring(L"BCd", false, &index));
    XPF_TEST_EXPECT_TRUE(size_t{ 1 } == index);

    //
    // Tests smaller buffer is not contained.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"axY", true, &index));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"axY", false, &index));

    //
    // Tests bigger buffer is not contained.
    //
    stringView.Assign(L"aBCd");
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"aBCdD", true, &index));
    XPF_TEST_EXPECT_TRUE(!stringView.Substring(L"aBCdD", false, &index));
}

/**
 * @brief       This tests the index operator.
 */
XPF_TEST_SCENARIO(TestStringView, IndexOperator)
{
    xpf::StringView<wchar_t> stringView = L"a1b2";

    XPF_TEST_EXPECT_TRUE(stringView[0] == L'a');
    XPF_TEST_EXPECT_TRUE(stringView[1] == L'1');
    XPF_TEST_EXPECT_TRUE(stringView[2] == L'b');
    XPF_TEST_EXPECT_TRUE(stringView[3] == L'2');

    //
    // Now test the out of bounds access of the view.
    //
    XPF_TEST_EXPECT_DEATH(stringView[1000] == L'\0');

    xpf::StringView<wchar_t> emptyView = L"";
    XPF_TEST_EXPECT_DEATH(emptyView[0] == L'\0');
}

/**
 * @brief       This tests the Remove Prefix
 */
XPF_TEST_SCENARIO(TestStringView, RemovePrefix)
{
    //
    // Remove from empty view.
    //
    xpf::StringView<wchar_t> emptyView = L"";
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemovePrefix(0);
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemovePrefix(100);
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    //
    // Remove from non-empty view.
    //
    xpf::StringView<wchar_t> nonEmptyView = L"abc";
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'a' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[1]);
    XPF_TEST_EXPECT_TRUE(L'c' == nonEmptyView[2]);

    nonEmptyView.RemovePrefix(0);
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'a' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[1]);
    XPF_TEST_EXPECT_TRUE(L'c' == nonEmptyView[2]);

    nonEmptyView.RemovePrefix(1);
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'c' == nonEmptyView[1]);

    nonEmptyView.RemovePrefix(4);
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == nonEmptyView.BufferSize());

    //
    // Remove all from non-empty view.
    //
    nonEmptyView.Assign(L"abcdefg");
    nonEmptyView.RemovePrefix(100);
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == nonEmptyView.BufferSize());
}

/**
 * @brief       This tests the Remove Suffix
 */
XPF_TEST_SCENARIO(TestStringView, RemoveSuffix)
{
    //
    // Remove from empty view.
    //
    xpf::StringView<wchar_t> emptyView = L"";
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemoveSuffix(0);
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    emptyView.RemoveSuffix(100);
    XPF_TEST_EXPECT_TRUE(emptyView.IsEmpty());

    //
    // Remove from non-empty view.
    //
    xpf::StringView<wchar_t> nonEmptyView = L"abc";
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'a' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[1]);
    XPF_TEST_EXPECT_TRUE(L'c' == nonEmptyView[2]);

    nonEmptyView.RemoveSuffix(0);
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'a' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[1]);
    XPF_TEST_EXPECT_TRUE(L'c' == nonEmptyView[2]);

    nonEmptyView.RemoveSuffix(1);
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == nonEmptyView.BufferSize());
    XPF_TEST_EXPECT_TRUE(L'a' == nonEmptyView[0]);
    XPF_TEST_EXPECT_TRUE(L'b' == nonEmptyView[1]);

    nonEmptyView.RemoveSuffix(2);
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == nonEmptyView.BufferSize());

    //
    // Remove all from non-empty view.
    //
    nonEmptyView.Assign(L"abcdefg");
    nonEmptyView.RemoveSuffix(100);
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == nonEmptyView.BufferSize());
}

/**
 * @brief       This tests the default constructor of string.
 */
XPF_TEST_SCENARIO(TestString, DefaultConstructorDestructor)
{
    //
    // Ansi string.
    //
    xpf::String<char> string;
    XPF_TEST_EXPECT_TRUE(string.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string.BufferSize());

    //
    // Should be the same size due to compressed_pair.
    //
    static_assert(sizeof(void*) * 3 + sizeof(size_t) == static_cast<size_t>(sizeof(string)),
                  "Compile time assert for size!");

    //
    // Wide string.
    //
    xpf::String<wchar_t> wstring;
    XPF_TEST_EXPECT_TRUE(wstring.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == wstring.BufferSize());

    //
    // Should be the same size due to compressed_pair.
    //
    static_assert(sizeof(void*) * 3 + sizeof(size_t) == static_cast<size_t>(sizeof(wstring)),
                  "Compile time assert for size!");
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestString, MoveConstructor)
{
    //
    // Move from a string to another.
    //
    xpf::String<char> string1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("1234")));

    xpf::String string2(xpf::Move(string1));

    XPF_TEST_EXPECT_TRUE(string1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!string2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == string2.BufferSize());
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestString, MoveAssignment)
{
    //
    // Move from a string to another.
    //
    xpf::String<char> string1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("1234")));

    xpf::String<char> string2;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string2.Append("ab")));

    XPF_TEST_EXPECT_TRUE(!string1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == string1.BufferSize());

    XPF_TEST_EXPECT_TRUE(!string2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == string2.BufferSize());

    //
    // Self-Move scenario.
    //
    string1 = xpf::Move(string1);
    XPF_TEST_EXPECT_TRUE(!string1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == string1.BufferSize());

    //
    // Legit move scenario.
    //
    string1 = xpf::Move(string2);

    XPF_TEST_EXPECT_TRUE(!string1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == string1.BufferSize());

    XPF_TEST_EXPECT_TRUE(string2.IsEmpty());;
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string2.BufferSize());

    //
    // Now move empty string.
    //
    string1 = xpf::Move(string2);

    XPF_TEST_EXPECT_TRUE(string1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string1.BufferSize());

    XPF_TEST_EXPECT_TRUE(string2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string2.BufferSize());
}

/**
 * @brief       This tests the index operator.
 */
XPF_TEST_SCENARIO(TestString, IndexOperator)
{
    xpf::String<wchar_t> string1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append(L"a1b2")));

    string1[0] = L'X';
    string1[1] = L'Y';
    string1[2] = L'z';
    string1[3] = L'7';

    const auto& constString1 = string1;
    XPF_TEST_EXPECT_TRUE(constString1[0] == L'X');
    XPF_TEST_EXPECT_TRUE(constString1[1] == L'Y');
    XPF_TEST_EXPECT_TRUE(constString1[2] == L'z');
    XPF_TEST_EXPECT_TRUE(constString1[3] == L'7');

    //
    // Now test the out of bounds access of the view.
    //
    XPF_TEST_EXPECT_DEATH(string1[1000] == L'\0');
}

/**
 * @brief       This tests the append and reset method.
 */
XPF_TEST_SCENARIO(TestString, AppendReset)
{
    xpf::String<char> string1;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));

    XPF_TEST_EXPECT_TRUE(string1[0] == L'a');
    XPF_TEST_EXPECT_TRUE(string1[1] == L'1');
    XPF_TEST_EXPECT_TRUE(string1[2] == L'b');
    XPF_TEST_EXPECT_TRUE(string1[3] == L'2');

    XPF_TEST_EXPECT_TRUE(string1[4] == L'a');
    XPF_TEST_EXPECT_TRUE(string1[5] == L'1');
    XPF_TEST_EXPECT_TRUE(string1[6] == L'b');
    XPF_TEST_EXPECT_TRUE(string1[7] == L'2');

    XPF_TEST_EXPECT_TRUE(string1[8] == L'a');
    XPF_TEST_EXPECT_TRUE(string1[9] == L'1');
    XPF_TEST_EXPECT_TRUE(string1[10] == L'b');
    XPF_TEST_EXPECT_TRUE(string1[11] == L'2');

    string1.Reset();
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == string1.BufferSize());

    //
    // Now we test the append with the same view.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("a1b2")));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append(string1.View())));  // a1b2a1b2
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append(string1.View())));  // a1b2a1b2a1b2a1b2

    XPF_TEST_EXPECT_TRUE(string1.View().Equals("a1b2a1b2a1b2a1b2", true));
}


/**
 * @brief       This tests the ToLower method.
 */
XPF_TEST_SCENARIO(TestString, ToLower)
{
    xpf::String<char> string1;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("AAaabb12BBCCs")));
    XPF_TEST_EXPECT_TRUE(string1.View().Equals("AAaabb12BBCCs", true));

    string1.ToLower();
    XPF_TEST_EXPECT_TRUE(string1.View().Equals("aaaabb12bbccs", true));

    string1.Reset();
    string1.ToLower();
    XPF_TEST_EXPECT_TRUE(string1.IsEmpty());
}

/**
 * @brief       This tests the ToUpper method.
 */
XPF_TEST_SCENARIO(TestString, ToUpper)
{
    xpf::String<char> string1;

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(string1.Append("AAaabb12BBCCs")));
    XPF_TEST_EXPECT_TRUE(string1.View().Equals("AAaabb12BBCCs", true));

    string1.ToUpper();
    XPF_TEST_EXPECT_TRUE(string1.View().Equals("AAAABB12BBCCS", true));

    string1.Reset();
    string1.ToUpper();
    XPF_TEST_EXPECT_TRUE(string1.IsEmpty());
}

/**
 * @brief       This tests the string conversion
 */
XPF_TEST_SCENARIO(TestStringConversion, TestUtf8ToWideAndBack)
{
    const xpf::StringView<char> utf8Input = "ab 12 x y z";
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    xpf::String<wchar_t> wideStr;
    xpf::String<char> utf8Str;

    status = xpf::StringConversion::UTF8ToWide(utf8Input, wideStr);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(wideStr.View().Equals(L"ab 12 x y z", true));

    status = xpf::StringConversion::WideToUTF8(wideStr.View(), utf8Str);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(utf8Str.View().Equals("ab 12 x y z", true));

    const xpf::StringView<wchar_t> wideInput = L"quick z\u00df\u6c34\U0001d10b fox";
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(xpf::StringConversion::WideToUTF8(wideInput, utf8Str)));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(xpf::StringConversion::UTF8ToWide(utf8Str.View(), wideStr)));
    XPF_TEST_EXPECT_TRUE(wideStr.View().Equals(L"quick z\u00df\u6c34\U0001d10b fox", true));
}
