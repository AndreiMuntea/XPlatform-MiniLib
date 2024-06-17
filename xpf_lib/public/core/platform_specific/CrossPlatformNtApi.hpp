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

#if defined XPF_PLATFORM_WIN_UM
EXTERN_C_START

#if defined XPF_COMPILER_MSVC
    #pragma comment(lib, "ntdll.lib")
#endif

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

EXTERN_C_END
#endif  // XPF_PLATFORM_WIN_UM
