/**
 * @file        xpf_lib/public/Filesystem/FileObject.hpp
 *
 * @brief       This contains the cross-platform file abstraction layer.
 *              It provides two classes:
 *                - FileObject: for file create/read/write operations.
 *                - FileOplock: for best-effort opportunistic locks.
 *
 *              Both classes use a private constructor with a static factory
 *              method that returns Optional<> to signal success or failure.
 *
 *              Platform-specific details:
 *                - Windows (UM and KM): Uses Zw* APIs (ZwCreateFile, ZwReadFile, etc.)
 *                - Linux (UM): Uses POSIX APIs (open, pread, pwrite, etc.)
 *
 *              See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatefile
 *              See: https://man7.org/linux/man-pages/man2/open.2.html
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2024.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/Memory/Optional.hpp"
#include "xpf_lib/public/Containers/String.hpp"


namespace xpf
{

//
// ************************************************************************************************
// This is the section containing the FileObject class definition.
// ************************************************************************************************
//

/**
 * @brief   Cross-platform file abstraction class.
 *          Provides create/read/write operations on files.
 *          Uses Zw* APIs on Windows and POSIX APIs on Linux.
 *          Instances are created via the static Create() factory method.
 *          The destructor automatically closes the file handle.
 */
class FileObject
{
 public:
    /**
     * @brief Destructor. Closes the underlying file handle.
     *        Must be called at PASSIVE_LEVEL in kernel mode.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    ~FileObject(
        void
    ) noexcept(true);

    /**
     * @brief Move constructor. Transfers ownership of the file handle.
     *
     * @param[in,out] Other - The other file object to move from.
     *                        Will be invalidated after this call.
     */
    FileObject(
        _Inout_ FileObject&& Other
    ) noexcept(true);

    /**
     * @brief Move assignment. Transfers ownership of the file handle.
     *
     * @param[in,out] Other - The other file object to move from.
     *                        Will be invalidated after this call.
     *
     * @return A reference to *this object after move.
     */
    FileObject&
    operator=(
        _Inout_ FileObject&& Other
    ) noexcept(true);

    /**
     * @brief Copy constructor - deleted. FileObject is move-only.
     */
    FileObject(
        _In_ _Const_ const FileObject& Other
    ) noexcept(true) = delete;

    /**
     * @brief Copy assignment - deleted. FileObject is move-only.
     */
    FileObject&
    operator=(
        _In_ _Const_ const FileObject& Other
    ) noexcept(true) = delete;

    /**
     * @brief Creates or opens a file, returning an Optional<FileObject> on success.
     *
     * @param[out] FileObj          - On success, receives the FileObject wrapped in Optional.
     *                                On failure, the Optional is empty.
     *
     * @param[in] FilePath          - The path to the file. On Windows, this should be a DOS-style path
     *                                (e.g., L"C:\\path\\to\\file") or an NT-style path
     *                                (e.g., L"\\??\\C:\\path\\to\\file").
     *                                On Linux, this is a standard POSIX path encoded as wchar_t.
     *
     * @param[in] ReadOnly          - If true, the file is opened for read-only access.
     *                                If false, the file is opened for read-write access.
     *
     * @param[in] CreateIfNotFound  - If true and the file does not exist, a new file is created.
     *                                If false and the file does not exist, the call fails.
     *
     * @return a proper NTSTATUS error code.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    static NTSTATUS
    XPF_API
    Create(
        _Out_ xpf::Optional<xpf::FileObject>* FileObj,
        _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
        _In_ bool ReadOnly,
        _In_ bool CreateIfNotFound
    ) noexcept(true);

    /**
     * @brief Reads data from the file at the specified offset.
     *
     * @param[out] Buffer           - The buffer to receive the read data.
     *
     * @param[in] BufferSize        - The maximum number of bytes to read.
     *
     * @param[in] Offset            - The byte offset in the file from which to begin reading.
     *
     * @param[out] BytesRead        - On return, contains the number of bytes actually read.
     *
     * @return a proper NTSTATUS error code.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    NTSTATUS
    XPF_API
    Read(
        _Out_ uint8_t* Buffer,
        _In_ size_t BufferSize,
        _In_ size_t Offset,
        _Out_ size_t* BytesRead
    ) noexcept(true);

    /**
     * @brief Writes data to the file at the specified offset.
     *
     * @param[in] Buffer            - The buffer containing the data to write.
     *
     * @param[in] BufferSize        - The number of bytes to write.
     *
     * @param[in] Offset            - The byte offset in the file at which to begin writing.
     *
     * @param[out] BytesWritten     - On return, contains the number of bytes actually written.
     *
     * @return a proper NTSTATUS error code.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    NTSTATUS
    XPF_API
    Write(
        _In_reads_bytes_(BufferSize) const uint8_t* Buffer,
        _In_ size_t BufferSize,
        _In_ size_t Offset,
        _Out_ size_t* BytesWritten
    ) noexcept(true);

 private:
    /**
     * @brief Private default constructor. Use Create() factory method to instantiate.
     */
    FileObject(
        void
    ) noexcept(true) = default;

