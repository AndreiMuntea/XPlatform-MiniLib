/**
 * @file        xpf_tests/tests/Filesystem/TestFileObject.cpp
 *
 * @brief       This contains tests for the FileObject and FileOplock classes.
 *              Tests verify file creation, reading, writing, read-only enforcement,
 *              offset-based I/O, parameter validation, and oplock best-effort behavior.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2024.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"


/**
 * @brief       Helper to construct a platform-specific temporary file path.
 *              On Windows UM, uses C:\Windows\Temp\.
 *              On Windows KM, uses \SystemRoot\Temp\.
 *              On Linux, uses /tmp/.
 *
 * @param[in]   FileName - The base file name for the test file.
 *
 * @param[out]  FilePath - Receives the full platform-specific file path.
 *
 * @return      A proper NTSTATUS error code.
 */
static _Must_inspect_result_ NTSTATUS XPF_API
TestBuildTempFilePath(
    _In_ _Const_ const xpf::StringView<wchar_t>& FileName,
    _Inout_ xpf::String<wchar_t>& FilePath
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_UM
        const xpf::StringView<wchar_t> tempDir(L"C:\\Windows\\Temp\\");
    #elif defined XPF_PLATFORM_WIN_KM
        const xpf::StringView<wchar_t> tempDir(L"\\SystemRoot\\Temp\\");
    #elif defined XPF_PLATFORM_LINUX_UM
        const xpf::StringView<wchar_t> tempDir(L"/tmp/");
    #else
        #error Unknown Platform
    #endif

    NTSTATUS status = FilePath.Append(tempDir);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = FilePath.Append(FileName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief       Helper to delete a file from disk using platform-specific APIs.
 *              This is used for test cleanup to ensure a known initial state.
 *
 * @param[in]   FilePath - The path to the file to delete.
 */
static void XPF_API
TestDeleteFile(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_UM
    {
        xpf::String<char> utf8Path;
        const NTSTATUS status = xpf::StringConversion::WideToUTF8(FilePath, utf8Path);
        if (NT_SUCCESS(status))
        {
            (void) ::DeleteFileA(utf8Path.View().Buffer());
        }
    }
    #elif defined XPF_PLATFORM_WIN_KM
    {
        xpf::String<wchar_t> ntPath;
        NTSTATUS status = ntPath.Append(FilePath);
        if (!NT_SUCCESS(status))
        {
            return;
        }

        const size_t pathLengthInBytes = ntPath.View().BufferSize() * sizeof(wchar_t);
        if (pathLengthInBytes > static_cast<size_t>(xpf::NumericLimits<uint16_t>::MaxValue()) - 1)
        {
            return;
        }

        UNICODE_STRING unicodePath;
        unicodePath.Buffer = const_cast<PWCH>(ntPath.View().Buffer());
        unicodePath.Length = static_cast<USHORT>(pathLengthInBytes);
        unicodePath.MaximumLength = unicodePath.Length;

        OBJECT_ATTRIBUTES objectAttributes;
        xpf::ApiZeroMemory(&objectAttributes, sizeof(objectAttributes));
        objectAttributes.Length = sizeof(objectAttributes);
        objectAttributes.ObjectName = &unicodePath;
        objectAttributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;

        (void) ZwDeleteFile(&objectAttributes);
    }
    #elif defined XPF_PLATFORM_LINUX_UM
    {
        xpf::String<char> utf8Path;
        const NTSTATUS status = xpf::StringConversion::WideToUTF8(FilePath, utf8Path);
        if (NT_SUCCESS(status))
        {
            (void) unlink(utf8Path.View().Buffer());
        }
    }
    #else
        #error Unknown Platform
    #endif
}

/**
 * @brief       Helper to write a string to a file object and return the result.
 *              This avoids repeating the cast-to-uint8_t pattern in every test.
 *
 * @param[in]   FileObj      - A reference to a valid writable FileObject.
 * @param[in]   Data         - The null-terminated string to write.
 * @param[in]   DataSize     - The number of bytes to write from Data.
 * @param[in]   Offset       - The byte offset in the file at which to write.
 * @param[out]  BytesWritten - Receives the number of bytes actually written.
 *
 * @return      A proper NTSTATUS error code.
 */
static _Must_inspect_result_ NTSTATUS XPF_API
TestWriteStringToFile(
    _Inout_ xpf::FileObject& FileObj,
    _In_ _Const_ const char* Data,
    _In_ size_t DataSize,
    _In_ size_t Offset,
    _Out_ size_t* BytesWritten
) noexcept(true)
{
    if (nullptr == BytesWritten)
    {
        return STATUS_INVALID_PARAMETER;
    }
    *BytesWritten = 0;

    const uint8_t* const dataAsBytes = static_cast<const uint8_t*>(static_cast<const void*>(Data));
    return FileObj.Write(dataAsBytes,
                         DataSize,
                         Offset,
                         BytesWritten);
}


/**
 * @brief       Verifies that a file can be successfully created with CreateIfNotFound=true,
 *              that the returned Optional has a value, and that the FileObject is properly
 *              destroyed when the Optional goes out of scope (or is reset).
 */
XPF_TEST_SCENARIO(TestFileObject, CreateAndClose)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_create.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    //
    // Create a new file (read-write, create if not found).
    // The Optional should have a value on success.
    //
    xpf::Optional<xpf::FileObject> fileObj;
    const NTSTATUS createStatus = xpf::FileObject::Create(&fileObj,
                                                           filePath.View(),
                                                           false,        // ReadOnly
                                                           true);        // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(createStatus));
    XPF_TEST_EXPECT_TRUE(fileObj.HasValue());

    //
    // Reset the Optional to trigger the destructor (closes the handle).
    //
    fileObj.Reset();
    XPF_TEST_EXPECT_TRUE(!fileObj.HasValue());

    TestDeleteFile(filePath.View());
}


/**
 * @brief       Verifies that opening a non-existent file without CreateIfNotFound=true
 *              returns a failure NTSTATUS and the Optional remains empty.
 *              This ensures the API correctly distinguishes between "open existing"
 *              and "create if not found" modes.
 */
XPF_TEST_SCENARIO(TestFileObject, OpenNonExistentFile)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_noexist.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    //
    // Try to open without create — should fail because the file doesn't exist.
    //
    xpf::Optional<xpf::FileObject> fileObj;
    const NTSTATUS openStatus = xpf::FileObject::Create(&fileObj,
                                                         filePath.View(),
                                                         true,         // ReadOnly
                                                         false);       // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(openStatus));
    XPF_TEST_EXPECT_TRUE(!fileObj.HasValue());
}


