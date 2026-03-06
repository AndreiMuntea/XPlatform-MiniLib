/**
 * @file        xpf_lib/private/Filesystem/FileCommonHelpers.hpp
 *
 * @brief       Shared internal helpers for filesystem operations.
 *              This header is private and should only be included
 *              by filesystem implementation files (.cpp).
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2024.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#pragma once

#include "xpf_lib/xpf.hpp"


//
// ************************************************************************************************
// Platform-specific constants that may not be available on all configurations.
// ************************************************************************************************
//

#if defined XPF_PLATFORM_WIN_UM
    //
    // These constants are defined in ntifs.h / wdm.h for kernel mode.
    // In user-mode they may not be available depending on SDK configuration.
    // Use #ifndef guards to avoid redefinitions.
    //
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
    //
    #ifndef FILE_OPEN
        #define FILE_OPEN                           0x00000001
    #endif
    #ifndef FILE_CREATE
        #define FILE_CREATE                         0x00000002
    #endif
    #ifndef FILE_OPEN_IF
        #define FILE_OPEN_IF                        0x00000003
    #endif
    #ifndef FILE_NON_DIRECTORY_FILE
        #define FILE_NON_DIRECTORY_FILE             0x00000040
    #endif
    #ifndef FILE_SYNCHRONOUS_IO_NONALERT
        #define FILE_SYNCHRONOUS_IO_NONALERT        0x00000020
    #endif
    #ifndef OBJ_CASE_INSENSITIVE
        #define OBJ_CASE_INSENSITIVE                0x00000040
    #endif

    //
    // FSCTL_REQUEST_BATCH_OPLOCK may be missing when WIN32_LEAN_AND_MEAN is defined.
    // See: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_request_batch_oplock
    //
    #ifndef FSCTL_REQUEST_BATCH_OPLOCK
        #define FSCTL_REQUEST_BATCH_OPLOCK           0x00090008
    #endif

    //
    // Declare the Zw* / Nt* APIs which are exported from ntdll.dll.
    // These are not declared in standard UM headers but are available at link time.
    //
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntreadfile
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntwritefile
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntfscontrolfile
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntclose
    //
    EXTERN_C_START

    NTSYSAPI NTSTATUS NTAPI
    ZwCreateFile(
        _Out_ PHANDLE FileHandle,
        _In_ ACCESS_MASK DesiredAccess,
        _In_ POBJECT_ATTRIBUTES ObjectAttributes,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_opt_ PLARGE_INTEGER AllocationSize,
        _In_ ULONG FileAttributes,
        _In_ ULONG ShareAccess,
        _In_ ULONG CreateDisposition,
        _In_ ULONG CreateOptions,
        _In_opt_ PVOID EaBuffer,
        _In_ ULONG EaLength
    );

    NTSYSAPI NTSTATUS NTAPI
    ZwReadFile(
        _In_ HANDLE FileHandle,
        _In_opt_ HANDLE Event,
        _In_opt_ PVOID ApcRoutine,
        _In_opt_ PVOID ApcContext,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _Out_ PVOID Buffer,
        _In_ ULONG Length,
        _In_opt_ PLARGE_INTEGER ByteOffset,
        _In_opt_ PULONG Key
    );

    NTSYSAPI NTSTATUS NTAPI
    ZwWriteFile(
        _In_ HANDLE FileHandle,
        _In_opt_ HANDLE Event,
        _In_opt_ PVOID ApcRoutine,
        _In_opt_ PVOID ApcContext,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_ PVOID Buffer,
        _In_ ULONG Length,
        _In_opt_ PLARGE_INTEGER ByteOffset,
        _In_opt_ PULONG Key
    );

    NTSYSAPI NTSTATUS NTAPI
    ZwFsControlFile(
        _In_ HANDLE FileHandle,
        _In_opt_ HANDLE Event,
        _In_opt_ PVOID ApcRoutine,
        _In_opt_ PVOID ApcContext,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_ ULONG FsControlCode,
        _In_opt_ PVOID InputBuffer,
        _In_ ULONG InputBufferLength,
        _Out_opt_ PVOID OutputBuffer,
        _In_ ULONG OutputBufferLength
    );

    NTSYSAPI NTSTATUS NTAPI
    ZwClose(
        _In_ HANDLE Handle
    );

    EXTERN_C_END

#elif defined XPF_PLATFORM_WIN_KM
    //
    // FSCTL_REQUEST_BATCH_OPLOCK may not be available in all WDK configurations.
    // See: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_request_batch_oplock
    //
    #ifndef FSCTL_REQUEST_BATCH_OPLOCK
        #define FSCTL_REQUEST_BATCH_OPLOCK           0x00090008
    #endif

#elif defined XPF_PLATFORM_LINUX_UM
    #include <fcntl.h>
    #include <sys/stat.h>
#endif


//
// ************************************************************************************************
// UNICODE_STRING has a maximum Length of USHORT (uint16_t) which is 65535 bytes.
// Since Length must be an even number of bytes (wchar_t = 2 bytes),
// the practical maximum is 65534 bytes.
//
// See: https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_unicode_string
// ************************************************************************************************
//
#if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    static constexpr size_t UNICODE_STRING_MAX_LENGTH_BYTES = static_cast<size_t>(
                                                                  xpf::NumericLimits<uint16_t>::MaxValue()) - 1;
#endif


namespace xpf
{
namespace FileCommonHelpers
{

#if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM

/**
 * @brief   Builds a proper NT-style path from a DOS-style or NT-style path.
 *          Zw* file APIs require NT-style paths (e.g., "\??\C:\path\to\file").
 *          DOS-style paths (e.g., "C:\path\to\file") are converted by
 *          prepending "\??\" which is the NT object namespace prefix for the
 *          DOS device namespace.
 *
 * @param[in]    FilePath - The input file path (DOS or NT style).
 *
 * @param[inout] NtPath   - Receives the resulting NT-style path.
 *
 * @return a proper NTSTATUS error code.
 *
 * @note   See: https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/object-names
 */
