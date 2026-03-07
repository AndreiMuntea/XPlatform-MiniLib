/**
 * @file        xpf_tests/tests/Utility/TestPeFileParser.cpp
 *
 * @brief       This contains tests for PE file parser.
 *              Tests cover valid PE32/PE32+ headers, corrupted buffers,
 *              truncated inputs, misleading sizes, boundary conditions,
 *              and arithmetic overflow scenarios.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"


/* ============================================================================================= */
/* ===                          Helper: Platform Temp Path & Delete                          === */
/* ============================================================================================= */

/**
 * @brief       Builds a full platform-specific temp file path from a base file name.
 *
 * @param[in]   FileName - The base file name for the test file.
 * @param[out]  FilePath - Receives the full platform-specific file path.
 *
 * @return      A proper NTSTATUS error code.
 */
static _Must_inspect_result_ NTSTATUS XPF_API
TestPeBuildTempFilePath(
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
 *
 * @param[in]   FilePath - The path to the file to delete.
 */
static void XPF_API
TestPeDeleteFile(
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
 * @brief       Helper to write a buffer to a temp file.
 *
 * @param[in]   FilePath    - The path to write to.
 * @param[in]   Data        - The data to write.
 * @param[in]   DataSize    - The number of bytes to write.
 *
 * @return      A proper NTSTATUS error code.
 */
static _Must_inspect_result_ NTSTATUS XPF_API
TestPeWriteBufferToFile(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _In_ const void* Data,
    _In_ size_t DataSize
) noexcept(true)
{
    xpf::Optional<xpf::FileObject> fileObj;
    NTSTATUS status = xpf::FileObject::Create(&fileObj,
                                              FilePath,
                                              false,        /* ReadOnly */
                                              true);        /* CreateIfNotFound */
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    size_t bytesWritten = 0;
    status = (*fileObj).Write(static_cast<const uint8_t*>(Data),
                             DataSize,
                             0,
                             &bytesWritten);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (bytesWritten != DataSize)
    {
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}


/**
 * @brief       Constructs a minimal valid PE32 (32-bit) image in a buffer.
 *              Layout:
 *                  - DOS header at offset 0
 *                  - PE signature at offset 0x40 (64 bytes = sizeof(DosHeader))
 *                  - COFF header at offset 0x44
 *                  - PE32 Optional header at offset 0x58
 *                  - 1 data directory at end of optional header
 *                  - 1 section header (.text) after optional header
 *
 * @param[out]  Buffer          - On success, contains the PE32 image data.
 * @param[in]   NumberOfSections - Number of section headers to include.
 *                                 Section data is zeroed (only headers are valid).
 *
 * @return      A proper NTSTATUS error code.
 */
static NTSTATUS
BuildMinimalPe32(
    _Out_ xpf::Buffer* Buffer,
    _In_ uint16_t NumberOfSections
) noexcept(true)
{
    /* Compute the total size needed. */
    const size_t dosHeaderSize = sizeof(xpf::pe::DosHeader);
    const size_t peSignatureSize = sizeof(uint32_t);
    const size_t coffHeaderSize = sizeof(xpf::pe::CoffHeader);
    const size_t optHeaderSize = sizeof(xpf::pe::OptionalHeader32);
    const size_t dataDirSize = sizeof(xpf::pe::DataDirectory);
    const size_t sectionHeaderSize = sizeof(xpf::pe::SectionHeader);

    const size_t headersSize = dosHeaderSize + peSignatureSize + coffHeaderSize
                             + optHeaderSize + dataDirSize
                             + (sectionHeaderSize * NumberOfSections);

    /* Round up to 512 (typical FileAlignment) for realism. */
    const size_t totalSize = (headersSize + 511) & ~size_t{ 511 };

    NTSTATUS status = Buffer->Resize(totalSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    xpf::ApiZeroMemory(Buffer->GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(Buffer->GetBuffer());
    size_t offset = 0;

    /* ---- DOS Header ---- */
    xpf::pe::DosHeader* dos = reinterpret_cast<xpf::pe::DosHeader*>(base + offset);
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(dosHeaderSize);
    offset += dosHeaderSize;

    /* ---- PE Signature ---- */
    uint32_t* peSig = reinterpret_cast<uint32_t*>(base + offset);
    *peSig = PE_NT_SIGNATURE;
    offset += peSignatureSize;

    /* ---- COFF Header ---- */
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + offset);
    coff->Machine = uint16_t{ 0x014C };           /* IMAGE_FILE_MACHINE_I386 */
    coff->NumberOfSections = NumberOfSections;
    coff->SizeOfOptionalHeader = static_cast<uint16_t>(optHeaderSize + dataDirSize);
    offset += coffHeaderSize;

    /* ---- PE32 Optional Header ---- */
    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + offset);
    opt->Magic = PE_OPTIONAL_HDR32_MAGIC;
    opt->ImageBase = uint32_t{ 0x00400000 };
    opt->SectionAlignment = uint32_t{ 0x1000 };
    opt->FileAlignment = uint32_t{ 0x200 };
    opt->SizeOfHeaders = static_cast<uint32_t>(totalSize);
    opt->NumberOfRvaAndSizes = 1;
    offset += optHeaderSize;

    /* ---- Data Directory (1 entry, zeroed) ---- */
    offset += dataDirSize;

    /* ---- Section Header(s) ---- */
    for (uint16_t i = 0; i < NumberOfSections; ++i)
    {
        xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + offset);
        section->Name[0] = '.';
        section->Name[1] = 't';
        section->Name[2] = 'x';
        section->Name[3] = 't';
        section->Misc.VirtualSize = uint32_t{ 0x1000 };
        section->VirtualAddress = uint32_t{ 0x1000 } * (static_cast<uint32_t>(i) + 1);
        section->SizeOfRawData = 0;
        section->PointerToRawData = 0;
        section->Characteristics = uint32_t{ 0x60000020 };   /* CODE | EXECUTE | READ */
        offset += sectionHeaderSize;
    }

    return STATUS_SUCCESS;
}


/**
 * @brief       Constructs a minimal valid PE32+ (64-bit) image in a buffer.
 *              Same layout as BuildMinimalPe32 but with PE32+ optional header.
 *
 * @param[out]  Buffer          - On success, contains the PE32+ image data.
 * @param[in]   NumberOfSections - Number of section headers to include.
 *
 * @return      A proper NTSTATUS error code.
 */
static NTSTATUS
BuildMinimalPe64(
    _Out_ xpf::Buffer* Buffer,
    _In_ uint16_t NumberOfSections
) noexcept(true)
{
    const size_t dosHeaderSize = sizeof(xpf::pe::DosHeader);
    const size_t peSignatureSize = sizeof(uint32_t);
    const size_t coffHeaderSize = sizeof(xpf::pe::CoffHeader);
    const size_t optHeaderSize = sizeof(xpf::pe::OptionalHeader64);
    const size_t dataDirSize = sizeof(xpf::pe::DataDirectory);
    const size_t sectionHeaderSize = sizeof(xpf::pe::SectionHeader);

    const size_t headersSize = dosHeaderSize + peSignatureSize + coffHeaderSize
                             + optHeaderSize + dataDirSize
                             + (sectionHeaderSize * NumberOfSections);
    const size_t totalSize = (headersSize + 511) & ~size_t{ 511 };

    NTSTATUS status = Buffer->Resize(totalSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    xpf::ApiZeroMemory(Buffer->GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(Buffer->GetBuffer());
    size_t offset = 0;

    /* ---- DOS Header ---- */
    xpf::pe::DosHeader* dos = reinterpret_cast<xpf::pe::DosHeader*>(base + offset);
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(dosHeaderSize);
    offset += dosHeaderSize;

    /* ---- PE Signature ---- */
    uint32_t* peSig = reinterpret_cast<uint32_t*>(base + offset);
    *peSig = PE_NT_SIGNATURE;
    offset += peSignatureSize;

    /* ---- COFF Header ---- */
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + offset);
    coff->Machine = uint16_t{ 0x8664 };           /* IMAGE_FILE_MACHINE_AMD64 */
    coff->NumberOfSections = NumberOfSections;
    coff->SizeOfOptionalHeader = static_cast<uint16_t>(optHeaderSize + dataDirSize);
    offset += coffHeaderSize;

    /* ---- PE32+ Optional Header ---- */
    xpf::pe::OptionalHeader64* opt = reinterpret_cast<xpf::pe::OptionalHeader64*>(base + offset);
    opt->Magic = PE_OPTIONAL_HDR64_MAGIC;
    opt->ImageBase = uint64_t{ 0x0000000140000000 };
    opt->SectionAlignment = uint32_t{ 0x1000 };
    opt->FileAlignment = uint32_t{ 0x200 };
    opt->SizeOfHeaders = static_cast<uint32_t>(totalSize);
    opt->NumberOfRvaAndSizes = 1;
    offset += optHeaderSize;

    /* ---- Data Directory (1 entry, zeroed) ---- */
    offset += dataDirSize;

    /* ---- Section Header(s) ---- */
    for (uint16_t i = 0; i < NumberOfSections; ++i)
    {
        xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + offset);
        section->Name[0] = '.';
        section->Name[1] = 't';
        section->Name[2] = 'x';
        section->Name[3] = 't';
        section->Misc.VirtualSize = uint32_t{ 0x1000 };
        section->VirtualAddress = uint32_t{ 0x1000 } * (static_cast<uint32_t>(i) + 1);
        section->SizeOfRawData = 0;
        section->PointerToRawData = 0;
        section->Characteristics = uint32_t{ 0x60000020 };   /* CODE | EXECUTE | READ */
        offset += sectionHeaderSize;
    }

    return STATUS_SUCCESS;
}


/* ============================================================================================= */
/* ===                                  Empty / Null Input Tests                             === */
/* ============================================================================================= */

/**
 * @brief       An empty buffer (size 0) should fail with STATUS_DATA_ERROR.
 */
XPF_TEST_SCENARIO(TestPeParser, EmptyBuffer)
{
    xpf::Buffer buffer;
    xpf::pe::PeInformation peInfo;

    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       A buffer with a single byte should fail (too small for DOS header).
 */
XPF_TEST_SCENARIO(TestPeParser, SingleByteBuffer)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(1)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), 1);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}


