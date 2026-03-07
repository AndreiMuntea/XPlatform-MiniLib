/**
 * @file        xpf_lib/private/Utility/PeFileParser.cpp
 *
 * @brief       In here you'll find functionality for parsing Portable Executable (PE) files.
 *              PE is the standard executable format for Windows (EXE, DLL, SYS) and is also
 *              used on UEFI platforms. This parser extracts and validates the core PE headers:
 *              DOS header, COFF header, optional header, data directories, and section headers.
 *
 *              The primary reference is the official Microsoft PE/COFF specification:
 *              https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 *              All structures are redefined in the header to ensure cross-platform compatibility.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into paged section.
 *          We shouldn't attempt PE parsing at higher IRQLs.
 */
XPF_SECTION_PAGED;

namespace xpf
{
namespace pe
{

/**
 * @brief       Validates the DOS header at the beginning of the PE file.
 *              Checks the 'MZ' signature and ensures e_lfanew points within bounds.
 *
 * @param[in]   FileData    - Pointer to the raw PE file data.
 * @param[in]   FileSize    - Size of the file data in bytes.
 * @param[out]  PeOffset    - On success, the file offset of the PE signature.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ValidateDosHeader(
    _In_ const void* FileData,
    _In_ const size_t& FileSize,
    _Out_ size_t* PeOffset
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != FileData);
    XPF_DEATH_ON_FAILURE(nullptr != PeOffset);

    /* Preinit output. */
    *PeOffset = 0;

    /* Ensure we have enough bytes for the DOS header. */
    if (FileSize < sizeof(xpf::pe::DosHeader))
    {
        return STATUS_DATA_ERROR;
    }

    const xpf::pe::DosHeader* dosHeader = static_cast<const xpf::pe::DosHeader*>(FileData);

    /*
     * Validate the DOS signature ('MZ' = 0x5A4D).
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#ms-dos-stub-image-only
     */
    if (dosHeader->e_magic != PE_DOS_SIGNATURE)
    {
        return STATUS_DATA_ERROR;
    }

    /*
     * e_lfanew must be positive and must leave room for at least the PE signature (4 bytes).
     * Negative values or values that would place the PE signature beyond the file are invalid.
     */
    if (dosHeader->e_lfanew < 0)
    {
        return STATUS_DATA_ERROR;
    }
    const size_t peSignatureOffset = static_cast<size_t>(static_cast<uint32_t>(dosHeader->e_lfanew));

    /*
     * Ensure the PE signature (uint32_t) fits within the file.
     * We need at least peSignatureOffset + sizeof(uint32_t) bytes.
     */
    size_t peSignatureEnd = 0;
    if (!xpf::ApiNumbersSafeAdd(peSignatureOffset, sizeof(uint32_t), &peSignatureEnd))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (peSignatureEnd > FileSize)
    {
        return STATUS_DATA_ERROR;
    }

    /*
     * Validate the PE signature itself ('PE\0\0' = 0x00004550).
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
     */
    const uint32_t* peSignature = static_cast<const uint32_t*>(
                                  xpf::AlgoAddToPointer(FileData, peSignatureOffset));
    if (*peSignature != PE_NT_SIGNATURE)
    {
        return STATUS_DATA_ERROR;
    }

    *PeOffset = peSignatureOffset;
    return STATUS_SUCCESS;
}


/**
 * @brief       Validates the COFF header and determines the optional header magic.
 *              Checks that the COFF header fits within the file and that the
 *              SizeOfOptionalHeader value is consistent with the optional header type.
 *
 * @param[in]   FileData            - Pointer to the raw PE file data.
 * @param[in]   FileSize            - Size of the file data in bytes.
 * @param[in]   PeSignatureOffset   - Offset of the PE signature within the file.
 * @param[out]  CoffHeaderOffset    - On success, the offset of the COFF header.
 * @param[out]  OptionalHeaderOffset - On success, the offset of the optional header.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ValidateCoffHeader(
    _In_ const void* FileData,
    _In_ const size_t& FileSize,
    _In_ const size_t& PeSignatureOffset,
    _Out_ size_t* CoffHeaderOffset,
    _Out_ size_t* OptionalHeaderOffset
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != FileData);
    XPF_DEATH_ON_FAILURE(nullptr != CoffHeaderOffset);
    XPF_DEATH_ON_FAILURE(nullptr != OptionalHeaderOffset);

    /* Preinit outputs. */
    *CoffHeaderOffset = 0;
    *OptionalHeaderOffset = 0;

