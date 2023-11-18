/**
  * @file        xpf_tests/tests/Memory/TestOptional.cpp
  *
  * @brief       This contains tests for Optional.
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
 * @brief       This tests the default constructor and destructor of an optional.
 */
XPF_TEST_SCENARIO(TestOptional, DefaultConstructorDestructor)
{
    {
        const xpf::Optional<uint64_t> optional;
        XPF_TEST_EXPECT_TRUE(!optional.HasValue());
    }

    {
        const xpf::Optional<uint64_t> optional;
        XPF_TEST_EXPECT_DEATH(*optional == 0);
    }

    {
        xpf::Optional<uint64_t> optional;
        XPF_TEST_EXPECT_DEATH(*optional == 0);
    }
}

/**
 * @brief       This tests the copy constructor.
 */
XPF_TEST_SCENARIO(TestOptional, CopyConstructor)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    //
    // Copy optional1 to optional2.
    //
    xpf::Optional<uint64_t> optional2{ optional1 };
    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional2);

    //
    // Reset optional2 - optional1 shouldn't be affected.
    //
    optional2.Reset();
    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    XPF_TEST_EXPECT_TRUE(!optional2.HasValue());

    //
    // Empty optional copy constructor.
    //
    xpf::Optional<uint64_t> optional3{ optional2 };
    XPF_TEST_EXPECT_TRUE(!optional3.HasValue());
    XPF_TEST_EXPECT_TRUE(!optional2.HasValue());
}

/**
 * @brief       This tests the copy assignment.
 */
XPF_TEST_SCENARIO(TestOptional, CopyAssingment)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    xpf::Optional<uint64_t> optional2;
    optional2.Emplace(200);

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    //
    // CLANG will warn about self assignment.
    // But this is intentional here. So we manually disable the warning
    //
    #if defined XPF_COMPILER_CLANG
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wself-assign-overloaded"
    #endif  // XPF_COMPILER_CLANG

        XPF_TEST_EXPECT_TRUE(optional2.HasValue());
        XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    #if defined XPF_COMPILER_CLANG
        #pragma clang diagnostic pop
    #endif  // XPF_COMPILER_CLANG

    //
    // Assign optional 1 to 2 - should be overwritten.
    //
    optional1 = optional2;

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional1);

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    //
    // Reset optional 1 - optional 2 should be intact.
    //
    optional1.Reset();

    XPF_TEST_EXPECT_TRUE(!optional1.HasValue());

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    //
    // Assign empty optional
    //
    optional2 = optional1;
    XPF_TEST_EXPECT_TRUE(!optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(!optional2.HasValue());
}

/**
 * @brief       This tests the move constructor.
 */
XPF_TEST_SCENARIO(TestOptional, MoveConstructor)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    //
    // Move optional1 to optional2.
    //
    xpf::Optional<uint64_t> optional2{ xpf::Move(optional1) };
    XPF_TEST_EXPECT_TRUE(!optional1.HasValue());

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional2);

    //
    // Empty optional move constructor.
    //
    xpf::Optional<uint64_t> optional3{ xpf::Move(optional1) };
    XPF_TEST_EXPECT_TRUE(!optional3.HasValue());
    XPF_TEST_EXPECT_TRUE(!optional1.HasValue());
}

/**
 * @brief       This tests the move assignment.
 */
XPF_TEST_SCENARIO(TestOptional, MoveAssignment)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 100 } == *optional1);

    xpf::Optional<uint64_t> optional2;
    optional2.Emplace(200);

    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    //
    // Self Move case.
    //
    optional2 = xpf::Move(optional2);
    XPF_TEST_EXPECT_TRUE(optional2.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional2);

    //
    // Assign optional 1 to 2 - should be overwritten.
    //
    optional1 = xpf::Move(optional2);

    XPF_TEST_EXPECT_TRUE(optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(uint64_t{ 200 } == *optional1);

    XPF_TEST_EXPECT_TRUE(!optional2.HasValue());

    //
    // Assign empty optional
    //
    optional1 = xpf::Move(optional2);
    XPF_TEST_EXPECT_TRUE(!optional1.HasValue());
    XPF_TEST_EXPECT_TRUE(!optional2.HasValue());
}
