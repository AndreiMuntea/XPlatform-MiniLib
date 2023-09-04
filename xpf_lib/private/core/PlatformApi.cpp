/**
 * @file        xpf_lib/private/core/PlatformApi.cpp
 *
 * @brief       This contains platform-specific APIs implementation that should be used.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#include "xpf_lib/xpf.hpp"

/**
 * @brief   By default all code in here goes into default section.
 *          This means it won't be paged out.
 */
XPF_SECTION_DEFAULT;

void
XPF_API
xpf::ApiPanic(
    _In_ NTSTATUS Status
) noexcept(true)
{
    XPF_VERIFY(!NT_SUCCESS(Status));

    #if defined XPF_PLATFORM_WIN_KM
        ::ExRaiseStatus(Status);
    #elif defined XPF_PLATFORM_WIN_UM
        ::RaiseException(ERROR_UNHANDLED_EXCEPTION, 0, 0, NULL);
    #elif defined XPF_PLATFORM_LINUX_UM
        ::raise(SIGSEGV);
    #else
        #error Unknown Platform!
    #endif
}

void
XPF_API
xpf::ApiCopyMemory(
    _Out_writes_bytes_all_(Size) void* Destination,
    _In_reads_bytes_(Size) void const* Source,
    _In_ size_t Size
) noexcept(true)
{
    if ((nullptr == Destination) || (nullptr == Source) || (0 == Size))
    {
        return;
    }

    #if defined XPF_PLATFORM_WIN_KM ||  defined XPF_PLATFORM_WIN_UM
        RtlMoveMemory(Destination, Source, Size);
    #elif defined XPF_PLATFORM_LINUX_UM
        (void) memmove(Destination, Source, Size);
    #else
        #error Unknown Platform!
    #endif
}

void
XPF_API
xpf::ApiZeroMemory(
    _Out_writes_bytes_all_(Size) void* Destination,
    _In_ size_t Size
) noexcept(true)
{
    if ((nullptr == Destination) || (0 == Size))
    {
        return;
    }

    #if defined XPF_PLATFORM_WIN_KM || defined XPF_PLATFORM_WIN_UM
        (void) ::RtlSecureZeroMemory(Destination, Size);
    #elif defined XPF_PLATFORM_LINUX_UM
        (void) ::explicit_bzero(Destination, Size);
    #else
        #error Unknown Platform!
    #endif
}

void
XPF_API
xpf::ApiFreeMemory(
    _Inout_ void** MemoryBlock
) noexcept(true)
{
    //
    // Sanity checks.
    //
    if ((nullptr == MemoryBlock) || (nullptr == (*MemoryBlock)))
    {
        return;
    }

    //
    // Invalidate the block of memory.
    //
    void* block = *MemoryBlock;
    *MemoryBlock = nullptr;

    //
    // And now dispose it.
    //
    #if defined XPF_PLATFORM_WIN_KM
        ::ExFreePoolWithTag(block,
                            'nmS+');
    #elif defined XPF_PLATFORM_WIN_UM
        BOOL result = ::HeapFree(::GetProcessHeap(),
                                 0,
                                 block);
        XPF_VERIFY(FALSE != result);
    #elif defined XPF_PLATFORM_LINUX_UM
        ::free(block);
    #else
        #error Unknown Platform!
    #endif
}

_Check_return_
_Ret_maybenull_
void*
XPF_API
xpf::ApiAllocateMemory(
    _In_ size_t BlockSize,
    _In_ bool CriticalAllocation
) noexcept(true)
{
    void* block = nullptr;
    size_t retries = 0;

    //
    // Avoid 0-size allocations.
    // Mimic CRT behavior on these.
    //
    if (BlockSize == 0)
    {
        BlockSize = 1;
    }

    while ((nullptr == block) && (retries < 5))
    {
        //
        // Use the platform api to alloc memory.
        //
        #if defined XPF_PLATFORM_WIN_KM

            //
            // We don't support IRQL higher than DISPATCH_LEVEL.
            // Bail early if this is the case.
            //
            if (DISPATCH_LEVEL < ::KeGetCurrentIrql())
            {
                break;
            }

            //
            // Default to paged pool allocation.
            //
            POOL_TYPE pool = PagedPool;

            //
            // Critical allocation means we need to do everything we need for the
            // allocation to succeed. So we allocate from NonPagedPool.
            //
            // Also if the current IRQL is DISPATCH_LEVEL, we can't allocate page pool.
            // So we fallback to NonPagedPool.
            //
            if (CriticalAllocation || (DISPATCH_LEVEL == ::KeGetCurrentIrql()))
            {
                pool = NonPagedPool;
            }

            //
            // The function is poorly annotated. It actually supports
            // being called at DISPATCH_LEVEL - but the SAL complies about this.
            // Trick him to think we are below dispatch level.
            //
            _Analysis_assume_(DISPATCH_LEVEL > ::KeGetCurrentIrql());
            block = ::ExAllocatePoolWithTag(pool,
                                            BlockSize,
                                            'nmS+');
        #elif defined XPF_PLATFORM_WIN_UM
            block = ::HeapAlloc(::GetProcessHeap(),
                                0,
                                BlockSize);

        #elif defined XPF_PLATFORM_LINUX_UM
            block = ::aligned_alloc(XPF_DEFAULT_ALIGNMENT,
                                    BlockSize);
        #else
            #error Unknown Platform!
        #endif

        //
        // Ensure the memory block is properly aligned.
        // If it is not, we consider a fail.
        //
        if (!xpf::AlgoIsNumberAligned(xpf::AlgoPointerToValue(block), XPF_DEFAULT_ALIGNMENT))
        {
            xpf::ApiFreeMemory(&block);
            block = nullptr;
        }

        //
        // Ensure no garbage is left in the new zone.
        //
        if (nullptr != block)
        {
            xpf::ApiZeroMemory(block, BlockSize);
            break;
        }

        //
        // Non-Critical allocations will NOT be retried.
        //
        if (!CriticalAllocation)
        {
            break;
        }

        //
        // Increase the numbers of retries - essential for CriticalAllocation.
        //
        ++retries;
    }
    return block;
}

