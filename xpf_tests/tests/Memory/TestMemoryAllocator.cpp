/**
 * @file        xpf_tests/tests/Memory/TestMemoryAllocator.cpp
 *
 * @brief       This contains tests for memory allocator object.
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
 * @brief       This tests the construction and destruction of a character
 *              using xpf::MemoryAllocator class.
 */
TEST(TestMemoryAllocator, TriviallyDestructibleCharacter)
{
    EXPECT_TRUE(xpf::IsTriviallyDestructible<char>());

    char* character = reinterpret_cast<char*>(xpf::MemoryAllocator::AllocateMemory(sizeof(char)));
    EXPECT_TRUE(nullptr != character);
    _Analysis_assume_(nullptr != character);

    xpf::MemoryAllocator::Construct(character, 'X');
    EXPECT_EQ(*character, 'X');

    xpf::MemoryAllocator::Destruct(character);

    xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&character));
    EXPECT_TRUE(nullptr == character);
}

/**
 * @brief       This tests the construction and destruction of a non trivial class
 *              using xpf::MemoryAllocator class.
 */
TEST(TestMemoryAllocator, NonTriviallyDestructibleObject)
{
    EXPECT_FALSE(xpf::IsTriviallyDestructible<xpf::mocks::Base>());

    auto object = xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::mocks::Base));
    EXPECT_TRUE(nullptr != object);
    _Analysis_assume_(nullptr != object);

    auto baseObject = reinterpret_cast<xpf::mocks::Base*>(object);
    xpf::MemoryAllocator::Construct(baseObject, 100);
    xpf::MemoryAllocator::Destruct(baseObject);

    xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&baseObject));
    EXPECT_TRUE(nullptr == baseObject);
}
