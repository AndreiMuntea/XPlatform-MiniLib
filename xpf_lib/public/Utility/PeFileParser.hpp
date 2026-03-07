/**
 * @file        xpf_lib/public/Utility/PeFileParser.hpp
 *
 * @brief       In here you'll find functionality for parsing Portable Executable (PE) files.
 *              PE is the standard executable format for Windows (EXE, DLL, SYS) and is also
 *              used on UEFI platforms. This parser extracts and validates the core PE headers:
 *              DOS header, COFF header, optional header, data directories, and section headers.
 *
 *              The primary reference is the official Microsoft PE/COFF specification:
 *              https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 *
 *              All structures are redefined here to ensure cross-platform compatibility
 *              (Windows user-mode, Windows kernel-mode, and Linux user-mode).
 *
 * @note        This is a header-only parser that operates on an in-memory buffer.
 *              It validates all offsets and sizes defensively to handle corrupted or
 *              malicious inputs. It does not perform file I/O directly; callers must
 *              first read the PE file into a xpf::Buffer.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2026.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#pragma once

#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/Containers/Vector.hpp"
#include "xpf_lib/public/Containers/String.hpp"

namespace xpf
{
namespace pe
{

///
/// --------------------------------------------------------------------------------------------
///  PE Header Structures.
///  All structures are packed to match the exact binary layout of the PE format.
///  Sources:
///      - Microsoft PE/COFF Specification:
///        https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
///      - winnt.h (Windows SDK)
/// --------------------------------------------------------------------------------------------
///


/**
 * @brief   The DOS header is the very first structure in every PE file.
 *          Only the e_magic and e_lfanew fields are relevant for modern PE parsing.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#ms-dos-stub-image-only
 *          See: winnt.h IMAGE_DOS_HEADER
 */
XPF_PACK(struct DosHeader
{
    /*
     * @brief   The magic number identifying a DOS executable.
     *          Must be 0x5A4D ('MZ' in little-endian).
     *
     *          See: IMAGE_DOS_SIGNATURE in winnt.h
     */
    uint16_t    e_magic;

    /*
     * @brief   Bytes on last page of file.
     */
    uint16_t    e_cblp;

    /*
     * @brief   Pages in file.
     */
    uint16_t    e_cp;

    /*
     * @brief   Relocations.
     */
    uint16_t    e_crlc;

    /*
     * @brief   Size of header in paragraphs.
     */
    uint16_t    e_cparhdr;

    /*
     * @brief   Minimum extra paragraphs needed.
     */
    uint16_t    e_minalloc;

    /*
     * @brief   Maximum extra paragraphs needed.
     */
    uint16_t    e_maxalloc;

    /*
     * @brief   Initial (relative) SS value.
     */
    uint16_t    e_ss;

    /*
     * @brief   Initial SP value.
     */
    uint16_t    e_sp;

    /*
     * @brief   Checksum.
     */
    uint16_t    e_csum;

    /*
     * @brief   Initial IP value.
     */
    uint16_t    e_ip;

    /*
     * @brief   Initial (relative) CS value.
     */
    uint16_t    e_cs;

    /*
     * @brief   File address of relocation table.
     */
    uint16_t    e_lfarlc;

    /*
     * @brief   Overlay number.
     */
    uint16_t    e_ovno;

    /*
     * @brief   Reserved words.
     */
    uint16_t    e_res[4];

    /*
     * @brief   OEM identifier (for e_oeminfo).
     */
    uint16_t    e_oemid;

    /*
     * @brief   OEM information; e_oemid specific.
     */
    uint16_t    e_oeminfo;

    /*
     * @brief   Reserved words.
     */
    uint16_t    e_res2[10];

    /*
     * @brief   File offset of the PE signature ("PE\0\0").
     *          This is the critical field: it points to the COFF header.
     *          Must be validated against the file size to prevent out-of-bounds access.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
     */
    int32_t     e_lfanew;
});

/**
 * @brief   IMAGE_DOS_SIGNATURE - the 'MZ' magic value expected at offset 0.
 *
 *          See: winnt.h IMAGE_DOS_SIGNATURE
 */
#define PE_DOS_SIGNATURE        uint16_t{ 0x5A4D }

/**
 * @brief   IMAGE_NT_SIGNATURE - the 'PE\0\0' signature (as a uint32_t).
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
 *          See: winnt.h IMAGE_NT_SIGNATURE
 */
#define PE_NT_SIGNATURE         uint32_t{ 0x00004550 }


/**
 * @brief   The COFF file header (also called "file header" or "image file header").
 *          This immediately follows the 4-byte PE signature.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
 *          See: winnt.h IMAGE_FILE_HEADER
 */
