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
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2024.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into paged section.
 *          We shouldn't attempt serialization at higher IRQLs
 */
XPF_SECTION_PAGED;

namespace xpf
{
namespace pdb
{

/**
 * @brief      At file offset 0 in an MSF file is the MSF header.
 *              "msf.cpp#946" in microsoft-pdb repo.
 *              The MSF file format consists of the following components:
 *                  - The header
 *                  - The Free Block Map (also know as Free Page Map, or FPM)
 *                  - Data
 *              Each component is stored as an indexed block, the length of which is
 *              specified in the header.
 */
XPF_PACK(struct MsfHeader
{
    /*
     * @brief   Must be equal to "Microsoft C / C++ MSF 7.00\\r\\n"
     *          followed by the bytes 1A 44 53 00 00 00.
     */
    uint8_t     Magic[32] = { 0 };

    /*
     * @brief   The block size of the internal file system.
     */
    uint32_t    BlockSize = 0;

    /*
     * @brief   The index of a block within the file, at which begins a bitfield
     *          representing the set of all blocks within the file which are "free"
     *          (i.e. the data within that block is not used).
     */
    uint32_t    FreeBlockMap = 0;

    /*
     * @brief   The total number of blocks in the file. NumBlocks * BlockSize should
     *          equal the size of the file on disk.
     */
    uint32_t    BlockCount = 0;

    /*
     * @brief   The size of the stream directory, in bytes. The stream directory contains
     *          information about each stream's size and the set of blocks that it occupies.
     */
    uint32_t    DirectorySizeInBytes = 0;

    /*
     * @brief   Reserved value. (Maybe padding)
     */
    uint32_t    Reserved = 0;

    /*
     * @brief   The index of a block within the MSF file. At this block is an array of ulittle32_t’s
     *          listing the blocks that the stream directory resides on. For large MSF files, the stream directory
     *          (which describes the block layout of each stream) may not fit entirely on a single block.
     *          As a result, this extra layer of indirection is introduced, whereby this block contains the
     *          list of blocks that the stream directory occupies, and the stream directory itself can be
     *          stitched together accordingly. 
     */
    uint32_t    BlockMapAddress = 0;
});

/**
 * @brief   The Stream Directory is the root of all access to the other streams in an MSF file.
 */
XPF_PACK(struct StreamDirectory
{
    /*
     * @brief   Holds the number of streams that are present in the pdb file.
     */
    uint32_t    NumberOfStreams = 0;

    // uint32_t StreamSizes[NumStreams];
    // uint32_t StreamBlocks[NumStreams][];
});

/**
 * @brief   "The PDB format  contains a number  of streams which describe various information
 *           such  as the types, symbols,  source files, and compilands of a program, as well
 *           as some additional  streams  containing hash tables that  are used by  debuggers
 *           and other tools to provide fast lookup of records and types by name, and various
 *           other  information  about  how the  program  was  compiled  such as the specific
 *           toolchain used, and more."
 */
enum class PdbStreamIndex : uint32_t
{
    /**
     * @brief   Contains the MSF Stream Directory.
     */
    StreamDirectory = 0,

    /**
     * @brief   Basic file information such as fields to match EXE to this PDB
     *          and map of named streams to stream indices.
     */
    PdbStream = 1,

    /**
     * @brief  Contains type records.
     */
    TypeRecords = 2,

