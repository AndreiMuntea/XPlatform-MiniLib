/**
 * @file        xpf_tests/tests/Utility/TestPdbSymbolParser.cpp
 *
 * @brief       This contains tests for PDB symbol parser.
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
 * @brief       Helper to append a symbol record to a buffer at a given offset.
 *              Layout: uint16_t RecLen | uint16_t RecKind | <StructBytes> | name\0
 */
static void
AppendSymbolRecord(
    _Inout_ xpf::Buffer* Buffer,
    _Inout_ size_t* CurrentOffset,
    _In_ uint16_t SymbolType,
    _In_ const void* StructData,
    _In_ size_t StructSize,
    _In_ const char* Name
) noexcept(true)
{
    size_t nameLen = xpf::ApiStringLength(Name) + 1;
    uint16_t recLen = static_cast<uint16_t>(sizeof(uint16_t) + StructSize + nameLen);

    void* dest = xpf::AlgoAddToPointer(Buffer->GetBuffer(), *CurrentOffset);
    xpf::ApiCopyMemory(dest, &recLen, sizeof(recLen));
    *CurrentOffset += sizeof(recLen);

    dest = xpf::AlgoAddToPointer(Buffer->GetBuffer(), *CurrentOffset);
    xpf::ApiCopyMemory(dest, &SymbolType, sizeof(SymbolType));
    *CurrentOffset += sizeof(SymbolType);

    dest = xpf::AlgoAddToPointer(Buffer->GetBuffer(), *CurrentOffset);
    xpf::ApiCopyMemory(dest, StructData, StructSize);
    *CurrentOffset += StructSize;

    dest = xpf::AlgoAddToPointer(Buffer->GetBuffer(), *CurrentOffset);
    xpf::ApiCopyMemory(dest, Name, nameLen);
    *CurrentOffset += nameLen;
}


/**
 * @brief       Constructs a complete minimal PDB in a buffer with BlockSize=4096, 6 blocks.
 *              Block 0: MsfHeader
 *              Block 1: Block map (directory is in block 2)
 *              Block 2: Stream directory (6 streams: 0-2 empty, 3=DBI, 4=symbols, 5=sections)
 *              Block 3: DBI header + optional debug header
 *              Block 4: Symbol record stream
 *              Block 5: Section headers stream
 */