void
XPF_API
xpf::ApiSleep(
    _In_ uint32_t NumberOfMilliSeconds
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_KM
        //
        // Specifies the absolute or relative time, in units of 100 nanoseconds,
        // for which the wait is to occur. A negative value indicates relative time
        //
        LARGE_INTEGER interval;
        xpf::ApiZeroMemory(&interval, sizeof(interval));

        //
        // 1 millisecond = 1000000 nanoseconds
        // 1 millisecond = 10000 (100 ns)
        //
        interval.QuadPart = static_cast<LONG64>(-10000) * static_cast<LONG64>(NumberOfMilliSeconds);

        //
        // We don't care about the return status.
        //
        const NTSTATUS status = ::KeDelayExecutionThread(KernelMode, FALSE, &interval);
        UNREFERENCED_PARAMETER(status);

    #elif defined XPF_PLATFORM_WIN_UM
        ::Sleep(NumberOfMilliSeconds);

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // We round the value up, so we sleep at least one second.
        //
        unsigned int numberOfSeconds = (NumberOfMilliSeconds / 1000);
        if (NumberOfMilliSeconds > 0)
        {
            numberOfSeconds += 1;
        }
        (void) ::sleep(numberOfSeconds);

    #else
        #error Unknown Platform!
    #endif
}

void
XPF_API
xpf::ApiYieldProcesor(
    void
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_KM || defined XPF_PLATFORM_WIN_UM
        YieldProcessor();
    #elif defined XPF_PLATFORM_LINUX_UM
        (void) sched_yield();
    #else
        #error Unknown Platform!
    #endif
}

uint64_t
XPF_API
xpf::ApiCurrentTime(
    void
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_KM
        LARGE_INTEGER largeInteger;
        xpf::ApiZeroMemory(&largeInteger, sizeof(largeInteger));

        KeQuerySystemTime(&largeInteger);

        return static_cast<uint64_t>(largeInteger.QuadPart);
    #elif defined XPF_PLATFORM_WIN_UM
        FILETIME fileTime;
        xpf::ApiZeroMemory(&fileTime, sizeof(fileTime));

        ::GetSystemTimeAsFileTime(&fileTime);

        ULARGE_INTEGER largeInteger = { 0 };
        largeInteger.LowPart = fileTime.dwLowDateTime;
        largeInteger.HighPart = fileTime.dwHighDateTime;

        return static_cast<uint64_t>(largeInteger.QuadPart);

    #elif defined XPF_PLATFORM_LINUX_UM
        //
        // First retrieve the time of the day.
        // If this fails, we can't do much - return 0;
        //
        timeval timeValue;
        xpf::ApiZeroMemory(&timeValue, sizeof(timeValue));

        if (0 != gettimeofday(&timeValue, NULL))
        {
            XPF_ASSERT(false);
            return 0;
        }

        //
        // The number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
        //
        uint64_t result = 11644473600;

        //
        // Get the number of seconds in 100-nano-seconds interval.
        // 10 to the power of -7 seconds (100-nanosecond intervals).
        //
        result += timeValue.tv_sec;
        result *= 10000000;

        //
        // And now add the microseconds
        // (also convert micro seconds to 100 nano seconds).
        //
        result += (timeValue.tv_usec * 10);

        //
        // And return the result.
        //
        return result;
    #else
        #error Unknown Platform!
    #endif
}

wchar_t
XPF_API
xpf::ApiCharToLower(
    _In_ wchar_t Character
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        return static_cast<wchar_t>(::towlower(static_cast<wint_t>(Character)));
    #elif defined XPF_PLATFORM_WIN_KM
        if (::KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            return ::RtlDowncaseUnicodeChar(Character);
        }
        else
        {
            //
            // This function can only work properly at PASSIVE LEVEL.
            // However don't fail the operation and do best effort.
            // If the character is ANSI we can xor with 0x20.
            // And leave all other characters intact.
            //
            if (Character >= L'A' && Character <= L'Z')
            {
                Character = Character ^ 0x20;
            }
            return Character;
        }
    #else
        #error Unknown Platform
    #endif
}

wchar_t
XPF_API
xpf::ApiCharToUpper(
    _In_ wchar_t Character
) noexcept(true)
{
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        return static_cast<wchar_t>(::towupper(static_cast<wint_t>(Character)));
    #elif defined XPF_PLATFORM_WIN_KM
        if (::KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            return ::RtlUpcaseUnicodeChar(Character);
        }
        else
        {
            //
            // This function can only work properly at PASSIVE LEVEL.
            // However don't fail the operation and do best effort.
            // If the character is ANSI we can xor with 0x20.
            // And leave all other characters intact.
            //
            if (Character >= L'a' && Character <= L'z')
            {
                Character = Character ^ 0x20;
            }
            return Character;
        }
    #else
        #error Unknown Platform
    #endif
}

bool
XPF_API
xpf::ApiEqualCharacters(
    _In_ wchar_t Left,
    _In_ wchar_t Right,
    _In_ bool CaseSensitive
) noexcept(true)
{
    //
    // If the comparison is case insensitive we need to lowercase the characters
    // before doing the actual comparison.
    //
    if (!CaseSensitive)
    {
        Left = xpf::ApiCharToLower(Left);
        Right = xpf::ApiCharToLower(Right);
    }

    return (Left == Right);
}
