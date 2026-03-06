/**
 * @file        xpf_lib/private/Filesystem/FileObject.cpp
 *
 * @brief       Platform-specific implementation of FileObject and FileOplock classes.
 *              Uses Zw* APIs on Windows (UM and KM) and POSIX APIs on Linux.
 *
 *              Windows API references:
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntreadfile
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntwritefile
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntfscontrolfile
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntclose
 *
 *              POSIX API references:
 *              See: https://man7.org/linux/man-pages/man2/open.2.html
 *              See: https://man7.org/linux/man-pages/man2/pread.2.html
 *              See: https://man7.org/linux/man-pages/man2/pwrite.2.html
 *              See: https://man7.org/linux/man-pages/man2/close.2.html
 *              See: https://man7.org/linux/man-pages/man2/fcntl.2.html  (F_SETLEASE section)
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/private/Filesystem/FileCommonHelpers.hpp"

/**
 * @brief   By default all code in here goes into paged section.
 *          File and oplock operations require PASSIVE_LEVEL.
 */
XPF_SECTION_PAGED;


//
// ************************************************************************************************
//                              FileObject implementation
// ************************************************************************************************
//

_IRQL_requires_max_(PASSIVE_LEVEL)
xpf::FileObject::~FileObject(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        //
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntclose
        //
        if (nullptr != this->m_NtFileHandle)
        {
            (void) ZwClose(this->m_NtFileHandle);
            this->m_NtFileHandle = nullptr;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // See: https://man7.org/linux/man-pages/man2/close.2.html
        //
        if (-1 != this->m_FileDescriptor)
        {
            (void) close(this->m_FileDescriptor);
            this->m_FileDescriptor = -1;
        }
    #else
        #error Unknown Platform
    #endif

    this->m_ReadOnly = true;
}

xpf::FileObject::FileObject(
    _Inout_ FileObject&& Other
) noexcept(true) : m_ReadOnly{Other.m_ReadOnly}
{
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        this->m_NtFileHandle = Other.m_NtFileHandle;
        Other.m_NtFileHandle = nullptr;
    #elif defined XPF_PLATFORM_LINUX_UM
        this->m_FileDescriptor = Other.m_FileDescriptor;
        Other.m_FileDescriptor = -1;
    #else
        #error Unknown Platform
    #endif

    Other.m_ReadOnly = true;
}

xpf::FileObject&
xpf::FileObject::operator=(
    _Inout_ FileObject&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        //
        // Close our current handle before taking the other's.
        //
        #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
            if (nullptr != this->m_NtFileHandle)
            {
                (void) ZwClose(this->m_NtFileHandle);
            }
            this->m_NtFileHandle = Other.m_NtFileHandle;
            Other.m_NtFileHandle = nullptr;
        #elif defined XPF_PLATFORM_LINUX_UM
            if (-1 != this->m_FileDescriptor)
            {
                (void) close(this->m_FileDescriptor);
            }
            this->m_FileDescriptor = Other.m_FileDescriptor;
            Other.m_FileDescriptor = -1;
        #else
            #error Unknown Platform
        #endif

        this->m_ReadOnly = Other.m_ReadOnly;
        Other.m_ReadOnly = true;
    }
    return *this;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
XPF_API
xpf::FileObject::Create(
    _Out_ xpf::Optional<xpf::FileObject>* FileObj,
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _In_ bool ReadOnly,
    _In_ bool CreateIfNotFound
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if (nullptr == FileObj)
    {
        return STATUS_INVALID_PARAMETER;
    }
    FileObj->Reset();

    if (FilePath.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Construct a local FileObject which we'll move into the Optional on success.
    //
    xpf::FileObject fileObject;
    fileObject.m_ReadOnly = ReadOnly;

    //
    // Platform-specific file creation.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    {
        IO_STATUS_BLOCK ioStatusBlock;
        xpf::ApiZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

        //
        // Build the proper NT path.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/object-names
        //
        xpf::String<wchar_t> ntPath;
        NTSTATUS status = xpf::FileCommonHelpers::BuildNtPath(FilePath, ntPath);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        //
        // Initialize UNICODE_STRING and OBJECT_ATTRIBUTES from the NT path.
        //
        UNICODE_STRING unicodePath;
        OBJECT_ATTRIBUTES objectAttributes;
        status = xpf::FileCommonHelpers::InitializeObjectAttributesFromPath(ntPath,
                                                                            &unicodePath,
                                                                            &objectAttributes);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        //
        // Determine the desired access and disposition.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
        //
        ACCESS_MASK desiredAccess = FILE_GENERIC_READ | SYNCHRONIZE;
        if (!ReadOnly)
        {
            desiredAccess |= FILE_GENERIC_WRITE;
        }

        const ULONG createDisposition = CreateIfNotFound ? FILE_OPEN_IF
                                                         : FILE_OPEN;
        const ULONG createOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;
        const ULONG shareAccess = FILE_SHARE_READ;

        //
        // Create or open the file using ZwCreateFile.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
        //
        status = ZwCreateFile(&fileObject.m_NtFileHandle,
                              desiredAccess,
                              &objectAttributes,
                              &ioStatusBlock,
                              nullptr,                  // AllocationSize
                              FILE_ATTRIBUTE_NORMAL,    // FileAttributes
                              shareAccess,
                              createDisposition,
                              createOptions,
                              nullptr,                  // EaBuffer
                              0);                       // EaLength
        if (!NT_SUCCESS(status))
        {
            fileObject.m_NtFileHandle = nullptr;
            return status;
        }
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        //
        // Convert the wide-character path to UTF-8.
        //
        xpf::String<char> utf8Path;
        NTSTATUS status = xpf::StringConversion::WideToUTF8(FilePath, utf8Path);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        //
        // Determine the open flags.
        // See: https://man7.org/linux/man-pages/man2/open.2.html
        //
        int flags = ReadOnly ? O_RDONLY : O_RDWR;
        if (CreateIfNotFound)
        {
            flags |= O_CREAT;
        }

        //
        // Open the file with standard permissions (owner rw, group r, others r).
        // See: https://man7.org/linux/man-pages/man2/open.2.html
        //
        const int fd = open(utf8Path.View().Buffer(),
                            flags,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            const int savedErrno = errno;
            switch (savedErrno)
            {
                case ENOENT:
                    return STATUS_NOT_FOUND;
                case EACCES:
                case EPERM:
                    return STATUS_UNSUCCESSFUL;
                default:
                    return NTSTATUS_FROM_PLATFORM_ERROR(savedErrno);
            }
        }
        fileObject.m_FileDescriptor = fd;
    }
    #else
        #error Unknown Platform
    #endif

    //
    // Move the local FileObject into the Optional.
    //
    FileObj->Emplace(xpf::Move(fileObject));
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
XPF_API
xpf::FileObject::Read(
    _Out_ uint8_t* Buffer,
    _In_ size_t BufferSize,
    _In_ size_t Offset,
    _Out_ size_t* BytesRead
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == Buffer) || (0 == BufferSize) || (nullptr == BytesRead))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesRead = 0;

    //
    // Platform-specific read.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    {
        if (nullptr == this->m_NtFileHandle)
        {
            return STATUS_INVALID_HANDLE;
        }

        //
        // The buffer size must fit in ULONG for ZwReadFile.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntreadfile
        //
        if (BufferSize > xpf::NumericLimits<uint32_t>::MaxValue())
        {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Validate offset does not overflow LONGLONG (max positive value for LARGE_INTEGER.QuadPart).
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntreadfile
        //
        if (Offset > static_cast<size_t>(xpf::NumericLimits<int64_t>::MaxValue()))
        {
            return STATUS_INVALID_PARAMETER;
        }

        IO_STATUS_BLOCK ioStatusBlock;
        xpf::ApiZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

        LARGE_INTEGER byteOffset;
        byteOffset.QuadPart = static_cast<LONGLONG>(Offset);

        const NTSTATUS status = ZwReadFile(this->m_NtFileHandle,
                                           nullptr,           // Event
                                           nullptr,           // ApcRoutine
                                           nullptr,           // ApcContext
                                           &ioStatusBlock,
                                           Buffer,
                                           static_cast<ULONG>(BufferSize),
                                           &byteOffset,
                                           nullptr);          // Key
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        *BytesRead = static_cast<size_t>(ioStatusBlock.Information);
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        if (-1 == this->m_FileDescriptor)
        {
            return STATUS_INVALID_HANDLE;
        }

        //
        // Validate offset does not overflow off_t.
        // See: https://man7.org/linux/man-pages/man2/pread.2.html
        //
        if (Offset > static_cast<size_t>(xpf::NumericLimits<int64_t>::MaxValue()))
        {
            return STATUS_INVALID_PARAMETER;
        }

        const ssize_t result = pread(this->m_FileDescriptor,
                                     Buffer,
                                     BufferSize,
                                     static_cast<off_t>(Offset));
        if (result < 0)
        {
            return NTSTATUS_FROM_PLATFORM_ERROR(errno);
        }

        *BytesRead = static_cast<size_t>(result);
    }
    #else
        #error Unknown Platform
    #endif

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
XPF_API
xpf::FileObject::Write(
    _In_reads_bytes_(BufferSize) const uint8_t* Buffer,
    _In_ size_t BufferSize,
    _In_ size_t Offset,
    _Out_ size_t* BytesWritten
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == Buffer) || (0 == BufferSize) || (nullptr == BytesWritten))
    {
        return STATUS_INVALID_PARAMETER;
    }

    *BytesWritten = 0;

    //
    // Check that the file was opened for writing.
    //
    if (this->m_ReadOnly)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    // Platform-specific write.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    {
        if (nullptr == this->m_NtFileHandle)
        {
            return STATUS_INVALID_HANDLE;
        }

        //
        // The buffer size must fit in ULONG for ZwWriteFile.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntwritefile
        //
        if (BufferSize > xpf::NumericLimits<uint32_t>::MaxValue())
        {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Validate offset does not overflow LONGLONG.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntwritefile
        //
        if (Offset > static_cast<size_t>(xpf::NumericLimits<int64_t>::MaxValue()))
        {
            return STATUS_INVALID_PARAMETER;
        }

        IO_STATUS_BLOCK ioStatusBlock;
        xpf::ApiZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

        LARGE_INTEGER byteOffset;
        byteOffset.QuadPart = static_cast<LONGLONG>(Offset);

        const NTSTATUS status = ZwWriteFile(this->m_NtFileHandle,
                                            nullptr,           // Event
                                            nullptr,           // ApcRoutine
                                            nullptr,           // ApcContext
                                            &ioStatusBlock,
                                            const_cast<uint8_t*>(Buffer),
                                            static_cast<ULONG>(BufferSize),
                                            &byteOffset,
                                            nullptr);          // Key
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        *BytesWritten = static_cast<size_t>(ioStatusBlock.Information);
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        if (-1 == this->m_FileDescriptor)
        {
            return STATUS_INVALID_HANDLE;
        }

        //
        // Validate offset does not overflow off_t.
        // See: https://man7.org/linux/man-pages/man2/pwrite.2.html
        //
        if (Offset > static_cast<size_t>(xpf::NumericLimits<int64_t>::MaxValue()))
        {
            return STATUS_INVALID_PARAMETER;
        }

        const ssize_t result = pwrite(this->m_FileDescriptor,
                                      Buffer,
                                      BufferSize,
                                      static_cast<off_t>(Offset));
        if (result < 0)
        {
            return NTSTATUS_FROM_PLATFORM_ERROR(errno);
        }

        *BytesWritten = static_cast<size_t>(result);
    }
    #else
        #error Unknown Platform
    #endif

    return STATUS_SUCCESS;
}


