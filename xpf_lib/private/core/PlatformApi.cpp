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

void*
XPF_PLATFORM_CONVENTION
operator new(
    size_t BlockSize,
    void* Location
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(0 != BlockSize);
    XPF_DEATH_ON_FAILURE(nullptr != Location);
    return Location;
}

void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    void* Location
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Pointer);
    XPF_DEATH_ON_FAILURE(nullptr != Location);
}

void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    size_t Size
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Pointer);
    XPF_DEATH_ON_FAILURE(0 != Size);
}

void
XPF_PLATFORM_CONVENTION
operator delete(
    void* Pointer,
    size_t Size,
    std::align_val_t Alignment
) noexcept(true)
{
    XPF_DEATH_ON_FAILURE(nullptr != Pointer);
    XPF_DEATH_ON_FAILURE(0 != Size);
    XPF_DEATH_ON_FAILURE(std::align_val_t{ 0 } != Alignment);
}

void
XPF_API
xpf::ApiPanic(
    _In_ NTSTATUS Status
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();
    XPF_VERIFY(!NT_SUCCESS(Status));

    #if defined XPF_PLATFORM_WIN_KM
        /* This can't be called at dispatch level. But if this is paged out, we still get bugcheck. */
        /* No biggie. We'll just lie to the static analyzer here. */
        _Analysis_assume_(KeGetCurrentIrql() <= APC_LEVEL);
        ::ExRaiseStatus(Status);
    #elif defined XPF_PLATFORM_WIN_UM
        ::RtlRaiseStatus(Status);
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
    XPF_MAX_DISPATCH_LEVEL();

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
    XPF_MAX_DISPATCH_LEVEL();

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

bool
XPF_API
xpf::ApiEqualMemory(
    _In_reads_bytes_(Size) void const* Source1,
    _In_reads_bytes_(Size) void const* Source2,
    _In_ size_t Size
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    if ((nullptr == Source1) || (nullptr == Source2) || (0 == Size))
    {
        return false;
    }

    return 0 == ::memcmp(Source1, Source2, Size);
}

void
XPF_API
xpf::ApiFreeMemory(
    _Inout_opt_ void* MemoryBlock
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    //
    // Sanity checks.
    //
    if (nullptr == MemoryBlock)
    {
        return;
    }

    //
    // And now dispose it.
    //
    #if defined XPF_PLATFORM_WIN_KM
        ::ExFreePoolWithTag(MemoryBlock,
                            'nmS+');
    #elif defined XPF_PLATFORM_WIN_UM
        PVOID heapHandle = NULL;
        ::RtlGetProcessHeaps(1, &heapHandle);

        const BOOLEAN result = ::RtlFreeHeap(heapHandle,
                                             0,
                                             MemoryBlock);
        /* We allocated from the process heap. This shouldn't fail. */
        XPF_DEATH_ON_FAILURE(FALSE != result);
    #elif defined XPF_PLATFORM_LINUX_UM
        ::free(MemoryBlock);
    #else
        #error Unknown Platform!
    #endif

    MemoryBlock = nullptr;
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
    XPF_MAX_DISPATCH_LEVEL();

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
            #pragma warning(push)
            #pragma warning(disable:4996)
            block = ::ExAllocatePoolWithTag(pool,
                                            BlockSize,
                                            'nmS+');
            #pragma warning(pop)
        #elif defined XPF_PLATFORM_WIN_UM
        PVOID heapHandle = NULL;
        ::RtlGetProcessHeaps(1, &heapHandle);

        block = ::RtlAllocateHeap(heapHandle,
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
            xpf::ApiFreeMemory(block);
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
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_KM || defined XPF_PLATFORM_WIN_UM
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
        #if defined XPF_PLATFORM_WIN_KM
        if (APC_LEVEL >= ::KeGetCurrentIrql())
        {
            const NTSTATUS status = ::KeDelayExecutionThread(KernelMode, FALSE, &interval);
            XPF_UNREFERENCED_PARAMETER(status);
        }
        #else
            const NTSTATUS status = ::NtDelayExecution(FALSE, &interval);
            XPF_UNREFERENCED_PARAMETER(status);
        #endif  // XPF_PLATFORM_WIN_KM
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
    XPF_MAX_DISPATCH_LEVEL();

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
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_KM
        LARGE_INTEGER largeInteger;
        xpf::ApiZeroMemory(&largeInteger, sizeof(largeInteger));

        KeQuerySystemTime(&largeInteger);

        return static_cast<uint64_t>(largeInteger.QuadPart);
    #elif defined XPF_PLATFORM_WIN_UM

        LARGE_INTEGER largeInteger;
        xpf::ApiZeroMemory(&largeInteger, sizeof(largeInteger));

        NTSTATUS status = ::NtQuerySystemTime(&largeInteger);
        if (!NT_SUCCESS(status))
        {
            largeInteger.QuadPart = 0;
        }
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
            XPF_DEATH_ON_FAILURE(false);
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
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_KM
        //
        // This function can only work properly at PASSIVE LEVEL.
        // However don't fail the operation and do best effort.
        // If the character is ANSI we can xor with 0x20.
        // And leave all other characters intact.
        //
        if (::KeGetCurrentIrql() != PASSIVE_LEVEL)
        {
            if (Character >= L'A' && Character <= L'Z')
            {
                Character = Character ^ 0x20;
            }
            return Character;
        }
    #endif  // XPF_PLATFORM_WIN_KM

    #if defined XPF_PLATFORM_WIN_KM || defined XPF_PLATFORM_WIN_UM
        return static_cast<wchar_t>(::RtlDowncaseUnicodeChar(static_cast<wint_t>(Character)));
    #elif defined XPF_PLATFORM_LINUX_UM
        return static_cast<wchar_t>(towlower(static_cast<wint_t>(Character)));
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
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_KM
        //
        // This function can only work properly at PASSIVE LEVEL.
        // However don't fail the operation and do best effort.
        // If the character is ANSI we can xor with 0x20.
        // And leave all other characters intact.
        //
        if (::KeGetCurrentIrql() != PASSIVE_LEVEL)
        {
            if (Character >= L'A' && Character <= L'Z')
            {
                Character = Character ^ 0x20;
            }
            return Character;
        }
    #endif  // XPF_PLATFORM_WIN_KM

    static_assert(sizeof(wint_t) == sizeof(wchar_t), "Invalid size for wchar_t!");

    #if defined XPF_PLATFORM_WIN_KM || defined XPF_PLATFORM_WIN_UM
        return static_cast<wchar_t>(::RtlUpcaseUnicodeChar(static_cast<wint_t>(Character)));
    #elif defined XPF_PLATFORM_LINUX_UM
        return static_cast<wchar_t>(towupper(static_cast<wint_t>(Character)));
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
    XPF_MAX_DISPATCH_LEVEL();

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

void
XPF_API
xpf::ApiRandomUuid(
    _Out_ uuid_t* NewUuid
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    uuid_t newUuid;
    xpf::ApiZeroMemory(&newUuid, sizeof(newUuid));

    #if defined XPF_PLATFORM_WIN_UM
        uint64_t seed64 = xpf::ApiCurrentTime();
        ULONG seed32 = LODWORD(seed64);

        newUuid.Data1 = ::RtlRandomEx(&seed32);
        newUuid.Data2 = LOWORD(::RtlRandomEx(&seed32));
        newUuid.Data3 = LOWORD(::RtlRandomEx(&seed32));

        newUuid.Data4[0] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[1] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[2] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[3] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;

        newUuid.Data4[4] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[5] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[6] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
        newUuid.Data4[7] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;

        status = STATUS_SUCCESS;

    #elif defined XPF_PLATFORM_WIN_KM
        //
        // This can only be called at passive, if not, take the long route below.
        //
        if (::KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            uint64_t seed64 = xpf::ApiCurrentTime();
            ULONG seed32 = LODWORD(seed64);

            newUuid.Data1 = ::RtlRandomEx(&seed32);
            newUuid.Data2 = LOWORD(::RtlRandomEx(&seed32));
            newUuid.Data3 = LOWORD(::RtlRandomEx(&seed32));

            newUuid.Data4[0] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[1] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[2] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[3] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;

            newUuid.Data4[4] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[5] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[6] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;
            newUuid.Data4[7] = LOWORD(::RtlRandomEx(&seed32)) % 0xFF;

            status = STATUS_SUCCESS;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        uuid_generate_random(newUuid);
        status = STATUS_SUCCESS;
    #else
        #error Unknown Platform
    #endif

    //
    // On fail, take the long route.
    //
    if (!NT_SUCCESS(status))
    {
        for (size_t i = 0; i < sizeof(newUuid); )
        {
            const uint64_t currentTime = xpf::ApiCurrentTime();
            const uint8_t lastByte = currentTime % 0xFF;

            if (xpf::ApiIsHexDigit(lastByte))
            {
                /* Grab the address of newUuid. */
                void* newUuidAddress = xpf::AddressOf(newUuid);

                /* And now interpret it as uint8_t byte array. */
                uint8_t* destination = static_cast<uint8_t*>(newUuidAddress);

                /* We can now just copy the memory. */
                xpf::ApiCopyMemory(&destination[i], xpf::AddressOf(lastByte), sizeof(lastByte));

                /* And move to the next byte. */
                ++i;
            }

            /* Allow some time. */
            xpf::ApiYieldProcesor();
            xpf::ApiCompilerBarrier();
        }
    }

    xpf::ApiCopyMemory(NewUuid, &newUuid, sizeof(newUuid));
}

bool
XPF_API
xpf::ApiAreUuidsEqual(
    _In_ const uuid_t First,
    _In_ const uuid_t Second
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
        return (FALSE != IsEqualIID(First, Second));
    #elif defined XPF_PLATFORM_LINUX_UM
        return (0 == uuid_compare(First, Second));
    #else
        #error Unknown Platform
    #endif
}

bool
XPF_API
xpf::ApiIsHexDigit(
    _In_ char Character
) noexcept(true)
{
    const bool isHexDigit = ((Character >= '0' && Character <= '9') ||
                             (Character >= 'A' && Character <= 'F') ||
                             (Character >= 'a' && Character <= 'f'));
    return isHexDigit;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ApiCaptureStackBacktrace(
    _Inout_ void** Frames,
    _In_ uint32_t Count,
    _Out_ uint32_t* CapturedFrames
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    /* Sanity checks. */
    if ((nullptr == Frames) || (0 == Count) || (nullptr == CapturedFrames))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Preinit output. */
    *CapturedFrames = 0;
    xpf::ApiZeroMemory(*Frames,
                       Count * sizeof(void*));

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    #if defined XPF_PLATFORM_WIN_UM
        __try
        {
            *CapturedFrames = ::RtlWalkFrameChain(&Frames[0],
                                                  Count,
                                                  0);
            status = STATUS_SUCCESS;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_UNHANDLED_EXCEPTION;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        __try
        {
            *CapturedFrames = ::RtlWalkFrameChain(&Frames[0],
                                                  Count,
                                                  0);
            if (::KeGetCurrentIrql() <= APC_LEVEL)
            {
                (*CapturedFrames) += ::RtlWalkFrameChain(&Frames[(*CapturedFrames)],
                                                         Count - (*CapturedFrames),
                                                         1);
            }
            status = STATUS_SUCCESS;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_UNHANDLED_EXCEPTION;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        if (Count <= xpf::NumericLimits<int32_t>::MaxValue())
        {
            int capturedFrames = backtrace(&Frames[0],
                                           static_cast<int>(Count));
            if (capturedFrames > 0)
            {
                *CapturedFrames = static_cast<uint32_t>(capturedFrames);
                status = STATUS_SUCCESS;
            }
        }
    #else
        #error Unknown Platform
    #endif

    /* Don't return garbage. */
    if (!NT_SUCCESS(status))
    {
        *CapturedFrames = 0;
        xpf::ApiZeroMemory(*Frames,
                           Count * sizeof(void*));
    }
    /* Propagate the status.*/
    return status;
}
