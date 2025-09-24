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
