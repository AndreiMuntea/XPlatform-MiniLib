/**
 * @file        xpf_lib/public/core/Core.hpp
 *
 * @brief       This contains the core definitions for xpf.
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
 * @brief Define the compiler. More can be added later.
 */
#if defined _MSC_VER

    /**
     * @brief The Microsoft Visual Studio compiler.
     */
    #define XPF_COMPILER_MSVC

#elif defined (__clang__)

    /**
     * @brief CLANG compiler.
     */
    #define XPF_COMPILER_CLANG

#elif defined (__GNUC__)

    /**
     * @brief GCC compiler.
     */
    #define XPF_COMPILER_GCC

#else
    #error Unsupported Compiler.
#endif


/**
 * @brief Define the platform.
 *        _WIN32 Defined as 1 when the compilation target is 32-bit ARM,
 *        64-bit ARM, x86, or x64. Otherwise, undefined.
 */
#if defined _WIN32

    #if defined _KERNEL_MODE

        /**
         * @brief We are on KM platform
         */
        #define XPF_PLATFORM_WIN_KM

    #else

        /**
         * @brief We are on UM platform
         */
        #define XPF_PLATFORM_WIN_UM
    #endif

#elif defined (__linux__)

    /**
     * @brief We are on Linuxs user mode.
     */
    #define XPF_PLATFORM_LINUX_UM

#else
    #error Unsupported Platform
#endif


/**
 * @brief Define the configuration mode (debug / release)
 */
#if defined NDEBUG

    /**
     * @brief We are on release mode
     */
    #define XPF_CONFIGURATION_RELEASE

#else

    /**
     * @brief We are on debug mode
     */
    #define XPF_CONFIGURATION_DEBUG
#endif


/**
 * @brief Compiler specific includes and definitions.
 */
#if defined XPF_COMPILER_MSVC
    #include <intrin.h>

    /**
     * @brief Helper macro to store the code and the data in paged section areas.
     *        This should be used only with code / data known to be at IRQL <= APC_LEVEL.
     *        Especially useful for Windows KM. Seems that the windows looks at "PAGE"
     *        page we can't really name them differently - otherwise they won't be paged.
     */
    #define XPF_SECTION_PAGED                   _Pragma("code_seg(  \"PAGE\"    )")     \
                                                _Pragma("data_seg(  \"PAGERW\"  )")     \
                                                _Pragma("const_seg( \"PAGERO\"  )")     \
                                                _Pragma("bss_seg(   \"PAGEBSS\" )")

    /**
     * @brief Helper macro to restore default sections.
     */
    #define XPF_SECTION_DEFAULT                 _Pragma("code_seg()")                   \
                                                _Pragma("data_seg()")                   \
                                                _Pragma("const_seg()")                  \
                                                _Pragma("bss_seg()")
    /**
     * @brief This will be used to allocate in the given section.
     */
    #define XPF_ALLOC_SECTION(Section)         __declspec(allocate(Section))

#elif defined XPF_COMPILER_CLANG || defined XPF_COMPILER_GCC

    /**
     * @brief On other compilers this translates to nothing.
     *        We can improve here when we want to. For now ignore this.
     *        It is useful only on WIN-KM.
     */
    #define XPF_SECTION_PAGED

    /**
     * @brief On other compilers this translates to nothing.
     *        We can improve here when we want to.For now ignore this.
     *        It is useful only on WIN-KM.
     */
    #define XPF_SECTION_DEFAULT

    /**
     * @brief       This will be used to allocate in the given section.
     *              We'll also mark the variable as being used to prevent its removal by the zealous linker.
     */
    #define XPF_ALLOC_SECTION(Section)         __attribute__((section(Section)))       \
                                               __attribute__((used))
#else
    #error Unsupported Compiler.
#endif

/**
 * @brief Platform specific includes.
 */
#if defined XPF_PLATFORM_WIN_KM

    #include <fltKernel.h>
    #include <ntstrsafe.h>
    #include <ntintsafe.h>

