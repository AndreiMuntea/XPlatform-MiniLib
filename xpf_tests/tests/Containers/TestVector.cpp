/**
 * @file        xpf_tests/tests/Containers/TestVector.cpp
 *
 * @brief       This contains tests for vector
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
 * @brief       This tests the default constructor of vector.
 */
TEST(TestVector, DefaultConstructorDestructor)
{
    xpf::Vector<uint64_t> vector;
    EXPECT_EQ(vector.Size(), size_t{ 0 });
    EXPECT_TRUE(vector.IsEmpty());

    const size_t expectedVectorSize = sizeof(void*) + 2 * sizeof(size_t);
    EXPECT_EQ(expectedVectorSize, static_cast<size_t>(sizeof(vector)));
}

/**
 * @brief       This tests the move constructor.
 */
TEST(TestVector, MoveConstructor)
{
    //
    // Move from a vector to another.
    //
    xpf::Vector<char> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));

    xpf::Vector<char> vector2(xpf::Move(vector1));

    EXPECT_TRUE(vector1.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, vector1.Size());

    EXPECT_FALSE(vector2.IsEmpty());
    EXPECT_EQ(size_t{ 3 }, vector2.Size());

    EXPECT_EQ('a', vector2[0]);
    EXPECT_EQ('b', vector2[1]);
    EXPECT_EQ('C', vector2[2]);
}

/**
 * @brief       This tests the move assignment.
 */
TEST(TestVector, MoveAssignment)
{
    //
    // Move from a vector to another.
    //
    xpf::Vector<char> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));

    xpf::Vector<char> vector2;
    EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('1')));
    EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('2')));
    EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('3')));
    EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('4')));

    //
    // Self-Move scenario.
    //
    vector1 = xpf::Move(vector1);
    EXPECT_FALSE(vector1.IsEmpty());
    EXPECT_EQ(size_t{ 3 }, vector1.Size());

    EXPECT_EQ('a', vector1[0]);
    EXPECT_EQ('b', vector1[1]);
    EXPECT_EQ('C', vector1[2]);

    //
    // Legit move scenario.
    //
    vector1 = xpf::Move(vector2);

    EXPECT_FALSE(vector1.IsEmpty());
    EXPECT_EQ(size_t{ 4 }, vector1.Size());

    EXPECT_TRUE(vector2.IsEmpty());;
    EXPECT_EQ(size_t{ 0 }, vector2.Size());

    EXPECT_EQ('1', vector1[0]);
    EXPECT_EQ('2', vector1[1]);
    EXPECT_EQ('3', vector1[2]);
    EXPECT_EQ('4', vector1[3]);

    //
    // Now move empty string.
    //
    vector1 = xpf::Move(vector2);

    EXPECT_TRUE(vector1.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, vector1.Size());

    EXPECT_TRUE(vector2.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, vector2.Size());
}

/**
 * @brief       This tests the index operator.
 */
TEST(TestVector, IndexOperator)
{
    xpf::Vector<char> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));

    vector1[0] = L'X';
    vector1[1] = L'Y';
    vector1[2] = L'z';
    vector1[3] = L'7';

    const auto& constVector1 = vector1;
    EXPECT_EQ(constVector1[0], L'X');
    EXPECT_EQ(constVector1[1], L'Y');
    EXPECT_EQ(constVector1[2], L'z');
    EXPECT_EQ(constVector1[3], L'7');

    //
    // Now test the out of bounds access of the view.
    //
    EXPECT_DEATH(vector1[1000], ".*");
}

/**
 * @brief       This tests the emplace and clear.
 */
TEST(TestVector, EmplaceClear)
{
    xpf::Vector<char> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));

    EXPECT_EQ(vector1[0], L'a');
    EXPECT_EQ(vector1[1], L'b');
    EXPECT_EQ(vector1[2], L'C');
    EXPECT_EQ(vector1[3], L'D');

    vector1.Clear();
    EXPECT_TRUE(vector1.IsEmpty());
    EXPECT_EQ(size_t{ 0 }, vector1.Size());
}

/**
 * @brief       This tests the Erase.
 */
TEST(TestVector, Erase)
{
    xpf::Vector<char> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('E')));

    EXPECT_FALSE(NT_SUCCESS(vector1.Erase(100)));

    //
    // Erase the first element.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Erase(0)));

    EXPECT_EQ(vector1[0], L'b');
    EXPECT_EQ(vector1[1], L'C');
    EXPECT_EQ(vector1[2], L'D');
    EXPECT_EQ(vector1[3], L'E');

    //
    // Erase the last element.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Erase(3)));

    EXPECT_EQ(vector1[0], L'b');
    EXPECT_EQ(vector1[1], L'C');
    EXPECT_EQ(vector1[2], L'D');

    //
    // Erase the mid element.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Erase(1)));

    EXPECT_EQ(vector1[0], L'b');
    EXPECT_EQ(vector1[1], L'D');

    //
    // Erase the last element.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Erase(1)));

    EXPECT_EQ(vector1[0], L'b');

    //
    // Erase the last element.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Erase(0)));

    EXPECT_EQ(size_t{ 0 }, vector1.Size());
}


/**
 * @brief       This tests the Resize.
 */
TEST(TestVector, Resize)
{
    xpf::Vector<wchar_t> vector1;
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'a')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'b')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'C')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'D')));
    EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'E')));

    //
    // The capacity is not large enough to hold all elements.
    //
    EXPECT_FALSE(NT_SUCCESS(vector1.Resize(1)));

    //
    // Overflow during computation
    //
    EXPECT_FALSE(NT_SUCCESS(vector1.Resize(xpf::NumericLimits<size_t>::MaxValue())));

    //
    // A bigger capacity.
    //
    EXPECT_TRUE(NT_SUCCESS(vector1.Resize(100000)));

    EXPECT_EQ(vector1[0], L'a');
    EXPECT_EQ(vector1[1], L'b');
    EXPECT_EQ(vector1[2], L'C');
    EXPECT_EQ(vector1[3], L'D');
    EXPECT_EQ(vector1[4], L'E');
}
