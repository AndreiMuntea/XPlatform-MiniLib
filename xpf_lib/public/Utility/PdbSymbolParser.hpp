/**
 * @file        xpf_lib/public/Utility/PdbSymbolParser.hpp
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

#pragma once

#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/Containers/Vector.hpp"
#include "xpf_lib/public/Containers/String.hpp"

namespace xpf
{
namespace pdb
{
/**
 * @brief   This is used to store information about a symbol.
 *          Module name is not stored in this as that would be duplicated
 *          information throughout all symbols from a given module.
 *          Caller can corelate them as they please.
 */
struct SymbolInformation
{
    /**
     * @brief   Default constructor.
     */
    SymbolInformation(void) noexcept(true) = default;

    /**
     * @brief   Default destructor.
     */
    ~SymbolInformation(void) noexcept(true) = default;

    /**
     * @brief   Struct is movable.
     */
    XPF_CLASS_MOVE_BEHAVIOR(SymbolInformation, default);

    /**
     * @brief   Struct is not copyable.
     */
    XPF_CLASS_COPY_BEHAVIOR(SymbolInformation, delete);

    /**
     * @brief   The name of the symbol, this can be mangled.
     */
    xpf::String<char> SymbolName;

    /**
     * @brief   The RVA of the symbol, can be used with module base
     *          to compute the actual address.
     */
    uint32_t SymbolRVA = xpf::NumericLimits<uint32_t>::MaxValue();
};  // struct SymbolInformation

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
     * @brief   The index of a block within the MSF file. At this block is an array of ulittle32_t's
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
     * Each substream's contents will be described in detail below. The length of the entire DBI Stream should equal 64
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
     * @brief    The offset in the names buffer of the PDB file containing this module's symbol information.
     */
    uint32_t PdbFilePathNameIndex = 0;

    // char ModuleName[];
    // char ObjFileName[];
});

/**
 * @brief   "Begins at offset 0 immediately after the EC Substream ends, and consumes
 *           Header->OptionalDbgHeaderSize bytes. This field is an array of stream indices
 *           (e.g. uint16_t's), each of which identifies a stream index in the larger MSF
 *           file which contains some additional debug information. Each position of this
 *           array has a special meaning, allowing one to determine what kind of debug
 *           information is at the referenced stream. 11 indices are currently understood,
 *           although it's possible there may be more. The layout of each stream generally
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
 * @brief       Extracts symbols from a pdb file.
 *
 * @param[in]   Pdb         - Pointer to where in memory the pdb file resides.
 * @param[in]   PdbSize     - The size in bytes of the Pdb buffer.
 * @param[out]  Symbols     - Extracted symbols sorted ascending by their RVA value.
 *
 * @return a proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS XPF_API
ExtractSymbols(
    _In_ const void* Pdb,
    _In_ const size_t& PdbSize,
    _Out_ xpf::Vector<xpf::pdb::SymbolInformation>* Symbols
) noexcept(true);
};  // namespace pdb
};  // namespace xpf
