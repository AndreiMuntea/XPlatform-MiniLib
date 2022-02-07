/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_CORE_HPP__
#define __XPLATFORM_CORE_HPP__

#if defined(_MSC_VER)
    //
    // MSVC-Specific definitions
    //
    #include <intrin.h>
    #include <sal.h>

    #ifdef _KERNEL_MODE
        //
        // Specific Windows Kernel-Mode includes -- when compiling with /kernel switch.
        //
        #include <fltKernel.h>
        #include <ntintsafe.h>
        #include <ntstrsafe.h>
    #else
        //
        // Specific Windows User-Mode includes -- when compiling without /kernel switch.
        //
        #define NOMINMAX
            #include <Windows.h>
            #include <winternl.h>
            #include <intsafe.h>
            #include <strsafe.h>
        #undef NOMINMAX
    #endif


    //
    // Platform-Specific instruction for assertion.
    // Can translate to __int2c() or __break(ASSERT_BREAKPOINT) or __emit(0xdefc).
    // https://docs.microsoft.com/en-us/previous-versions/jj635749(v=vs.85)
    //
    #ifdef NDEBUG
        #define XPLATFORM_ASSERT(expression)        ((void)0)
    #else
        #define XPLATFORM_ASSERT(expression)        { if (!(expression)) { DbgRaiseAssertionFailure(); } }
    #endif

    //
    // Definition for unreferenced parameter
    //
    #define XPLATFORM_UNREFERENCED_PARAMETER(P)     UNREFERENCED_PARAMETER(P)

    //
    // Definition for default allocation alignment
    //
    #define XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT   MEMORY_ALLOCATION_ALIGNMENT

    //
    // Definition for default cache alignment
    //
    #define XPLATFORM_CACHE_ALIGNMENT               SYSTEM_CACHE_ALIGNMENT_SIZE


    //
    // Data types definitions for cross platform usage
    //
    using xp_int8_t  = signed char;
    using xp_int16_t = short;
    using xp_int32_t = int;
    using xp_int64_t = long long;

    using xp_uint8_t  = unsigned char;
    using xp_uint16_t = unsigned short;
    using xp_uint32_t = unsigned int;
    using xp_uint64_t = unsigned long long;

    using xp_char8_t  = char;
    using xp_char16_t = char16_t;       // Interchangeable with wchar_t on Windows
    using xp_char32_t = char32_t;


    ///
    /// Placement new and placement delete definition. Not always required. 
    /// When needed, add XPLATFORM_PLACEMENT_NEW_DEFINITION preprocessor definition.
    ///
    #ifdef XPLATFORM_PLACEMENT_NEW_DEFINITION
        inline void* __cdecl operator new (size_t, void* Location) noexcept
        {
            return Location;
        }
        inline void __cdecl operator delete(void*, void*) noexcept
        {
            return;
        }
    #endif // XPLATFORM_PLACEMENT_NEW_DEFINITION

#else
    #error Unsuported Compiler
#endif


///
/// Safety asserts to catch possible errors during compile time
///
static_assert(sizeof(xp_int8_t)  == 1, "xp_int8_t  should have 1 byte(s)");
static_assert(sizeof(xp_int16_t) == 2, "xp_int16_t should have 2 byte(s)");
static_assert(sizeof(xp_int32_t) == 4, "xp_int32_t should have 4 byte(s)");
static_assert(sizeof(xp_int64_t) == 8, "xp_int64_t should have 8 byte(s)");

static_assert(sizeof(xp_uint8_t)  == 1, "xp_uint8_t  should have 1 byte(s)");
static_assert(sizeof(xp_uint16_t) == 2, "xp_uint16_t should have 2 byte(s)");
static_assert(sizeof(xp_uint32_t) == 4, "xp_uint32_t should have 4 byte(s)");
static_assert(sizeof(xp_uint64_t) == 8, "xp_uint64_t should have 8 byte(s)");

static_assert(sizeof(xp_char8_t)  == 1, "xp_char8_t  should have 1 byte(s)");
static_assert(sizeof(xp_char16_t) == 2, "xp_char16_t should have 2 byte(s)");
static_assert(sizeof(xp_char32_t) == 4, "xp_char32_t should have 4 byte(s)");

#endif // __XPLATFORM_CORE_HPP__