#elif defined XPF_PLATFORM_WIN_UM

    /**
     * @brief Do not include ntstatus as it will be included by ntstatus header.
     */
    #define WIN32_NO_STATUS
    /**
     * @brief This conflicts with some definitions in STL.
     */
    #define NOMINMAX
        #include <Windows.h>
        #include <winternl.h>
        #include <strsafe.h>
        #include <intsafe.h>
    #undef WIN32_NO_STATUS
        #include <ntstatus.h>
    #undef NOMINMAX

#elif defined XPF_PLATFORM_LINUX_UM

    #include <unistd.h>
    #include <sched.h>
    #include <sys/time.h>
    #include <pthread.h>
    #include <cstdlib>
    #include <cassert>
    #include <cstdint>
    #include <cstdio>
    #include <climits>
    #include <csignal>
    #include <cwctype>
    #include <cstring>

#else
    #error Unsupported Platform

#endif


/**
 * @brief Common platform includes.
 */
#include "platform_specific/CrossPlatformSal.hpp"
#include "platform_specific/CrossPlatformStatus.hpp"


/**
 * @brief Platform specific definitions.
 */
#if defined XPF_PLATFORM_WIN_KM

    /**
     * @brief Macro definition for ASSERT.
     */
    #define XPF_ASSERT                   NT_ASSERT

    /**
     * @brief Macro definition for Verify.
     */
    #define XPF_VERIFY                      NT_VERIFY

    /**
     * @brief Use the same calling convention as NT APIs.
     */
    #define XPF_API                         NTAPI

    /**
     * @brief Use the same alignment as default.
     */
    #define XPF_DEFAULT_ALIGNMENT           size_t{MEMORY_ALLOCATION_ALIGNMENT}

    /**
     * @brief Macro definition for nothing.
     */
    #define XPF_NOTHING()                   { (void)(0); }

    /**
     * @brief Asserts on debug that the code is indeed running at PASSIVE LEVEL.
     *        This is valid only for windows KM.
     */
    #define XPF_MAX_PASSIVE_LEVEL()         XPF_ASSERT(::KeGetCurrentIrql() == PASSIVE_LEVEL);  \
                                            PAGED_CODE();

    /**
     * @brief Asserts on debug that the code is indeed running at max APC LEVEL.
     *        This is valid only for windows KM.
     */
    #define XPF_MAX_APC_LEVEL()             XPF_ASSERT(::KeGetCurrentIrql() <= APC_LEVEL);      \
                                            PAGED_CODE();

    /**
     * @brief Asserts on debug that the code is indeed running at max DISPATCH LEVEL.
     */
    #define XPF_MAX_DISPATCH_LEVEL()        XPF_ASSERT(::KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /**
     * @brief The platform convention for placement new is CLRCALL or CDECL.
     */
    #define XPF_PLATFORM_CONVENTION         __CLRCALL_OR_CDECL

    /**
     *
     * @brief stdint header is not available in UM-KM.
     *        The simplest solution is to just alias to what we already have.
     *        So we are going to do just that.
     */
     using uint8_t   = UINT8;
     using uint16_t  = UINT16;
     using uint32_t  = UINT32;
     using uint64_t  = UINT64;

     using int8_t  = INT8;
     using int16_t = INT16;
     using int32_t = INT32;
     using int64_t = INT64;