/* ============================================================================================= */
/* ===                              DOS Header Validation Tests                              === */
/* ============================================================================================= */

/**
 * @brief       Buffer exactly sizeof(DosHeader) but with wrong magic should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, InvalidDosMagic)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(sizeof(xpf::pe::DosHeader))));
    xpf::ApiZeroMemory(buffer.GetBuffer(), buffer.GetSize());

    /* Set wrong magic. */
    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = uint16_t{ 0x1234 };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid DOS magic but e_lfanew is negative should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, NegativeELfanew)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(sizeof(xpf::pe::DosHeader))));
    xpf::ApiZeroMemory(buffer.GetBuffer(), buffer.GetSize());

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = -1;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid DOS magic but e_lfanew points beyond the file should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, ELfanewBeyondFile)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(sizeof(xpf::pe::DosHeader))));
    xpf::ApiZeroMemory(buffer.GetBuffer(), buffer.GetSize());

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = 0x1000;    /* Way beyond our tiny buffer. */

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid DOS header but wrong PE signature at e_lfanew should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, InvalidPeSignature)
{
    xpf::Buffer buffer;
    const size_t totalSize = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(sizeof(xpf::pe::DosHeader));

    /* Write garbage instead of 'PE\0\0'. */
    uint32_t* sig = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(buffer.GetBuffer())
                                                + sizeof(xpf::pe::DosHeader));
    *sig = uint32_t{ 0xDEADBEEF };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}


