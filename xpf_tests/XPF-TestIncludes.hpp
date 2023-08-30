/**
 * @file        xpf_tests/XPF-TestIncludes.hpp
 *
 * @brief       This file contains the includes for test project.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once

/**
 *
 * @brief We include the gtest library first.
 *        This will also contain the new/delete implementation
 *        by including the platform header (on win MSVC - vcruntime_new.h)
 */
#include <gtest/gtest.h>

/**
 *
 * @brief And then the xpf-library.
 *        Must come after gtest dependecy.
 */
#include <xpf_lib/xpf.hpp>
