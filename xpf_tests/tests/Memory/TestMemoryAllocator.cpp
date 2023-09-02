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
XPF_TEST_SCENARIO(TestMemoryAllocator, TriviallyDestructibleCharacter)
{
    XPF_TEST_EXPECT_TRUE(xpf::IsTriviallyDestructible<char>());

    char* character = reinterpret_cast<char*>(xpf::MemoryAllocator::AllocateMemory(sizeof(char)));
    XPF_TEST_EXPECT_TRUE(nullptr != character);

    xpf::MemoryAllocator::Construct(character, 'X');
    XPF_TEST_EXPECT_TRUE(*character == 'X');

    xpf::MemoryAllocator::Destruct(character);

    xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&character));
    XPF_TEST_EXPECT_TRUE(nullptr == character);
}

/**
 * @brief       This tests the construction and destruction of a non trivial class
 *              using xpf::MemoryAllocator class.
 */
XPF_TEST_SCENARIO(TestMemoryAllocator, NonTriviallyDestructibleObject)
{
    XPF_TEST_EXPECT_TRUE(!xpf::IsTriviallyDestructible<xpf::mocks::Base>());

    auto object = xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::mocks::Base));
    XPF_TEST_EXPECT_TRUE(nullptr != object);

    auto baseObject = reinterpret_cast<xpf::mocks::Base*>(object);
    xpf::MemoryAllocator::Construct(baseObject, 100);
    xpf::MemoryAllocator::Destruct(baseObject);

    xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&baseObject));
    XPF_TEST_EXPECT_TRUE(nullptr == baseObject);
}
