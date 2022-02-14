// X-Platform.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#if defined(_MSC_VER)
    // For linux we can use valgrind for memory leaks.
    // For windows we use crt
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

#include "../XPF/XPlatform/XPlatformIncludes.hpp"

#include <iostream>
#include <limits>

#if defined(_MSC_VER)
    // There are some warnings in gtest that won't let the compilation succeed with W4 on MSVC compiler.
    #pragma warning(push)
    #pragma warning(disable:4389)
#endif

#include <gtest/gtest.h>

#if defined(_MSC_VER)
    // Restore warngins
    #pragma warning(pop)
#endif

// Helper structs and test definitions
#include "test_headers/TestDummyStruct.hpp"
#include "test_headers/TestMemoryLeak.hpp"