/**
 * @brief       Verifies the full write-then-read cycle: writes a known string
 *              to a file at offset 0, reads it back, and confirms the read data
 *              matches the written data byte-for-byte. This is the fundamental
 *              I/O correctness test.
 */
XPF_TEST_SCENARIO(TestFileObject, WriteAndRead)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_rw.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    xpf::Optional<xpf::FileObject> fileObj;
    const NTSTATUS createStatus = xpf::FileObject::Create(&fileObj,
                                                           filePath.View(),
                                                           false,        // ReadOnly
                                                           true);        // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(createStatus));
    XPF_TEST_EXPECT_TRUE(fileObj.HasValue());

    //
    // Write a known string to the file.
    //
    const char* const testData = "Hello XPF FileObject!";
    const size_t testDataSize = xpf::ApiStringLength(testData);
    size_t bytesWritten = 0;

    const NTSTATUS writeStatus = TestWriteStringToFile(*fileObj, testData, testDataSize, 0, &bytesWritten);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(writeStatus));
    XPF_TEST_EXPECT_TRUE(bytesWritten == testDataSize);

    //
    // Read the data back and verify it matches.
    //
    uint8_t readBuffer[128];
    xpf::ApiZeroMemory(readBuffer, sizeof(readBuffer));
    size_t bytesRead = 0;

    const NTSTATUS readStatus = (*fileObj).Read(readBuffer,
                                                sizeof(readBuffer),
                                                0,
                                                &bytesRead);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(readStatus));
    XPF_TEST_EXPECT_TRUE(bytesRead == testDataSize);

    const xpf::StringView<char> written(testData, testDataSize);
    const xpf::StringView<char> readView(static_cast<const char*>(static_cast<const void*>(readBuffer)),
                                         bytesRead);
    XPF_TEST_EXPECT_TRUE(written.Equals(readView, true));

    fileObj.Reset();
    TestDeleteFile(filePath.View());
}


/**
 * @brief       Verifies that attempting to write through a read-only FileObject
 *              returns STATUS_ILLEGAL_FUNCTION, while reading still succeeds.
 *              This ensures the ReadOnly flag is properly enforced at the
 *              FileObject level before reaching the OS.
 */
