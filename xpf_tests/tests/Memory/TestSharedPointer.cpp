/**
 * @file        xpf_tests/tests/Memory/TestSharedPointer.cpp
 *
 * @brief       This contains tests for shared pointer.
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
 * @brief       This tests the default constructor and destructor of shared pointer.
 */
TEST(TestSharedPointer, DefaultConstructorDestructor)
{
    xpf::SharedPointer<int> ptr;
    EXPECT_TRUE(ptr.IsEmpty());

    auto ptrSize = sizeof(void*);           // reference counter
    EXPECT_TRUE(sizeof(ptr) == ptrSize);
}

/**
 * @brief       This tests the make shared method.
 */
TEST(TestSharedPointer, MakeShared)
{
    const auto ptr1 = xpf::MakeShared<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(50, (*ptr2));
}

/**
 * @brief       This tests the reset method.
 */
TEST(TestSharedPointer, Reset)
{
    auto ptr1 = xpf::MakeShared<int>(100);
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
TEST(TestSharedPointer, MoveConstructor)
{
    auto ptr1 = xpf::MakeShared<int>(100);
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
TEST(TestSharedPointer, MoveAssignment)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
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
 * @brief       This tests the copy constructor.
 */
TEST(TestSharedPointer, CopyConstructor)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2{ ptr1 };
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(100, (*ptr2));

    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));


    xpf::SharedPointer<int> emptyPtr;
    EXPECT_TRUE(emptyPtr.IsEmpty());

    xpf::SharedPointer<int> copiedEmptyPtr(emptyPtr);
    EXPECT_TRUE(copiedEmptyPtr.IsEmpty());
}

/**
 * @brief       This tests the copy assignment.
 */
TEST(TestSharedPointer, CopyAssignment)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(50, (*ptr2));

    ptr1 = ptr1;
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    ptr1 = ptr2;
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(50, (*ptr1));

    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(50, (*ptr2));

    xpf::SharedPointer<int> emptyPtr;
    EXPECT_TRUE(emptyPtr.IsEmpty());

    xpf::SharedPointer<int> copiedEmptyPtr = xpf::MakeShared<int>(50);
    EXPECT_FALSE(copiedEmptyPtr.IsEmpty());
    EXPECT_EQ(50, (*copiedEmptyPtr));

    copiedEmptyPtr = emptyPtr;
    EXPECT_TRUE(copiedEmptyPtr.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with same types.
 */
TEST(TestSharedPointer, DynamicSharedPointerCastSameType)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));

    auto ptr2 = xpf::DynamicSharedPointerCast<int>(ptr1);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_EQ(100, (*ptr2));

    EXPECT_FALSE(ptr1.IsEmpty());
    EXPECT_EQ(100, (*ptr1));
}

/**
 * @brief       This tests the dynamic cast with different types.
 */
TEST(TestSharedPointer, DynamicSharedPointerCastDifferentType)
{
    auto ptr1 = xpf::MakeShared<xpf::mocks::Derived>(100);
    EXPECT_FALSE(ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicSharedPointerCast<xpf::mocks::Base>(ptr1);
    EXPECT_FALSE(ptr2.IsEmpty());
    EXPECT_FALSE(ptr1.IsEmpty());

    auto ptr3 = xpf::DynamicSharedPointerCast<xpf::mocks::Derived>(ptr2);
    EXPECT_FALSE(ptr3.IsEmpty());
    EXPECT_FALSE(ptr2.IsEmpty());
}
