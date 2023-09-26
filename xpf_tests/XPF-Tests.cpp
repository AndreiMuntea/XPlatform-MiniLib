/**
 * @file        xpf_tests/XPF-Tests.cpp
 *
 * @brief       This file contains the entry point for xpf tests.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_tests/XPF-TestIncludes.hpp"


/**
 *
 * @brief Main entry point for XPF-Library Tests.
 *
 * @param[in] Argc - Arguments count passed to command line.
 *
 * @param[in] Argv - The array of command line arguments.
 *
 * @return The result of RUN_ALL_TESTS from xpf_tests function.
 */
int
XPF_PLATFORM_CONVENTION
main(
    _In_ int Argc,
    _In_ char* Argv[]
)
{
    //
    // For now these are not used anywhere.
    //
    XPF_UNREFERENCED_PARAMETER(Argc);
    XPF_UNREFERENCED_PARAMETER(Argv);

    return xpf_test::RunAllTests();
}


/**
 *
 * @brief Placement new declaration - required for cpp support.
 *
 * @param[in] BlockSize - Unused.
 * 
 * @param[in,out] Location - Unused. Will be returned.
 *
 * @return Location
 *
 * @note It is the caller responsibility to provide an implementation
 *       for this API. If "new" header is available, it can be included,
 *       otherwise, a simple implementation can be provided.
 */
void*
XPF_PLATFORM_CONVENTION
operator new(
    size_t BlockSize,
    void* Location
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(0 != BlockSize);
    XPF_DEATH_ON_FAILURE(nullptr != Location);
    return Location;
}

/**
 *
 * @brief Placement delete declaration  - required for cpp support.
 *
 * @param[in] Pointer - Unused.
 *
 * @param[in] Location - Unused.
 *
 * @return void.
 *
 * @note It is the caller responsibility to provide an implementation
 *       for this API. If "new" header is available, it can be included,
 *       otherwise, a simple implementation can be provided.
 */
void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    void* Location
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Pointer);
    XPF_DEATH_ON_FAILURE(nullptr != Location);
}

/**
 *
 * @brief Placement delete declaration  - required for cpp support.
 *
 * @param[in] Pointer - Unused.
 *
 * @param[in] Size - Unused.
 *
 * @return void.
 *
 * @note It is the caller responsibility to provide an implementation
 *       for this API. If "new" header is available, it can be included,
 *       otherwise, a simple implementation can be provided.
 */
void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    size_t Size
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Pointer);
    XPF_DEATH_ON_FAILURE(0 != Size);
}