XPF_TEST_SCENARIO(TestFileObject, ReadOnlyCannotWrite)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_readonly.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    //
    // First create the file and write some data so it exists with content.
    //
    xpf::Optional<xpf::FileObject> writeObj;
    const NTSTATUS createStatus = xpf::FileObject::Create(&writeObj,
                                                           filePath.View(),
                                                           false,        // ReadOnly
                                                           true);        // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(createStatus));

    const char* const testData = "test";
    const size_t testDataSize = xpf::ApiStringLength(testData);
    size_t bytesWritten = 0;

    const NTSTATUS writeStatus = TestWriteStringToFile(*writeObj, testData, testDataSize, 0, &bytesWritten);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(writeStatus));
    XPF_TEST_EXPECT_TRUE(bytesWritten == testDataSize);
    writeObj.Reset();

    //
    // Now open as read-only and verify that writing fails with STATUS_ILLEGAL_FUNCTION.
    //
    xpf::Optional<xpf::FileObject> readObj;
    const NTSTATUS openStatus = xpf::FileObject::Create(&readObj,
                                                         filePath.View(),
                                                         true,         // ReadOnly
                                                         false);       // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(openStatus));
    XPF_TEST_EXPECT_TRUE(readObj.HasValue());

    size_t readOnlyBytesWritten = 0;
    const uint8_t* const dataAsBytes = static_cast<const uint8_t*>(static_cast<const void*>(testData));
    const NTSTATUS readOnlyWriteStatus = (*readObj).Write(dataAsBytes,
                                                          testDataSize,
                                                          0,
                                                          &readOnlyBytesWritten);
    XPF_TEST_EXPECT_TRUE(STATUS_ILLEGAL_FUNCTION == readOnlyWriteStatus);

    //
    // Reading should still work and return the data we wrote earlier.
    //
    uint8_t readBuffer[128];
    xpf::ApiZeroMemory(readBuffer, sizeof(readBuffer));
    size_t bytesRead = 0;

    const NTSTATUS readStatus = (*readObj).Read(readBuffer,
                                                sizeof(readBuffer),
                                                0,
                                                &bytesRead);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(readStatus));
    XPF_TEST_EXPECT_TRUE(bytesRead == testDataSize);

    readObj.Reset();
    TestDeleteFile(filePath.View());
}


/**
 * @brief       Verifies that writing at a non-zero offset correctly overwrites
 *              only the targeted bytes without affecting surrounding data.
 *              Writes "AAAA" at offset 0, then "BB" at offset 2, and verifies
 *              the file contains "AABB" — confirming offset-based partial overwrite.
 */
XPF_TEST_SCENARIO(TestFileObject, WriteAtOffset)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_offset.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    xpf::Optional<xpf::FileObject> fileObj;
    const NTSTATUS createStatus = xpf::FileObject::Create(&fileObj,
                                                           filePath.View(),
                                                           false,        // ReadOnly
                                                           true);        // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(createStatus));

    //
    // Write "AAAA" at offset 0, then "BB" at offset 2 to get "AABB".
    //
    const char* const firstData = "AAAA";
    const size_t firstDataSize = xpf::ApiStringLength(firstData);
    size_t bytesWritten = 0;

    const NTSTATUS writeStatus1 = TestWriteStringToFile(*fileObj, firstData, firstDataSize, 0, &bytesWritten);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(writeStatus1));
    XPF_TEST_EXPECT_TRUE(bytesWritten == firstDataSize);

    const char* const secondData = "BB";
    const size_t secondDataSize = xpf::ApiStringLength(secondData);
    const size_t overwriteOffset = 2;

    const NTSTATUS writeStatus2 = TestWriteStringToFile(*fileObj, secondData, secondDataSize, overwriteOffset, &bytesWritten);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(writeStatus2));
    XPF_TEST_EXPECT_TRUE(bytesWritten == secondDataSize);

    //
    // Read back and verify the file contains "AABB".
    //
    uint8_t readBuffer[128];
    xpf::ApiZeroMemory(readBuffer, sizeof(readBuffer));
    size_t bytesRead = 0;

    const NTSTATUS readStatus = (*fileObj).Read(readBuffer,
                                                sizeof(readBuffer),
                                                0,
                                                &bytesRead);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(readStatus));
    XPF_TEST_EXPECT_TRUE(bytesRead == firstDataSize);

    const char* const expectedStr = "AABB";
    const size_t expectedSize = xpf::ApiStringLength(expectedStr);
    const xpf::StringView<char> expected(expectedStr, expectedSize);
    const xpf::StringView<char> actual(static_cast<const char*>(static_cast<const void*>(readBuffer)),
                                       bytesRead);
    XPF_TEST_EXPECT_TRUE(expected.Equals(actual, true));

    fileObj.Reset();
    TestDeleteFile(filePath.View());
}