static NTSTATUS
BuildMinimalPdb(
    _Out_ xpf::Buffer* PdbBuffer,
    _In_ const void* SymbolData,
    _In_ size_t SymbolDataSize,
    _In_ const void* SectionData,
    _In_ size_t SectionDataSize
) noexcept(true)
{
    const uint32_t blockSize = 4096;
    const uint32_t blockCount = 6;
    const size_t totalSize = static_cast<size_t>(blockSize) * blockCount;

    NTSTATUS status = PdbBuffer->Resize(totalSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    xpf::ApiZeroMemory(PdbBuffer->GetBuffer(), totalSize);

    uint8_t* base = static_cast<uint8_t*>(PdbBuffer->GetBuffer());

    /* ---- Block 0: MSF Header ---- */
    xpf::pdb::MsfHeader* header = static_cast<xpf::pdb::MsfHeader*>(PdbBuffer->GetBuffer());
    xpf::ApiCopyMemory(header->Magic, xpf::pdb::MSFHEADER_SIGNATURE, sizeof(xpf::pdb::MSFHEADER_SIGNATURE));
    header->BlockSize = blockSize;
    header->FreeBlockMap = 0;
    header->BlockCount = blockCount;
    header->BlockMapAddress = 1;

    /*
     * Stream directory layout (in block 2):
     *   uint32_t NumberOfStreams = 6
     *   uint32_t StreamSizes[6] = { 0, 0, 0, dbiStreamSize, symbolDataSize, sectionDataSize }
     *   uint32_t StreamBlocks for stream 3 = { 3 }
     *   uint32_t StreamBlocks for stream 4 = { 4 }
     *   uint32_t StreamBlocks for stream 5 = { 5 }
     *
     * Total directory size = 4 + 6*4 + 3*4 = 40 bytes
     */
    uint32_t dbiStreamSize = static_cast<uint32_t>(sizeof(xpf::pdb::DebugInformationHeader)
                                                   + sizeof(xpf::pdb::DebugInformationOptionalDebugHeader));
    header->DirectorySizeInBytes = 4 + 6 * 4 + 3 * 4;

    /* ---- Block 1: Block map ---- */
    uint32_t* blockMap = reinterpret_cast<uint32_t*>(base + blockSize);
    blockMap[0] = 2;  /* directory is in block 2 */

    /* ---- Block 2: Stream directory ---- */
    uint32_t* directory = reinterpret_cast<uint32_t*>(base + 2 * blockSize);
    directory[0] = 6;                                                       /* NumberOfStreams */
    directory[1] = 0;                                                       /* StreamSize[0] */
    directory[2] = 0;                                                       /* StreamSize[1] */
    directory[3] = 0;                                                       /* StreamSize[2] */
    directory[4] = dbiStreamSize;                                           /* StreamSize[3] = DBI */
    directory[5] = static_cast<uint32_t>(SymbolDataSize);                   /* StreamSize[4] = symbols */
    directory[6] = static_cast<uint32_t>(SectionDataSize);                  /* StreamSize[5] = sections */
    directory[7] = 3;                                                       /* StreamBlocks[3] = block 3 */
    directory[8] = 4;                                                       /* StreamBlocks[4] = block 4 */
    directory[9] = 5;                                                       /* StreamBlocks[5] = block 5 */

    /* ---- Block 3: DBI stream ---- */
    xpf::pdb::DebugInformationHeader* dbi = reinterpret_cast<xpf::pdb::DebugInformationHeader*>(
                                            base + 3 * blockSize);
    dbi->VersionSignature = PDB_DEBUGINFO_VERSION_SIGNATURE;
    dbi->VersionHeader = PDB_DEBUGINFO_VERSION_HEADER;
    dbi->SymRecordStream = 4;
    dbi->OptionalDbgHeaderSize = static_cast<int32_t>(sizeof(xpf::pdb::DebugInformationOptionalDebugHeader));
    /* All substream sizes except OptionalDbgHeaderSize are 0. */

    xpf::pdb::DebugInformationOptionalDebugHeader* optDbg =
        reinterpret_cast<xpf::pdb::DebugInformationOptionalDebugHeader*>(base + 3 * blockSize
                                                                        + sizeof(xpf::pdb::DebugInformationHeader));
    optDbg->FPO = 0xFFFF;
    optDbg->Exception = 0xFFFF;
    optDbg->Fixup = 0xFFFF;
    optDbg->OmapToSource = 0xFFFF;
    optDbg->OmapFromSource = 0xFFFF;
    optDbg->SectionHdr = 5;
    optDbg->TokenRidMap = 0xFFFF;
    optDbg->XData = 0xFFFF;
    optDbg->PData = 0xFFFF;
    optDbg->NewFPO = 0xFFFF;
    optDbg->SectionHdrOriginal = 0xFFFF;

    /* ---- Block 4: Symbol stream ---- */
    if (SymbolData != nullptr && SymbolDataSize > 0)
    {
        xpf::ApiCopyMemory(base + 4 * blockSize, SymbolData, SymbolDataSize);
    }

    /* ---- Block 5: Section headers stream ---- */
    if (SectionData != nullptr && SectionDataSize > 0)
    {
        xpf::ApiCopyMemory(base + 5 * blockSize, SectionData, SectionDataSize);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief       Creates a standard .text code section header for testing.
 */
static xpf::pdb::ImageSectionHeader
MakeCodeSection(
    _In_ uint32_t VirtualAddress,
    _In_ uint32_t VirtualSize
) noexcept(true)
{
    xpf::pdb::ImageSectionHeader section;
    xpf::ApiZeroMemory(&section, sizeof(section));

    section.Name[0] = '.';
    section.Name[1] = 't';
    section.Name[2] = 'e';
    section.Name[3] = 'x';
    section.Name[4] = 't';
    section.VirtualAddress = VirtualAddress;
    section.Misc.VirtualSize = VirtualSize;
    section.Characteristics = 0x60000020;   /* IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ */

    return section;
}


/* ============================================================================================= */
/* ===                                  Error Path Tests                                     === */
/* ============================================================================================= */

/**
 * @brief       PDB smaller than MSF header should fail.
 */
XPF_TEST_SCENARIO(TestPdbParser, TruncatedInput)
{
    uint8_t tinyBuf[10] = { 0 };
    xpf::Vector<xpf::pdb::SymbolInformation> symbols;

    NTSTATUS status = xpf::pdb::ExtractSymbols(tinyBuf, sizeof(tinyBuf), &symbols);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       56 bytes with garbage magic should fail.
 */
XPF_TEST_SCENARIO(TestPdbParser, InvalidMagic)
{
    uint8_t buf[sizeof(xpf::pdb::MsfHeader)] = { 0 };
    buf[0] = 'X';
    xpf::Vector<xpf::pdb::SymbolInformation> symbols;

    NTSTATUS status = xpf::pdb::ExtractSymbols(buf, sizeof(buf), &symbols);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid magic but BlockSize=0 should fail.
 */
XPF_TEST_SCENARIO(TestPdbParser, ZeroBlockSize)
{
    xpf::pdb::MsfHeader header;
    xpf::ApiZeroMemory(&header, sizeof(header));
    xpf::ApiCopyMemory(header.Magic, xpf::pdb::MSFHEADER_SIGNATURE, sizeof(xpf::pdb::MSFHEADER_SIGNATURE));
    header.BlockSize = 0;

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(&header, sizeof(header), &symbols);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Full PDB with invalid DBI VersionSignature should fail.
 */
XPF_TEST_SCENARIO(TestPdbParser, InvalidDbiVersionSignature)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);
    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, nullptr, 0, &section, sizeof(section))));

    /* Patch VersionSignature to 0 (invalid). */
    uint8_t* base = static_cast<uint8_t*>(pdbBuffer.GetBuffer());
    xpf::pdb::DebugInformationHeader* dbi = reinterpret_cast<xpf::pdb::DebugInformationHeader*>(base + 3 * 4096);
    dbi->VersionSignature = 0;

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Full PDB with invalid DBI VersionHeader should fail.
 */
XPF_TEST_SCENARIO(TestPdbParser, InvalidDbiVersionHeader)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);
    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, nullptr, 0, &section, sizeof(section))));

    /* Patch VersionHeader to 0 (invalid). */
    uint8_t* base = static_cast<uint8_t*>(pdbBuffer.GetBuffer());
    xpf::pdb::DebugInformationHeader* dbi = reinterpret_cast<xpf::pdb::DebugInformationHeader*>(base + 3 * 4096);
    dbi->VersionHeader = 0;

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(!NT_SUCCESS(status));
}

