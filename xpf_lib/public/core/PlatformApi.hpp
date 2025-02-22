/**
  * @file        xpf_lib/public/core/PlatformApi.hpp
  *
  * @brief       This contains platform-specific APIs that should be used.
  *
  * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
  *
  * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
  *              All rights reserved.
  *
  * @license     See top-level directory LICENSE file.
  */


#pragma once

#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"
#include "xpf_lib/public/core/Algorithm.hpp"


namespace xpf
{
/**
 * @brief Signals that an unexpected exception occured.
 * 
 * @param[in] Status - To signal what happened.
 * 
 * @return None.
 * 
 * @note - This API is used only when the invariants are violated
 *         for example when a shared pointer's refcounter reaches -1.
 * 
 *         As a rule of thumb, this shouldn't be used elsewhere,
 *         only to prevent inconistent states / errors from where we can't recover
 *         and is not safe to continue.
 */
void
XPF_API
ApiPanic(
    _In_ NTSTATUS Status
) noexcept(true);
};  // namespace xpf


/**
 * @brief Helper macro for verifying an invariant.
 *        If the condition is not met, an assertion is raised regardless on configuration!
 *        This is useful to validate logic bugs - both on debug and release.
 */
#define XPF_DEATH_ON_FAILURE(Expression)     ((Expression) ? true                                                       \
                                                           : (xpf::ApiPanic(STATUS_UNHANDLED_EXCEPTION), false))

/**
 * @brief Helpers macro for ASSERT and VERIFY.
 *        Similar with NT_ASSERT and NT_VERIFY from Windows.
 */
#if defined XPF_CONFIGURATION_RELEASE

    /**
     * @brief Macro definition for ASSERT. This is not evaluated on release.
     */
    #define XPF_ASSERT(Expression)      (((void) 0), true)
    /**
     * @brief Macro definition for VERIFY. This is evaluated on release.
     */
    #define XPF_VERIFY(Expression)      ((Expression) ? true : false)

#elif defined XPF_CONFIGURATION_DEBUG

    /**
     * @brief Macro definition for ASSERT. This is evaluated on debug.
     */
    #define XPF_ASSERT(Expression)      ((Expression) ? true                                                            \
                                                      : (xpf::ApiPanic(STATUS_UNHANDLED_EXCEPTION), false))
    /**
     * @brief Macro definition for VERIFY. This is evaluated on debug.
     */
    #define XPF_VERIFY                  XPF_ASSERT
#else

    #error Unknown Configuration
#endif

