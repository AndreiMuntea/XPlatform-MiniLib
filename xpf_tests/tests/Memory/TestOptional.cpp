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
TEST(TestOptional, DefaultConstructorDestructor)
{
    {
        const xpf::Optional<uint64_t> optional;
        EXPECT_FALSE(optional.HasValue());
    }

    {
        const xpf::Optional<uint64_t> optional;
        EXPECT_DEATH(*optional, ".*");
    }

    {
        xpf::Optional<uint64_t> optional;
        EXPECT_DEATH(*optional, ".*");
    }
}

/**
 * @brief       This tests the copy constructor.
 */
TEST(TestOptional, CopyConstructor)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    //
    // Copy optional1 to optional2.
    //
    xpf::Optional<uint64_t> optional2{ optional1 };
    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional2);

    //
    // Reset optional2 - optional1 shouldn't be affected.
    //
    optional2.Reset();
    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    EXPECT_FALSE(optional2.HasValue());

    //
    // Empty optional copy constructor.
    //
    xpf::Optional<uint64_t> optional3{ optional2 };
    EXPECT_FALSE(optional3.HasValue());
    EXPECT_FALSE(optional2.HasValue());
}

/**
 * @brief       This tests the copy assignment.
 */
TEST(TestOptional, CopyAssingment)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    xpf::Optional<uint64_t> optional2;
    optional2.Emplace(200);

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Self Assign case.
    //
    optional2 = optional2;
    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Assign optional 1 to 2 - should be overwritten.
    //
    optional1 = optional2;

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional1);

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Reset optional 1 - optional 2 should be intact.
    //
    optional1.Reset();

    EXPECT_FALSE(optional1.HasValue());

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Assign empty optional
    //
    optional2 = optional1;
    EXPECT_FALSE(optional1.HasValue());
    EXPECT_FALSE(optional2.HasValue());
}

/**
 * @brief       This tests the move constructor.
 */
TEST(TestOptional, MoveConstructor)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    //
    // Move optional1 to optional2.
    //
    xpf::Optional<uint64_t> optional2{ xpf::Move(optional1) };
    EXPECT_FALSE(optional1.HasValue());

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional2);

    //
    // Empty optional move constructor.
    //
    xpf::Optional<uint64_t> optional3{ xpf::Move(optional1) };
    EXPECT_FALSE(optional3.HasValue());
    EXPECT_FALSE(optional1.HasValue());
}

/**
 * @brief       This tests the move assignment.
 */
TEST(TestOptional, MoveAssignment)
{
    xpf::Optional<uint64_t> optional1;
    optional1.Emplace(100);

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 100 }, *optional1);

    xpf::Optional<uint64_t> optional2;
    optional2.Emplace(200);

    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Self Move case.
    //
    optional2 = xpf::Move(optional2);
    EXPECT_TRUE(optional2.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional2);

    //
    // Assign optional 1 to 2 - should be overwritten.
    //
    optional1 = xpf::Move(optional2);

    EXPECT_TRUE(optional1.HasValue());
    EXPECT_EQ(uint64_t{ 200 }, *optional1);

    EXPECT_FALSE(optional2.HasValue());

    //
    // Assign empty optional
    //
    optional1 = xpf::Move(optional2);
    EXPECT_FALSE(optional1.HasValue());
    EXPECT_FALSE(optional2.HasValue());
}
