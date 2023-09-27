/**
 * @file        xpf_tests/win_km_build/CppSupport/CppSupport.hpp
 *
 * @brief       Header file with routines to control cpp support in km.
 * 
 * @details     This file contains the definition of init/deinit routines
 *              for cpp support to function properly in KM.
 *              Please see http://www.osronline.com/article.cfm%5earticle=57.htm
 *              for more details.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include <xpf_lib/xpf.hpp>

/**
 * @brief CPP support was not yet initialized.
 *        ensure these APIs are not name mangled
 *        so the KM compiler will recognize them.
 *
 */
XPF_EXTERN_C_START();


/**
 * @brief This will be called in driver entry routine and is used to
 *        initialize the cpp support in KM.
 *
 *
 * @return STATUS_SUCCESS if everything went good, or STATUS_NOT_SUPPORTED if not.
 *         In such scenarios it means something went wrong with the build
 *         and internal pointers are considered invalid.
 *         You should check the compile options and ensure everthing is in order.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
XpfInitializeCppSupport(
    void
);

/**
 * @brief This will be called in driver unload routine and is used to
 *        deinitialize the cpp support in KM.
 *        Do not construct any other objects or use any cpp after this is called.
 *
 * 
 * @return Nothing.
 */
void
XPF_API
XpfDeinitializeCppSupport(
    void
);

/**
 * @brief Don't add anything after this macro.
 *        Stop the C-Specific section here.
 */
XPF_EXTERN_C_END();