/* ============================================================================================= */
/* ===                             COFF Header Validation Tests                              === */
/* ============================================================================================= */

/**
 * @brief       Valid DOS+PE signature but truncated before COFF header ends should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, TruncatedCoffHeader)
{
    /* Only room for DOS header + PE signature + partial COFF header. */
    const size_t totalSize = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t) + 4;
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());

    xpf::pe::DosHeader* dos = reinterpret_cast<xpf::pe::DosHeader*>(base);
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(sizeof(xpf::pe::DosHeader));

    uint32_t* sig = reinterpret_cast<uint32_t*>(base + sizeof(xpf::pe::DosHeader));
    *sig = PE_NT_SIGNATURE;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       COFF header with SizeOfOptionalHeader=0 (too small for magic) should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, ZeroSizeOfOptionalHeader)
{
    const size_t totalSize = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());

    xpf::pe::DosHeader* dos = reinterpret_cast<xpf::pe::DosHeader*>(base);
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(sizeof(xpf::pe::DosHeader));

    uint32_t* sig = reinterpret_cast<uint32_t*>(base + sizeof(xpf::pe::DosHeader));
    *sig = PE_NT_SIGNATURE;

    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(
                                 base + sizeof(xpf::pe::DosHeader) + sizeof(uint32_t));
    coff->SizeOfOptionalHeader = 0;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       COFF header claiming too many sections (above PE_MAX_SECTIONS) should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, TooManySections)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    /* Patch NumberOfSections to an absurd value. */
    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);
    coff->NumberOfSections = uint16_t{ 0xFFFF };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       SizeOfOptionalHeader pointing beyond file end should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, SizeOfOptionalHeaderBeyondFile)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);
    coff->SizeOfOptionalHeader = uint16_t{ 0xFFF0 };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}