    /*
     * The COFF header immediately follows the 4-byte PE signature.
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
     */
    size_t coffOffset = 0;
    if (!xpf::ApiNumbersSafeAdd(PeSignatureOffset, sizeof(uint32_t), &coffOffset))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    /* Ensure the COFF header fits. */
    size_t coffEnd = 0;
    if (!xpf::ApiNumbersSafeAdd(coffOffset, sizeof(xpf::pe::CoffHeader), &coffEnd))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (coffEnd > FileSize)
    {
        return STATUS_DATA_ERROR;
    }

    const xpf::pe::CoffHeader* coffHeader = static_cast<const xpf::pe::CoffHeader*>(
                                             xpf::AlgoAddToPointer(FileData, coffOffset));

    /*
     * SizeOfOptionalHeader must be large enough to at least hold the Magic field.
     * We will validate the full size later when we know whether this is PE32 or PE32+.
     */
    if (coffHeader->SizeOfOptionalHeader < sizeof(xpf::pe::OptionalHeaderCommon))
    {
        return STATUS_DATA_ERROR;
    }

    /*
     * NumberOfSections is capped to prevent absurd allocations from crafted files.
     * The PE specification historically limits image files to 96 sections.
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
     */
    if (coffHeader->NumberOfSections > PE_MAX_SECTIONS)
    {
        return STATUS_DATA_ERROR;
    }

    /*
     * The optional header begins immediately after the COFF header.
     * Ensure that SizeOfOptionalHeader bytes actually fit in the file.
     */
    size_t optionalOffset = coffEnd;
    size_t optionalEnd = 0;
    if (!xpf::ApiNumbersSafeAdd(optionalOffset, size_t{ coffHeader->SizeOfOptionalHeader }, &optionalEnd))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (optionalEnd > FileSize)
    {
        return STATUS_DATA_ERROR;
    }

    *CoffHeaderOffset = coffOffset;
    *OptionalHeaderOffset = optionalOffset;
    return STATUS_SUCCESS;
}