/**
 * @brief       Valid PDB with empty symbol stream and one code section should succeed with 0 symbols.
 */
XPF_TEST_SCENARIO(TestPdbParser, EmptySymbolStream)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);
    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, nullptr, 0, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 0);
}


/* ============================================================================================= */
/* ===                              Single Symbol Type Tests                                 === */
/* ============================================================================================= */

/**
 * @brief       Single S_GPROC32 symbol should produce 1 symbol with correct RVA.
 */
XPF_TEST_SCENARIO(TestPdbParser, SingleGProc32)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    /* Build symbol record. */
    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));
    proc.Off = 0x100;
    proc.Seg = 1;

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "TestFunction");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 1);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1100);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolName.View().Equals("TestFunction", true));
}

/**
 * @brief       Single S_PUB32 symbol should produce 1 symbol with correct RVA.
 */
XPF_TEST_SCENARIO(TestPdbParser, SinglePub32)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::PubSymbol pub;
    xpf::ApiZeroMemory(&pub, sizeof(pub));
    pub.Off = 0x200;
    pub.Seg = 1;

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "PublicSymbol");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 1);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1200);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolName.View().Equals("PublicSymbol", true));
}

/**
 * @brief       Single S_THUNK32 symbol should produce 1 symbol with correct RVA.
 */
