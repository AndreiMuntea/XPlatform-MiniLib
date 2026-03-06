/**
 * @file        xpf_lib/private/Utility/PdbSymbolParser.cpp
 *
 * @brief       In here you'll find functionality for parsing program database (pdb) files.
 *              These files store information useful for debugging a program.
 *              While not properly documented, Microsoft has published the code for generating
 *              such files, and you can find it at https://github.com/Microsoft/microsoft-pdb
 *              Another very useful resource is the llvm documentation available at
 *              https://llvm.org/docs/PDB/index.html Comments and structure are extracted from
 *              these two sources. So I'd like to extend my thanks to both of these for making the
 *              documentation open. This work would have been infinitely harder without these two
 *              sources.
 *
 * @note        This is not a complete pdb parser as it does not cover all edge cases but rather a
 *              minimalistic one tailor made from public M$ symbols which is useful for decorating
 *              stacks with descriptive symbols. Functionality will be added when it is required.
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
 *          We shouldn't attempt PDB parsing at higher IRQLs
 */
XPF_SECTION_PAGED;

namespace xpf
{
namespace pdb
{

/**
 * @brief       Data in pdb files is stored in streams which are represented as blocks of information.
 *              This is a helper method to compute the number of blocks required to represent a specific size.
 *
 * @param[in]   Size            - The size we want to convert in number of blocks.
 * @param[in]   BlockSize       - Size of a single block.
 * @param[out]  NumberOfBlocks  - On output will store the number of blocks of size BlockSize required to store
 *                                the given Size.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
GetNumberOfBlocks(
    _In_ const uint32_t& Size,
    _In_ const uint32_t& BlockSize,
    _Out_ uint32_t* NumberOfBlocks
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity check. */
    XPF_DEATH_ON_FAILURE(NumberOfBlocks != nullptr);

    /* Preinit output. */
    *NumberOfBlocks = 0;

    /* We can't divide by 0. */
    if (BlockSize == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Size != 0)
    {
        if (!xpf::ApiNumbersSafeAdd(Size, BlockSize - 1, NumberOfBlocks))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        *NumberOfBlocks = (*NumberOfBlocks) / BlockSize;
    }
    return STATUS_SUCCESS;
}


/**
 * @brief       This is a helper method which lets us skip a given number of blocks.
 *              Useful for navigating in pdb file faster.
 *
 * @param[in]   PdbHeader       - The beginning of the pdb file.
 * @param[in]   PdbSize         - The size of the pdb file.
 * @param[in]   BlockSize       - The number of bytes a given block has.
 * @param[in]   BlocksToSkip    - Number of blocks to skip from the beginning of the file.
 * @param[out]  Result          - Resulted address after skipping the number of blocks.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
SkipBlocks(
    _In_ const void* PdbHeader,
    _In_ const size_t& PdbSize,
    _In_ const uint32_t& BlockSize,
    _In_ const uint32_t& BlocksToSkip,
    _Out_ const void** Result
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != PdbHeader);
    XPF_DEATH_ON_FAILURE(nullptr != Result);

    /* Preinit output. */
    *Result = nullptr;

    uint32_t offsetInBytes = 0;
    if (!xpf::ApiNumbersSafeMul(BlockSize, BlocksToSkip, &offsetInBytes))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (offsetInBytes > PdbSize)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    *Result = xpf::AlgoAddToPointer(PdbHeader, offsetInBytes);
    return STATUS_SUCCESS;
}