XPF_PACK(struct CoffHeader
{
    /*
     * @brief   Identifies the type of target machine.
     *          Common values:
     *              0x014C = IMAGE_FILE_MACHINE_I386  (x86)
     *              0x8664 = IMAGE_FILE_MACHINE_AMD64 (x64)
     *              0xAA64 = IMAGE_FILE_MACHINE_ARM64
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
     */
    uint16_t    Machine;

    /*
     * @brief   The number of sections (also called "sections" or "section table entries").
     *          This indicates the size of the section table, which immediately follows the headers.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
     */
    uint16_t    NumberOfSections;

    /*
     * @brief   The low 32 bits of the time stamp of the image.
     */
    uint32_t    TimeDateStamp;

    /*
     * @brief   The file offset of the COFF symbol table.
     *          Zero if no COFF symbol table is present.
     *          This value should be zero for an image because COFF debugging information is deprecated.
     */
    uint32_t    PointerToSymbolTable;

    /*
     * @brief   The number of entries in the symbol table.
     *          Zero if no COFF symbol table is present.
     */
    uint32_t    NumberOfSymbols;

    /*
     * @brief   The size of the optional header, in bytes.
     *          This is required for image files but not for object files.
     *          For PE32 this is typically 224; for PE32+ it is typically 240.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
     */
    uint16_t    SizeOfOptionalHeader;

    /*
     * @brief   The flags that indicate the attributes of the file.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#characteristics
     */
    uint16_t    Characteristics;
});


/**
 * @brief   The "optional" header (despite the name, it is required for image files).
 *          This structure covers only the fields common to PE32 and PE32+,
 *          i.e. just the Magic field used to distinguish between the two.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-image-only
 */
XPF_PACK(struct OptionalHeaderCommon
{
    /*
     * @brief   Identifies the format of the optional header.
     *              0x10B = PE32  (32-bit)
     *              0x20B = PE32+ (64-bit)
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-standard-fields-image-only
     */
    uint16_t    Magic;
});

/**
 * @brief   PE32 Optional Header magic value.
 *
 *          See: winnt.h IMAGE_NT_OPTIONAL_HDR32_MAGIC
 */
#define PE_OPTIONAL_HDR32_MAGIC     uint16_t{ 0x10B }

/**
 * @brief   PE32+ Optional Header magic value.
 *
 *          See: winnt.h IMAGE_NT_OPTIONAL_HDR64_MAGIC
 */
#define PE_OPTIONAL_HDR64_MAGIC     uint16_t{ 0x20B }


/**
 * @brief   The full PE32 (32-bit) optional header.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-standard-fields-image-only
 *          See: winnt.h IMAGE_OPTIONAL_HEADER32
 */
