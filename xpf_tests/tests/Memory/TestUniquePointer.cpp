/**
 * @file        xpf_tests/tests/Memory/TestUniquePointer.cpp
 *
 * @brief       This contains tests for unique pointer.
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
 * @brief       This tests the default constructor and destructor of unique pointer.
 */
TEST(TestUniquePointer, DefaultConstructorDestructor)
{
    xpf::UniquePointer<int> ptr;
    EXPECT_TRUE(ptr.IsEmpty());

    auto ptrSize = sizeof(void*);           // compressed_pair(allocator, buffer)
    EXPECT_EQ(sizeof(ptr), ptrSize);
}

/**
 * @brief       This tests the make unique method.
 */
TEST(TestUniquePointer, MakeUniqueMethod)
{
    const auto ptr1 = xpf::MakeUnique<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::MakeUnique<int>(50);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(50, (*ptr2));
}

/**
 * @brief       This tests the reset method.
 */
TEST(TestUniquePointer, ResetMethod)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    ptr1.Reset();
    EXPECT_TRUE(ptr1.IsEmpty());

    ptr1.Reset();
    EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the move constructor.
 */
TEST(TestUniquePointer, MoveConstructor)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    const auto ptr2{ xpf::Move(ptr1) };
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(100, (*ptr2));

    EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the move assignment.
 */
TEST(TestUniquePointer, MoveAssignment)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::MakeUnique<int>(50);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(50, (*ptr2));

    ptr1 = xpf::Move(ptr1);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    ptr1 = xpf::Move(ptr2);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(50, (*ptr1));

    EXPECT_TRUE(ptr2.IsEmpty());
}


/**
 * @brief       This tests the dynamic cast with same types.
 */
TEST(TestUniquePointer, DynamicUniquePointerCastSameType)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::DynamicUniquePointerCast<int>(ptr1);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(100, (*ptr2));
    EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with different types.
 */
TEST(TestUniquePointer, DynamicUniquePointerCastDifferentType)
{
    auto ptr1 = xpf::MakeUnique<xpf::mocks::Derived>(100);
    EXPECT_FALSE(ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicUniquePointerCast<xpf::mocks::Base>(ptr1);
    EXPECT_FALSE(ptr2.IsEmpty());

    auto ptr3 = xpf::DynamicUniquePointerCast<xpf::mocks::Derived>(ptr2);
    EXPECT_FALSE(ptr3.IsEmpty());
    EXPECT_TRUE(ptr2.IsEmpty());
}