//
// ************************************************************************************************
//                              FileOplock implementation
// ************************************************************************************************
//

_IRQL_requires_max_(PASSIVE_LEVEL)
xpf::FileOplock::~FileOplock(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    {
        //
        // Closing the file handle cancels any pending oplock FSCTL
        // and releases the oplock. The close waits for I/O completion.
        //
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntclose
        //
        if (nullptr != this->m_FileHandle)
        {
            (void) ZwClose(this->m_FileHandle);
            this->m_FileHandle = nullptr;
        }
        if (nullptr != this->m_EventHandle)
        {
            (void) ZwClose(this->m_EventHandle);
            this->m_EventHandle = nullptr;
        }
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        if (-1 != this->m_FileDescriptor)
        {
            //
            // Remove the lease before closing the fd.
            // See: https://man7.org/linux/man-pages/man2/fcntl.2.html  (F_SETLEASE section)
            //
            if (this->m_OplockActive)
            {
                (void) fcntl(this->m_FileDescriptor, F_SETLEASE, F_UNLCK);
            }
            //
            // See: https://man7.org/linux/man-pages/man2/close.2.html
            //
            (void) close(this->m_FileDescriptor);
            this->m_FileDescriptor = -1;
        }
    }
    #else
        #error Unknown Platform
    #endif

    this->m_OplockActive = false;
}