/* ============================================================================================= */
/* ===                          Optional Header Validation Tests                             === */
/* ============================================================================================= */

/**
 * @brief       Unknown optional header magic (not PE32 or PE32+) should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, UnknownOptionalHeaderMagic)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    /* Patch the magic to an invalid value. */
    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeaderCommon* opt = reinterpret_cast<xpf::pe::OptionalHeaderCommon*>(
                                         base + optOffset);
    opt->Magic = uint16_t{ 0xBEEF };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       PE32 with SizeOfOptionalHeader too small for OptionalHeader32 should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, Pe32OptionalHeaderTooSmall)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);

    /* Make SizeOfOptionalHeader just big enough for the common header but not the full PE32 header. */
    coff->SizeOfOptionalHeader = sizeof(xpf::pe::OptionalHeaderCommon);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       SizeOfHeaders exceeding file size should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, SizeOfHeadersBeyondFile)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + optOffset);
    opt->SizeOfHeaders = uint32_t{ 0xFFFFFFFF };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       NumberOfRvaAndSizes set to an absurd value (> PE_MAX_DATA_DIRECTORIES)
 *              should be capped to PE_MAX_DATA_DIRECTORIES (16) and the parse should
 *              succeed if the file has sufficient room for 16 entries.
 */
XPF_TEST_SCENARIO(TestPeParser, ExcessiveDataDirectoriesCapped)
{
    /*
     * Build a buffer large enough to hold 16 data directories.
     * We use BuildMinimalPe32 then enlarge SizeOfOptionalHeader to accommodate 16 entries.
     */
    const size_t dosHeaderSize = sizeof(xpf::pe::DosHeader);
    const size_t peSignatureSize = sizeof(uint32_t);
    const size_t coffHeaderSize = sizeof(xpf::pe::CoffHeader);
    const size_t optHeaderSize = sizeof(xpf::pe::OptionalHeader32);
    const size_t dataDirSize = sizeof(xpf::pe::DataDirectory) * PE_MAX_DATA_DIRECTORIES;
    const size_t sectionHeaderSize = sizeof(xpf::pe::SectionHeader);

    const size_t headersSize = dosHeaderSize + peSignatureSize + coffHeaderSize
                             + optHeaderSize + dataDirSize + sectionHeaderSize;
    const size_t totalSize = (headersSize + 511) & ~size_t{ 511 };

    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    size_t offset = 0;

    xpf::pe::DosHeader* dos = reinterpret_cast<xpf::pe::DosHeader*>(base);
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(dosHeaderSize);
    offset = dosHeaderSize;

    uint32_t* peSig = reinterpret_cast<uint32_t*>(base + offset);
    *peSig = PE_NT_SIGNATURE;
    offset += peSignatureSize;

    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + offset);
    coff->Machine = uint16_t{ 0x014C };
    coff->NumberOfSections = 1;
    coff->SizeOfOptionalHeader = static_cast<uint16_t>(optHeaderSize + dataDirSize);
    offset += coffHeaderSize;

    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + offset);
    opt->Magic = PE_OPTIONAL_HDR32_MAGIC;
    opt->ImageBase = uint32_t{ 0x00400000 };
    opt->SizeOfHeaders = static_cast<uint32_t>(totalSize);
    opt->NumberOfRvaAndSizes = uint32_t{ 0x7FFFFFFF };   /* Absurd value, should be capped. */
    offset += optHeaderSize + dataDirSize;

    xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + offset);
    section->Name[0] = '.';
    section->Name[1] = 't';
    section->Name[2] = 'x';
    section->Name[3] = 't';
    section->Misc.VirtualSize = uint32_t{ 0x1000 };
    section->VirtualAddress = uint32_t{ 0x1000 };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);

    /* Should succeed with capped data directory count. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfDataDirectories == PE_MAX_DATA_DIRECTORIES);
}


/* ============================================================================================= */
/* ===                           Section Header Validation Tests                             === */
/* ============================================================================================= */

