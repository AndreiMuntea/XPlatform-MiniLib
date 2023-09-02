/**
 * @file        xpf_lib/public/core/platform_specific/CrossPlatformStatus.hpp
 *
 * @brief       This file with the status definition throughout the project.
 *              Use this only for non-windows platforms as on windows
 *              this is NTSTATUS.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#pragma once

#if defined (__linux__)

    /**
     * @brief Use a signed int to store the status.
     *        This will make the computations easier.
     */
    typedef signed int NTSTATUS;

    /**
     * @brief Helper MACRO to test for success - if the first bit is set,
     *        it means it is an error (a negative number).
     */
    #define NT_SUCCESS(Status)                  (((NTSTATUS)(Status)) >= 0)

    /**
     * @brief Helper MACRO to convert an error to an NTSTATUS.
     *        Just set the first bit.
     */
    #define NTSTATUS_FROM_PLATFORM_ERROR(x)     ((NTSTATUS) (((x) | 0xC0000000)))

    //
    // SUCCESS TYPES
    //
    #define STATUS_SUCCESS                      ((NTSTATUS)0x00000000L)

    //
    // ERROR TYPES
    //
    #define STATUS_BUFFER_OVERFLOW              ((NTSTATUS)0x80000005L)
    #define STATUS_NO_DATA_DETECTED             ((NTSTATUS)0x80000022L)
    #define STATUS_UNSUCCESSFUL                 ((NTSTATUS)0xC0000001L)
    #define STATUS_INVALID_PARAMETER            ((NTSTATUS)0xC000000DL)
    #define STATUS_QUOTA_EXCEEDED               ((NTSTATUS)0xC0000044L)
    #define STATUS_MUTANT_NOT_OWNED             ((NTSTATUS)0xC0000046L)
    #define STATUS_INTEGER_OVERFLOW             ((NTSTATUS)0xC0000095L)
    #define STATUS_INSUFFICIENT_RESOURCES       ((NTSTATUS)0xC000009AL)
    #define STATUS_NOT_SUPPORTED                ((NTSTATUS)0xC00000BBL)
    #define STATUS_INVALID_BUFFER_SIZE          ((NTSTATUS)0xC0000206L)
    #define STATUS_SHUTDOWN_IN_PROGRESS         ((NTSTATUS)0xC00002FEL)
    #define STATUS_INVALID_STATE_TRANSITION     ((NTSTATUS)0xC000A003L)

#endif
