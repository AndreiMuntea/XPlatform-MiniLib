/**
 * @file        xpf_tests/tests/Containers/TestSpan.cpp
 *
 * @brief       This contains tests for Span.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"

/**
 * @brief       This tests the default constructor of Span.
 */
XPF_TEST_SCENARIO(TestSpan, DefaultConstructor)
{
    xpf::Span<int> span;

    XPF_TEST_EXPECT_TRUE(span.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(span.IsEmpty());
    XPF_TEST_EXPECT_TRUE(nullptr == span.Buffer());
}

/**
 * @brief       This tests constructing a span from a pointer and size.
 */
XPF_TEST_SCENARIO(TestSpan, FromPointerAndSize)
{
    int data[] = { 10, 20, 30, 40, 50 };
    xpf::Span<int> span(&data[0], 5);

    XPF_TEST_EXPECT_TRUE(span.Size() == size_t{ 5 });
    XPF_TEST_EXPECT_TRUE(!span.IsEmpty());
    XPF_TEST_EXPECT_TRUE(span.Buffer() == &data[0]);

    XPF_TEST_EXPECT_TRUE(span[0] == 10);
    XPF_TEST_EXPECT_TRUE(span[1] == 20);
    XPF_TEST_EXPECT_TRUE(span[2] == 30);
    XPF_TEST_EXPECT_TRUE(span[3] == 40);
    XPF_TEST_EXPECT_TRUE(span[4] == 50);
}

/**
 * @brief       This tests constructing a span from a C-style array.
 */
XPF_TEST_SCENARIO(TestSpan, FromCArray)
{
    int data[] = { 100, 200, 300 };
    xpf::Span<int> span(data);

    XPF_TEST_EXPECT_TRUE(span.Size() == size_t{ 3 });
    XPF_TEST_EXPECT_TRUE(!span.IsEmpty());
    XPF_TEST_EXPECT_TRUE(span[0] == 100);
    XPF_TEST_EXPECT_TRUE(span[1] == 200);
    XPF_TEST_EXPECT_TRUE(span[2] == 300);
}

/**
 * @brief       This tests SubSpan with valid offset and count.
 */
XPF_TEST_SCENARIO(TestSpan, SubSpan)
{
    int data[] = { 10, 20, 30, 40 };
    xpf::Span<int> span(data);

    xpf::Span<int> sub = span.SubSpan(1, 2);
    XPF_TEST_EXPECT_TRUE(sub.Size() == size_t{ 2 });
    XPF_TEST_EXPECT_TRUE(sub[0] == 20);
    XPF_TEST_EXPECT_TRUE(sub[1] == 30);
}

/**
 * @brief       This tests SubSpan when offset + count exceeds the size.
 *              The count should be clamped.
 */
XPF_TEST_SCENARIO(TestSpan, SubSpanClampsToBounds)
{
    int data[] = { 10, 20, 30, 40 };
    xpf::Span<int> span(data);

    /* Offset 2, count 100 should be clamped to 2 elements (30, 40). */
    xpf::Span<int> sub = span.SubSpan(2, 100);
    XPF_TEST_EXPECT_TRUE(sub.Size() == size_t{ 2 });
    XPF_TEST_EXPECT_TRUE(sub[0] == 30);
    XPF_TEST_EXPECT_TRUE(sub[1] == 40);
}

/**
 * @brief       This tests SubSpan when offset equals size.
 *              Should return an empty span.
 */
XPF_TEST_SCENARIO(TestSpan, SubSpanFromEnd)
{
    int data[] = { 10, 20, 30 };
    xpf::Span<int> span(data);

    xpf::Span<int> sub = span.SubSpan(3, 5);
    XPF_TEST_EXPECT_TRUE(sub.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(sub.IsEmpty());
}

/**
 * @brief       This tests the copy constructor.
 *              Both spans should point to the same data.
 */
XPF_TEST_SCENARIO(TestSpan, CopyConstructor)
{
    int data[] = { 1, 2, 3 };
    xpf::Span<int> span1(data);
    xpf::Span<int> span2(span1);

    XPF_TEST_EXPECT_TRUE(span2.Size() == size_t{ 3 });
    XPF_TEST_EXPECT_TRUE(span2.Buffer() == span1.Buffer());
    XPF_TEST_EXPECT_TRUE(span2[0] == 1);
    XPF_TEST_EXPECT_TRUE(span2[1] == 2);
    XPF_TEST_EXPECT_TRUE(span2[2] == 3);
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestSpan, MoveConstructor)
{
    int data[] = { 5, 10, 15 };
    xpf::Span<int> span1(data);
    xpf::Span<int> span2(xpf::Move(span1));

    XPF_TEST_EXPECT_TRUE(span2.Size() == size_t{ 3 });
    XPF_TEST_EXPECT_TRUE(span2[0] == 5);
    XPF_TEST_EXPECT_TRUE(span2[1] == 10);
    XPF_TEST_EXPECT_TRUE(span2[2] == 15);
}

/**
 * @brief       This tests the copy assignment.
 */
XPF_TEST_SCENARIO(TestSpan, CopyAssignment)
{
    int data[] = { 42, 43, 44 };
    xpf::Span<int> span1(data);
    xpf::Span<int> span2;

    span2 = span1;

    XPF_TEST_EXPECT_TRUE(span2.Size() == size_t{ 3 });
    XPF_TEST_EXPECT_TRUE(span2.Buffer() == span1.Buffer());
    XPF_TEST_EXPECT_TRUE(span2[0] == 42);
}

/**
 * @brief       This tests that out-of-bounds access triggers death on debug.
 */
XPF_TEST_SCENARIO(TestSpan, IndexOperatorBoundsCheck)
{
    int data[] = { 1, 2, 3 };
    xpf::Span<int> span(data);

    /* Accessing index 100 on a span of 3 elements should trigger death on debug. */
    XPF_TEST_EXPECT_DEATH(span[100]);
}