    /**
     * @brief Indicates whether the file was opened for read-only access.
     */
    bool m_ReadOnly = true;

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        /**
         * @brief The NT file handle returned by ZwCreateFile.
         */
        HANDLE m_NtFileHandle = nullptr;
    #elif defined XPF_PLATFORM_LINUX_UM
        /**
         * @brief The POSIX file descriptor returned by open().
         */
        int m_FileDescriptor = -1;
    #else
        #error Unknown Platform
    #endif
};  // class FileObject


//
// ************************************************************************************************
// This is the section containing the FileOplock class definition.
// ************************************************************************************************
//

/**
 * @brief   Cross-platform opportunistic lock abstraction class.
 *          Provides a best-effort mechanism to prevent other processes
 *          from interfering with a file while we hold a handle open.
 *          If the filesystem does not support oplocks, the operation
 *          silently succeeds without an active oplock.
 *          Instances are created via the static Request() factory method.
 *          The destructor automatically releases the oplock.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_request_batch_oplock
 *          See: https://man7.org/linux/man-pages/man2/fcntl.2.html (F_SETLEASE section)
 */
class FileOplock
{
 public:
    /**
     * @brief Destructor. Releases the oplock and closes any associated handles.
     *        Must be called at PASSIVE_LEVEL in kernel mode.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    ~FileOplock(
        void
    ) noexcept(true);

    /**
     * @brief Move constructor. Transfers ownership of the oplock.
     *
     * @param[in,out] Other - The other oplock object to move from.
     *                        Will be invalidated after this call.
     */
    FileOplock(
        _Inout_ FileOplock&& Other
    ) noexcept(true);

    /**
     * @brief Move assignment. Transfers ownership of the oplock.
     *
     * @param[in,out] Other - The other oplock object to move from.
     *                        Will be invalidated after this call.
     *
     * @return A reference to *this object after move.
     */
    FileOplock&
    operator=(
        _Inout_ FileOplock&& Other
    ) noexcept(true);

    /**
     * @brief Copy constructor - deleted. FileOplock is move-only.
     */
    FileOplock(
        _In_ _Const_ const FileOplock& Other
    ) noexcept(true) = delete;

    /**
     * @brief Copy assignment - deleted. FileOplock is move-only.
     */
    FileOplock&
    operator=(
        _In_ _Const_ const FileOplock& Other
    ) noexcept(true) = delete;

    /**
     * @brief Requests an opportunistic lock on the specified file.
     *        This is a best-effort operation. If the filesystem does not
     *        support oplocks, the function still returns STATUS_SUCCESS
     *        but the oplock will simply not be active.
     *
     * @param[in] FilePath  - The path to the file. Same format as FileObject::Create.
     *
     * @param[out] Oplock   - On return, receives the FileOplock wrapped in Optional.
     *
     * @return STATUS_SUCCESS on success (oplock may or may not be active),
     *         STATUS_INSUFFICIENT_RESOURCES on allocation failure,
     *         STATUS_INVALID_PARAMETER on invalid arguments.
     */
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    static NTSTATUS
    XPF_API
    Request(
        _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
        _Out_ xpf::Optional<xpf::FileOplock>* Oplock
    ) noexcept(true);

 private:
    /**
     * @brief Private default constructor. Use Request() factory method to instantiate.
     */
    FileOplock(
        void
    ) noexcept(true) = default;

    /**
     * @brief Indicates whether the oplock was successfully acquired.
     */
    bool m_OplockActive = false;

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        /**
         * @brief The NT file handle opened for the oplock request.
         */
        HANDLE m_FileHandle = nullptr;

        /**
         * @brief The event handle used for the async oplock FSCTL.
         */
        HANDLE m_EventHandle = nullptr;

        /**
         * @brief The I/O status block for the pending oplock FSCTL.
         */
        IO_STATUS_BLOCK m_IoStatusBlock = { };
    #elif defined XPF_PLATFORM_LINUX_UM
        /**
         * @brief The file descriptor opened for the lease request.
         */
        int m_FileDescriptor = -1;
    #else
        #error Unknown Platform
    #endif
};  // class FileOplock

};  // namespace xpf