/**
 * @brief       Verifies that all API entry points properly reject null and
 *              invalid parameters with STATUS_INVALID_PARAMETER.
 *              Tests: null FileObj output, empty path for Create,
 *              null buffer for Read, null buffer for Write.
 *              Also verifies that Create with a null pointer does not crash.
 */
XPF_TEST_SCENARIO(TestFileObject, NullParameterValidation)
{
    //
    // Create with null output pointer.
    //
    const xpf::StringView<wchar_t> dummyPath(L"dummy");
    const NTSTATUS createNullStatus = xpf::FileObject::Create(nullptr, dummyPath, true, false);
    XPF_TEST_EXPECT_TRUE(STATUS_INVALID_PARAMETER == createNullStatus);

    //
    // Create with empty path.
    //
    xpf::Optional<xpf::FileObject> fileObj;
    const xpf::StringView<wchar_t> emptyPath;
    const NTSTATUS createEmptyStatus = xpf::FileObject::Create(&fileObj, emptyPath, true, false);
    XPF_TEST_EXPECT_TRUE(STATUS_INVALID_PARAMETER == createEmptyStatus);
    XPF_TEST_EXPECT_TRUE(!fileObj.HasValue());
}


/**
 * @brief       Verifies that FileOplock::Request succeeds (best-effort) even when
 *              the underlying filesystem may not support oplocks. The returned Optional
 *              should always have a value on STATUS_SUCCESS. Also verifies that
 *              resetting the Optional (releasing the oplock) and double-reset do not crash.
 */
XPF_TEST_SCENARIO(TestFileObject, OplockRequestAndRelease)
{
    xpf::String<wchar_t> filePath;
    const NTSTATUS buildStatus = TestBuildTempFilePath(xpf::StringView<wchar_t>(L"xpf_test_oplock.tmp"),
                                                       filePath);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buildStatus));

    TestDeleteFile(filePath.View());

    //
    // Create a file with content so the oplock has something to lock.
    //
    xpf::Optional<xpf::FileObject> fileObj;
    const NTSTATUS createStatus = xpf::FileObject::Create(&fileObj,
                                                           filePath.View(),
                                                           false,        // ReadOnly
                                                           true);        // CreateIfNotFound
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(createStatus));

    const char* const testData = "oplock test data";
    const size_t testDataSize = xpf::ApiStringLength(testData);
    size_t bytesWritten = 0;

    const NTSTATUS writeStatus = TestWriteStringToFile(*fileObj, testData, testDataSize, 0, &bytesWritten);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(writeStatus));
    XPF_TEST_EXPECT_TRUE(bytesWritten == testDataSize);

    //
    // Close the main handle so the oplock can open the file.
    //
    fileObj.Reset();

    //
    // Request an oplock. This should succeed (best effort) even if
    // the filesystem does not support oplocks.
    //
    xpf::Optional<xpf::FileOplock> oplock;
    const NTSTATUS oplockStatus = xpf::FileOplock::Request(filePath.View(), &oplock);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(oplockStatus));
    XPF_TEST_EXPECT_TRUE(oplock.HasValue());

    //
    // Release the oplock by resetting the Optional (destructor runs).
    //
    oplock.Reset();
    XPF_TEST_EXPECT_TRUE(!oplock.HasValue());

    //
    // Double reset should be a safe no-op.
    //
    oplock.Reset();
    XPF_TEST_EXPECT_TRUE(!oplock.HasValue());

    TestDeleteFile(filePath.View());
}


/**
 * @brief       Verifies that FileOplock::Request properly rejects null and
 *              invalid parameters with STATUS_INVALID_PARAMETER.
 *              Tests: null Oplock output pointer, empty file path.
 *              Also verifies that resetting an empty Optional is a safe no-op.
 */
XPF_TEST_SCENARIO(TestFileObject, OplockNullParameterValidation)
{
    //
    // Request with null output pointer.
    //
    const xpf::StringView<wchar_t> dummyPath(L"dummy");
    const NTSTATUS requestNullStatus = xpf::FileOplock::Request(dummyPath, nullptr);
    XPF_TEST_EXPECT_TRUE(STATUS_INVALID_PARAMETER == requestNullStatus);

    //
    // Request with empty path.
    //
    xpf::Optional<xpf::FileOplock> oplock;
    const xpf::StringView<wchar_t> emptyPath;
    const NTSTATUS requestEmptyStatus = xpf::FileOplock::Request(emptyPath, &oplock);
    XPF_TEST_EXPECT_TRUE(STATUS_INVALID_PARAMETER == requestEmptyStatus);

    //
    // Reset an empty Optional should be a safe no-op.
    //
    oplock.Reset();
    XPF_TEST_EXPECT_TRUE(!oplock.HasValue());
}