/**
 * @brief       The directory stream is not continous in memory.
 *              This API is used to defragment the stream, making the whole
 *              parsing easier.
 *
 * @param[in]   PdbHeader       - The beginning of the pdb file.
 * @param[in]   PdbSize         - The size of the pdb file.
 * @param[out]  DirectoryStream - On output will store the continuous DirectoryStream.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
DefragmentDirectoryStream(
    _In_ const void* PdbHeader,
    _In_ const size_t& PdbSize,
    _Out_ xpf::Buffer* DirectoryStream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != PdbHeader);
    XPF_DEATH_ON_FAILURE(nullptr != DirectoryStream);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    const xpf::pdb::MsfHeader* msfHeader = nullptr;
    const void* blockMap = nullptr;

    uint32_t streamDirectoryNoBlocks = 0;
    uint32_t streamDirectorySize = 0;

    /* Preinit output. */
    DirectoryStream->Clear();

    /* The pdb size does not include the msf header. Bail. */
    if (PdbSize < sizeof(xpf::pdb::MsfHeader))
    {
        return STATUS_INVALID_PARAMETER;
    }
    msfHeader = static_cast<const xpf::pdb::MsfHeader*>(PdbHeader);

    /* File not a pdb. */
    if (!xpf::ApiEqualMemory(xpf::pdb::MSFHEADER_SIGNATURE, msfHeader->Magic, sizeof(MSFHEADER_SIGNATURE)))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (msfHeader->BlockSize == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Calculate how many blocks are needed to store the stream directory. */
    status = xpf::pdb::GetNumberOfBlocks(msfHeader->DirectorySizeInBytes,
                                         msfHeader->BlockSize,
                                         &streamDirectoryNoBlocks);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (!xpf::ApiNumbersSafeMul(streamDirectoryNoBlocks, msfHeader->BlockSize, &streamDirectorySize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    status = DirectoryStream->Resize(streamDirectorySize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Go to the block map where we'll find an array of uint32_t which describe the pages */
    /* which describe where the stream directory is actually found. */
    status = xpf::pdb::SkipBlocks(msfHeader,
                                  PdbSize,
                                  msfHeader->BlockSize,
                                  msfHeader->BlockMapAddress,
                                  &blockMap);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Make stream directory continuous. */
    for (size_t i = 0; i < streamDirectoryNoBlocks; ++i)
    {
        /* BlockMap[i] describes the next block where stream directory data resides. */
        const uint32_t blockMapIndex = (static_cast<const uint32_t*>(blockMap))[i];
        const void* blockChunk = nullptr;

        status = xpf::pdb::SkipBlocks(msfHeader,
                                      PdbSize,
                                      msfHeader->BlockSize,
                                      blockMapIndex,
                                      &blockChunk);
        if (STATUS_SUCCESS != status)
        {
            return status;
        }

        /* Append data to make it continuous. */
        xpf::ApiCopyMemory(xpf::AlgoAddToPointer(DirectoryStream->GetBuffer(), i * msfHeader->BlockSize),
                           blockChunk,
                           msfHeader->BlockSize);
    }

    /* Sanity check that the stream sizes fit. */
    const xpf::pdb::StreamDirectory* streamDirectory = static_cast<const xpf::pdb::StreamDirectory*>(
                                                       DirectoryStream->GetBuffer());
    uint32_t directoryStreamSize = 0;
    if (!xpf::ApiNumbersSafeAdd(uint32_t{ sizeof(uint32_t) }, streamDirectory->NumberOfStreams, &directoryStreamSize))
    {
        return STATUS_DATA_ERROR;
    }
    if (directoryStreamSize >= PdbSize)
    {
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}


/**
 * @brief       Streams are also not continous.
 *              This function can be used to defragment a specific stream identified by
 *              its index in the stream directory.
 *
 * @param[in]   PdbHeader       - The beginning of the pdb file.
 * @param[in]   PdbSize         - The size of the pdb file.
 * @param[in]   DirectoryStream - The defragmented directorys stream.
 * @param[in]   StreamIndex     - Index of the stream to be defragmented.
 * @param[out]  Stream          - On output will store the continuous defragmented stream.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
DefragmentStream(
    _In_ const xpf::pdb::MsfHeader* PdbHeader,
    _In_ const size_t& PdbSize,
    _In_ const xpf::pdb::StreamDirectory* DirectoryStream,
    _In_ const uint32_t& StreamIndex,
    _Out_ xpf::Buffer* Stream
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != PdbHeader);
    XPF_DEATH_ON_FAILURE(nullptr != DirectoryStream);
    XPF_DEATH_ON_FAILURE(nullptr != Stream);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* After the stream directory there is the StreamSizes[NumStreams]. */
    const uint32_t* streamSizes = static_cast<const uint32_t*>(xpf::AlgoAddToPointer(DirectoryStream,
                                                                                     sizeof(uint32_t)));
    /* After the stream sizes there is the StreamBlocks[NumStreams][]. */
    const uint32_t* streamBlocks = streamSizes + DirectoryStream->NumberOfStreams;

    /* Preinit output. */
    Stream->Clear();

    /* Stream is not found. */
    if (StreamIndex >= DirectoryStream->NumberOfStreams)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Find the blocks associated with this stream, i.e skip over all the other blocks from before. */
    for (size_t i = 0; i < StreamIndex; ++i)
    {
        uint32_t streamSizeInBlocks = 0;
        status = xpf::pdb::GetNumberOfBlocks(streamSizes[i],
                                             PdbHeader->BlockSize,
                                             &streamSizeInBlocks);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        streamBlocks += streamSizeInBlocks;
    }

    /* And now compute the stream size. */
    uint32_t streamSizeInBlocks = 0;
    status = xpf::pdb::GetNumberOfBlocks(streamSizes[StreamIndex],
                                         PdbHeader->BlockSize,
                                         &streamSizeInBlocks);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    uint32_t streamSizeInBytes = 0;
    if (!xpf::ApiNumbersSafeMul(PdbHeader->BlockSize, streamSizeInBlocks, &streamSizeInBytes))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    status = Stream->Resize(streamSizeInBytes);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* And properly defragment the stream. */
    for (size_t i = 0; i < streamSizeInBlocks; ++i)
    {
        const void* blockChunk = nullptr;
        status = xpf::pdb::SkipBlocks(PdbHeader,
                                      PdbSize,
                                      PdbHeader->BlockSize,
                                      streamBlocks[i],
                                      &blockChunk);
        if (STATUS_SUCCESS != status)
        {
            return status;
        }

        /* Append data to make it continuous. */
        xpf::ApiCopyMemory(xpf::AlgoAddToPointer(Stream->GetBuffer(), i * PdbHeader->BlockSize),
                           blockChunk,
                           PdbHeader->BlockSize);
    }

    /* Stream is aligned to block size. Remove extra bytes that we added to simplify copy operations. */
    return Stream->Resize(streamSizes[StreamIndex]);
}


/**
 * @brief       This can be used to extract modules information from the
 *              DebugInformationHeader.
 *
 * @param[in]   DebugInformationHeader - A continuous defragmented stream which points to
 *                                       debug information data.
 * @param[out]  ModulesInformation     - Extracted modules information.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ParseDebugInfoModules(
    _In_ const xpf::pdb::DebugInformationHeader* DebugInformationHeader,
    _Out_ xpf::Vector<xpf::pdb::DebugInformationModuleInfoEntry>* ModulesInformation
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != DebugInformationHeader);
    XPF_DEATH_ON_FAILURE(nullptr != ModulesInformation);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    const xpf::pdb::DebugInformationModuleInfoEntry* modulesStream = nullptr;
    int32_t modulesStreamSize = 0;


    /* Preinit output. */
    ModulesInformation->Clear();

    /* Module information is right after the debug header. */
    modulesStream = static_cast<const xpf::pdb::DebugInformationModuleInfoEntry*>(
                    xpf::AlgoAddToPointer(DebugInformationHeader,
                                          sizeof(xpf::pdb::DebugInformationHeader)));
    modulesStreamSize = DebugInformationHeader->ModInfoSize;

    /* Parse modules now. */
    for (int32_t crtOffset = 0; crtOffset < modulesStreamSize;)
    {
        int32_t finalOffset = 0;
        const xpf::pdb::DebugInformationModuleInfoEntry* entry = nullptr;

        /* Sanity check that we have enough bytes in module stream. */
        if (!xpf::ApiNumbersSafeAdd(crtOffset, int32_t{ sizeof(xpf::pdb::DebugInformationModuleInfoEntry) }, &finalOffset))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (finalOffset > modulesStreamSize)
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        /* Grab the entry and emplace it. */
        entry = static_cast<const xpf::pdb::DebugInformationModuleInfoEntry*>(
                xpf::AlgoAddToPointer(modulesStream,
                                      static_cast<uint32_t>(crtOffset)));
        status = ModulesInformation->Emplace(*entry);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Advance the offset. */
        crtOffset = finalOffset;

        /* We also need to skip the module name. */
        const char* moduleName = static_cast<const char*>(xpf::AlgoAddToPointer(modulesStream,
                                                                                crtOffset));
        size_t moduleNameSize = xpf::ApiStringLength(moduleName);
        if (!xpf::ApiNumbersSafeAdd(moduleNameSize, size_t{ static_cast<uint32_t>(crtOffset) }, &moduleNameSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (!xpf::ApiNumbersSafeAdd(moduleNameSize, sizeof('\0'), &moduleNameSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (moduleNameSize > static_cast<uint32_t>(modulesStreamSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        crtOffset = static_cast<uint32_t>(moduleNameSize);

        /* And the object name. */
        const char* objectName = static_cast<const char*>(xpf::AlgoAddToPointer(modulesStream,
                                                                                crtOffset));
        size_t objectNameSize = xpf::ApiStringLength(objectName);
        if (!xpf::ApiNumbersSafeAdd(objectNameSize, size_t{ static_cast<uint32_t>(crtOffset) }, &objectNameSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (!xpf::ApiNumbersSafeAdd(objectNameSize, sizeof('\0'), &objectNameSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        if (objectNameSize > static_cast<uint32_t>(modulesStreamSize))
        {
            return STATUS_INTEGER_OVERFLOW;
        }
        crtOffset = static_cast<uint32_t>(objectNameSize);

        /* Keep the offset aligned. */
        crtOffset = xpf::AlgoAlignValueUp(crtOffset,
                                          int32_t{ alignof(uint32_t) });
    }

    return STATUS_SUCCESS;
}

/**
 * @brief       This can be used to extract sections from debug information optional header.
 *
 * @param[in]   OptionalDebugHeader - A continuous defragmented strema which points to
 *                                    the optional debug header data.
 * @param[out]  SectionHeaders      - Extracted section headers.
 *
 * @return      A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ParseSectionHeaders(
    _In_ const xpf::pdb::MsfHeader* PdbHeader,
    _In_ const size_t& PdbSize,
    _In_ const xpf::pdb::StreamDirectory* DirectoryStream,
    _In_ const xpf::pdb::DebugInformationOptionalDebugHeader* OptionalDebugHeader,
    _Out_ xpf::Vector<xpf::pdb::ImageSectionHeader>* SectionHeaders
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Sanity checks. */
    XPF_DEATH_ON_FAILURE(nullptr != OptionalDebugHeader);
    XPF_DEATH_ON_FAILURE(nullptr != SectionHeaders);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer sectionHeadersStream{ SectionHeaders->GetAllocator() };

    /* Preinit output. */
    SectionHeaders->Clear();

    /* If section header information is not present, we're done. */
    if (OptionalDebugHeader->SectionHdr == xpf::NumericLimits<uint16_t>::MaxValue())
    {
        return STATUS_SUCCESS;
    }

    /* Defrag section header stream. */
    status = xpf::pdb::DefragmentStream(PdbHeader,
                                        PdbSize,
                                        DirectoryStream,
                                        OptionalDebugHeader->SectionHdr,
                                        &sectionHeadersStream);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Now walk the section stream. */
    if (sectionHeadersStream.GetSize() >= sizeof(xpf::pdb::ImageSectionHeader))
    {
        for (size_t i = 0; i < sectionHeadersStream.GetSize(); i += sizeof(xpf::pdb::ImageSectionHeader))
        {
            const xpf::pdb::ImageSectionHeader* imageSection = static_cast<const xpf::pdb::ImageSectionHeader*>(
                                                               xpf::AlgoAddToPointer(sectionHeadersStream.GetBuffer(), i));
            uint32_t endOfSection = 0;

            /* Sanity check that the section limits are valid. */
            if (!xpf::ApiNumbersSafeAdd(imageSection->VirtualAddress, imageSection->Misc.VirtualSize, &endOfSection))
            {
                return STATUS_INTEGER_OVERFLOW;
            }

            status = SectionHeaders->Emplace(*imageSection);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    }
    return STATUS_SUCCESS;
}


/**
 * @brief       This is used to find the RVA of the symbol.
 *
 * @param[in]   Sections        - previously parsed sections as found in debug optional header.
 * @param[in]   SymbolSection   - section in which the symbol belongs. These are 1-indexed in .pdb files,
 *                                meaning section 1 is actually 0, section 2 is actually 1 and so on.
 * @param[in]   SymbolOffset    - The offset of the symbol in its given section.
 *
 * @return      An optional value - empty if the symbol is malformed and is not found in section,
 *                                  otherwise its actual RVA as translated with the section base.
 *
 * @note        For now, this will skip symbols that points to sections that do not contain code.
 */
static xpf::Optional<uint32_t> XPF_API
SymbolToRva(
    _In_ const xpf::Vector<xpf::pdb::ImageSectionHeader>& Sections,
    _In_ uint32_t SymbolSection,
    _In_ uint32_t SymbolOffset
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::Optional<uint32_t> result;

    /* Shouldn't happen. Malformed PDB.*/
    if (SymbolSection == 0)
    {
        return result;
    }

    /* Actual index in sections. */
    SymbolSection -= 1;

    /* Shouldn't happen. Malformed PDB.*/
    if (SymbolSection >= Sections.Size())
    {
        return result;
    }

    const xpf::pdb::ImageSectionHeader& section = Sections[SymbolSection];

    /* Addition can't overflow as we checked in ParseSectionHeaders. */
    if (SymbolOffset >= section.VirtualAddress + section.Misc.VirtualSize)
    {
        return result;
    }

    /* IMAGE_SCN_CNT_CODE == 0x20 */
    if ((section.Characteristics & 0x20) != 0)
    {
        result.Emplace(SymbolOffset + section.VirtualAddress);
    }

    return result;
}

/**
 * @brief           This is responsible for pasing the symbol information.
 *                  For now we're only looking at a subset of symbols, as we do not want to
 *                  pollute. It can be extended as needed.
 *
 * @param[in]       Sections        - previously parsed sections as found in debug optional header.
 * @param[in]       Stream          - the inspected symbol stream.
 * @param[in]       StreamSize      - the total stream size, in bytes.
 * @param[in,out]   CurrentOffset   - current offset in stream, this is adjusted at the end of this function
 *                                    to skip over this symbol.
 * @param[in,out]   Symbols         - currently parsed symbols, on output this is extended with the new symbol.
 *
 * @return          A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ParseSymbolInformation(
    _In_ const xpf::Vector<xpf::pdb::ImageSectionHeader>& Sections,
    _In_ const void* Stream,
    _In_ const size_t& StreamSize,
    _Inout_ size_t* CurrentOffset,
    _Inout_ xpf::Vector<xpf::pdb::SymbolInformation>& Symbols
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(nullptr != Stream);
    XPF_DEATH_ON_FAILURE(nullptr != CurrentOffset);

    size_t currentOffset = *CurrentOffset;

    /* Read symbol size and symbol type. These are uint16_t - so both are uint32_t */
    if (StreamSize < sizeof(uint32_t) || currentOffset > StreamSize - sizeof(uint32_t))
    {
        /* We reached the end. */
        *CurrentOffset = StreamSize;
        return STATUS_SUCCESS;
    }
    const uint16_t symbolSize = *static_cast<const uint16_t*>(xpf::AlgoAddToPointer(Stream,
                                                                                    currentOffset));
    currentOffset += sizeof(uint16_t);
    const uint16_t symbolType = *static_cast<const uint16_t*>(xpf::AlgoAddToPointer(Stream,
                                                                                    currentOffset));
    currentOffset += sizeof(uint16_t);

    uint16_t symbolSection = xpf::NumericLimits<uint16_t>::MaxValue();
    uint32_t symbolOffset = xpf::NumericLimits<uint32_t>::MaxValue();

    const char* symbolName = nullptr;
    xpf::Optional<uint32_t> symbolRva;

    switch (symbolType)
    {
        case S_LPROC32_ST:
        case S_GPROC32_ST:
        case S_LPROC32:
        case S_GPROC32:
        case S_LPROC32_ID:
        case S_GPROC32_ID:
        case S_LPROC32_DPC:
        case S_LPROC32_DPC_ID:
        {
            const xpf::pdb::ProcSymbol* symbol = static_cast<const xpf::pdb::ProcSymbol*>(xpf::AlgoAddToPointer(Stream,
                                                                                                                currentOffset));
            symbolName = static_cast<const char*>(xpf::AlgoAddToPointer(Stream,
                                                                        currentOffset + sizeof(xpf::pdb::ProcSymbol)));
            symbolSection = symbol->Seg;
            symbolOffset = symbol->Off;
            break;
        }

        case S_THUNK32:
        case S_THUNK32_ST:
        {
            const xpf::pdb::ThunkSymbol* symbol = static_cast<const xpf::pdb::ThunkSymbol*>(xpf::AlgoAddToPointer(Stream,
                                                                                                                  currentOffset));
            symbolName = static_cast<const char*>(xpf::AlgoAddToPointer(Stream,
                                                                        currentOffset + sizeof(xpf::pdb::ThunkSymbol)));
            symbolSection = symbol->Seg;
            symbolOffset = symbol->Off;
            break;
        }

        case S_PUB32:
        case S_PUB32_ST:
        {
            const xpf::pdb::PubSymbol* symbol = static_cast<const xpf::pdb::PubSymbol*>(xpf::AlgoAddToPointer(Stream,
                                                                                                              currentOffset));
            symbolName = static_cast<const char*>(xpf::AlgoAddToPointer(Stream,
                                                                        currentOffset + sizeof(xpf::pdb::PubSymbol)));
            symbolSection = symbol->Seg;
            symbolOffset = symbol->Off;
            break;
        }

        case S_LDATA32:
        case S_LDATA32_ST:
        case S_GDATA32:
        case S_GDATA32_ST:
        case S_LMANDATA:
        case S_LMANDATA_ST:
        case S_GMANDATA:
        case S_GMANDATA_ST:
        {
            const xpf::pdb::DataSymbol* symbol = static_cast<const xpf::pdb::DataSymbol*>(xpf::AlgoAddToPointer(Stream,
                                                                                                                currentOffset));
            symbolName = static_cast<const char*>(xpf::AlgoAddToPointer(Stream,
                                                                        currentOffset + sizeof(xpf::pdb::DataSymbol)));
            symbolSection = symbol->Seg;
            symbolOffset = symbol->Off;
            break;
        }
    }

    /* Now get the RVA. */
    symbolRva = xpf::pdb::SymbolToRva(Sections,
                                      symbolSection,
                                      symbolOffset);

    /* To be a valid symbol we need both name and RVA. */
    if (nullptr != symbolName && symbolRva.HasValue())
    {
        xpf::pdb::SymbolInformation symInfo;

        NTSTATUS status = symInfo.SymbolName.Append(xpf::StringView<char>(symbolName));
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        symInfo.SymbolRVA = *symbolRva;

        /* ??_C@ are mangled strings - we will skip them as they only pollute. */
        if (!symInfo.SymbolName.View().StartsWith("??_c@_", false))
        {
            status = Symbols.Emplace(xpf::Move(symInfo));
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
    }

    /* Now skip over the symbol. A bit tricky here as symbolSize includes symbolType, so we need to go back first. */
    currentOffset -= sizeof(uint16_t);
    currentOffset += symbolSize;

    *CurrentOffset = currentOffset;
    return STATUS_SUCCESS;
}

/**
 * @brief           This is responsible for pasing the symbol information in a whole stream.
 *                  For now we're only looking at a subset of symbols, as we do not want to
 *                  pollute. It can be extended as needed.
 *
 * @param[in]       PdbHeader       - The beginning of the pdb file.
 * @param[in]       PdbSize         - The size of the pdb file.
 * @param[in]       DirectoryStream - The defragmented directorys stream.
 * @parma[in]       StreamIndex     - Index of the stream to be defragmented.
 * @param[in]       Sections        - previously parsed sections as found in debug optional header.
 * @param[in,out]   Symbols         - currently parsed symbols, on output this is extended with the new symbol.
 *
 * @return          A proper NTSTATUS error code.
 */
_Must_inspect_result_
static NTSTATUS XPF_API
ParseSymbolsFromStream(
    _In_ const xpf::pdb::MsfHeader* PdbHeader,
    _In_ const size_t& PdbSize,
    _In_ const xpf::pdb::StreamDirectory* DirectoryStream,
    _In_ const uint32_t& StreamIndex,
    _In_ const xpf::Vector<xpf::pdb::ImageSectionHeader>& Sections,
    _Inout_ xpf::Vector<xpf::pdb::SymbolInformation>& Symbols
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::Buffer defragmentedStream{ Symbols.GetAllocator() };

    /* Make stream continuous. */
    status = xpf::pdb::DefragmentStream(PdbHeader,
                                        PdbSize,
                                        DirectoryStream,
                                        StreamIndex,
                                        &defragmentedStream);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* And now parse it. */
    for (size_t crtOffset = 0; crtOffset < defragmentedStream.GetSize(); )
    {
        status = xpf::pdb::ParseSymbolInformation(Sections,
                                                  defragmentedStream.GetBuffer(),
                                                  defragmentedStream.GetSize(),
                                                  &crtOffset,
                                                  Symbols);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    return STATUS_SUCCESS;
};

_Must_inspect_result_
NTSTATUS XPF_API
ExtractSymbols(
    _In_ const void* Pdb,
    _In_ const size_t& PdbSize,
    _Out_ xpf::Vector<xpf::pdb::SymbolInformation>* Symbols
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(nullptr != Symbols);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    xpf::Buffer directoryStreamBuffer{ Symbols->GetAllocator() };
    xpf::Buffer debugInfoStreamBuffer{ Symbols->GetAllocator() };
    xpf::Vector<xpf::pdb::DebugInformationModuleInfoEntry> modules{ Symbols->GetAllocator() };
    xpf::Vector<xpf::pdb::ImageSectionHeader> sectionHeaders{ Symbols->GetAllocator() };

    const xpf::pdb::MsfHeader* msfHeader = static_cast<const xpf::pdb::MsfHeader*>(Pdb);
    const xpf::pdb::StreamDirectory* streamDirectory = nullptr;
    const xpf::pdb::DebugInformationHeader* debugInfoHeader = nullptr;
    const xpf::pdb::DebugInformationOptionalDebugHeader* optionalDebugInfoHeader = nullptr;

    /* Preinit output. */
    Symbols->Clear();

    /* First we extract the directory header. */
    status = xpf::pdb::DefragmentDirectoryStream(Pdb,
                                                 PdbSize,
                                                 &directoryStreamBuffer);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    streamDirectory = static_cast<const xpf::pdb::StreamDirectory*>(directoryStreamBuffer.GetBuffer());

    /* Get the debug info header. */
    status = xpf::pdb::DefragmentStream(msfHeader,
                                        PdbSize,
                                        streamDirectory,
                                        static_cast<uint32_t>(PdbStreamIndex::DebugInformation),
                                        &debugInfoStreamBuffer);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    debugInfoHeader = static_cast<const xpf::pdb::DebugInformationHeader*>(debugInfoStreamBuffer.GetBuffer());

    if (debugInfoHeader->VersionSignature != PDB_DEBUGINFO_VERSION_SIGNATURE ||
        debugInfoHeader->VersionHeader != PDB_DEBUGINFO_VERSION_HEADER)
    {
        return STATUS_DATA_ERROR;
    }

    /* Grab modules. */
    status = xpf::pdb::ParseDebugInfoModules(debugInfoHeader,
                                             &modules);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Grab sections. */
    optionalDebugInfoHeader = static_cast<const xpf::pdb::DebugInformationOptionalDebugHeader*>(
                              xpf::AlgoAddToPointer(debugInfoHeader,
                                                    sizeof(xpf::pdb::DebugInformationHeader)
                                                    + debugInfoHeader->ModInfoSize
                                                    + debugInfoHeader->SectionContributionSize
                                                    + debugInfoHeader->SectionMapSize
                                                    + debugInfoHeader->FileInfoSize
                                                    + debugInfoHeader->SrcModuleSize
                                                    + debugInfoHeader->ECSubstreamSize));
    status = xpf::pdb::ParseSectionHeaders(msfHeader,
                                           PdbSize,
                                           streamDirectory,
                                           optionalDebugInfoHeader,
                                           &sectionHeaders);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Now the symbols. First the global symbols. */
    status = xpf::pdb::ParseSymbolsFromStream(msfHeader,
                                              PdbSize,
                                              streamDirectory,
                                              debugInfoHeader->SymRecordStream,
                                              sectionHeaders,
                                              *Symbols);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Then per module symbols. */
    for (size_t i = 0; i < modules.Size(); ++i)
    {
        if (modules[i].ModuleSymStream == xpf::NumericLimits<uint16_t>::MaxValue())
        {
            continue;
        }

        status = xpf::pdb::ParseSymbolsFromStream(msfHeader,
                                                  PdbSize,
                                                  streamDirectory,
                                                  modules[i].ModuleSymStream,
                                                  sectionHeaders,
                                                  *Symbols);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    /* Now we sort the symbols using RVA values so they can be easily binary searched. */
    Symbols->Sort([&](const xpf::pdb::SymbolInformation& Left, const xpf::pdb::SymbolInformation& Right)
                  {
                        return Left.SymbolRVA < Right.SymbolRVA;
                  });

    /* Also remove duplicates - Now that they are sorted we can easily do so. */
    for (size_t i = 1; i < Symbols->Size(); )
    {
        if ((*Symbols)[i].SymbolRVA == (*Symbols)[i - 1].SymbolRVA)
        {
            status = Symbols->Erase(i);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        }
        else
        {
            i++;
        }
    }

    return STATUS_SUCCESS;
}
};  // namespace pdb
};  // namespace xpf