    /**
     * @brief Module/Compiland Information such as Indices of public / global streams.
     *        Also contains Source File Information and references to streams containing FPO / PGO Data.
     */
    DebugInformation = 3,
};

/**
 * @brief   "The PDB DBI Stream (Index 3) is one of the largest and most important streams in a PDB file.
 *           It contains information about how the program was compiled, (e.g. compilation flags, etc),
 *           the compilands (e.g. object files) that were used to link together the program, the source
 *           files which were used to build the program, as well as references to other streams that contain
 *           more detailed information about each compiland, such as the CodeView symbol records contained
 *           within each compiland and the source and line information for functions and other symbols
 *           within each compiland."
 */
XPF_PACK(struct DebugInformationHeader
{
    /*
     * @brief   Unknown meaning. This is always -1.
     */
    int32_t VersionSignature = 0;

    /*
     * @brief  Can be one of the following values:
     *              VC41 = 930803,
     *              V50 = 19960307,
     *              V60 = 19970606,
     *              V70 = 19990903,
     *              V110 = 20091201
     *         In practice, this value always appears to be V70, and it is not clear what the other values are for.
     */
    uint32_t VersionHeader = 0;

    /*
     * @brief The number of times the PDB has been written.
     */
    uint32_t Age = 0;

    /*
     * @brief The index of the Global Symbol Stream, which contains CodeView symbol records for all global symbols.
     *        Actual records are stored in the symbol record stream, and are referenced from this stream.
     */
    uint16_t GlobalStreamIndex = 0;

    /*
     * @brief Values representing the major and minor version number of the toolchain.
     */
    uint16_t BuildNumber = 0;

    /*
     * @brief The index of the Public Symbol Stream, which contains CodeView symbol records for all public symbols.
     *        Actual records are stored in the symbol record stream, and are referenced from this stream.
     */
    uint16_t PublicStreamIndex = 0;

    /*
     * @brief The version number of mspdbXXXX.dll used to produce this PDB.
     */
    uint16_t PdbDllVersion = 0;

    /*
     * @brief The stream containing all CodeView symbol records used by the program.
     */
    uint16_t SymRecordStream = 0;

    /*
     * @brief Unknown value.
     */
    uint16_t PdbDllRbld = 0;

    /*
     * @brief The length of the Module Info Substream. 
     */
    int32_t ModInfoSize = 0;

    /*
     * @brief The length of the Section Contribution Substream.
     */
    int32_t SectionContributionSize = 0;

    /*
     * @brief The length of the Section Map Substream.
     */
    int32_t SectionMapSize = 0;

    /*
     * @brief  The length of the File Info Substream.
     */
    uint32_t FileInfoSize = 0;

    /*
     * @brief  The length of the Source Info Substream.
     */
    int32_t SrcModuleSize = 0;

    /*
     * @brief The index of the MFC type server in the Type Server Map Substream.
     */
    uint32_t MFCTypeServerIndex = 0;

    /*
     * @brief The length of the Optional Debug Header Stream.
     */
    int32_t OptionalDbgHeaderSize = 0;

    /*
     * @brief The length of the EC Substream.
     */
    int32_t ECSubstreamSize = 0;

    /*
     * @brief     A bitfield with the following layout, containing various information about how the program was built:
     *
     *                  uint16_t WasIncrementallyLinked : 1;
     *                  uint16_t ArePrivateSymbolsStripped : 1;
     *                  uint16_t HasConflictingTypes : 1;
     *                  uint16_t Reserved : 13;
     *
     *           The only one of these that is not self-explanatory is HasConflictingTypes. Although undocumented, link.exe
     *           contains a hidden flag /DEBUG:CTYPES. If it is passed to link.exe, this field will be set.
     *           Otherwise it will not be set. It is unclear what this flag does, although it seems to have subtle implications
     *           on the algorithm used to look up type records.
     */
    uint16_t Flags = 0;

    /*
     * @brief  Common values are 0x8664 (x86-64) and 0x14C (x86).
     */
    uint16_t Machine = 0;

    /*
     * @brief  To keep this structure aligned.
     */
    uint32_t Padding = 0;

    /*
     * Immediately after the fixed-size DBI Stream header are 7 variable-length substreams.
     * The following 7 fields of the DBI Stream header specify the number of bytes of the corresponding substream.
     * Each substream’s contents will be described in detail below. The length of the entire DBI Stream should equal 64
     * (the length of the header above) plus the value of each of the following 7 fields.
     *
     *    () ModInfoSize               - The length of the Module Info Substream.
     *    () SectionContributionSize   - The length of the Section Contribution Substream.
     *    () SectionMapSize            - The length of the Section Map Substream.
     *    () SourceInfoSize            - The length of the File Info Substream.
     *    () TypeServerMapSize         - The length of the Type Server Map Substream.
     *    () OptionalDbgHeaderSize     - The length of the Optional Debug Header Stream.
     *    () ECSubstreamSize           - The length of the EC Substream.
     */
});


/**
 * @brief Expected signature for VersionSignature of DebugInformationHeader
 */
#define PDB_DEBUGINFO_VERSION_SIGNATURE     int32_t{ -1 }

/**
 * @brief Expected signature for VersionHeader of DebugInformationHeader
 */
#define PDB_DEBUGINFO_VERSION_HEADER        uint32_t{ 19990903 }


/**
 * @brief "Begins at offset 0 immediately after the PDB_DEBUGINFORMATION_HEADER.
 *         The module info substream is an array of variable-length records, each
 *         one describing a single module (e.g. object file) linked into the program.
 *         Each record in the array has this format."
 */
XPF_PACK(struct DebugInformationModuleInfoEntry
{
    /*
     * @brief   This is unknown - does not seem to be used anymore.
     */
    uint32_t    Unused1 = 0;

    /*
     * @brief   Describes the properties of the section in the final binary
     *          which contain the code and data from this module.
     *          Can be expanded to the following struct:
     *              struct
     *              {
     *                  uint16_t    Section;            // offset: 0
     *                  char        Padding1[2];        // offset: 2
     *                  int32_t     Offset;             // offset: 4
     *                  int32_t     Size;               // offset: 8
     *                  uint32_t    Characteristics;    // offset: 12
     *                  uint16_t    ModuleIndex;        // offset: 16
     *                  char        Padding2[2];        // offset: 18
     *                  uint32_t    DataCrc;            // offset: 20
     *                  uint32_t    RelocCrc;           // offset: 24
     *              } SectionContr;
     */
    uint8_t SectionContr[28] = { 0 };

    /*
     * @brief   A bitfield with the following format:
     *          // ``true`` if this ModInfo has been written since reading the PDB.  This is
     *          // likely used to support incremental linking, so that the linker can decide
     *          // if it needs to commit changes to disk.
     *              uint16_t Dirty : 1;
     *          // ``true`` if EC information is present for this module. EC is presumed to
     *          // stand for "Edit & Continue", which LLVM does not support.  So this flag
     *          // will always be false.
     *              uint16_t EC : 1;
     *              uint16_t Unused : 6;
     *          // Type Server Index for this module.  This is assumed to be related to /Zi,
     *          // but as LLVM treats /Zi as /Z7, this field will always be invalid for LLVM
     *          // generated PDBs.
     *              uint16_t TSM : 8;
     */
    uint16_t Flags = 0;

    /*
     * @brief   The index of the stream that contains symbol information for this module.
     *          If this field is -1, then no additional debug info will be present for this module
     *          (for example, this is what happens when you strip private symbols from a PDB.
     */
    uint16_t ModuleSymStream = 0;

    /*
     * @brief   The number of bytes of data from the stream identified by ModuleSymStream
     *          that represent CodeView symbol records.
     */
    uint32_t SymByteSize = 0;

    /*
     * @brief   The number of bytes of data from the stream identified by ModuleSymStream that
     *          represent C11-style CodeView line information.
     */
    uint32_t C11ByteSize = 0;

    /*
     * @brief   The number of bytes of data from the stream identified by ModuleSymStream that represent
     *          C13-style CodeView line information. At most one of C11ByteSize and C13ByteSize will be non-zero.
     *          Modern PDBs always use C13 instead of C11.
     */
    uint32_t C13ByteSize = 0;

    /*
     * @brief   The number of source files that contributed to this module during compilation.
     */
    uint16_t SourceFileCount = 0;

    /*
     * @brief   To keep this aligned.
     */
    uint8_t Padding[2] = { 0 };

    /*
     * @brief   This is no longer used anymore.
     */
    uint32_t Unused2 = 0;

    /*
     * @brief   The offset in the names buffer of the primary translation unit used to build this module.
     */
    uint32_t SourceFileNameIndex = 0;

    /*
     * @brief    The offset in the names buffer of the PDB file containing this module’s symbol information. 
     */
    uint32_t PdbFilePathNameIndex = 0;

    // char ModuleName[];
    // char ObjFileName[];
});

/**
 * @brief   "Begins at offset 0 immediately after the EC Substream ends, and consumes
 *           Header->OptionalDbgHeaderSize bytes. This field is an array of stream indices
 *           (e.g. uint16_t’s), each of which identifies a stream index in the larger MSF
 *           file which contains some additional debug information. Each position of this
 *           array has a special meaning, allowing one to determine what kind of debug
 *           information is at the referenced stream. 11 indices are currently understood,
 *           although it’s possible there may be more. The layout of each stream generally
 *           corresponds exactly to a particular type of debug data directory from the PE/COFF
 *           file. The format of these fields can be found in the Microsoft PE/COFF Specification.
 *           If any of these fields is -1, it means the corresponding type of debug info is not
 *           present in the PDB."
 */
XPF_PACK(struct DebugInformationOptionalDebugHeader
{
    /*
     * @brief DbgStreamArray[0]. The data in the referenced stream is an array of FPO_DATA structures.
     *        This contains the relocated contents of any .debug$F section from any of the linker inputs.
     *        See https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-fpo_data
     */
    uint16_t FPO = 0;

    /*
     * @brief DbgStreamArray[1]. The data in the referenced stream is a debug data directory
     *        of type IMAGE_DEBUG_TYPE_EXCEPTION.
     *        See https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_debug_directory
     */
    uint16_t Exception = 0;

    /*
     * @brief DbgStreamArray[2]. The data in the referenced stream is a debug data directory
     *        of type IMAGE_DEBUG_TYPE_FIXUP.
     *        See https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-image_debug_directory
     */
    uint16_t Fixup = 0;

    /*
     * @brief  DbgStreamArray[3]. The data in the referenced stream is a debug data directory of
     *         type IMAGE_DEBUG_TYPE_OMAP_TO_SRC. This mapping helps to translate addresses (RVAs)
     *         from the optimized, final image back to the original, unoptimized source image.
     */
    uint16_t OmapToSource = 0;

    /*
     * @brief  DbgStreamArray[4]. The data in the referenced stream is a debug data directory of
     *         type IMAGE_DEBUG_TYPE_OMAP_FROM_SRC. This mapping allows addresses to be translated
     *         from the source image to the final image.
     */
    uint16_t OmapFromSource = 0;

    /*
     * @brief  DbgStreamArray[5]. A dump of all section headers from the original executable.
     */
    uint16_t SectionHdr = 0;

    /*
     * @brief DbgStreamArray[6]. The layout of this stream is not understood, but it is assumed
     *        to be a mapping from CLR Token to CLR Record ID.
     */
    uint16_t TokenRidMap = 0;

    /*
     * @brief DbgStreamArray[7]. A copy of the .xdata section from the executable.
     */
    uint16_t XData = 0;

    /*
     * @brief DbgStreamArray[8]. This is assumed to be a copy of the .pdata section from the executable,
     *        but that would make it identical to DbgStreamArray[1]. The difference between these two
     *        indices is not well understood. 
     */
    uint16_t PData = 0;

    /*
     * @brief  DbgStreamArray[9]. The data in the referenced stream is a debug data directory of type IMAGE_DEBUG_TYPE_FPO.
     *         Note that this is different from DbgStreamArray[0] in that .debug$F sections are only emitted by MASM.
     *         Thus, it is possible for both to appear in the same PDB if both MASM object files and cl object files are
     *         linked into the same program.
     */
    uint16_t NewFPO = 0;

    /*
     * @brief  DbgStreamArray[10]. Similar to DbgStreamArray[5], but contains the section headers before any binary translation
     *         has been performed. This can be used in conjunction with DebugStreamArray[3] and DbgStreamArray[4] to map
     *         instrumented and uninstrumented addresses.
     */
    uint16_t SectionHdrOriginal = 0;
});


///
/// These are extracted from cvinfo.h - see M$ repo.
///

/**
 * @brief   Local procedure.
 */
#define S_LPROC32           0x110f
/**
 * @brief   Global procedure.
 */
#define S_GPROC32           0x1110
/**
 * @brief   Local procedure start.
 */
#define S_LPROC32_ST        0x100a
/**
 * @brief   Global procedure start.
 */
#define S_GPROC32_ST        0x100b
/**
 * @brief   Local procedure which references id instead of type.
 */
#define S_LPROC32_ID        0x1146
/**
 * @brief   Global procedure which references id instead of type.
 */
#define S_GPROC32_ID        0x1147
/**
 * @brief   DPC Local Procedure.
 */
#define S_LPROC32_DPC       0x1155
/**
 * @brief   DPC Local Procedure which references id instead of type.
 */
#define S_LPROC32_DPC_ID    0x1156

/**
 * @brief   Thunk.
 */
#define S_THUNK32           0x1102
/**
 * @brief   Thunk start.
 */
#define S_THUNK32_ST        0x0206

/**
 * @brief   Public symbol.
 */
#define S_PUB32             0x110e
/**
 * @brief   Public symbol start.
 */
#define S_PUB32_ST          0x1009

/**
 * @brief   Module Local Symbol.
 */
#define S_LDATA32           0x110c
/**
 * @brief   Module Local Symbol Start.
 */
#define S_LDATA32_ST        0x1007
/**
 * @brief   Global Data Symbol.
 */
#define S_GDATA32           0x110d
/**
 * @brief   Global Data Symbol Start.
 */
#define S_GDATA32_ST        0x1008
/**
 * @brief   Module Local Mangled Data.
 */
#define S_LMANDATA          0x111c
/**
 * @brief   Module Local Mangled Data Start.
 */
#define S_LMANDATA_ST       0x1020
/**
 * @brief   Global Mangled Data.
 */
#define S_GMANDATA          0x111d
/**
 * @brief   Global Mangled Data Start.
 */
#define S_GMANDATA_ST       0x1021

/**
 * @brief Structure defining a procedure symbol.
 */
XPF_PACK(struct ProcSymbol
{
    /*
     * @brief Pointer to the parent.
     */
    uint32_t Parent = 0;

    /*
     * @brief Pointer to this blocks end.
     */
    uint32_t End = 0;

    /*
     * @brief Pointer to the next symbol.
     */
    uint32_t Next = 0;

    /*
     * @brief Procedure length.
     */
    uint32_t Len = 0;

    /*
     * @brief Debug start offset.
     */
    uint32_t DbgStart = 0;

    /*
     * @brief Debug end offset.
     */
    uint32_t DbgEnd = 0;

    /*
     * @brief Type index.
     */
    uint32_t Typind = 0;

    /*
     * @brief Offset in section.
     */
    uint32_t Off = 0;

    /*
     * @brief Section in which this symbol resides.
     */
    uint16_t Seg = 0;

    /*
     * @brief (CV_PROCFLAGS) Proc flags.
     */
    uint8_t Flags = 0;
});

/**
 * @brief Structure defining a thunk symbol.
 */
XPF_PACK(struct ThunkSymbol
{
    /*
     * @brief Pointer to the parent.
     */
    uint32_t Parent = 0;

    /*
     * @brief Pointer to this blocks end.
     */
    uint32_t End = 0;

    /*
     * @brief Pointer to next symbol.
     */
    uint32_t Next = 0;

    /*
     * @brief Offset in section.
     */
    uint32_t Off = 0;

    /*
     * @brief Section in which this symbol resides.
     */
    uint16_t Seg = 0;

    /*
     * @brief Length of thunk.
     */
    uint16_t Len = 0;

    /*
     * @brief THUNK_ORDINAL specifying type of thunk.
     */
    uint8_t Ord = 0;
});

/**
 * @brief Structure defining a public symbol.
 */
XPF_PACK(struct PubSymbol
{
    /*
     * @brief   Public symbol flags - we're not using it.
     */
    uint32_t Pubsymflags = 0;

    /*
     * @brief   Offset in section.
     */
    uint32_t Off = 0;

    /*
     * @brief   Section in which this symbol resides.
     */
    uint16_t Seg = 0;
});

/**
 * @brief Structure defining a data symbol.
 */
XPF_PACK(struct DataSymbol
{
    /*
     * @brief   Type index - we're not using it.
     */
    uint32_t TypInd = 0;

    /*
     * @brief   Offset in section.
     */
    uint32_t Off = 0;

    /*
     * @brief   Section in which this symbol resides.
     */
    uint16_t Seg = 0;
});

///
/// This is exposed on windows, but not on linux.
/// In respect with cross platform we redefine it here.
///
struct ImageSectionHeader
{
    char Name[8] = { 0 };