/**
 * @brief       A section with SizeOfRawData + PointerToRawData overflowing should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, SectionRawDataOverflow)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());

    /* Find the section header. */
    const size_t sectionOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                               + sizeof(xpf::pe::CoffHeader) + sizeof(xpf::pe::OptionalHeader32)
                               + sizeof(xpf::pe::DataDirectory);
    xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + sectionOffset);
    section->PointerToRawData = xpf::NumericLimits<uint32_t>::MaxValue();
    section->SizeOfRawData = uint32_t{ 0x100 };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       A section with SizeOfRawData extending beyond file end should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, SectionRawDataBeyondFile)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());

    const size_t sectionOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                               + sizeof(xpf::pe::CoffHeader) + sizeof(xpf::pe::OptionalHeader32)
                               + sizeof(xpf::pe::DataDirectory);
    xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + sectionOffset);
    section->PointerToRawData = uint32_t{ 0 };
    section->SizeOfRawData = static_cast<uint32_t>(buffer.GetSize() + 1);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       A section with VirtualAddress + VirtualSize overflowing uint32_t should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, SectionVirtualAddressOverflow)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());

    const size_t sectionOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                               + sizeof(xpf::pe::CoffHeader) + sizeof(xpf::pe::OptionalHeader32)
                               + sizeof(xpf::pe::DataDirectory);
    xpf::pe::SectionHeader* section = reinterpret_cast<xpf::pe::SectionHeader*>(base + sectionOffset);
    section->VirtualAddress = xpf::NumericLimits<uint32_t>::MaxValue();
    section->Misc.VirtualSize = uint32_t{ 1 };

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       A section with zero SizeOfRawData is valid (uninitialized data).
 */
XPF_TEST_SCENARIO(TestPeParser, SectionWithZeroRawData)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    /* The default BuildMinimalPe32 has SizeOfRawData=0, so this should succeed. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 1);
}


/* ============================================================================================= */
/* ===                              Valid PE32 (32-bit) Tests                                 === */
/* ============================================================================================= */

/**
 * @brief       A minimal valid PE32 with 1 section should parse successfully
 *              and extract correct field values.
 */
XPF_TEST_SCENARIO(TestPeParser, ValidPe32SingleSection)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    /* Verify extracted fields. */
    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x014C });      /* IMAGE_FILE_MACHINE_I386 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfDataDirectories == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x00400000 });
    XPF_TEST_EXPECT_TRUE(peInfo.m_SectionHeadersOffset != 0);
    XPF_TEST_EXPECT_TRUE(peInfo.m_DataDirectoryOffset != 0);
    XPF_TEST_EXPECT_TRUE(peInfo.m_SizeOfHeaders > 0);
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() > 0);
}

/**
 * @brief       A valid PE32 with 0 sections should parse successfully.
 *              (Rare but technically allowed by the COFF spec for object files.)
 */
XPF_TEST_SCENARIO(TestPeParser, ValidPe32ZeroSections)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 0)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 0);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
}

/**
 * @brief       A valid PE32 with multiple sections should parse successfully.
 */
XPF_TEST_SCENARIO(TestPeParser, ValidPe32MultipleSections)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 5)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 5);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x014C });
}


/* ============================================================================================= */
/* ===                             Valid PE32+ (64-bit) Tests                                 === */
/* ============================================================================================= */

/**
 * @brief       A minimal valid PE32+ with 1 section should parse successfully
 *              and correctly identify it as 64-bit.
 */