xpf::FileOplock::FileOplock(
    _Inout_ FileOplock&& Other
) noexcept(true) : m_OplockActive{Other.m_OplockActive}
{
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        this->m_FileHandle = Other.m_FileHandle;
        this->m_EventHandle = Other.m_EventHandle;
        this->m_IoStatusBlock = Other.m_IoStatusBlock;

        Other.m_FileHandle = nullptr;
        Other.m_EventHandle = nullptr;
        xpf::ApiZeroMemory(&Other.m_IoStatusBlock, sizeof(Other.m_IoStatusBlock));
    #elif defined XPF_PLATFORM_LINUX_UM
        this->m_FileDescriptor = Other.m_FileDescriptor;
        Other.m_FileDescriptor = -1;
    #else
        #error Unknown Platform
    #endif

    Other.m_OplockActive = false;
}

xpf::FileOplock&
xpf::FileOplock::operator=(
    _Inout_ FileOplock&& Other
) noexcept(true)
{
    if (this != xpf::AddressOf(Other))
    {
        //
        // Release our current oplock before taking the other's.
        //
        #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        {
            if (nullptr != this->m_FileHandle)
            {
                (void) ZwClose(this->m_FileHandle);
            }
            if (nullptr != this->m_EventHandle)
            {
                (void) ZwClose(this->m_EventHandle);
            }

            this->m_FileHandle = Other.m_FileHandle;
            this->m_EventHandle = Other.m_EventHandle;
            this->m_IoStatusBlock = Other.m_IoStatusBlock;

            Other.m_FileHandle = nullptr;
            Other.m_EventHandle = nullptr;
            xpf::ApiZeroMemory(&Other.m_IoStatusBlock, sizeof(Other.m_IoStatusBlock));
        }
        #elif defined XPF_PLATFORM_LINUX_UM
        {
            if (-1 != this->m_FileDescriptor)
            {
                if (this->m_OplockActive)
                {
                    (void) fcntl(this->m_FileDescriptor, F_SETLEASE, F_UNLCK);
                }
                (void) close(this->m_FileDescriptor);
            }

            this->m_FileDescriptor = Other.m_FileDescriptor;
            Other.m_FileDescriptor = -1;
        }
        #else
            #error Unknown Platform
        #endif

        this->m_OplockActive = Other.m_OplockActive;
        Other.m_OplockActive = false;
    }
    return *this;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
