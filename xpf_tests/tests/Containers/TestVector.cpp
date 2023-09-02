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
XPF_TEST_SCENARIO(TestVector, DefaultConstructorDestructor)
{
    xpf::Vector<uint64_t> vector;
    XPF_TEST_EXPECT_TRUE(vector.Size() == size_t{ 0 });
    XPF_TEST_EXPECT_TRUE(vector.IsEmpty());

    const size_t expectedVectorSize = sizeof(void*) + 2 * sizeof(size_t);
    static_assert(expectedVectorSize == static_cast<size_t>(sizeof(vector)),
                  "Compile time size check!");
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestVector, MoveConstructor)
{
    //
    // Move from a vector to another.
    //
    xpf::Vector<char> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));

    xpf::Vector<char> vector2(xpf::Move(vector1));

    XPF_TEST_EXPECT_TRUE(vector1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector1.Size());

    XPF_TEST_EXPECT_TRUE(!vector2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == vector2.Size());

    XPF_TEST_EXPECT_TRUE('a' == vector2[0]);
    XPF_TEST_EXPECT_TRUE('b' == vector2[1]);
    XPF_TEST_EXPECT_TRUE('C' == vector2[2]);
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestVector, MoveAssignment)
{
    //
    // Move from a vector to another.
    //
    xpf::Vector<char> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));

    xpf::Vector<char> vector2;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('1')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('2')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('3')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector2.Emplace('4')));

    //
    // Self-Move scenario.
    //
    vector1 = xpf::Move(vector1);
    XPF_TEST_EXPECT_TRUE(!vector1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 3 } == vector1.Size());

    XPF_TEST_EXPECT_TRUE('a' == vector1[0]);
    XPF_TEST_EXPECT_TRUE('b' == vector1[1]);
    XPF_TEST_EXPECT_TRUE('C' == vector1[2]);

    //
    // Legit move scenario.
    //
    vector1 = xpf::Move(vector2);

    XPF_TEST_EXPECT_TRUE(!vector1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == vector1.Size());

    XPF_TEST_EXPECT_TRUE(vector2.IsEmpty());;
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector2.Size());

    XPF_TEST_EXPECT_TRUE('1' == vector1[0]);
    XPF_TEST_EXPECT_TRUE('2' == vector1[1]);
    XPF_TEST_EXPECT_TRUE('3' == vector1[2]);
    XPF_TEST_EXPECT_TRUE('4' == vector1[3]);

    //
    // Now move empty string.
    //
    vector1 = xpf::Move(vector2);

    XPF_TEST_EXPECT_TRUE(vector1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector1.Size());

    XPF_TEST_EXPECT_TRUE(vector2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector2.Size());
}

/**
 * @brief       This tests the index operator.
 */
XPF_TEST_SCENARIO(TestVector, IndexOperator)
{
    xpf::Vector<char> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));

    vector1[0] = L'X';
    vector1[1] = L'Y';
    vector1[2] = L'z';
    vector1[3] = L'7';

    const auto& constVector1 = vector1;
    XPF_TEST_EXPECT_TRUE(constVector1[0] == L'X');
    XPF_TEST_EXPECT_TRUE(constVector1[1] == L'Y');
    XPF_TEST_EXPECT_TRUE(constVector1[2] == L'z');
    XPF_TEST_EXPECT_TRUE(constVector1[3] == L'7');

    //
    // Now test the out of bounds access of the view.
    //
    XPF_TEST_EXPECT_DEATH(vector1[1000] == L'X');
}

/**
 * @brief       This tests the emplace and clear.
 */
XPF_TEST_SCENARIO(TestVector, EmplaceClear)
{
    xpf::Vector<char> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'a');
    XPF_TEST_EXPECT_TRUE(vector1[1] == L'b');
    XPF_TEST_EXPECT_TRUE(vector1[2] == L'C');
    XPF_TEST_EXPECT_TRUE(vector1[3] == L'D');

    vector1.Clear();
    XPF_TEST_EXPECT_TRUE(vector1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector1.Size());
}

/**
 * @brief       This tests the Erase.
 */
XPF_TEST_SCENARIO(TestVector, Erase)
{
    xpf::Vector<char> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('C')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('D')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace('E')));

    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(vector1.Erase(100)));

    //
    // Erase the first element.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Erase(0)));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'b');
    XPF_TEST_EXPECT_TRUE(vector1[1] == L'C');
    XPF_TEST_EXPECT_TRUE(vector1[2] == L'D');
    XPF_TEST_EXPECT_TRUE(vector1[3] == L'E');

    //
    // Erase the last element.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Erase(3)));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'b');
    XPF_TEST_EXPECT_TRUE(vector1[1] == L'C');
    XPF_TEST_EXPECT_TRUE(vector1[2] == L'D');

    //
    // Erase the mid element.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Erase(1)));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'b');
    XPF_TEST_EXPECT_TRUE(vector1[1] == L'D');

    //
    // Erase the last element.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Erase(1)));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'b');

    //
    // Erase the last element.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Erase(0)));
    XPF_TEST_EXPECT_TRUE(size_t{ 0 } == vector1.Size());
}


/**
 * @brief       This tests the Resize.
 */
XPF_TEST_SCENARIO(TestVector, Resize)
{
    xpf::Vector<wchar_t> vector1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'a')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'b')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'C')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'D')));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Emplace(L'E')));

    //
    // The capacity is not large enough to hold all elements.
    //
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(vector1.Resize(1)));

    //
    // Overflow during computation
    //
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(vector1.Resize(xpf::NumericLimits<size_t>::MaxValue())));

    //
    // A bigger capacity.
    //
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(vector1.Resize(100000)));

    XPF_TEST_EXPECT_TRUE(vector1[0] == L'a');
    XPF_TEST_EXPECT_TRUE(vector1[1] == L'b');
    XPF_TEST_EXPECT_TRUE(vector1[2] == L'C');
    XPF_TEST_EXPECT_TRUE(vector1[3] == L'D');
    XPF_TEST_EXPECT_TRUE(vector1[4] == L'E');
}