XPF_TEST_SCENARIO(TestPeParser, ValidPe64SingleSection)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 1)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == true);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x8664 });      /* IMAGE_FILE_MACHINE_AMD64 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfDataDirectories == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x0000000140000000 });
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() > 0);
}

/**
 * @brief       PE32+ with multiple sections should parse correctly.
 */
XPF_TEST_SCENARIO(TestPeParser, ValidPe64MultipleSections)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 3)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == true);
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 3);
}


/* ============================================================================================= */
/* ===                       Crafted Buffer / Stress / Edge Case Tests                       === */
/* ============================================================================================= */

/**
 * @brief       e_lfanew pointing to the very last byte of the file should fail
 *              because there's no room for the 4-byte PE signature.
 */
XPF_TEST_SCENARIO(TestPeParser, ELfanewAtLastByte)
{
    xpf::Buffer buffer;
    const size_t totalSize = sizeof(xpf::pe::DosHeader) + 1;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(totalSize - 1);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       e_lfanew pointing back to offset 0 (self-referencing) should fail
 *              because the PE signature at offset 0 would read 'MZ' not 'PE\0\0'.
 */
XPF_TEST_SCENARIO(TestPeParser, ELfanewSelfReference)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_lfanew = 0;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       e_lfanew equal to INT32_MAX should fail (overflow when computing PE sig end).
 */
XPF_TEST_SCENARIO(TestPeParser, ELfanewIntMaxOverflow)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(sizeof(xpf::pe::DosHeader) + 16)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), buffer.GetSize());

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = xpf::NumericLimits<int32_t>::MaxValue();

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       All header bytes set to 0xFF (maximum entropy) should fail gracefully
 *              rather than crash. This tests that the parser doesn't dereference
 *              pointers into invalid memory.
 */
XPF_TEST_SCENARIO(TestPeParser, AllBytesMaxValue)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(4096)));

    /* Fill entire buffer with 0xFF. */
    uint8_t* bytes = static_cast<uint8_t*>(buffer.GetBuffer());
    for (size_t i = 0; i < 4096; ++i)
    {
        bytes[i] = 0xFF;
    }

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       A very large e_lfanew that is exactly at the file boundary (no room for COFF)
 *              should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, ELfanewAtFileBoundary)
{
    const size_t totalSize = 256;
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(totalSize)));
    xpf::ApiZeroMemory(buffer.GetBuffer(), totalSize);

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    /* Place PE signature at offset where only 4 bytes remain - enough for signature but not COFF. */
    dos->e_lfanew = static_cast<int32_t>(totalSize - sizeof(uint32_t));

    uint32_t* sig = reinterpret_cast<uint32_t*>(
                    static_cast<uint8_t*>(buffer.GetBuffer()) + totalSize - sizeof(uint32_t));
    *sig = PE_NT_SIGNATURE;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       SizeOfHeaders smaller than actual header end should fail.
 *              This tests that we validate SizeOfHeaders >= sectionHeadersEnd.
 */
XPF_TEST_SCENARIO(TestPeParser, SizeOfHeadersTooSmall)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + optOffset);

    /* Set SizeOfHeaders to a very small value that cannot cover the section headers. */
    opt->SizeOfHeaders = sizeof(xpf::pe::DosHeader);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid PE32+ with SizeOfOptionalHeader too small for OptionalHeader64 should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, Pe64OptionalHeaderTooSmall)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);

    /* Set SizeOfOptionalHeader to only 2 bytes (just enough for magic, not the full header). */
    coff->SizeOfOptionalHeader = sizeof(xpf::pe::OptionalHeaderCommon);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Verify that after a successful parse, the buffer ownership is transferred
 *              to PeInfo.m_FileData (source buffer should be empty).
 */
XPF_TEST_SCENARIO(TestPeParser, BufferOwnershipTransfer)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    const size_t originalSize = buffer.GetSize();
    XPF_TEST_EXPECT_TRUE(originalSize > 0);

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    /* The PeInfo should now own the data. */
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() == originalSize);
}

/**
 * @brief       Verify offsets are correctly computed by reading back data at the
 *              section headers offset and data directory offset.
 */