XPF_PACK(struct OptionalHeader32
{
    /*
     * @brief   Must be PE_OPTIONAL_HDR32_MAGIC (0x10B).
     */
    uint16_t    Magic;

    /*
     * @brief   The linker major version number.
     */
    uint8_t     MajorLinkerVersion;

    /*
     * @brief   The linker minor version number.
     */
    uint8_t     MinorLinkerVersion;

    /*
     * @brief   The size of the code (.text) section, or the sum of all code sections.
     */
    uint32_t    SizeOfCode;

    /*
     * @brief   The size of the initialized data section, or the sum of all such sections.
     */
    uint32_t    SizeOfInitializedData;

    /*
     * @brief   The size of the uninitialized data section (BSS), or the sum of all such sections.
     */
    uint32_t    SizeOfUninitializedData;

    /*
     * @brief   The address of the entry point relative to the image base.
     */
    uint32_t    AddressOfEntryPoint;

    /*
     * @brief   The address that is relative to the image base of the beginning-of-code section.
     */
    uint32_t    BaseOfCode;

    /*
     * @brief   The address that is relative to the image base of the beginning-of-data section.
     *          This field is only present in PE32 (not in PE32+).
     */
    uint32_t    BaseOfData;

    /*
     * @brief   The preferred address of the first byte of image when loaded into memory.
     *          Must be a multiple of 64K. Default for DLLs is 0x10000000, for EXEs is 0x00400000.
     */
    uint32_t    ImageBase;

    /*
     * @brief   The alignment (in bytes) of sections when loaded into memory.
     *          Must be >= FileAlignment. Default is the page size (4096).
     */
    uint32_t    SectionAlignment;

    /*
     * @brief   The alignment factor (in bytes) for raw data of sections in the image file.
     *          Must be a power of 2 between 512 and 64K inclusive. Default is 512.
     */
    uint32_t    FileAlignment;

    /*
     * @brief   The major version number of the required operating system.
     */
    uint16_t    MajorOperatingSystemVersion;

    /*
     * @brief   The minor version number of the required operating system.
     */
    uint16_t    MinorOperatingSystemVersion;

    /*
     * @brief   The major version number of the image.
     */
    uint16_t    MajorImageVersion;

    /*
     * @brief   The minor version number of the image.
     */
    uint16_t    MinorImageVersion;

    /*
     * @brief   The major version number of the subsystem.
     */
    uint16_t    MajorSubsystemVersion;

    /*
     * @brief   The minor version number of the subsystem.
     */
    uint16_t    MinorSubsystemVersion;

    /*
     * @brief   Reserved, must be zero.
     */
    uint32_t    Win32VersionValue;

    /*
     * @brief   The size (in bytes) of the image, including all headers,
     *          as the image is loaded in memory.
     */
    uint32_t    SizeOfImage;

    /*
     * @brief   The combined size of the DOS stub, PE header, and section headers
     *          rounded up to a multiple of FileAlignment.
     */
    uint32_t    SizeOfHeaders;

    /*
     * @brief   The image file checksum.
     */
    uint32_t    CheckSum;

    /*
     * @brief   The subsystem required to run this image.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#windows-subsystem
     */
    uint16_t    Subsystem;

    /*
     * @brief   DLL characteristics flags.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#dll-characteristics
     */
    uint16_t    DllCharacteristics;

    /*
     * @brief   The size of the stack to reserve.
     */
    uint32_t    SizeOfStackReserve;

    /*
     * @brief   The size of the stack to commit.
     */
    uint32_t    SizeOfStackCommit;

    /*
     * @brief   The size of the local heap space to reserve.
     */
    uint32_t    SizeOfHeapReserve;

    /*
     * @brief   The size of the local heap space to commit.
     */
    uint32_t    SizeOfHeapCommit;

    /*
     * @brief   Reserved, must be zero.
     */
    uint32_t    LoaderFlags;

    /*
     * @brief   The number of data-directory entries in the remainder of the optional header.
     *          Each describes a location and size.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
     */
    uint32_t    NumberOfRvaAndSizes;
});


/**
 * @brief   The full PE32+ (64-bit) optional header.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-standard-fields-image-only
 *          See: winnt.h IMAGE_OPTIONAL_HEADER64
 */
XPF_PACK(struct OptionalHeader64
{
    /*
     * @brief   Must be PE_OPTIONAL_HDR64_MAGIC (0x20B).
     */
    uint16_t    Magic;

    /*
     * @brief   The linker major version number.
     */
    uint8_t     MajorLinkerVersion;

    /*
     * @brief   The linker minor version number.
     */
    uint8_t     MinorLinkerVersion;

    /*
     * @brief   The size of the code (.text) section, or the sum of all code sections.
     */
    uint32_t    SizeOfCode;

    /*
     * @brief   The size of the initialized data section.
     */
    uint32_t    SizeOfInitializedData;

    /*
     * @brief   The size of the uninitialized data section (BSS).
     */
    uint32_t    SizeOfUninitializedData;

    /*
     * @brief   The address of the entry point relative to the image base.
     */
    uint32_t    AddressOfEntryPoint;

    /*
     * @brief   RVA of the beginning-of-code section.
     */
    uint32_t    BaseOfCode;

    /*
     * @brief   The preferred address of the first byte of image when loaded into memory.
     *          For PE32+ this is a 64-bit value.
     */
    uint64_t    ImageBase;

    /*
     * @brief   The alignment (in bytes) of sections when loaded into memory.
     */
    uint32_t    SectionAlignment;

    /*
     * @brief   The alignment factor for raw data of sections in the image file.
     */
    uint32_t    FileAlignment;

    /*
     * @brief   The major version number of the required operating system.
     */
    uint16_t    MajorOperatingSystemVersion;

    /*
     * @brief   The minor version number of the required operating system.
     */
    uint16_t    MinorOperatingSystemVersion;

    /*
     * @brief   The major version number of the image.
     */
    uint16_t    MajorImageVersion;

    /*
     * @brief   The minor version number of the image.
     */
    uint16_t    MinorImageVersion;

    /*
     * @brief   The major version number of the subsystem.
     */
    uint16_t    MajorSubsystemVersion;

    /*
     * @brief   The minor version number of the subsystem.
     */
    uint16_t    MinorSubsystemVersion;

    /*
     * @brief   Reserved, must be zero.
     */
    uint32_t    Win32VersionValue;

    /*
     * @brief   The size (in bytes) of the image, including all headers.
     */
    uint32_t    SizeOfImage;

    /*
     * @brief   The combined size of the DOS stub, PE header, and section headers.
     */
    uint32_t    SizeOfHeaders;

    /*
     * @brief   The image file checksum.
     */
    uint32_t    CheckSum;

    /*
     * @brief   The subsystem required to run this image.
     */
    uint16_t    Subsystem;

    /*
     * @brief   DLL characteristics flags.
     */
    uint16_t    DllCharacteristics;

    /*
     * @brief   The size of the stack to reserve (64-bit).
     */
    uint64_t    SizeOfStackReserve;

    /*
     * @brief   The size of the stack to commit (64-bit).
     */
    uint64_t    SizeOfStackCommit;

    /*
     * @brief   The size of the local heap space to reserve (64-bit).
     */
    uint64_t    SizeOfHeapReserve;

    /*
     * @brief   The size of the local heap space to commit (64-bit).
     */
    uint64_t    SizeOfHeapCommit;

    /*
     * @brief   Reserved, must be zero.
     */
    uint32_t    LoaderFlags;

    /*
     * @brief   The number of data-directory entries in the remainder of the optional header.
     */
    uint32_t    NumberOfRvaAndSizes;
});