#elif defined XPF_PLATFORM_WIN_UM

    #if defined XPF_CONFIGURATION_RELEASE

        /**
         * @brief Macro definition for ASSERT.
         */
        #define XPF_ASSERT(Expression)      (((void) 0), true)
        /**
         * @brief Macro definition for VERIFY.
         */
        #define XPF_VERIFY(Expression)      ((Expression) ? true : false)

    #elif defined XPF_CONFIGURATION_DEBUG

        /**
         * @brief Macro definition for ASSERT.
         */
        #define XPF_ASSERT(Expression)      ((Expression) ? true                                                                \
                                                          : (::RaiseException(ERROR_UNHANDLED_EXCEPTION, 0, 0, NULL), false))
        /**
         * @brief Macro definition for VERIFY.
         */
        #define XPF_VERIFY                  XPF_ASSERT
    #else

        #error Unknown Configuration
    #endif

    /**
     * @brief Use the same calling convention as NT APIs.
     */
    #define XPF_API                         NTAPI

    /**
     * @brief Use the same alignment as default.
     */
    #define XPF_DEFAULT_ALIGNMENT           size_t{MEMORY_ALLOCATION_ALIGNMENT}

    /**
     * @brief Macro definition for nothing.
     */
    #define XPF_NOTHING()                   { (void)(0); }

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_PASSIVE_LEVEL           XPF_NOTHING

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_APC_LEVEL               XPF_NOTHING

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_DISPATCH_LEVEL          XPF_NOTHING

    /**
     * @brief The platform convention for placement new is CLRCALL or CDECL.
     */
    #define XPF_PLATFORM_CONVENTION         __CLRCALL_OR_CDECL

    /**
     *
     * @brief stdint header is not available in UM-KM.
     *        The simplest solution is to just alias to what we already have.
     *        So we are going to do just that.
     */
     using uint8_t   = UINT8;
     using uint16_t  = UINT16;
     using uint32_t  = UINT32;
     using uint64_t  = UINT64;

     using int8_t  = INT8;
     using int16_t = INT16;
     using int32_t = INT32;
     using int64_t = INT64;

#elif defined XPF_PLATFORM_LINUX_UM

    #if defined XPF_CONFIGURATION_RELEASE

        /**
         * @brief Macro definition for ASSERT.
         */
        #define XPF_ASSERT(Expression)  (((void) 0), true)
        /**
         * @brief Macro definition for VERIFY.
         */
        #define XPF_VERIFY(Expression)   ((Expression) ? true : false)

    #elif defined XPF_CONFIGURATION_DEBUG

        /**
         * @brief Macro definition for ASSERT.
         */
        #define XPF_ASSERT(Expression)   ((Expression) ? true : (::raise(SIGSEGV), false))
        /**
         * @brief Macro definition for VERIFY.
         */
        #define XPF_VERIFY               XPF_ASSERT
    #else

        #error Unknown Configuration
    #endif

    /**
     * @brief Use the same calling convention as MS ABI.
     */
    #define XPF_API                     __attribute__((ms_abi))

    /**
     * @brief Use the same alignment as default.
     */
    #define XPF_DEFAULT_ALIGNMENT        size_t{sizeof(void*) * 2}

    /**
     * @brief Macro definition for nothing.
     */
    #define XPF_NOTHING()                { (void)(0); }

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_PASSIVE_LEVEL        XPF_NOTHING

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_APC_LEVEL            XPF_NOTHING

    /**
     * @brief On all other platforms this translates to nothig.
     */
    #define XPF_MAX_DISPATCH_LEVEL       XPF_NOTHING

    /**
     * @brief Leave the default calling convention
     */
    #define XPF_PLATFORM_CONVENTION

#else
    #error Unsupported Platform

#endif


/**
 * @brief Common Platform definitions.
 */

/**
 * @brief Helper macro to get the number of elements in an array.
 */
#define XPF_ARRAYSIZE(elements)                         (sizeof((elements)) / sizeof((elements)[0]))

/**
 * @brief Helper macro to retrieve the base address of an instance of a structure
 * given the type of the structure and the address of a field within the containing structure.
 */
#define XPF_CONTAINING_RECORD(address, type, field)     ((type *)((uint8_t*)(address) - (size_t)(&((type *)0)->field)))         // NOLINT(*)

 /**
  * @brief Useful macro for unreferenced parameter.
  */
#define XPF_UNREFERENCED_PARAMETER(Argument)            { (void)(Argument); }

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
extern void*
XPF_PLATFORM_CONVENTION
operator new(
    size_t BlockSize,
    void* Location
) noexcept(true);

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
extern void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    void* Location
) noexcept(true);
