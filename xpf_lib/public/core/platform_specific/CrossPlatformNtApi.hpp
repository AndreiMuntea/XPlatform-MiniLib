//
// @file        xpf_lib/public/core/platform_specific/CrossPlatformNtApi.hpp
//
// @brief       This file fills nt api which are exported but not documented.
//
// @note        This file is skipped from doxygen generation.
//              So the comments are intended to be with slash rather than asterix.
//
// @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
//
// @copyright   Copyright Andrei-Marius MUNTEA 2020-2023.
//              All rights reserved.
//
// @license     See top-level directory LICENSE file.
//

#pragma once

#if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
EXTERN_C_START

#if defined XPF_COMPILER_MSVC && defined XPF_PLATFORM_WIN_UM
    #pragma comment(lib, "ntdll.lib")
#endif  // XPF_COMPILER_MSVC && XPF_PLATFORM_WIN_UM


//
// COMMON AREA - for both um and km
//


NTSYSAPI ULONG NTAPI
RtlWalkFrameChain(
    _Out_ PVOID* Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
);


//
// Only UM fill.
//


#if defined XPF_PLATFORM_WIN_UM

NTSYSAPI WCHAR NTAPI
RtlDowncaseUnicodeChar(
    _In_ WCHAR SourceCharacter
);

NTSYSAPI WCHAR NTAPI
RtlUpcaseUnicodeChar(
    _In_ WCHAR SourceCharacter
);

NTSYSAPI ULONG NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed
);

NTSYSAPI ULONG NTAPI
RtlUnicodeStringToInteger(
    _In_ PCUNICODE_STRING String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
);
#endif  // XPF_PLATFORM_WIN_UM

EXTERN_C_END
#endif  // XPF_PLATFORM_WIN_UM || XPF_PLATFORM_WIN_KM