static _Must_inspect_result_ NTSTATUS XPF_API
BuildNtPath(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _Inout_ xpf::String<wchar_t>& NtPath
) noexcept(true)
{
    //
    // If the path already starts with '\', assume it's a proper NT path.
    //
    if (!FilePath.IsEmpty() && FilePath.Buffer()[0] == L'\\')
    {
        const NTSTATUS status = NtPath.Append(FilePath);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        return STATUS_SUCCESS;
    }

    //
    // Otherwise, prepend \??\ to convert a DOS-style path to NT path.
    // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/object-names
    //
    const xpf::StringView<wchar_t> prefix(L"\\??\\");

    NTSTATUS status = NtPath.Append(prefix);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = NtPath.Append(FilePath);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief   Sets up a UNICODE_STRING and OBJECT_ATTRIBUTES from an NT path string.
 *
 * @param[in]  NtPath              - The NT-style path string.
 *
 * @param[out] OutUnicodePath      - Receives the initialized UNICODE_STRING.
 *                                   The buffer points into NtPath (no copy).
 *
 * @param[out] OutObjectAttributes - Receives the initialized OBJECT_ATTRIBUTES.
 *
 * @return a proper NTSTATUS error code.
 *
 * @note   See: https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_unicode_string
 *         See: https://learn.microsoft.com/en-us/windows/win32/api/ntdef/nf-ntdef-initializeobjectattributes
 */
static _Must_inspect_result_ NTSTATUS XPF_API
InitializeObjectAttributesFromPath(
    _In_ _Const_ const xpf::String<wchar_t>& NtPath,
    _Out_ UNICODE_STRING* OutUnicodePath,
    _Out_ OBJECT_ATTRIBUTES* OutObjectAttributes
) noexcept(true)
{
    //
    // Validate output parameters.
    //
    if ((nullptr == OutUnicodePath) || (nullptr == OutObjectAttributes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    xpf::ApiZeroMemory(OutUnicodePath, sizeof(*OutUnicodePath));
    xpf::ApiZeroMemory(OutObjectAttributes, sizeof(*OutObjectAttributes));

    //
    // Ensure the path length fits in UNICODE_STRING.Length (USHORT, max 65534 bytes).
    // See: https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_unicode_string
    //
    const size_t pathLengthInBytes = NtPath.View().BufferSize() * sizeof(wchar_t);
    if (pathLengthInBytes > UNICODE_STRING_MAX_LENGTH_BYTES)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set up UNICODE_STRING for the path.
    //
    OutUnicodePath->Buffer = const_cast<PWCH>(NtPath.View().Buffer());
    OutUnicodePath->Length = static_cast<USHORT>(pathLengthInBytes);
    OutUnicodePath->MaximumLength = OutUnicodePath->Length;

    //
    // Set up OBJECT_ATTRIBUTES.
    // See: https://learn.microsoft.com/en-us/windows/win32/api/ntdef/nf-ntdef-initializeobjectattributes
    //
    OutObjectAttributes->Length = sizeof(*OutObjectAttributes);
    OutObjectAttributes->ObjectName = OutUnicodePath;
    OutObjectAttributes->Attributes = OBJ_CASE_INSENSITIVE;

    #if defined XPF_PLATFORM_WIN_KM
        OutObjectAttributes->Attributes |= OBJ_KERNEL_HANDLE;
    #endif

    return STATUS_SUCCESS;
}

#endif  // XPF_PLATFORM_WIN_UM || XPF_PLATFORM_WIN_KM

};  // namespace FileCommonHelpers
};  // namespace xpf
