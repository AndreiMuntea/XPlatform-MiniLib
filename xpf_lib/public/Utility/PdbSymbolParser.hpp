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