namespace xpf
{
/**
 * @brief Copies the contents of a source memory block to a destination memory block.
 *        It supports overlapping source and destination memory blocks.
 *
 * @param[out] Destination - A pointer to the destination memory block to copy the bytes to.
 *
 * @param[in] Source - A pointer to the source memory block to copy the bytes from.
 *
 * @param[in] Size - The number of bytes to copy from the source to the destination.
 *
 * @return None.
 */
void
XPF_API
ApiCopyMemory(
    _Out_writes_bytes_all_(Size) void* Destination,
    _In_reads_bytes_(Size) void const* Source,
    _In_ size_t Size
) noexcept(true);

/**
 * @brief Fills a block of memory with zeros
 *
 * @param[out] Destination - Pointer to the memory buffer to be filled with zeros.
 *
 * @param[in] Size - Specifies the number of bytes to be filled with zeros.
 *
 * @return None.
 */
void
XPF_API
ApiZeroMemory(
    _Out_writes_bytes_all_(Size) void* Destination,
    _In_ size_t Size
) noexcept(true);

/**
 * @brief Compares two blocks of memory to determine whether the specified number of bytes are identical.
 *
 * @param[in] Source1 - A pointer to a caller-allocated block of memory to compare.
 *
 * @param[in] Source2 - A pointer to a caller-allocated block of memory that is compared to the block of
 *                      memory to which Source1 points.
 *
 * @param[in] Size    - Specifies the number of bytes to be compared.
 *
 * @return true if Source1 and Source2 are equivalent; otherwise, it returns false.
 */
bool
XPF_API
ApiEqualMemory(
    _In_reads_bytes_(Size) void const* Source1,
    _In_reads_bytes_(Size) void const* Source2,
    _In_ size_t Size
) noexcept(true);

/**
 * @brief Frees a block of memory.
 * 
 * @param[in,out] MemoryBlock - To be freed.
 * 
 * @return None
 */
void
XPF_API
ApiFreeMemory(
    _Inout_opt_ void* MemoryBlock
) noexcept(true);

/**
 * @brief Allocates a block of memory with the required size.
 * 
 * @param[in] BlockSize - The requsted block size.
 * 
 * @param[in] CriticalAllocation - The requested allocation is critical
 *                                 So on fail, we will attempt several times to realloc.
 *                                 By default, this parameter is false.
 * 
 * @return A block of memory with the required size, or null on failure.
 * 
 * @note On windows KM if a critical allocation is required, it will be attempted from NonPagedPool.
 *       This pool shouldn't fail - only with driver verifier.
 */
_Check_return_
_Ret_maybenull_
void*
XPF_API
ApiAllocateMemory(
    _In_ size_t BlockSize,
    _In_ bool CriticalAllocation = false
) noexcept(true);

/**
 * @brief Suspends the execution of the current thread until the time-out interval elapses.
 *
 * @param[in] NumberOfMilliSeconds - The time interval for which execution is to be suspended, in milliseconds.
 *                                   A value of zero causes the thread to relinquish the remainder of
 *                                   its time slice to any other thread that is ready to run.
 * @return None.
 */
void
XPF_API
ApiSleep(
    _In_ uint32_t NumberOfMilliSeconds
) noexcept(true);

/**
 * @brief Signals to the processor to give resources to threads that are waiting for them.
 * 
 * @return None.
 */
void
XPF_API
ApiYieldProcesor(
    void
) noexcept(true);

/**
 * @brief Limits the compiler optimizations that can reorder memory accesses across the point of the call..
 *
 * @return None.
 */
inline void
XPF_API
ApiCompilerBarrier(
    void
) noexcept(true)
{
    #if defined XPF_COMPILER_MSVC
        _ReadWriteBarrier();
        MemoryBarrier();
    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG
        asm volatile("" ::: "memory");
    #else
        #error Unknown Compiler
    #endif
}

/**
 * @brief Retrieves a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
 * 
 * @return System time as a count of 100-nanosecond intervals since January 1, 1601.
 *         This value is computed for the GMT time zone
 */
uint64_t
XPF_API
ApiCurrentTime(
    void
) noexcept(true);

/**
 * @brief Increments (increases by one) the value of the specified variable as an atomic operation.
 * 
 * @param[in,out] Number - A pointer to the variable to be incremented.
 * 
 * @return The function returns the resulting incremented value.
 * 
 * @note The Number must be properly aligned to pointer size, otherwise it will lead to undefined behavior.
 */
template <class Type>
inline Type
ApiAtomicIncrement(
    _Inout_ volatile Type* Number
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    //
    // The address needs to be properly aligned otherwise it leads to
    // undefined behavior. So assert here to catch this invalid usage
    // on debug builds.
    //
    XPF_DEATH_ON_FAILURE(xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(Number),
                                                  alignof(Type)));

    //
    // We don't want the compiler to play around with these instructions.
    //
    xpf::ApiCompilerBarrier();

    #if defined XPF_COMPILER_MSVC
        //
        // We'll cast the number to the proper type below.
        // First we'll cast to generic volatile void*. We want to avoid reinterpret cast.
        //
        volatile void* number = static_cast<volatile void*>(Number);