XPF_TEST_SCENARIO(TestPeParser, OffsetsPointToCorrectData)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    /* Read section header at the stored offset. */
    const xpf::pe::SectionHeader* section = static_cast<const xpf::pe::SectionHeader*>(
                                            xpf::AlgoAddToPointer(peInfo.m_FileData.GetBuffer(),
                                                                  peInfo.m_SectionHeadersOffset));

    /* The first section should have the name '.txt\0...' as set by BuildMinimalPe32. */
    XPF_TEST_EXPECT_TRUE(section->Name[0] == '.');
    XPF_TEST_EXPECT_TRUE(section->Name[1] == 't');
    XPF_TEST_EXPECT_TRUE(section->Name[2] == 'x');
    XPF_TEST_EXPECT_TRUE(section->Name[3] == 't');

    /* Data directory at the stored offset should be zero (as initialized). */
    const xpf::pe::DataDirectory* dataDir = static_cast<const xpf::pe::DataDirectory*>(
                                            xpf::AlgoAddToPointer(peInfo.m_FileData.GetBuffer(),
                                                                  peInfo.m_DataDirectoryOffset));
    XPF_TEST_EXPECT_TRUE(dataDir->VirtualAddress == 0);
    XPF_TEST_EXPECT_TRUE(dataDir->Size == 0);
}

/**
 * @brief       A buffer that is exactly sizeof(DosHeader) with valid MZ magic
 *              but e_lfanew pointing to offset sizeof(DosHeader) (right at the end)
 *              should fail because no room for PE signature.
 */
XPF_TEST_SCENARIO(TestPeParser, ExactDosHeaderSizeNoRoom)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(sizeof(xpf::pe::DosHeader))));
    xpf::ApiZeroMemory(buffer.GetBuffer(), buffer.GetSize());

    xpf::pe::DosHeader* dos = static_cast<xpf::pe::DosHeader*>(buffer.GetBuffer());
    dos->e_magic = PE_DOS_SIGNATURE;
    dos->e_lfanew = static_cast<int32_t>(sizeof(xpf::pe::DosHeader));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       PE32 with NumberOfRvaAndSizes=0 should parse with 0 data directories.
 */
XPF_TEST_SCENARIO(TestPeParser, ZeroDataDirectories)
{
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + optOffset);
    opt->NumberOfRvaAndSizes = 0;

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseHeaders(xpf::Move(buffer), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfDataDirectories == 0);
}


/* ============================================================================================= */
/* ===                              ParseFromFile Tests                                      === */
/* ============================================================================================= */

