/// 
/// MIT License
/// 
/// Copyright(c) 2020 - 2022 MUNTEA ANDREI-MARIUS (munteaandrei17@gmail.com)
/// 
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files(the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions :
/// 
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.
/// 

#ifndef __XPLATFORM_SPECIFIC_API_HPP__
#define __XPLATFORM_SPECIFIC_API_HPP__

//
// This file contains the standalone APIs that are used along the project.
// They are compiler-specific or platform specific.
// It groups them togheter to ensure the same output.
//

namespace XPF
{

    //
    // Uses compiler intrinsics to atomically increment a given value.
    // Number is the address of the number to be atomically incremented.
    // Returns the value after the increment was performed.
    //    The same value is also stored to location pointed by the Number parameter.
    // This API does not account for overflow!
    //
    template <class T>
    _Must_inspect_result_
    inline T
    ApiAtomicIncrement(
        _Inout_ volatile T* Number
    ) noexcept
    {
        static_assert(
            XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_int8_t>  ||
            XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t> ||
            XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t> ||
            XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>,
            "Unsuported Type Operation"
        );

        /// This API should not be used with an unaligned pointer.
        /// Interlocked APIs have an undefined behavior on unaligned pointers.
        /// Assert here and investigate the case on debug builds.
        XPLATFORM_ASSERT(XPF::IsPointerAligned(Number, sizeof(T)));

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t> || XPF::IsSameType<T, xp_int8_t>)
            {
                return static_cast<T>(InterlockedExchangeAdd8(reinterpret_cast<volatile xp_char8_t*>(Number), xp_char8_t{ 1 })) + T{ 1 };
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t>)
            {
                return static_cast<T>(InterlockedIncrement16(reinterpret_cast<volatile xp_int16_t*>(Number)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t>)
            {
                return static_cast<T>(InterlockedIncrement(reinterpret_cast<volatile xp_uint32_t*>(Number)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>)
            {
                return static_cast<T>(InterlockedIncrement64(reinterpret_cast<volatile xp_int64_t*>(Number)));
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return __sync_add_and_fetch(Number, T{ 1 });
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Uses compiler intrinsics to atomically decrement a given value.
    // Number is the address of the number to be atomically decremented.
    // Returns the value after the decrement was performed.
    //    The same value is also stored to location pointed by the Number parameter.
    // This API does not account for underflow!
    //
    template <class T>
    _Must_inspect_result_
    inline T
    ApiAtomicDecrement(
        _Inout_ volatile T* Number
    ) noexcept
    {
        static_assert(
            XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_int8_t>  ||
            XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t> ||
            XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t> ||
            XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>,
            "Unsuported Type Operation"
        );

        /// This API should not be used with an unaligned pointer.
        /// Interlocked APIs have an undefined behavior on unaligned pointers.
        /// Assert here and investigate the case on debug builds.
        XPLATFORM_ASSERT(XPF::IsPointerAligned(Number, sizeof(T)));

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t> || XPF::IsSameType<T, xp_int8_t>)
            {
                return static_cast<T>(InterlockedExchangeAdd8(reinterpret_cast<volatile xp_char8_t*>(Number), xp_char8_t{ -1 })) - T{ 1 };
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t>)
            {
                return static_cast<T>(InterlockedDecrement16(reinterpret_cast<volatile xp_int16_t*>(Number)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t>)
            {
                return static_cast<T>(InterlockedDecrement(reinterpret_cast<volatile xp_uint32_t*>(Number)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>)
            {
                return static_cast<T>(InterlockedDecrement64(reinterpret_cast<volatile xp_int64_t*>(Number)));
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return __sync_sub_and_fetch(Number, T{ 1 });
        #else
            #error Unsuported Compiler
        #endif
    }


    //
    // Uses compiler intrinsics to atomically change a value.
    // Target is the address of the number to be atomically changed.
    // Returns the initial value after the decrement was performed.
    //
    template <class T>
    _Must_inspect_result_
    inline T
    ApiAtomicExchange(
        _Inout_ volatile T* Target,
        _In_  const T& Value
    ) noexcept
    {
        static_assert(
            XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_int8_t>  ||
            XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t> ||
            XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t> ||
            XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>,
            "Unsuported Type Operation"
        );

        /// This API should not be used with an unaligned pointer.
        /// Interlocked APIs have an undefined behavior on unaligned pointers.
        /// Assert here and investigate the case on debug builds.
        XPLATFORM_ASSERT(XPF::IsPointerAligned(Target, sizeof(T)));

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t> || XPF::IsSameType<T, xp_int8_t>)
            {
                return static_cast<T>(InterlockedExchange8(reinterpret_cast<volatile xp_char8_t*>(Target), static_cast<xp_char8_t>(Value)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t> || XPF::IsSameType<T, xp_int16_t>)
            {
                return static_cast<T>(InterlockedExchange16(reinterpret_cast<volatile xp_int16_t*>(Target), static_cast<xp_int16_t>(Value)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_int32_t>)
            {
                return static_cast<T>(InterlockedExchange(reinterpret_cast<volatile xp_uint32_t*>(Target), static_cast<xp_uint32_t>(Value)));
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t> || XPF::IsSameType<T, xp_int64_t>)
            {
                return static_cast<T>(InterlockedExchange64(reinterpret_cast<volatile xp_int64_t*>(Target), static_cast<xp_int64_t>(Value)));
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return __sync_lock_test_and_set(Target, Value);
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Fills a block of memory with zero.
    //
    inline void 
    ApiZeroMemory(
        _Out_writes_bytes_all_(Length) void* Destination,
        _In_ size_t Length
    ) noexcept
    {
        #if defined(XPLATFORM_WINDOWS_USER_MODE) || defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            RtlSecureZeroMemory(Destination, Length);
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            memset(Destination, 0, Length);
        #else
            #error Unsuported Platform
        #endif
    }

    //
    // Copies the contents of a source memory block to a destination memory block.
    //
    inline void
    ApiCopyMemory(
        _Out_writes_bytes_all_(Length) void* Destination,
        _In_reads_bytes_(Length) const void* const Source,
        _In_ size_t Length
    ) noexcept
    {
        #if defined(XPLATFORM_WINDOWS_USER_MODE) || defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            RtlCopyMemory(Destination, Source, Length);
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            memcpy(Destination, Source, Length);
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Checks if memory blocks are equal
    //
    inline bool
    ApiEqualMemory(
        _In_reads_bytes_(Length) const void* const Block1,
        _In_reads_bytes_(Length) const void* const Block2,
        _In_ size_t Length
    ) noexcept
    {
        #if defined(XPLATFORM_WINDOWS_USER_MODE) || defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            return RtlEqualMemory(Block1, Block2, Length);
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            return 0 == memcmp(Block1, Block2, Length);
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Allocates memory on a XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT alignment boundary.
    //
    _Ret_maybenull_
    _Must_inspect_result_
    inline void* 
    ApiAllocMemory(
        _In_ size_t Size
    ) noexcept
    {
        // Avoid 0 size allocations. Mimic CRT behavior on these
        if (0 == Size)
        {
            Size = 1;
        }

        #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            return ExAllocatePoolWithTag(POOL_TYPE::PagedPool, Size, 'ppc#');
        #elif defined(XPLATFORM_WINDOWS_USER_MODE)
            return HeapAlloc(GetProcessHeap(), 0, Size);
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            return aligned_alloc(XPLATFORM_MEMORY_ALLOCATION_ALIGNMENT, Size);
        #else
            #error Unsuported Platform
        #endif
    }

    //
    // Frees a block of memory that was allocated with ApiAllocMemory.
    //
    inline void 
    ApiFreeMemory(
        _Pre_maybenull_ _Post_invalid_ void* Memory
    ) noexcept
    {
        if (nullptr != Memory)
        {
            #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                ExFreePoolWithTag(Memory, 'ppc#');
            #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                (void) HeapFree(GetProcessHeap(), 0, Memory);
            #elif defined (XPLATFORM_LINUX_USER_MODE)
                free(Memory);
            #else
                #error Unsuported Platform
            #endif
        }
    }

    //
    // Adds two uint values with respect for arithmetic overflow.
    // Returns true if the addition can be performed without arithmetic overflows, false otherwise.
    //
    template <class T>
    _Success_(return != false)
    _Must_inspect_result_
    inline bool
    ApiUIntAdd(
        _In_ _Const_ const T& Augend,
        _In_ _Const_ const T& Addend,
        _Out_ T* Result
    )
    {
        static_assert( XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_uint16_t> ||
                       XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_uint64_t>,
                       "Unsuported Type Operation"
        );

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt8Add(Augend, Addend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt8Add(Augend, Addend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt16Add(Augend, Addend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt16Add(Augend, Addend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt32Add(Augend, Addend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt32Add(Augend, Addend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt64Add(Augend, Addend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt64Add(Augend, Addend, Result));
                #endif
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return (true != __builtin_add_overflow(Augend, Addend, Result));
        #else
            #error Unsuported Compiler
        #endif
    }

    
    //
    // Substracts one value of type uint from another with respect for arithmetic underflow.
    // Returns true if the subtraction can be performed without arithmetic underflows, false otherwise.
    //
    template <class T>
    _Success_(return != false)
    _Must_inspect_result_
    inline T
    ApiUIntSub(
        _In_ _Const_ const T& Minuend,
        _In_ _Const_ const T& Subtrahend,
        _Out_ T* Result
    ) noexcept
    {
        static_assert( XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_uint16_t> ||
                       XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_uint64_t>,
                       "Unsuported Type Operation"
        );

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt8Sub(Minuend, Subtrahend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt8Sub(Minuend, Subtrahend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt16Sub(Minuend, Subtrahend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt16Sub(Minuend, Subtrahend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt32Sub(Minuend, Subtrahend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt32Sub(Minuend, Subtrahend, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt64Sub(Minuend, Subtrahend, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt64Sub(Minuend, Subtrahend, Result));
                #endif
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return (true != __builtin_sub_overflow(Minuend, Subtrahend, Result));
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Multiplies one value of type uint from another with respect for arithmetic overflow.
    // Returns true if the multiplication can be performed without arithmetic overflow, false otherwise.
    //
    template <class T>
    _Success_(return != false)
    _Must_inspect_result_
    inline bool
    ApiUIntMult(
        _In_ _Const_ const T& Multiplicand,
        _In_ _Const_ const T& Multiplier,
        _Out_ T* Result
    ) noexcept
    {
        static_assert( XPF::IsSameType<T, xp_uint8_t>  || XPF::IsSameType<T, xp_uint16_t> ||
                       XPF::IsSameType<T, xp_uint32_t> || XPF::IsSameType<T, xp_uint64_t>,
                       "Unsuported Type Operation"
        );

        #if defined(_MSC_VER)
            if constexpr (XPF::IsSameType<T, xp_uint8_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt8Mult(Multiplicand, Multiplier, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt8Mult(Multiplicand, Multiplier, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint16_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt16Mult(Multiplicand, Multiplier, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt16Mult(Multiplicand, Multiplier, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint32_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt32Mult(Multiplicand, Multiplier, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt32Mult(Multiplicand, Multiplier, Result));
                #endif
            }
            else if constexpr (XPF::IsSameType<T, xp_uint64_t>)
            {
                #if defined(XPLATFORM_WINDOWS_KERNEL_MODE)
                    return NT_SUCCESS(RtlUInt64Mult(Multiplicand, Multiplier, Result));
                #elif defined(XPLATFORM_WINDOWS_USER_MODE)
                    return SUCCEEDED(UInt64Mult(Multiplicand, Multiplier, Result));
                #endif
            }
            else
            {
                // This is a workaround -- on this else branch, it is guaranteed that T is not xp_uint64_t.
                static_assert(XPF::IsSameType<T, xp_uint64_t>, "Unsuported type operation!");
            }
        #elif defined (__GNUC__) || defined (__clang__)
            return (true != __builtin_mul_overflow(Multiplicand, Multiplier, Result));
        #else
            #error Unsuported Compiler
        #endif
    }

    //
    // Computes the length of a string and determines whether it exceeds the specified length, in characters.
    // For now, this API fails if the string is larger or equal to MAX_INT32 characters.
    // Cand be adjusted when dealing with larger strings.
    //
    template <class CharType>
    _Success_(return != false)
    _Must_inspect_result_
    inline bool 
    ApiStringLength(
        _In_ _Const_ const CharType* const String,
        _Out_ size_t* Length
    ) noexcept
    {
        static_assert(XPF::IsSameType<CharType, xp_char8_t>  ||
                      XPF::IsSameType<CharType, xp_char16_t> ||
                      XPF::IsSameType<CharType, xp_char32_t>,
                      "Unsuported type operation!");

        constexpr size_t   MAX_CHARACTERS = XPF::NumericLimits<xp_int32_t>::MaxValue;
        constexpr CharType NULL_CHARACTER = 0;

        if (nullptr == String || nullptr == Length)
        {
            return false;
        }

        size_t crtLength = 0;
        while ((crtLength < MAX_CHARACTERS) && (String[crtLength] != NULL_CHARACTER))
        {
            crtLength++;
        }

        if (crtLength == MAX_CHARACTERS)
        {
            return false;
        }

        *Length = crtLength;
        return true;
    }

    //
    // Converts an uppercase alphabet to a lowercase alphabet.
    // If the conversion is not possible, returns the original character.
    //
    template <class CharType>
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    inline CharType
    ApiCharToLower(
        _In_ CharType Character
    ) noexcept
    {
        static_assert(XPF::IsSameType<CharType, xp_char8_t> ||
                      XPF::IsSameType<CharType, xp_char16_t> ||
                      XPF::IsSameType<CharType, xp_char32_t>,
                      "Unsuported type operation!");

        #if defined(XPLATFORM_WINDOWS_USER_MODE)
            //
            // On windows the conversion is possible only if the value can be stored on 2 bytes.
            // Otherwise we simply return the original character
            //
            if(static_cast<xp_uint32_t>(Character) <= XPF::NumericLimits<xp_uint16_t>::MaxValue)
            {
                wchar_t character = static_cast<wchar_t>(Character);
                CharLowerBuffW(&character, 1);
                Character = static_cast<CharType>(character);
            }
            return Character;
        #elif defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            if(static_cast<xp_uint32_t>(Character) <= XPF::NumericLimits<xp_uint16_t>::MaxValue)
            {
                wchar_t character = static_cast<wchar_t>(Character);
                Character = static_cast<CharType>(RtlDowncaseUnicodeChar(character));
            }
            return Character;
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            return static_cast<CharType>(towlower(static_cast<wchar_t>(Character)));
        #else
            #error Unsuported Compiler
        #endif
    }

        //
    // Converts an uppercase alphabet to a uppercase alphabet.
    // If the conversion is not possible, returns the original character.
    //
    template <class CharType>
    _IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    inline CharType
    ApiCharToUpper(
        _In_ CharType Character
    ) noexcept
            {
        static_assert(XPF::IsSameType<CharType, xp_char8_t> ||
                      XPF::IsSameType<CharType, xp_char16_t> ||
                      XPF::IsSameType<CharType, xp_char32_t>,
                      "Unsuported type operation!");

        #if defined(XPLATFORM_WINDOWS_USER_MODE)
            //
            // On windows the conversion is possible only if the value can be stored on 2 bytes.
            // Otherwise we simply return the original character
            //
            if(static_cast<xp_uint32_t>(Character) <= XPF::NumericLimits<xp_uint16_t>::MaxValue)
            {
                wchar_t character = static_cast<wchar_t>(Character);
                CharUpperBuffW(&character, 1);
                Character = static_cast<CharType>(character);
            }
            return Character;
        #elif defined(XPLATFORM_WINDOWS_KERNEL_MODE)
            if(static_cast<xp_uint32_t>(Character) <= XPF::NumericLimits<xp_uint16_t>::MaxValue)
            {
                wchar_t character = static_cast<wchar_t>(Character);
                Character = static_cast<CharType>(RtlUpcaseUnicodeChar(character));
            }
            return Character;
        #elif defined (XPLATFORM_LINUX_USER_MODE)
            return static_cast<CharType>(towupper(static_cast<wchar_t>(Character)));
        #else
            #error Unsuported Compiler
        #endif
    }
}


#endif // __XPLATFORM_SPECIFIC_API_HPP__