/**
 * @brief   A data directory entry. Each entry provides the RVA and size
 *          of a particular table or string that is located in one of the sections.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
 *          See: winnt.h IMAGE_DATA_DIRECTORY
 */
XPF_PACK(struct DataDirectory
{
    /*
     * @brief   The RVA of the table. The RVA is the address of the table
     *          when the table is loaded, relative to the base address of the image.
     */
    uint32_t    VirtualAddress;

    /*
     * @brief   The size in bytes of the table.
     */
    uint32_t    Size;
});


/**
 * @brief   Section header (one per section in the section table).
 *          The section table immediately follows the optional header.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-table-section-headers
 *          See: winnt.h IMAGE_SECTION_HEADER
 *
 * @note    This is intentionally NOT packed because the natural alignment
 *          already matches the PE binary layout (40 bytes, no padding).
 *          This mirrors the pattern used for ImageSectionHeader in PdbSymbolParser.hpp.
 */
struct SectionHeader
{
    /**
     * @brief   An 8-byte, null-padded UTF-8 encoded string.
     *          If the string is exactly 8 characters long, there is no terminating null.
     *          For longer names, this field contains a slash (/) followed by an ASCII
     *          representation of an offset into the string table.
     */
    char        Name[8];

    /*
     * @brief   The total size of the section when loaded into memory.
     *          If this value is greater than SizeOfRawData, the section is zero-padded.
     */
    union
    {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;

    /**
     * @brief   The address of the first byte of the section relative to the image base
     *          when the section is loaded into memory.
     */
    uint32_t    VirtualAddress;

    /**
     * @brief   The size of the section (for object files) or the size of the initialized
     *          data on disk (for image files).
     */
    uint32_t    SizeOfRawData;

    /**
     * @brief   The file pointer to the first page of the section within the COFF file.
     */
    uint32_t    PointerToRawData;

    /**
     * @brief   The file pointer to the beginning of relocation entries for the section.
     */
    uint32_t    PointerToRelocations;

    /**
     * @brief   The file pointer to the beginning of line-number entries for the section.
     */
    uint32_t    PointerToLinenumbers;

    /**
     * @brief   The number of relocation entries for the section.
     */
    uint16_t    NumberOfRelocations;

    /**
     * @brief   The number of line-number entries for the section.
     */
    uint16_t    NumberOfLinenumbers;

    /**
     * @brief   The flags that describe the characteristics of the section.
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-flags
     */
    uint32_t    Characteristics;
};


/**
 * @brief   Maximum number of data directories allowed.
 *          The PE specification defines 16 entries (indices 0-15).
 *          We cap validation at this value to reject malformed files
 *          claiming an absurd number of data directories.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
 *          See: winnt.h IMAGE_NUMBEROF_DIRECTORY_ENTRIES
 */
#define PE_MAX_DATA_DIRECTORIES     uint32_t{ 16 }

/**
 * @brief   Maximum number of sections allowed in a PE file.
 *          The COFF specification limits this to 96 for image files
 *          (though the uint16_t field could hold 65535).
 *          We use a generous upper bound for robustness.
 *
 *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
 *          (The PE spec historically notes max 96 sections for executables.)
 */
#define PE_MAX_SECTIONS             uint16_t{ 96 }


/**
 * @brief   This is used to store the parsed information from a PE file.
 *          It holds the raw file data in a buffer and provides quick access
 *          to the key fields extracted during header validation.
 */
struct PeInformation
{
    /**
     * @brief   Default constructor.
     */
    PeInformation(void) noexcept(true) = default;