/**
 * @brief       ParseFromFile with a non-existent file path should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFileNonExistent)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe_noexist_12345.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       ParseFromFile with a file containing garbage data should fail.
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFileGarbageData)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe_garbage.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Write 256 bytes of zero (no valid MZ signature). */
    uint8_t garbage[256];
    xpf::ApiZeroMemory(garbage, sizeof(garbage));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            garbage,
                                                            sizeof(garbage))));

    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile with a valid PE32 (x86 EXE) written to disk.
 *              Verifies that the file-based path produces the same results
 *              as the in-memory ParseHeaders.
 *
 *              This also confirms that an x86 PE can be parsed regardless
 *              of the host architecture (cross-architecture support).
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFilePe32Exe)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe32_exe.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Build a minimal PE32 EXE image. */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 2)));

    /* Write it to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x014C });          /* IMAGE_FILE_MACHINE_I386 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 2);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x00400000 });
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() == buffer.GetSize());

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile with a valid PE32 (x86 DLL) written to disk.
 *              The DLL characteristic is set in the COFF header Characteristics field.
 *
 *              IMAGE_FILE_DLL = 0x2000
 *              See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#characteristics
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFilePe32Dll)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe32_dll.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Build a minimal PE32 image and set DLL characteristic. */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);
    coff->Characteristics = uint16_t{ 0x2000 };    /* IMAGE_FILE_DLL */

    /* Write to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x014C });          /* IMAGE_FILE_MACHINE_I386 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x00400000 });

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile with a valid PE32+ (x64 EXE) written to disk.
 *              Verifies cross-architecture support: an x86-compiled binary
 *              can parse an x64 PE file (and vice-versa), because the parser
 *              handles both PE32 and PE32+ via uint64_t fields.
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFilePe64Exe)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe64_exe.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Build a minimal PE32+ EXE image. */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 3)));

    /* Write to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == true);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x8664 });          /* IMAGE_FILE_MACHINE_AMD64 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 3);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x0000000140000000 });
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() == buffer.GetSize());

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile with a valid PE32+ (x64 DLL) written to disk.
 *              Sets IMAGE_FILE_DLL (0x2000) in COFF Characteristics.
 *
 *              See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#characteristics
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFilePe64Dll)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe64_dll.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Build a minimal PE32+ image and set DLL characteristic. */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 1)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t coffOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t);
    xpf::pe::CoffHeader* coff = reinterpret_cast<xpf::pe::CoffHeader*>(base + coffOffset);
    coff->Characteristics = uint16_t{ 0x2000 };    /* IMAGE_FILE_DLL */

    /* Write to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == true);
    XPF_TEST_EXPECT_TRUE(peInfo.m_Machine == uint16_t{ 0x8664 });          /* IMAGE_FILE_MACHINE_AMD64 */
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 1);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x0000000140000000 });

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile with a PE file larger than one read chunk (64 KB).
 *              Verifies that the chunk-based reading loop correctly handles
 *              multi-chunk files and reassembles the buffer.
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFileLargeBuffer)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe_large.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /*
     * Build a PE32 image and pad it to > 64KB to force multiple read chunks.
     * The PE headers are valid but the extra space is just padding (zeros).
     */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe32(&buffer, 1)));

    /* Grow the buffer to 128 KB. Resize copies old data and zeros new memory. */
    const size_t largeSize = size_t{ 128 } * size_t{ 1024 };
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(buffer.Resize(largeSize)));

    /* Update SizeOfHeaders to reflect the new total size. */
    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeader32* opt = reinterpret_cast<xpf::pe::OptionalHeader32*>(base + optOffset);
    opt->SizeOfHeaders = static_cast<uint32_t>(largeSize);

    /* Write to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == false);
    XPF_TEST_EXPECT_TRUE(peInfo.m_FileData.GetSize() == largeSize);

    TestPeDeleteFile(filePath.View());
}

/**
 * @brief       ParseFromFile cross-architecture: parse a PE32+ (64-bit) file
 *              when the library might be compiled as 32-bit. The parser must
 *              handle 64-bit ImageBase and PE32+ optional header regardless
 *              of the host architecture, because all relevant fields use
 *              explicitly-sized types (uint64_t, uint32_t, etc.).
 */
XPF_TEST_SCENARIO(TestPeParser, ParseFromFileCrossArchPe64OnAnyHost)
{
    xpf::String<wchar_t> filePath;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeBuildTempFilePath(
        xpf::StringView<wchar_t>(L"xpf_pe_crossarch.tmp"), filePath)));

    TestPeDeleteFile(filePath.View());

    /* Build PE32+ with a high 64-bit ImageBase that only makes sense for x64. */
    xpf::Buffer buffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPe64(&buffer, 2)));

    uint8_t* base = static_cast<uint8_t*>(buffer.GetBuffer());
    const size_t optOffset = sizeof(xpf::pe::DosHeader) + sizeof(uint32_t)
                           + sizeof(xpf::pe::CoffHeader);
    xpf::pe::OptionalHeader64* opt = reinterpret_cast<xpf::pe::OptionalHeader64*>(base + optOffset);
    opt->ImageBase = uint64_t{ 0x00007FF600000000 };   /* Typical x64 high address. */

    /* Write to disk. */
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(TestPeWriteBufferToFile(filePath.View(),
                                                            buffer.GetBuffer(),
                                                            buffer.GetSize())));

    /* Parse from file. */
    xpf::pe::PeInformation peInfo;
    NTSTATUS status = xpf::pe::ParseFromFile(filePath.View(), &peInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(peInfo.m_Is64Bit == true);
    XPF_TEST_EXPECT_TRUE(peInfo.m_ImageBase == uint64_t{ 0x00007FF600000000 });
    XPF_TEST_EXPECT_TRUE(peInfo.m_NumberOfSections == 2);

    TestPeDeleteFile(filePath.View());
}
