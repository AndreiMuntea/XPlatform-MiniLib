//
// @file        xpf_lib/public/core/platform_specific/CrossPlatformNtApi.hpp
//
// @brief       This file fills nt api which are exported but not documented.
//
// @note        This file is skipped from doxygen generation.
//              So the comments are intended to be with slash rather than asterix.
//
// @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
//
// @copyright   Copyright Andrei-Marius MUNTEA 2020-2023.
//              All rights reserved.
//
// @license     See top-level directory LICENSE file.
//

#pragma once



#if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_WIN_KM
EXTERN_C_START

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Stack backtrace                                                                         |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT     8

NTSYSAPI ULONG NTAPI
RtlWalkFrameChain(
    _Out_writes_(Count - (Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT)) PVOID* Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
);

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         System Information                                                                      |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

typedef enum _XPF_SYSTEM_INFORMATION_CLASS
{
    XpfSystemBasicInformation = 0x0,
    XpfSystemRegisterFirmwareTableInformationHandler = 0x4B,
} XPF_SYSTEM_INFORMATION_CLASS;

NTSYSAPI NTSTATUS NTAPI
ZwSetSystemInformation(
    _In_ XPF_SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_ PVOID SystemInformation,
    _In_ ULONG SystemInformationLength
);



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ------------------------------------------------------------------------------------------------------------------- ///
/// | ****************************************************************************************************************| ///
/// |                         UM FILL                                                                                 | ///
/// | ****************************************************************************************************************| ///
/// ------------------------------------------------------------------------------------------------------------------- ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined XPF_PLATFORM_WIN_UM

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Char apiset                                                                             |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI WCHAR NTAPI
RtlDowncaseUnicodeChar(
    _In_ WCHAR SourceCharacter
);

NTSYSAPI WCHAR NTAPI
RtlUpcaseUnicodeChar(
    _In_ WCHAR SourceCharacter
);

NTSYSAPI NTSTATUS NTAPI
RtlUnicodeToUTF8N(
    _Out_ PCHAR UTF8StringDestination,
    _In_ ULONG UTF8StringMaxByteCount,
    _Out_opt_ PULONG UTF8StringActualByteCount,
    _In_reads_bytes_(UnicodeStringByteCount) PCWCH UnicodeStringSource,
    _In_ ULONG UnicodeStringByteCount
);

NTSYSAPI NTSTATUS NTAPI
RtlUTF8ToUnicodeN(
    _Out_ PWSTR UnicodeStringDestination,
    _In_ ULONG UnicodeStringMaxByteCount,
    _Out_opt_ PULONG UnicodeStringActualByteCount,
    _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
    _In_ ULONG UTF8StringByteCount
);

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         RtlRandomEx                                                                             |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI ULONG NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed
);

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         RtlRaiseStatus                                                                          |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI VOID NTAPI
RtlRaiseStatus(
    _In_ NTSTATUS Status
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         NtDelayExecution                                                                        |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI NTSTATUS NTAPI
NtDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Event-related API                                                                       |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

#define NT_EVENT_TYPE_NOTIFICATION      0x0
#define NT_EVENT_TYPE_SYNCHRONIZATION   0x1

NTSYSAPI NTSTATUS NTAPI
NtCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG EventType,
    _In_ BOOLEAN InitialState
);

NTSYSAPI NTSTATUS NTAPI
NtSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
);

NTSYSAPI NTSTATUS NTAPI
NtResetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Thread-related API                                                                      |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)

typedef NTSTATUS(NTAPI* PUSER_THREAD_START_ROUTINE)(_In_ PVOID ThreadParameter);

NTSYSAPI NTSTATUS NTAPI
NtCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _In_ ULONG CreateFlags,
    _In_ SIZE_T ZeroBits,
    _In_ SIZE_T StackSize,
    _In_ SIZE_T MaximumStackSize,
    _In_opt_ PVOID AttributeList
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Resource Lock -related API                                                              |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION CriticalSection;
    HANDLE SharedSemaphore;
    volatile ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    volatile ULONG NumberOfWaitingExclusive;
    volatile LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;
    ULONG Flags;
    PRTL_RESOURCE_DEBUG DebugInfo;
} RTL_RESOURCE, * PRTL_RESOURCE;

NTSYSAPI VOID NTAPI
RtlInitializeResource(
    _Out_ PRTL_RESOURCE Resource
);

NTSYSAPI VOID NTAPI
RtlDeleteResource(
    _Inout_ PRTL_RESOURCE Resource
);

NTSYSAPI BOOLEAN NTAPI
RtlAcquireResourceShared(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
);

NTSYSAPI BOOLEAN NTAPI
RtlAcquireResourceExclusive(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
);

NTSYSAPI VOID NTAPI
RtlReleaseResource(
    _Inout_ PRTL_RESOURCE Resource
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Heap -related API                                                                       |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI ULONG NTAPI
RtlGetProcessHeaps(
    _In_ ULONG NumberOfHeaps,
    _Out_ PVOID* ProcessHeaps
);

NTSYSAPI PVOID NTAPI
RtlAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
);

NTSYSAPI BOOLEAN NTAPI
RtlFreeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
);

#endif  // XPF_PLATFORM_WIN_UM


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ------------------------------------------------------------------------------------------------------------------- ///
/// | ****************************************************************************************************************| ///
/// |                        KM FILL                                                                                  | ///
/// | ****************************************************************************************************************| ///
/// ------------------------------------------------------------------------------------------------------------------- ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined XPF_PLATFORM_WIN_KM

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         APC Environment                                                                         |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT;

typedef VOID (NTAPI* PKNORMAL_ROUTINE)(
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2
);

typedef VOID (NTAPI* PKKERNEL_ROUTINE)(
    _In_ PKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ PVOID* NormalContext,
    _Inout_ PVOID* SystemArgument1,
    _Inout_ PVOID* SystemArgument2
);

typedef VOID (NTAPI* PKRUNDOWN_ROUTINE) (
    _In_ PKAPC Apc
);

NTSYSAPI VOID NTAPI
KeInitializeApc(
    _Out_ PRKAPC Apc,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKKERNEL_ROUTINE KernelRoutine,
    _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ KPROCESSOR_MODE Mode,
    _In_opt_ PVOID NormalContext
);

NTSYSAPI BOOLEAN NTAPI
KeInsertQueueApc(
    _Inout_ PRKAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY Increment
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         ObGetObjectType                                                                         |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI POBJECT_TYPE NTAPI
ObGetObjectType(
    _In_ PVOID Object
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         RtlImageDirectoryEntryToData                                                            |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI PVOID NTAPI
RtlImageDirectoryEntryToData(
    _In_ PVOID BaseOfImage,
    _In_ BOOLEAN MappedAsImage,
    _In_ USHORT DirectoryEntry,
    _Out_ PULONG Size
);

#endif  // XPF_PLATFORM_WIN_KM


EXTERN_C_END
#endif  // XPF_PLATFORM_WIN_UM || XPF_PLATFORM_WIN_KM