    /**
     * @brief   Default destructor.
     */
    ~PeInformation(void) noexcept(true) = default;

    /**
     * @brief   Struct is movable.
     */
    XPF_CLASS_MOVE_BEHAVIOR(PeInformation, default);

    /**
     * @brief   Struct is not copyable (contains a Buffer which is move-only).
     */
    XPF_CLASS_COPY_BEHAVIOR(PeInformation, delete);

    /**
     * @brief   The raw PE file data. Owned by this structure.
     */
    xpf::Buffer m_FileData;

    /**
     * @brief   True if this is a PE32+ (64-bit) image, false if PE32 (32-bit).
     */
    bool m_Is64Bit = false;

    /**
     * @brief   The target machine type from the COFF header.
     *          Common values: 0x014C (x86), 0x8664 (x64), 0xAA64 (ARM64).
     *
     *          See: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#machine-types
     */
    uint16_t m_Machine = 0;

    /**
     * @brief   The number of sections in the section table.
     */
    uint16_t m_NumberOfSections = 0;

    /**
     * @brief   The combined size of all headers (DOS stub, PE header, section headers)
     *          rounded up to FileAlignment.
     */
    uint32_t m_SizeOfHeaders = 0;

    /**
     * @brief   The number of data directory entries present in the optional header.
     *          Capped at PE_MAX_DATA_DIRECTORIES.
     */
    uint32_t m_NumberOfDataDirectories = 0;

    /**
     * @brief   The offset (in bytes from the start of the file) where
     *          the section headers begin.
     */
    size_t m_SectionHeadersOffset = 0;

    /**
     * @brief   The offset (in bytes from the start of the file) where
     *          the data directory array begins.
     */
    size_t m_DataDirectoryOffset = 0;

    /**
     * @brief   The preferred load address of the image.
     *          32-bit for PE32, 64-bit for PE32+.
     */
    uint64_t m_ImageBase = 0;
};  // struct PeInformation


/**
 * @brief       Parses and validates the headers of a PE file from an in-memory buffer.
 *              On success, the PeInfo structure is populated with the extracted header data.
 *              The raw file data is moved into PeInfo->m_FileData.
 *
 * @param[in]   FileData    - Buffer containing the raw PE file data.
 *                            Ownership is transferred to PeInfo on success.
 *                            On failure the buffer is left in a valid but unspecified state.
 * @param[out]  PeInfo      - On success, populated with the parsed PE header information.
 *                            On failure, the state is cleared.
 *
 * @return      A proper NTSTATUS error code:
 *                  STATUS_SUCCESS          - Headers parsed and validated successfully.
 *                  STATUS_DATA_ERROR       - The buffer does not contain a valid PE file
 *                                           (bad magic, bad signature, inconsistent sizes).
 *                  STATUS_INTEGER_OVERFLOW - Arithmetic overflow during validation.
 */
_Must_inspect_result_
NTSTATUS XPF_API
ParseHeaders(
    _Inout_ xpf::Buffer&& FileData,
    _Out_ xpf::pe::PeInformation* PeInfo
) noexcept(true);

/**
 * @brief       Reads a PE file from disk and parses its headers.
 *              This is a convenience wrapper that opens the file using FileObject,
 *              reads the entire content into a Buffer via chunk-based reading,
 *              then delegates to ParseHeaders for validation.
 *
 * @param[in]   FilePath    - The path to the PE file. On Windows, this should be a DOS-style
 *                            path (e.g., L"C:\\path\\to\\file.exe") or an NT-style path.
 *                            On Linux, this is a standard POSIX path encoded as wchar_t.
 * @param[out]  PeInfo      - On success, populated with the parsed PE header information.
 *                            On failure, the state is cleared.
 *
 * @return      A proper NTSTATUS error code:
 *                  STATUS_SUCCESS              - File read and headers parsed successfully.
 *                  STATUS_DATA_ERROR           - The file does not contain a valid PE file.
 *                  STATUS_INTEGER_OVERFLOW     - Arithmetic overflow during validation.
 *                  Other                       - File I/O errors from FileObject::Create or Read.
 */
_Must_inspect_result_
NTSTATUS XPF_API
ParseFromFile(
    _In_ _Const_ const xpf::StringView<wchar_t>& FilePath,
    _Out_ xpf::pe::PeInformation* PeInfo
) noexcept(true);

};  // namespace pe
};  // namespace xpf
