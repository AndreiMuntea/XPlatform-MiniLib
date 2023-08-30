/**
 * @file        xpf_tests/tests/Memory/TestCompressedPair.cpp
 *
 * @brief       This contains tests for CompressedPair.
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
 * @brief       This is a mock empty struct without any members.
 */
struct MockCompressedPairEmptyClass
{
};

/**
 * @brief       This is a mock non-empty struct without any members.
 */
struct MockCompressedPairNonEmptyClass
{
    int SomeValue = 0;
};

/**
 * @brief       This is a mock empty struct without any members
 *              which is also final (can't be inherited from!)
 */
struct MockCompressedPairEmptyFinalClass final
{
};


/**
 * @brief       This tests that Empty Base Class Optimization can happen.
 *
 */
TEST(TestCompressedPair, EBCO)
{
    //
    // Type1 empty and non final - type2 - not empty
    //
    xpf::CompressedPair<MockCompressedPairEmptyClass,
                        MockCompressedPairNonEmptyClass> pair1;
    EXPECT_TRUE(sizeof(pair1) == sizeof(MockCompressedPairNonEmptyClass));

    //
    // Type1 empty and non final - type2 - empty and final;
    //
    xpf::CompressedPair<MockCompressedPairEmptyClass,
                        MockCompressedPairEmptyFinalClass> pair2;
    EXPECT_TRUE(sizeof(pair2) == sizeof(MockCompressedPairEmptyFinalClass));
}

/**
 * @brief       This tests that we still function correctly wen EBCO can't happen.
 *
 */
TEST(TestCompressedPair, NonEBCO)
{
    //
    // Type1 non empty and non final - type2 - non empty and non final
    //
    xpf::CompressedPair<MockCompressedPairNonEmptyClass,
                        MockCompressedPairNonEmptyClass> pair1;
    EXPECT_TRUE(sizeof(pair1) == sizeof(MockCompressedPairNonEmptyClass) +
                                 sizeof(MockCompressedPairNonEmptyClass));

    //
    // Type1 empty but final - type2 - non empty and non final
    //
    xpf::CompressedPair<MockCompressedPairEmptyFinalClass,
                        MockCompressedPairEmptyClass> pair2;
    EXPECT_TRUE(sizeof(pair2) == sizeof(MockCompressedPairEmptyFinalClass) +
                                 sizeof(MockCompressedPairEmptyClass));
}
