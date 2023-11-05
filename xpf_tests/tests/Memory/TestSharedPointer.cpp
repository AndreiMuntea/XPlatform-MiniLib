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
XPF_TEST_SCENARIO(TestSharedPointer, DefaultConstructorDestructor)
{
    xpf::SharedPointer<int> ptr;
    XPF_TEST_EXPECT_TRUE(ptr.IsEmpty());

    auto ptrSize = sizeof(void*);           // reference counter
    XPF_TEST_EXPECT_TRUE(sizeof(ptr) == ptrSize);
}

/**
 * @brief       This tests the make shared method.
 */
XPF_TEST_SCENARIO(TestSharedPointer, MakeShared)
{
    const auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr2));
}

/**
 * @brief       This tests the reset method.
 */
XPF_TEST_SCENARIO(TestSharedPointer, Reset)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    ptr1.Reset();
    XPF_TEST_EXPECT_TRUE(ptr1.IsEmpty());

    ptr1.Reset();
    XPF_TEST_EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestSharedPointer, MoveConstructor)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    const auto ptr2{ xpf::Move(ptr1) };
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr2));

    XPF_TEST_EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestSharedPointer, MoveAssignment)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr2));

    ptr1 = xpf::Move(ptr1);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    ptr1 = xpf::Move(ptr2);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr1));

    XPF_TEST_EXPECT_TRUE(ptr2.IsEmpty());
}

/**
 * @brief       This tests the copy constructor.
 */
XPF_TEST_SCENARIO(TestSharedPointer, CopyConstructor)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2{ ptr1 };
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr2));

    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));


    xpf::SharedPointer<int> emptyPtr;
    XPF_TEST_EXPECT_TRUE(emptyPtr.IsEmpty());

    xpf::SharedPointer<int> copiedEmptyPtr(emptyPtr);
    XPF_TEST_EXPECT_TRUE(copiedEmptyPtr.IsEmpty());
}

/**
 * @brief       This tests the copy assignment.
 */
XPF_TEST_SCENARIO(TestSharedPointer, CopyAssignment)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::MakeShared<int>(50);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr2));

    ptr1 = ptr1;
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    ptr1 = ptr2;
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr1));

    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr2));

    xpf::SharedPointer<int> emptyPtr;
    XPF_TEST_EXPECT_TRUE(emptyPtr.IsEmpty());

    xpf::SharedPointer<int> copiedEmptyPtr = xpf::MakeShared<int>(50);
    XPF_TEST_EXPECT_TRUE(!copiedEmptyPtr.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*copiedEmptyPtr));

    copiedEmptyPtr = emptyPtr;
    XPF_TEST_EXPECT_TRUE(copiedEmptyPtr.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with same types.
 */
XPF_TEST_SCENARIO(TestSharedPointer, DynamicSharedPointerCastSameType)
{
    auto ptr1 = xpf::MakeShared<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::DynamicSharedPointerCast<int>(ptr1);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr2));

    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));
}

/**
 * @brief       This tests the dynamic cast with different types.
 */
XPF_TEST_SCENARIO(TestSharedPointer, DynamicSharedPointerCastDifferentType)
{
    auto ptr1 = xpf::MakeShared<xpf::mocks::Derived>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicSharedPointerCast<xpf::mocks::Base>(ptr1);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());

    auto ptr3 = xpf::DynamicSharedPointerCast<xpf::mocks::Derived>(ptr2);
    XPF_TEST_EXPECT_TRUE(!ptr3.IsEmpty());
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with different and virtual inheritance.
 *              This will make the address a bit different.
 */
XPF_TEST_SCENARIO(TestSharedPointer, DynamicSharedPointerCastVirtualInheritance)
{
    xpf::mocks::VirtualInheritanceDerived object(100);
    xpf::mocks::Base* objectBase = static_cast<xpf::mocks::Base*>(xpf::AddressOf(object));

    XPF_TEST_EXPECT_TRUE(xpf::AlgoPointerToValue(xpf::AddressOf(object)) != xpf::AlgoPointerToValue(objectBase))

    auto ptr1 = xpf::MakeShared<xpf::mocks::VirtualInheritanceDerived>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicSharedPointerCast<xpf::mocks::Base>(ptr1);
    XPF_TEST_EXPECT_TRUE(ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
}