XPF_API
xpf::FileOplock::Request(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _Out_ xpf::Optional<xpf::FileOplock>* Oplock
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if (nullptr == Oplock)
    {
        return STATUS_INVALID_PARAMETER;
    }
    Oplock->Reset();

    if (FilePath.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Construct a local FileOplock which we'll move into the Optional on success.
    //
    xpf::FileOplock oplock;

    //
    // Platform-specific oplock request.
    // All failures in this section are treated as best-effort:
    // the oplock won't be active but the operation still succeeds.
    //

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
    {
        IO_STATUS_BLOCK ioStatusBlock;
        xpf::ApiZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

        //
        // Build the proper NT path.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/object-names
        //
        xpf::String<wchar_t> ntPath;
        NTSTATUS status = xpf::FileCommonHelpers::BuildNtPath(FilePath, ntPath);
        if (!NT_SUCCESS(status))
        {
            //
            // Best effort - path construction failed, return without oplock.
            //
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }

        //
        // Initialize UNICODE_STRING and OBJECT_ATTRIBUTES from the NT path.
        //
        UNICODE_STRING unicodePath;
        OBJECT_ATTRIBUTES objectAttributes;
        status = xpf::FileCommonHelpers::InitializeObjectAttributesFromPath(ntPath,
                                                                            &unicodePath,
                                                                            &objectAttributes);
        if (!NT_SUCCESS(status))
        {
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }

        //
        // Open a separate handle for the oplock request.
        // We use FILE_READ_ATTRIBUTES as minimal access.
        // The file must be opened without FILE_SYNCHRONOUS_IO_NONALERT
        // so that the oplock FSCTL can pend asynchronously.
        //
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
        //
        const ULONG oplockShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        status = ZwCreateFile(&oplock.m_FileHandle,
                              FILE_READ_ATTRIBUTES,
                              &objectAttributes,
                              &ioStatusBlock,
                              nullptr,                  // AllocationSize
                              FILE_ATTRIBUTE_NORMAL,    // FileAttributes
                              oplockShareAccess,
                              FILE_OPEN,                // CreateDisposition
                              FILE_NON_DIRECTORY_FILE,  // CreateOptions
                              nullptr,                  // EaBuffer
                              0);                       // EaLength
        if (!NT_SUCCESS(status))
        {
            //
            // Best effort - can't open file, continue without oplock.
            //
            oplock.m_FileHandle = nullptr;
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }

        //
        // Create an event for the async oplock FSCTL.
        //
        #if defined XPF_PLATFORM_WIN_UM
        {
            status = NtCreateEvent(&oplock.m_EventHandle,
                                   EVENT_ALL_ACCESS,
                                   nullptr,
                                   NT_EVENT_TYPE_SYNCHRONIZATION,
                                   FALSE);
        }
        #elif defined XPF_PLATFORM_WIN_KM
        {
            OBJECT_ATTRIBUTES eventOa;
            xpf::ApiZeroMemory(&eventOa, sizeof(eventOa));
            eventOa.Length = sizeof(eventOa);
            eventOa.Attributes = OBJ_KERNEL_HANDLE;

            status = ZwCreateEvent(&oplock.m_EventHandle,
                                   EVENT_ALL_ACCESS,
                                   &eventOa,
                                   SynchronizationEvent,
                                   FALSE);
        }
        #endif

        if (!NT_SUCCESS(status))
        {
            //
            // Best effort - can't create event, close file and continue.
            //
            (void) ZwClose(oplock.m_FileHandle);
            oplock.m_FileHandle = nullptr;
            oplock.m_EventHandle = nullptr;
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }

        //
        // Request a batch oplock using ZwFsControlFile.
        // STATUS_PENDING means the oplock was granted and is being held.
        // The FSCTL will complete when the oplock breaks.
        //
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntfscontrolfile
        // See: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_request_batch_oplock
        //
        xpf::ApiZeroMemory(&oplock.m_IoStatusBlock, sizeof(oplock.m_IoStatusBlock));
        status = ZwFsControlFile(oplock.m_FileHandle,
                                 oplock.m_EventHandle,
                                 nullptr,                       // ApcRoutine
                                 nullptr,                       // ApcContext
                                 &oplock.m_IoStatusBlock,
                                 FSCTL_REQUEST_BATCH_OPLOCK,
                                 nullptr,                       // InputBuffer
                                 0,                             // InputBufferLength
                                 nullptr,                       // OutputBuffer
                                 0);                            // OutputBufferLength
        if (STATUS_PENDING == status || NT_SUCCESS(status))
        {
            oplock.m_OplockActive = true;
        }
        else
        {
            //
            // Best effort - oplock not supported by this filesystem.
            // Close the event and file handle since we don't need them.
            //
            (void) ZwClose(oplock.m_EventHandle);
            (void) ZwClose(oplock.m_FileHandle);
            oplock.m_EventHandle = nullptr;
            oplock.m_FileHandle = nullptr;
            oplock.m_OplockActive = false;
        }
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        //
        // Convert the wide-character path to UTF-8.
        //
        xpf::String<char> utf8Path;
        NTSTATUS status = xpf::StringConversion::WideToUTF8(FilePath, utf8Path);
        if (!NT_SUCCESS(status))
        {
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }

        //
        // Open a separate file descriptor for the lease.
        // See: https://man7.org/linux/man-pages/man2/open.2.html
        //
        const int fd = open(utf8Path.View().Buffer(), O_RDONLY);
        if (fd < 0)
        {
            Oplock->Emplace(xpf::Move(oplock));
            return STATUS_SUCCESS;
        }
        oplock.m_FileDescriptor = fd;

        //
        // Request a read lease (similar to an oplock).
        // See: https://man7.org/linux/man-pages/man2/fcntl.2.html  (F_SETLEASE section)
        //
        const int leaseResult = fcntl(fd, F_SETLEASE, F_RDLCK);
        if (0 == leaseResult)
        {
            oplock.m_OplockActive = true;
        }
        else
        {
            //
            // Best effort - lease not supported (e.g., NFS, tmpfs).
            //
            oplock.m_OplockActive = false;
        }
    }
    #else
        #error Unknown Platform
    #endif

    //
    // Move into the Optional regardless of whether the oplock is active.
    //
    Oplock->Emplace(xpf::Move(oplock));
    return STATUS_SUCCESS;
}