    union
    {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc = { 0 };

    uint32_t VirtualAddress = 0;
    uint32_t SizeOfRawData = 0;
    uint32_t PointerToRawData = 0;
    uint32_t PointerToRelocations = 0;
    uint32_t PointerToLinenumbers = 0;
    uint16_t NumberOfRelocations = 0;
    uint16_t NumberOfLinenumbers = 0;
    uint32_t Characteristics = 0;
};

/**
 * @brief Valid .pdb files are expected to start with these bytes.
 */
static constexpr const char MSFHEADER_SIGNATURE[] = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0";


/**
 * @brief       Data in pdb files is stored in streams which are represented as blocks of information.
 *              This is a helper method to compute the number of blocks required to represent a specific size.
 *
 * @param[in]   Size            - The size we want to convert in number of blocks.
 * @param[in]   BlockSize       - Size of a single block.
 * @parma[out]  NumberOfBlocks  - On output will store the number of blocks of size BlockSize required to store
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
    _Out_ xpf::Buffer<>* DirectoryStream
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
 * @parma[in]   StreamIndex     - Index of the stream to be defragmented.
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
    _Out_ xpf::Buffer<>* Stream
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
    xpf::Buffer<> sectionHeadersStream;

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
        return STATUS_DATA_ERROR;
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
    xpf::Buffer<> defragmentedStream;

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

    xpf::Buffer<> directoryStreamBuffer;
    xpf::Buffer<> debugInfoStreamBuffer;

    xpf::Vector<xpf::pdb::DebugInformationModuleInfoEntry> modules;
    xpf::Vector<xpf::pdb::ImageSectionHeader> sectionHeaders;

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