XPF_TEST_SCENARIO(TestPdbParser, SingleThunk32)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ThunkSymbol thunk;
    xpf::ApiZeroMemory(&thunk, sizeof(thunk));
    thunk.Off = 0x50;
    thunk.Seg = 1;

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_THUNK32, &thunk, sizeof(thunk), "_thunk_fn");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 1);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1050);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolName.View().Equals("_thunk_fn", true));
}

/**
 * @brief       Single S_GDATA32 symbol should produce 1 symbol with correct RVA.
 */
XPF_TEST_SCENARIO(TestPdbParser, SingleGData32)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::DataSymbol data;
    xpf::ApiZeroMemory(&data, sizeof(data));
    data.Off = 0x300;
    data.Seg = 1;

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_GDATA32, &data, sizeof(data), "GlobalData");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 1);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1300);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolName.View().Equals("GlobalData", true));
}


/* ============================================================================================= */
/* ===                           Filtering and Deduplication Tests                           === */
/* ============================================================================================= */

/**
 * @brief       Mangled string names (??_c@_...) should be filtered out.
 */
XPF_TEST_SCENARIO(TestPdbParser, FilterMangledStrings)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::PubSymbol pub;
    xpf::ApiZeroMemory(&pub, sizeof(pub));
    pub.Seg = 1;

    size_t offset = 0;

    /* This should be filtered (mangled string literal). */
    pub.Off = 0x100;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "??_c@_0BAA@HASH@lit");

    /* This should be kept. */
    pub.Off = 0x200;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "ValidFunc");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 1);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolName.View().Equals("ValidFunc", true));
}

/**
 * @brief       Symbols in a non-code section (no IMAGE_SCN_CNT_CODE bit) should be filtered.
 */
XPF_TEST_SCENARIO(TestPdbParser, FilterNonCodeSection)
{
    xpf::pdb::ImageSectionHeader section;
    xpf::ApiZeroMemory(&section, sizeof(section));
    section.Name[0] = '.';
    section.Name[1] = 'd';
    section.Name[2] = 'a';
    section.Name[3] = 't';
    section.Name[4] = 'a';
    section.VirtualAddress = 0x1000;
    section.Misc.VirtualSize = 0x2000;
    section.Characteristics = 0x40000040;   /* IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA - no code */

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));
    proc.Off = 0x100;
    proc.Seg = 1;

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "SomeFunc");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 0);
}

/**
 * @brief       Symbol with Seg=0 (invalid section index) should produce 0 symbols.
 */
XPF_TEST_SCENARIO(TestPdbParser, FilterSectionIndexZero)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::PubSymbol pub;
    xpf::ApiZeroMemory(&pub, sizeof(pub));
    pub.Off = 0x100;
    pub.Seg = 0;    /* Invalid - sections are 1-indexed. */

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "BadSeg");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 0);
}

/**
 * @brief       Symbol with Seg beyond section count should produce 0 symbols.
 */
XPF_TEST_SCENARIO(TestPdbParser, SegmentBeyondSectionCount)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::PubSymbol pub;
    xpf::ApiZeroMemory(&pub, sizeof(pub));
    pub.Off = 0x100;
    pub.Seg = 3;    /* Only 1 section exists, so section index 3 is out of range. */

    size_t offset = 0;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "TooFar");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 0);
}

/**
 * @brief       Duplicate RVAs should be deduplicated.
 */
XPF_TEST_SCENARIO(TestPdbParser, DuplicateRvasDedup)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));
    proc.Seg = 1;

    size_t offset = 0;

    /* A and B have same offset -> same RVA -> should be deduped. */
    proc.Off = 0x100;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncA");
    proc.Off = 0x100;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncB");
    proc.Off = 0x200;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncC");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 2);
}


/* ============================================================================================= */
/* ===                          Multiple Symbols and Sorting Tests                           === */
/* ============================================================================================= */

/**
 * @brief       Multiple procs in reverse offset order should be sorted ascending by RVA.
 */
XPF_TEST_SCENARIO(TestPdbParser, MultipleSortedByRva)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));
    proc.Seg = 1;

    size_t offset = 0;

    /* Insert in reverse order. */
    proc.Off = 0x300;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncC");
    proc.Off = 0x200;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncB");
    proc.Off = 0x100;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncA");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 3);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1100);
    XPF_TEST_EXPECT_TRUE(symbols[1].SymbolRVA == 0x1200);
    XPF_TEST_EXPECT_TRUE(symbols[2].SymbolRVA == 0x1300);
}