        if constexpr (xpf::IsSameType<Type, uint8_t> || xpf::IsSameType<Type, int8_t>)
        {
            static_assert(sizeof(uint8_t) == sizeof(char) && sizeof(int8_t) == sizeof(char), "Invalid size!");                      // NOLINT(*)
            return static_cast<Type>(InterlockedExchangeAdd8(static_cast<volatile char*>(number), char{1})) + Type{ 1 };            // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>)
        {
            static_assert(sizeof(uint16_t) == sizeof(short) && sizeof(int16_t) == sizeof(short), "Invalid size!");                  // NOLINT(*)
            return static_cast<Type>(InterlockedIncrement16(static_cast<volatile short*>(number)));                                 // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>)
        {
            static_assert(sizeof(uint32_t) == sizeof(LONG) && sizeof(int32_t) == sizeof(LONG), "Invalid size!");                    // NOLINT(*)
            return static_cast<Type>(InterlockedIncrement(static_cast<volatile LONG*>(number)));                                    // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>)
        {
            static_assert(sizeof(uint64_t) == sizeof(LONG64) && sizeof(int64_t) == sizeof(LONG64), "Invalid size!");                // NOLINT(*)
            return static_cast<Type>(InterlockedIncrement64(static_cast<volatile LONG64*>(number)));                                // NOLINT(*)
        }

    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG
            return __sync_add_and_fetch(Number, Type{ 1 });

    #else
        #error Unknown Compiler
    #endif
}

/**
 * @brief Decrements (decreases by one) the value of the specified variable as an atomic operation.
 * 
 * @param[in,out] Number - A pointer to the variable to be decremented.
 * 
 * @return The function returns the resulting decremented value.
 * 
 * @note The Number must be properly aligned to pointer size, otherwise it will lead to undefined behavior.
 */
template <class Type>
inline Type
ApiAtomicDecrement(
    _Inout_ volatile Type* Number
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    //
    // The address needs to be properly aligned otherwise it leads to
    // undefined behavior. So assert here to catch this invalid usage
    // on debug builds.
    //
    XPF_DEATH_ON_FAILURE(xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(Number),
                                                  alignof(Type)));

    //
    // We don't want the compiler to play around with these instructions.
    //
    xpf::ApiCompilerBarrier();

    #if defined XPF_COMPILER_MSVC
        //
        // We'll cast the number to the proper type below.
        // First we'll cast to generic volatile void*. We want to avoid reinterpret cast.
        //
        volatile void* number = static_cast<volatile void*>(Number);

        if constexpr (xpf::IsSameType<Type, uint8_t> || xpf::IsSameType<Type, int8_t>)
        {
            static_assert(sizeof(uint8_t) == sizeof(char) && sizeof(int8_t) == sizeof(char), "Invalid size!");                      // NOLINT(*)
            return static_cast<Type>(InterlockedExchangeAdd8(static_cast<volatile char*>(number), char{-1})) - Type{ 1 };           // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>)
        {
            static_assert(sizeof(uint16_t) == sizeof(short) && sizeof(int16_t) == sizeof(short), "Invalid size!");                  // NOLINT(*)
            return static_cast<Type>(InterlockedDecrement16(static_cast<volatile short*>(number)));                                 // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>)
        {
            static_assert(sizeof(uint32_t) == sizeof(LONG) && sizeof(int32_t) == sizeof(LONG), "Invalid size!");                    // NOLINT(*)
            return static_cast<Type>(InterlockedDecrement(static_cast<volatile LONG*>(number)));                                    // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>)
        {
            static_assert(sizeof(uint64_t) == sizeof(LONG64) && sizeof(int64_t) == sizeof(LONG64), "Invalid size!");                // NOLINT(*)
            return static_cast<Type>(InterlockedDecrement64(static_cast<volatile LONG64*>(number)));                                // NOLINT(*)
        }

    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG

        return __sync_sub_and_fetch(Number, Type{ 1 });

    #else
        #error Unknown Compiler
    #endif
}

/**
 * @brief Performs an atomic compare-and-exchange operation on the specified values.
 *        The function compares two specified values and exchanges with another value based on the outcome of the comparison.
 * 
 * @param[in,out] Destination - A pointer to the destination value.
 * 
 * @param[in] Exchange - The exchange value.
 * 
 * @param[in] Comperand The value to compare to Destination.
 * 
 * @return The function returns the initial value of the Destination parameter.
 * 
 * @note The Number must be properly aligned to pointer size, otherwise it will lead to undefined behavior.
 */
template <class Type>
inline Type
ApiAtomicCompareExchange(
    _Inout_ volatile Type* Destination,
    _In_ _Const_ const Type& Exchange,
    _In_ _Const_ const Type& Comperand
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    //
    // The address needs to be properly aligned otherwise it leads to
    // undefined behavior. So assert here to catch this invalid usage
    // on debug builds.
    //
    XPF_DEATH_ON_FAILURE(xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(Destination),
                                                  alignof(Type)));

    //
    // We don't want the compiler to play around with these instructions.
    //
    xpf::ApiCompilerBarrier();

    #if defined XPF_COMPILER_MSVC
        //
        // We'll cast the destination to the proper type below.
        // First we'll cast to generic volatile void*. We want to avoid reinterpret cast.
        //
        volatile void* destination = static_cast<volatile void*>(Destination);

        if constexpr (xpf::IsSameType<Type, uint8_t> || xpf::IsSameType<Type, int8_t>)
        {
            static_assert(sizeof(uint8_t) == sizeof(char) && sizeof(int8_t) == sizeof(char), "Invalid size!");                      // NOLINT(*)
            return static_cast<Type>(_InterlockedCompareExchange8(static_cast<volatile char*>(destination),                         // NOLINT(*)
                                                                  static_cast<char>(Exchange),                                      // NOLINT(*)
                                                                  static_cast<char>(Comperand)));                                   // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>)
        {
            static_assert(sizeof(uint16_t) == sizeof(short) && sizeof(int16_t) == sizeof(short), "Invalid size!");                  // NOLINT(*)
            return static_cast<Type>(InterlockedCompareExchange16(static_cast<volatile short*>(destination),                        // NOLINT(*)
                                                                  static_cast<short>(Exchange),                                     // NOLINT(*)
                                                                  static_cast<short>(Comperand)));                                  // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>)
        {
            static_assert(sizeof(uint32_t) == sizeof(LONG) && sizeof(int32_t) == sizeof(LONG), "Invalid size!");                    // NOLINT(*)
            return static_cast<Type>(InterlockedCompareExchange(static_cast<volatile LONG*>(destination),                           // NOLINT(*)
                                                                static_cast<LONG>(Exchange),                                        // NOLINT(*)
                                                                static_cast<LONG>(Comperand)));                                     // NOLINT(*)
        }
        else if constexpr (xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>)
        {
            static_assert(sizeof(uint64_t) == sizeof(LONG64) && sizeof(int64_t) == sizeof(LONG64), "Invalid size!");                // NOLINT(*)
            return static_cast<Type>(InterlockedCompareExchange64(static_cast<volatile LONG64*>(destination),                       // NOLINT(*)
                                                                  static_cast<LONG64>(Exchange),                                    // NOLINT(*)
                                                                  static_cast<LONG64>(Comperand)));                                 // NOLINT(*)
        }

    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG

        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
    #else
        #error Unknown Compiler
    #endif
}

/**
 * @brief Performs an atomic compare-and-exchange operation on the specified values.
 *        The function compares two specified values and exchanges with another value based on the outcome of the comparison.
 * 
 * @param[in,out] Destination - A pointer to the destination value.
 * 
 * @param[in] Exchange - The exchange value.
 * 
 * @param[in] Comperand The value to compare to Destination.
 * 
 * @return The function returns the initial value of the Destination parameter.
 * 
 * @note The Destination must be properly aligned to pointer size, otherwise it will lead to undefined behavior.
 */
inline void*
ApiAtomicCompareExchangePointer(
    _Inout_ void* volatile* Destination,
    _In_opt_ void* Exchange,
    _In_opt_ void* Comperand
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(Destination),
                                                  alignof(void*)));

    //
    // We don't want the compiler to play around with these instructions.
    //
    xpf::ApiCompilerBarrier();

    #if defined XPF_COMPILER_MSVC

        return InterlockedCompareExchangePointer(Destination, Exchange, Comperand);

    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG

        return __sync_val_compare_and_swap(Destination, Comperand, Exchange);

    #else
        #error Unknown Compiler
    #endif
}


/**
 * @brief This is one of a set of inline functions designed to provide arithmetic operations
 *        and perform validity checks with minimal impact on performance. Adds one value of type Type to another.
 *
 * @param[in] Augend - The first value in the equation.
 *
 * @param[in] Addend - The value to add to Augend.
 *
 * @param[out] Result - A pointer to the sum. If the operation results in a value that overflows or underflows
 *             the capacity of the type, the function returns false and this parameter is not valid.
 *
 * @return true if the operation is successful, false otherwise.
 */
template <class Type>
inline bool
ApiNumbersSafeAdd(
    _In_ _Const_ const Type& Augend,
    _In_ _Const_ const Type& Addend,
    _Inout_ Type* Result
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    #if defined XPF_COMPILER_MSVC
        //
        // MSVC does not have intrinsics for this. So we'll use platform-specific APIs.
        //
        #if defined XPF_PLATFORM_WIN_UM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return SUCCEEDED(UInt8Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return SUCCEEDED(UInt16Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return SUCCEEDED(UInt32Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return SUCCEEDED(UInt64Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return SUCCEEDED(Int8Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return SUCCEEDED(Int16Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return SUCCEEDED(Int32Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return SUCCEEDED(Int64Add(Augend, Addend, Result));
            }
        #elif defined XPF_PLATFORM_WIN_KM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return NT_SUCCESS(RtlUInt8Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return NT_SUCCESS(RtlUInt16Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return NT_SUCCESS(RtlUInt32Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return NT_SUCCESS(RtlUInt64Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return NT_SUCCESS(RtlInt8Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return NT_SUCCESS(RtlInt16Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return NT_SUCCESS(RtlInt32Add(Augend, Addend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return NT_SUCCESS(RtlInt64Add(Augend, Addend, Result));
            }
        #else
            #error Unsupported Platform
        #endif
    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG
            return (false == __builtin_add_overflow(Augend, Addend, Result));
    #else
        #error Unsupported Compiler
    #endif
}

/**
 * @brief This is one of a set of inline functions designed to provide arithmetic operations
 *        and perform validity checks with minimal impact on performance. Subtracts one value of type Type from another.
 *
 * @param[in] Minuend - The value from which Subtrahend is subtracted.
 *
 * @param[in] Subtrahend - The value to subtract from Minuend.
 *
 * @param[out] Result - A pointer to the result. If the operation results in a value that overflows or underflows the capacity of the type,
 *                      the function returns false and this parameter is not valid.
 *
 * @return true if the operation is successful, false otherwise.
 */
template <class Type>
inline bool
ApiNumbersSafeSub(
    _In_ _Const_ const Type& Minuend,
    _In_ _Const_ const Type& Subtrahend,
    _Inout_ Type* Result
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    #if defined XPF_COMPILER_MSVC
        //
        // MSVC does not have intrinsics for this. So we'll use platform-specific APIs.
        //
        #if defined XPF_PLATFORM_WIN_UM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return SUCCEEDED(UInt8Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return SUCCEEDED(UInt16Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return SUCCEEDED(UInt32Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return SUCCEEDED(UInt64Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return SUCCEEDED(Int8Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return SUCCEEDED(Int16Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return SUCCEEDED(Int32Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return SUCCEEDED(Int64Sub(Minuend, Subtrahend, Result));
            }
        #elif defined XPF_PLATFORM_WIN_KM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return NT_SUCCESS(RtlUInt8Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return NT_SUCCESS(RtlUInt16Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return NT_SUCCESS(RtlUInt32Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return NT_SUCCESS(RtlUInt64Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return NT_SUCCESS(RtlInt8Sub(Minuend, Subtrahend, Result));;
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return NT_SUCCESS(RtlInt16Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return NT_SUCCESS(RtlInt32Sub(Minuend, Subtrahend, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return NT_SUCCESS(RtlInt64Sub(Minuend, Subtrahend, Result));
            }
        #else
            #error Unsupported Platform
        #endif
    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG
            return (false == __builtin_sub_overflow(Minuend, Subtrahend, Result));
    #else
        #error Unsupported Compiler
    #endif
}

/**
 * @brief This is one of a set of inline functions designed to provide arithmetic operations
 *        and perform validity checks with minimal impact on performance. Multiplies one value of type Type to another.
 *
 * @param[in] Multiplicand - The value to be multiplied by Multiplier.
 *
 * @param[in] Multiplier - The value by which to multiply Multiplicand.
 *
 * @param[out] Result - A pointer to the result. If the operation results in a value that overflows or underflows the capacity of the type,
 *                      the function returns false and this parameter is not valid.
 *
 * @return true if the operation is successful, false otherwise.
 */
template <class Type>
inline bool
ApiNumbersSafeMul(
    _In_ _Const_ const Type& Multiplicand,
    _In_ _Const_ const Type& Multiplier,
    _Inout_ Type* Result
) noexcept(true)
{
    //
    // Restrict this operation to integers only.
    //
    static_assert(xpf::IsSameType<Type, uint8_t>  || xpf::IsSameType<Type, int8_t>    ||
                  xpf::IsSameType<Type, uint16_t> || xpf::IsSameType<Type, int16_t>   ||
                  xpf::IsSameType<Type, uint32_t> || xpf::IsSameType<Type, int32_t>   ||
                  xpf::IsSameType<Type, uint64_t> || xpf::IsSameType<Type, int64_t>,
                  "Unsupported Type!");

    #if defined XPF_COMPILER_MSVC
        //
        // MSVC does not have intrinsics for this. So we'll use platform-specific APIs.
        //
        #if defined XPF_PLATFORM_WIN_UM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return SUCCEEDED(UInt8Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return SUCCEEDED(UInt16Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return SUCCEEDED(UInt32Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return SUCCEEDED(UInt64Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return SUCCEEDED(Int8Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return SUCCEEDED(Int16Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return SUCCEEDED(Int32Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return SUCCEEDED(Int64Mult(Multiplicand, Multiplier, Result));
            }
        #elif defined XPF_PLATFORM_WIN_KM
            if constexpr (xpf::IsSameType<Type, uint8_t>)
            {
                return NT_SUCCESS(RtlUInt8Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint16_t>)
            {
                return NT_SUCCESS(RtlUInt16Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint32_t>)
            {
                return NT_SUCCESS(RtlUInt32Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, uint64_t>)
            {
                return NT_SUCCESS(RtlUInt64Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int8_t>)
            {
                return NT_SUCCESS(RtlInt8Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int16_t>)
            {
                return NT_SUCCESS(RtlInt16Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int32_t>)
            {
                return NT_SUCCESS(RtlInt32Mult(Multiplicand, Multiplier, Result));
            }
            else if constexpr (xpf::IsSameType<Type, int64_t>)
            {
                return NT_SUCCESS(RtlInt64Mult(Multiplicand, Multiplier, Result));
            }
        #else
            #error Unsupported Platform
        #endif
    #elif defined XPF_COMPILER_GCC || defined XPF_COMPILER_CLANG
            return (false == __builtin_mul_overflow(Multiplicand, Multiplier, Result));
    #else
        #error Unsupported Compiler
    #endif
}

/**
 * @brief Determine the length, in characters, of a supplied string
 * 
 * @param[in] String - A pointer to a buffer that contains a null-terminated string,
 *                     the length of which will be checked.
 * 
 * @return The maximum number of characters allowed in the buffer pointed to by psz,
 *         not including the terminating null character.
 */
template <class CharType>
constexpr inline size_t
ApiStringLength(
    _In_opt_ _Const_ const CharType* String
) noexcept(true)
{
    static_assert(xpf::IsSameType<CharType, char>     ||
                  xpf::IsSameType<CharType, wchar_t>,
                  "Unsupported Character Type!");

    size_t length = 0;
    if (nullptr != String)
    {
        while (String[length] != static_cast<CharType>(0))
        {
            length++;
        }
    }
    return length;
}

/**
 * @brief Converts a given character to lowercase.
 * 
 * @param[in] Character - Character to be lowercased.
 * 
 * @return The lowercased character or the original value if conversion is not possible.
 */
wchar_t
XPF_API
ApiCharToLower(
    _In_ wchar_t Character
) noexcept(true);

/**
 * @brief Converts a given character to uppercase.
 * 
 * @param[in] Character - Character to be uppercased.
 * 
 * @return The upcased character or the original value if conversion is not possible.
 */
wchar_t
XPF_API
ApiCharToUpper(
    _In_ wchar_t Character
) noexcept(true);

/**
 * @brief Compare two characters to check for equality.
 * 
 * @param[in] Left - First Character.
 * 
 * @param[in] Right - Second Character.
 * 
 * @param[in] CaseSensitive - Controls whether the comparison is case sensitive or not.
 * 
 * @return true if the left character equal right character(with respect to case sensitivity),
 *         false otherwise
 */
bool
XPF_API
ApiEqualCharacters(
    _In_ wchar_t Left,
    _In_ wchar_t Right,
    _In_ bool CaseSensitive
) noexcept(true);

/**
 * @brief Generates a random 128-bit unique identifier.
 * 
 * @param NewUuid - The Newly Generated uuid.
 *
 * @return void.
 */
void
XPF_API
ApiRandomUuid(
    _Out_ uuid_t* NewUuid
) noexcept(true);

/**
 * @brief Checks if two uuid_t variables are equal or not.
 * 
 * @param[in] First - first UUID to be compared.
 * 
 * @param[in] Second - second UUID to be compared.
 *
 * @return true if First == Second,
 *         false otherwise.
 */
bool
XPF_API
ApiAreUuidsEqual(
    _In_ const uuid_t First,
    _In_ const uuid_t Second
) noexcept(true);

/**
 * @brief Checks if a character is an hex digit.
 *
 * @param[in] Character - The character to be checked.
 *
 * @return true if Character is an hex digit,
 *         false otherwise.
 */
bool
XPF_API
ApiIsHexDigit(
    _In_ char Character
) noexcept(true);

/**
 * @brief   Captures a backtrace for the calling thread, in the
 *          array pointed to by Frames. A stack backtrace is the series of
 *          currently active function calls for the thread.
 *
 * @param[in,out]  Frames          - Will contain the caller addresses,
 * @param[in]      Count           - The number of elements that the Frames array can store.
 * @param[out]     CapturedFrames  - The number of captured frames.
 *
 * @return A proper ntstatus error code.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
ApiCaptureStackBacktrace(
    _Inout_ void** Frames,
    _In_ uint32_t Count,
    _Out_ uint32_t* CapturedFrames
) noexcept(true);
};  // namespace xpf

