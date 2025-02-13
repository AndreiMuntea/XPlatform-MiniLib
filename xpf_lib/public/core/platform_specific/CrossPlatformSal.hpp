//
// @file        xpf_lib/public/core/platform_specific/CrossPlatformSal.hpp
//
// @brief       This file helps by including cross platform definition for
//              source-code annotation language (SAL).
//              We take the easy route and only define it for MSVC and all other
//              compilers will translate the definitions to none.
//              When we use a new SAL definition we'll add it here (lazy).
//
// @note        This file is skipped from doxygen generation.
//              So the comments are intended to be with slash rather than asterix.
//
// @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
//
// @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
//              All rights reserved.
//
// @license     See top-level directory LICENSE file.
//

#pragma once

#if defined _MSC_VER
    #include <sal.h>
#else

    #if defined _In_
        #undef _In_
    #endif  // _In_
    #define _In_

    #if defined _Out_
        #undef _Out_
    #endif  // _Out_
    #define _Out_

    #if defined _Inout_
        #undef _Inout
    #endif  // _Inout_
    #define _Inout_

    #if defined _In_opt_
        #undef _In_opt_
    #endif  // _In_opt_
    #define _In_opt_

    #if defined _Out_opt_
        #undef _Out_opt_
    #endif  // _Out_opt_
    #define _Out_opt_

    #if defined _Inout_opt_
        #undef _Inout_opt_
    #endif  // _Inout_opt_
    #define _Inout_opt_

    #if defined _Const_
        #undef _Const_
    #endif  // _Const_
    #define _Const_

    #if defined _Check_return_
        #undef _Check_return_
    #endif  // _Check_return_
    #define _Check_return_

    #if defined _Ret_maybenull_
        #undef _Ret_maybenull_
    #endif  // _Ret_maybenull_
    #define _Ret_maybenull_

    #if defined _Must_inspect_result_
        #undef _Must_inspect_result_
    #endif  // _Must_inspect_result_
    #define _Must_inspect_result_

    #if defined _Out_writes_bytes_all_
        #undef _Out_writes_bytes_all_
    #endif  // _Out_writes_bytes_all_
    #define _Out_writes_bytes_all_(Unused)

    #if defined _In_reads_bytes_
        #undef _In_reads_bytes_
    #endif  // _In_reads_bytes_
    #define _In_reads_bytes_(Unused)

    #if defined _Analysis_assume_
        #undef _Analysis_assume_
    #endif  // _Analysis_assume_
    #define _Analysis_assume_(Unused)

    #if defined _IRQL_requires_max_
        #undef _IRQL_requires_max_
    #endif  // _IRQL_requires_max_
    #define _IRQL_requires_max_(Unused)

    #if defined _Use_decl_annotations_
        #undef _Use_decl_annotations_
    #endif  // _Use_decl_annotations_
    #define _Use_decl_annotations_

#endif  // _MSC_VER