/**
 * @brief       One symbol of each type: GPROC32, PUB32, THUNK32, GDATA32 -> 4 symbols sorted.
 */
XPF_TEST_SCENARIO(TestPdbParser, MultipleSymbolTypes)
{
    xpf::pdb::ImageSectionHeader section = MakeCodeSection(0x1000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    size_t offset = 0;

    /* GPROC32 */
    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));
    proc.Off = 0x400;
    proc.Seg = 1;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "ProcFunc");

    /* PUB32 */
    xpf::pdb::PubSymbol pub;
    xpf::ApiZeroMemory(&pub, sizeof(pub));
    pub.Off = 0x100;
    pub.Seg = 1;
    AppendSymbolRecord(&symBuf, &offset, S_PUB32, &pub, sizeof(pub), "PubFunc");

    /* THUNK32 */
    xpf::pdb::ThunkSymbol thunk;
    xpf::ApiZeroMemory(&thunk, sizeof(thunk));
    thunk.Off = 0x200;
    thunk.Seg = 1;
    AppendSymbolRecord(&symBuf, &offset, S_THUNK32, &thunk, sizeof(thunk), "ThunkFunc");

    /* GDATA32 */
    xpf::pdb::DataSymbol data;
    xpf::ApiZeroMemory(&data, sizeof(data));
    data.Off = 0x300;
    data.Seg = 1;
    AppendSymbolRecord(&symBuf, &offset, S_GDATA32, &data, sizeof(data), "DataSym");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, &section, sizeof(section))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 4);

    /* Should be sorted by RVA. */
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1100);   /* PUB32:  0x1000 + 0x100 */
    XPF_TEST_EXPECT_TRUE(symbols[1].SymbolRVA == 0x1200);   /* THUNK:  0x1000 + 0x200 */
    XPF_TEST_EXPECT_TRUE(symbols[2].SymbolRVA == 0x1300);   /* GDATA:  0x1000 + 0x300 */
    XPF_TEST_EXPECT_TRUE(symbols[3].SymbolRVA == 0x1400);   /* GPROC:  0x1000 + 0x400 */
}

/**
 * @brief       Two code sections: symbols from each should have correct RVAs.
 */
XPF_TEST_SCENARIO(TestPdbParser, MultipleCodeSections)
{
    xpf::pdb::ImageSectionHeader sections[2];
    sections[0] = MakeCodeSection(0x1000, 0x2000);
    sections[1] = MakeCodeSection(0x3000, 0x2000);

    xpf::Buffer symBuf;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(symBuf.Resize(4096)));
    xpf::ApiZeroMemory(symBuf.GetBuffer(), symBuf.GetSize());

    xpf::pdb::ProcSymbol proc;
    xpf::ApiZeroMemory(&proc, sizeof(proc));

    size_t offset = 0;

    /* Symbol in section 1. */
    proc.Off = 0x100;
    proc.Seg = 1;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncInSec1");

    /* Symbol in section 2. */
    proc.Off = 0x200;
    proc.Seg = 2;
    AppendSymbolRecord(&symBuf, &offset, S_GPROC32, &proc, sizeof(proc), "FuncInSec2");

    xpf::Buffer pdbBuffer;
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(BuildMinimalPdb(&pdbBuffer, symBuf.GetBuffer(), offset, sections, sizeof(sections))));

    xpf::Vector<xpf::pdb::SymbolInformation> symbols;
    NTSTATUS status = xpf::pdb::ExtractSymbols(pdbBuffer.GetBuffer(), pdbBuffer.GetSize(), &symbols);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    XPF_TEST_EXPECT_TRUE(symbols.Size() == 2);
    XPF_TEST_EXPECT_TRUE(symbols[0].SymbolRVA == 0x1100);   /* section 1: 0x1000 + 0x100 */
    XPF_TEST_EXPECT_TRUE(symbols[1].SymbolRVA == 0x3200);   /* section 2: 0x3000 + 0x200 */
}
