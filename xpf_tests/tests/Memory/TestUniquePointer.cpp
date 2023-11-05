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
XPF_TEST_SCENARIO(TestUniquePointer, DefaultConstructorDestructor)
{
    xpf::UniquePointer<int> ptr;
    XPF_TEST_EXPECT_TRUE(ptr.IsEmpty());

    auto ptrSize = sizeof(void*);           // compressed_pair(allocator, buffer)
    XPF_TEST_EXPECT_TRUE(sizeof(ptr) == ptrSize);
}

/**
 * @brief       This tests the make unique method.
 */
XPF_TEST_SCENARIO(TestUniquePointer, MakeUniqueMethod)
{
    const auto ptr1 = xpf::MakeUnique<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::MakeUnique<int>(50);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(50 == (*ptr2));
}

/**
 * @brief       This tests the reset method.
 */
XPF_TEST_SCENARIO(TestUniquePointer, ResetMethod)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
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
XPF_TEST_SCENARIO(TestUniquePointer, MoveConstructor)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
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
XPF_TEST_SCENARIO(TestUniquePointer, MoveAssignment)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::MakeUnique<int>(50);
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
 * @brief       This tests the dynamic cast with same types.
 */
XPF_TEST_SCENARIO(TestUniquePointer, DynamicUniquePointerCastSameType)
{
    auto ptr1 = xpf::MakeUnique<int>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr1));

    auto ptr2 = xpf::DynamicUniquePointerCast<int>(ptr1);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(100 == (*ptr2));
    XPF_TEST_EXPECT_TRUE(ptr1.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with different types.
 */
XPF_TEST_SCENARIO(TestUniquePointer, DynamicUniquePointerCastDifferentType)
{
    auto ptr1 = xpf::MakeUnique<xpf::mocks::Derived>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicUniquePointerCast<xpf::mocks::Base>(ptr1);
    XPF_TEST_EXPECT_TRUE(!ptr2.IsEmpty());

    auto ptr3 = xpf::DynamicUniquePointerCast<xpf::mocks::Derived>(ptr2);
    XPF_TEST_EXPECT_TRUE(!ptr3.IsEmpty());
    XPF_TEST_EXPECT_TRUE(ptr2.IsEmpty());
}

/**
 * @brief       This tests the dynamic cast with different and virtual inheritance.
 *              This will make the address a bit different.
 */
XPF_TEST_SCENARIO(TestUniquePointer, DynamicUniquePointerCastVirtualInheritance)
{
    xpf::mocks::VirtualInheritanceDerived object(100);
    xpf::mocks::Base* objectBase = static_cast<xpf::mocks::Base*>(xpf::AddressOf(object));

    XPF_TEST_EXPECT_TRUE(xpf::AlgoPointerToValue(xpf::AddressOf(object)) != xpf::AlgoPointerToValue(objectBase))

    auto ptr1 = xpf::MakeUnique<xpf::mocks::VirtualInheritanceDerived>(100);
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());

    auto ptr2 = xpf::DynamicUniquePointerCast<xpf::mocks::Base>(ptr1);
    XPF_TEST_EXPECT_TRUE(ptr2.IsEmpty());
    XPF_TEST_EXPECT_TRUE(!ptr1.IsEmpty());
}