/**
 * @brief       Validates the optional header, extracts key fields, and computes
 *              the offsets for the data directory array and section headers.
 *
 * @param[in]   FileData              - Pointer to the raw PE file data.
 * @param[in]   FileSize              - Size of the file data in bytes.
 * @param[in]   CoffHeaderOffset      - Offset of the COFF header.
 * @param[in]   OptionalHeaderOffset  - Offset of the optional header.
 * @param[out]  PeInfo                - Populated with extracted fields on success.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ValidateOptionalHeaderAndSections(
    _In_ const void* FileData,
    _In_ const size_t& FileSize,
    _In_ const size_t& CoffHeaderOffset,
    _In_ const size_t& OptionalHeaderOffset,
    _Out_ xpf::pe::PeInformation* PeInfo
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != FileData);
    XPF_DEATH_ON_FAILURE(nullptr != PeInfo);

    const xpf::pe::CoffHeader* coffHeader = static_cast<const xpf::pe::CoffHeader*>(
                                             xpf::AlgoAddToPointer(FileData, CoffHeaderOffset));
    const xpf::pe::OptionalHeaderCommon* optCommon = static_cast<const xpf::pe::OptionalHeaderCommon*>(
                                                     xpf::AlgoAddToPointer(FileData, OptionalHeaderOffset));

    /* Extract machine and number of sections from COFF header. */
    PeInfo->m_Machine = coffHeader->Machine;
    PeInfo->m_NumberOfSections = coffHeader->NumberOfSections;

    /*
     * Determine whether this is PE32 or PE32+ based on the Magic field.
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-standard-fields-image-only
     */
    size_t dataDirOffset = 0;

    if (optCommon->Magic == PE_OPTIONAL_HDR32_MAGIC)
    {
        /* PE32 (32-bit) */
        PeInfo->m_Is64Bit = false;

        /*
         * Validate that SizeOfOptionalHeader is large enough for the PE32 optional header.
         * The actual SizeOfOptionalHeader may be larger if data directories are present.
         */
        if (coffHeader->SizeOfOptionalHeader < sizeof(xpf::pe::OptionalHeader32))
        {
            return STATUS_DATA_ERROR;
        }

        const xpf::pe::OptionalHeader32* opt32 = static_cast<const xpf::pe::OptionalHeader32*>(
                                                  xpf::AlgoAddToPointer(FileData, OptionalHeaderOffset));

        PeInfo->m_ImageBase = static_cast<uint64_t>(opt32->ImageBase);
        PeInfo->m_SizeOfHeaders = opt32->SizeOfHeaders;
        PeInfo->m_NumberOfDataDirectories = opt32->NumberOfRvaAndSizes;

        /* Data directories start right after the fixed OptionalHeader32 fields. */
        if (!xpf::ApiNumbersSafeAdd(OptionalHeaderOffset, sizeof(xpf::pe::OptionalHeader32), &dataDirOffset))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
    }
    else if (optCommon->Magic == PE_OPTIONAL_HDR64_MAGIC)
    {
        /* PE32+ (64-bit) */
        PeInfo->m_Is64Bit = true;

        if (coffHeader->SizeOfOptionalHeader < sizeof(xpf::pe::OptionalHeader64))
        {
            return STATUS_DATA_ERROR;
        }

        const xpf::pe::OptionalHeader64* opt64 = static_cast<const xpf::pe::OptionalHeader64*>(
                                                  xpf::AlgoAddToPointer(FileData, OptionalHeaderOffset));

        PeInfo->m_ImageBase = opt64->ImageBase;
        PeInfo->m_SizeOfHeaders = opt64->SizeOfHeaders;
        PeInfo->m_NumberOfDataDirectories = opt64->NumberOfRvaAndSizes;

        /* Data directories start right after the fixed OptionalHeader64 fields. */
        if (!xpf::ApiNumbersSafeAdd(OptionalHeaderOffset, sizeof(xpf::pe::OptionalHeader64), &dataDirOffset))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
    }
    else
    {
        /* Unknown optional header magic - not a valid PE file. */
        return STATUS_DATA_ERROR;
    }

    /*
     * Cap NumberOfDataDirectories to the PE specification maximum (16).
     * A malicious file could claim millions of entries to cause huge reads.
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
     */
    if (PeInfo->m_NumberOfDataDirectories > PE_MAX_DATA_DIRECTORIES)
    {
        PeInfo->m_NumberOfDataDirectories = PE_MAX_DATA_DIRECTORIES;
    }

    /*
     * Validate that the data directory array fits within the file.
     * dataDirectorySize = NumberOfDataDirectories * sizeof(DataDirectory)
     */
    size_t dataDirectorySize = 0;
    if (!xpf::ApiNumbersSafeMul(size_t{ PeInfo->m_NumberOfDataDirectories },
                                sizeof(xpf::pe::DataDirectory),
                                &dataDirectorySize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    size_t dataDirectoryEnd = 0;
    if (!xpf::ApiNumbersSafeAdd(dataDirOffset, dataDirectorySize, &dataDirectoryEnd))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (dataDirectoryEnd > FileSize)
    {
        return STATUS_DATA_ERROR;
    }
    PeInfo->m_DataDirectoryOffset = dataDirOffset;

    /*
     * The section headers begin right after the optional header (including data directories).
     * sectionHeadersOffset = OptionalHeaderOffset + SizeOfOptionalHeader
     *
     * See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-table-section-headers
     */
    size_t sectionHeadersOffset = 0;
    if (!xpf::ApiNumbersSafeAdd(OptionalHeaderOffset,
                                size_t{ coffHeader->SizeOfOptionalHeader },
                                &sectionHeadersOffset))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    /*
     * Validate that all section headers fit within the file.
     * totalSectionSize = NumberOfSections * sizeof(SectionHeader)
     */
    size_t totalSectionSize = 0;
    if (!xpf::ApiNumbersSafeMul(size_t{ PeInfo->m_NumberOfSections },
                                sizeof(xpf::pe::SectionHeader),
                                &totalSectionSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    size_t sectionHeadersEnd = 0;
    if (!xpf::ApiNumbersSafeAdd(sectionHeadersOffset, totalSectionSize, &sectionHeadersEnd))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (sectionHeadersEnd > FileSize)
    {
        return STATUS_DATA_ERROR;
    }
    PeInfo->m_SectionHeadersOffset = sectionHeadersOffset;

    /*
     * Validate SizeOfHeaders: it must be large enough to cover everything up to
     * and including the section headers, and must not exceed the file size.
     */
    if (PeInfo->m_SizeOfHeaders > FileSize)
    {
        return STATUS_DATA_ERROR;
    }
    if (PeInfo->m_SizeOfHeaders < sectionHeadersEnd)
    {
        return STATUS_DATA_ERROR;
    }

    /*
     * Validate each section header for internal consistency.
     * Specifically, ensure that PointerToRawData + SizeOfRawData does not exceed the file size,
     * and VirtualAddress + VirtualSize does not overflow.
     */
    for (uint16_t i = 0; i < PeInfo->m_NumberOfSections; ++i)
    {
        size_t currentSectionOffset = 0;
        if (!xpf::ApiNumbersSafeAdd(sectionHeadersOffset,
                                    size_t{ i } * sizeof(xpf::pe::SectionHeader),
                                    &currentSectionOffset))
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        const xpf::pe::SectionHeader* section = static_cast<const xpf::pe::SectionHeader*>(
                                                 xpf::AlgoAddToPointer(FileData, currentSectionOffset));

        /* Validate that raw data range does not exceed file size. */
        if (section->SizeOfRawData != 0)
        {
            size_t rawDataEnd = 0;
            if (!xpf::ApiNumbersSafeAdd(size_t{ section->PointerToRawData },
                                        size_t{ section->SizeOfRawData },
                                        &rawDataEnd))
            {
                return STATUS_INTEGER_OVERFLOW;
            }
            if (rawDataEnd > FileSize)
            {
                return STATUS_DATA_ERROR;
            }
        }

        /* Validate that virtual address + virtual size does not overflow. */
        uint32_t virtualEnd = 0;
        if (!xpf::ApiNumbersSafeAdd(section->VirtualAddress, section->Misc.VirtualSize, &virtualEnd))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
    }

    return STATUS_SUCCESS;
}


_Must_inspect_result_
NTSTATUS XPF_API
ParseHeaders(
    _Inout_ xpf::Buffer&& FileData,
    _Out_ xpf::pe::PeInformation* PeInfo
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(nullptr != PeInfo);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    size_t peSignatureOffset = 0;
    size_t coffHeaderOffset = 0;
    size_t optionalHeaderOffset = 0;

    /* Preinit output. */
    PeInfo->m_FileData.Clear();
    PeInfo->m_Is64Bit = false;
    PeInfo->m_Machine = 0;
    PeInfo->m_NumberOfSections = 0;
    PeInfo->m_SizeOfHeaders = 0;
    PeInfo->m_NumberOfDataDirectories = 0;
    PeInfo->m_SectionHeadersOffset = 0;
    PeInfo->m_DataDirectoryOffset = 0;
    PeInfo->m_ImageBase = 0;

    const void* fileBuffer = FileData.GetBuffer();
    const size_t fileSize = FileData.GetSize();

    /* Must have data to parse. */
    if (fileBuffer == nullptr || fileSize == 0)
    {
        return STATUS_DATA_ERROR;
    }

    /* Step 1: Validate the DOS header and locate the PE signature. */
    status = xpf::pe::ValidateDosHeader(fileBuffer, fileSize, &peSignatureOffset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Step 2: Validate the COFF header. */
    status = xpf::pe::ValidateCoffHeader(fileBuffer, fileSize, peSignatureOffset,
                                         &coffHeaderOffset, &optionalHeaderOffset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Step 3: Validate optional header, data directories, and section headers. */
    status = xpf::pe::ValidateOptionalHeaderAndSections(fileBuffer, fileSize,
                                                        coffHeaderOffset, optionalHeaderOffset,
                                                        PeInfo);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* All validation passed. Transfer ownership of the buffer. */
    PeInfo->m_FileData = xpf::Move(FileData);
    return STATUS_SUCCESS;
}


_Must_inspect_result_
NTSTATUS XPF_API
ParseFromFile(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _Out_ xpf::pe::PeInformation* PeInfo
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(nullptr != PeInfo);

    /* Preinit output. */
    PeInfo->m_FileData.Clear();
    PeInfo->m_Is64Bit = false;
    PeInfo->m_Machine = 0;
    PeInfo->m_NumberOfSections = 0;
    PeInfo->m_SizeOfHeaders = 0;
    PeInfo->m_NumberOfDataDirectories = 0;
    PeInfo->m_SectionHeadersOffset = 0;
    PeInfo->m_DataDirectoryOffset = 0;
    PeInfo->m_ImageBase = 0;

    /* Open the file for reading. Do not create if it doesn't exist. */
    xpf::Optional<xpf::FileObject> fileObj;
    NTSTATUS status = xpf::FileObject::Create(&fileObj,
                                              FilePath,
                                              true,         /* ReadOnly */
                                              false);       /* CreateIfNotFound */
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /*
     * Read the file contents into a buffer using chunk-based reading.
     * FileObject does not expose a file-size query, so we read in chunks
     * until EOF (bytesRead == 0 or bytesRead < chunkSize).
     *
     * We use a 64 KB chunk size as a reasonable balance between
     * number of syscalls and memory usage.
     */
    const size_t chunkSize = size_t{ 65536 };
    xpf::Buffer fileData;
    size_t totalBytesRead = 0;

    for (;;)
    {
        /* Ensure the buffer has room for the next chunk. */
        size_t requiredSize = 0;
        if (!xpf::ApiNumbersSafeAdd(totalBytesRead, chunkSize, &requiredSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        status = fileData.Resize(requiredSize);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Read the next chunk at the current offset. */
        uint8_t* destination = static_cast<uint8_t*>(
                               xpf::AlgoAddToPointer(fileData.GetBuffer(), totalBytesRead));
        size_t bytesRead = 0;

        status = (*fileObj).Read(destination,
                                chunkSize,
                                totalBytesRead,
                                &bytesRead);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Accumulate the bytes read. */
        size_t newTotal = 0;
        if (!xpf::ApiNumbersSafeAdd(totalBytesRead, bytesRead, &newTotal))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        totalBytesRead = newTotal;

        /* EOF reached when no bytes were returned or a partial chunk was read. */
        if (bytesRead == 0 || bytesRead < chunkSize)
        {
            break;
        }
    }

    /* Trim the buffer to the actual number of bytes read. */
    if (totalBytesRead == 0)
    {
        return STATUS_DATA_ERROR;
    }
    status = fileData.Resize(totalBytesRead);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Delegate to ParseHeaders for validation and field extraction. */
    return xpf::pe::ParseHeaders(xpf::Move(fileData), PeInfo);
}

};  // namespace pe
};  // namespace xpf
