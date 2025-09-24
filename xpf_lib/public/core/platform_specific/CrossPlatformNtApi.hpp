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
// @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
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
// |                         PEB Information                                                                         |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

//
// Pointer size is 32 bits for wow
//
#define XPF_WOW64_POINTER(Type)             ULONG

//
// Required for WoW
//
typedef struct _XPF_UNICODE_STRING32
{
    USHORT   Length;
    USHORT   MaximumLength;
    ULONG    Buffer;
} XPF_UNICODE_STRING32;

//
// Required for WoW
//
typedef struct _XPF_LIST_ENTRY32
{
    ULONG Flink;
    ULONG Blink;
} XPF_LIST_ENTRY32;

//
// ntdll!_LDR_DATA_TABLE_ENTRY
//      +0x000 InLoadOrderLinks             : _LIST_ENTRY
//      +0x010 InMemoryOrderLinks           : _LIST_ENTRY
//      +0x020 InInitializationOrderLinks   : _LIST_ENTRY
//      +0x030 DllBase                      : Ptr64 Void
//      +0x038 EntryPoint                   : Ptr64 Void
//      +0x040 SizeOfImage                  : Uint4B
//      +0x048 FullDllName                  : _UNICODE_STRING
//      <...> SNIP <...>
//
typedef struct _XPF_LDR_DATA_TABLE_ENTRY_NATIVE
{
    /* 0x000 */   LIST_ENTRY      InLoadOrderLinks;
    /* 0x010 */   LIST_ENTRY      InMemoryOrderLinks;
    /* 0x020 */   LIST_ENTRY      InInitializationOrderLinks;
    /* 0x030 */   PVOID           DllBase;
    /* 0x038 */   PVOID           EntryPoint;
    /* 0x040 */   ULONG           SizeOfImage;
    /* 0x048 */   UNICODE_STRING  FullDllName;
} XPF_LDR_DATA_TABLE_ENTRY_NATIVE;

//
// ntdll!_LDR_DATA_TABLE_ENTRY
//      +0x000 InLoadOrderLinks                 : _LIST_ENTRY
//      +0x008 InMemoryOrderLinks               : _LIST_ENTRY
//      +0x010 InInitializationOrderLinks       : _LIST_ENTRY
//      +0x018 DllBase                          : Ptr32 Void
//      +0x01c EntryPoint                       : Ptr32 Void
//      +0x020 SizeOfImage                      : Uint4B
//      +0x024 FullDllName                      : _UNICODE_STRING
//      <...> SNIP <...>
//
typedef struct _XPF_LDR_DATA_TABLE_ENTRY32
{
    /* 0x000 */   XPF_LIST_ENTRY32                   InLoadOrderLinks;
    /* 0x008 */   XPF_LIST_ENTRY32                   InMemoryOrderLinks;
    /* 0x010 */   XPF_LIST_ENTRY32                   InInitializationOrderLinks;
    /* 0x018 */   XPF_WOW64_POINTER(PVOID)           DllBase;
    /* 0x01c */   XPF_WOW64_POINTER(PVOID)           EntryPoint;
    /* 0x020 */   ULONG                              SizeOfImage;
    /* 0x024 */   XPF_UNICODE_STRING32               FullDllName;
} XPF_LDR_DATA_TABLE_ENTRY32;

//
// ntdll!_PEB_LDR_DATA
//      +0x000 Length                               : Uint4B
//      +0x004 Initialized                          : UChar
//      +0x008 SsHandle                             : Ptr64 Void
//      +0x010 InLoadOrderModuleList                : _LIST_ENTRY
//      +0x020 InMemoryOrderModuleList              : _LIST_ENTRY
//      +0x030 InInitializationOrderModuleList      : _LIST_ENTRY
//      +0x040 EntryInProgress                      : Ptr64 Void
//      +0x048 ShutdownInProgress                   : UChar
//      +0x050 ShutdownThreadId                     : Ptr64 Void
//
typedef struct _XPF_PEB_LDR_DATA_NATIVE
{
    /* 0x000 */   ULONG           Length;
    /* 0x004 */   BOOLEAN         Initialized;
    /* 0x008 */   HANDLE          SsHandle;
    /* 0x010 */   LIST_ENTRY      InLoadOrderModuleList;
    /* 0x020 */   LIST_ENTRY      InMemoryOrderModuleList;
    /* 0x030 */   LIST_ENTRY      InInitializationOrderModuleList;
    /* 0x040 */   PVOID           EntryInProgress;
    /* 0x048 */   BOOLEAN         ShutdownInProgress;
    /* 0x050 */   HANDLE          ShutdownThreadId;
} XPF_PEB_LDR_DATA_NATIVE;

//
// dt ntdll!_PEB_LDR_DATA
//         +0x000 Length                            : Uint4B
//         +0x004 Initialized                       : UChar
//         +0x008 SsHandle                          : Ptr32 Void
//         +0x00c InLoadOrderModuleList             : _LIST_ENTRY
//         +0x014 InMemoryOrderModuleList           : _LIST_ENTRY
//         +0x01c InInitializationOrderModuleList   : _LIST_ENTRY
//         +0x024 EntryInProgress                   : Ptr32 Void
//         +0x028 ShutdownInProgress                : UChar
//         +0x02c ShutdownThreadId                  : Ptr32 Void
//
typedef struct _XPF_PEB_LDR_DATA32
{
    /* 0x000 */   ULONG                         Length;
    /* 0x004 */   BOOLEAN                       Initialized;
    /* 0x008 */   XPF_WOW64_POINTER(HANDLE)     SsHandle;
    /* 0x00C */   XPF_LIST_ENTRY32              InLoadOrderModuleList;
    /* 0x014 */   XPF_LIST_ENTRY32              InMemoryOrderModuleList;
    /* 0x01C */   XPF_LIST_ENTRY32              InInitializationOrderModuleList;
    /* 0x024 */   XPF_WOW64_POINTER(PVOID)      EntryInProgress;
    /* 0x028 */   BOOLEAN                       ShutdownInProgress;
    /* 0x02C */   XPF_WOW64_POINTER(HANDLE)     ShutdownThreadId;
} XPF_PEB_LDR_DATA32;

//
// dt ntdll!_PEB
//         +0x000 InheritedAddressSpace          : UChar
//         +0x001 ReadImageFileExecOptions       : UChar
//         +0x002 BeingDebugged                  : UChar
//         +0x003 BitField                       : UChar
//         +0x003 ImageUsesLargePages            : Pos 0, 1 Bit
//         +0x003 IsProtectedProcess             : Pos 1, 1 Bit
//         +0x003 IsImageDynamicallyRelocated    : Pos 2, 1 Bit
//         +0x003 SkipPatchingUser32Forwarders   : Pos 3, 1 Bit
//         +0x003 IsPackagedProcess              : Pos 4, 1 Bit
//         +0x003 IsAppContainer                 : Pos 5, 1 Bit
//         +0x003 IsProtectedProcessLight        : Pos 6, 1 Bit
//         +0x003 IsLongPathAwareProcess         : Pos 7, 1 Bit
//         +0x004 Padding0                       : [4] UChar
//         +0x008 Mutant                         : Ptr64 Void
//         +0x010 ImageBaseAddress               : Ptr64 Void
//         +0x018 Ldr                            : Ptr64 _PEB_LDR_DATA
//         <...> SNIP <...>
//
typedef struct _XPF_PEB_NATIVE
{
    /* 0x000 */   BOOLEAN                       InheritedAddressSpace;
    /* 0x001 */   BOOLEAN                       ReadImageFileExecOptions;
    /* 0x002 */   BOOLEAN                       BeingDebugged;
    /* 0x003 */   BOOLEAN                       BitField;
    /* 0x008 */   HANDLE                        Mutant;
    /* 0x010 */   PVOID                         ImageBaseAddress;
    /* 0x018 */   XPF_PEB_LDR_DATA_NATIVE*      Ldr;
} XPF_PEB_NATIVE;

//
// dt ntdll!_PEB
//         +0x000 InheritedAddressSpace : UChar
//         +0x001 ReadImageFileExecOptions : UChar
//         +0x002 BeingDebugged    : UChar
//         +0x003 BitField         : UChar
//         +0x003 ImageUsesLargePages : Pos 0, 1 Bit
//         +0x003 IsProtectedProcess : Pos 1, 1 Bit
//         +0x003 IsImageDynamicallyRelocated : Pos 2, 1 Bit
//         +0x003 SkipPatchingUser32Forwarders : Pos 3, 1 Bit
//         +0x003 IsPackagedProcess : Pos 4, 1 Bit
//         +0x003 IsAppContainer   : Pos 5, 1 Bit
//         +0x003 IsProtectedProcessLight : Pos 6, 1 Bit
//         +0x003 IsLongPathAwareProcess : Pos 7, 1 Bit
//         +0x004 Mutant           : Ptr32 Void
//         +0x008 ImageBaseAddress : Ptr32 Void
//         +0x00c Ldr              : Ptr32 _PEB_LDR_DATA
//         <...> SNIP <...>
//
typedef struct _XPF_PEB32
{
    /* 0x000 */   BOOLEAN                                InheritedAddressSpace;
    /* 0x001 */   BOOLEAN                                ReadImageFileExecOptions;
    /* 0x002 */   BOOLEAN                                BeingDebugged;
    /* 0x003 */   BOOLEAN                                BitField;
    /* 0x004 */   XPF_WOW64_POINTER(HANDLE)              Mutant;
    /* 0x008 */   XPF_WOW64_POINTER(PVOID)               ImageBaseAddress;
    /* 0x00c */   XPF_WOW64_POINTER(XPF_PEB_LDR_DATA32*) Ldr;
} XPF_PEB32;

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
    XpfSystemProcessInformation = 0x05,       // XPF_SYSTEM_PROCESS_INFORMATION
    XpfSystemModuleInformation = 0x0B,        // XPF_RTL_PROCESS_MODULES
    XpfSystemSingleModuleInformation = 0xA7,  // XPF_SYSTEM_SINGLE_MODULE_INFORMATION
    XpfSystemRegisterFirmwareTableInformationHandler = 0x4B,
} XPF_SYSTEM_INFORMATION_CLASS;

typedef struct _XPF_RTL_PROCESS_MODULE_INFORMATION
{
    PVOID  Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    UINT32 ImageSize;
    UINT32 Flags;
    UINT16 LoadOrderIndex;
    UINT16 InitOrderIndex;
    UINT16 LoadCount;
    UINT16 OffsetToFileName;
    CHAR   FullPathName[256];
} XPF_RTL_PROCESS_MODULE_INFORMATION;

typedef struct _XPF_RTL_PROCESS_MODULES
{
    UINT32 NumberOfModules;
    XPF_RTL_PROCESS_MODULE_INFORMATION Modules[1];
} XPF_RTL_PROCESS_MODULES;

typedef struct _XPF_SYSTEM_PROCESS_INFORMATION {
    UINT32 NextEntryOffset;
    UINT32 NumberOfThreads;
    UINT8 Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    PVOID  Reserved2;
    UINT32 HandleCount;
    UINT32 SessionId;
    PVOID  Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    UINT32 Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID  Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID  Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} XPF_SYSTEM_PROCESS_INFORMATION;

typedef struct _XPF_THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} XPF_THREAD_BASIC_INFORMATION;

typedef struct _XPF_RTL_PROCESS_MODULE_INFORMATION_EX
{
    USHORT NextOffset;
    XPF_RTL_PROCESS_MODULE_INFORMATION BaseInfo;
    ULONG ImageChecksum;
    ULONG TimeDateStamp;
    PVOID DefaultBase;
} XPF_RTL_PROCESS_MODULE_INFORMATION_EX;

typedef struct _XPF_SYSTEM_SINGLE_MODULE_INFORMATION
{
    PVOID TargetModuleAddress;
    XPF_RTL_PROCESS_MODULE_INFORMATION_EX ExInfo;
} XPF_SYSTEM_SINGLE_MODULE_INFORMATION;

NTSYSAPI NTSTATUS NTAPI
ZwSetSystemInformation(
    _In_ XPF_SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_ PVOID SystemInformation,
    _In_ ULONG SystemInformationLength
);

NTSYSAPI NTSTATUS NTAPI
ZwQuerySystemInformation(
    _In_ XPF_SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Inout_ PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
);

//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         Process and Thread Information                                                          |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//


NTSYSAPI NTSTATUS NTAPI
ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI NTSTATUS NTAPI
ZwQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
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
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
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


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         PsGetProcessImageFileName                                                               |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI PCHAR NTAPI
PsGetProcessImageFileName(
    _In_ PEPROCESS Process
);


//
// -------------------------------------------------------------------------------------------------------------------
// | ****************************************************************************************************************|
// |                         PsGetProcessPeb                                                                         |
// | ****************************************************************************************************************|
// -------------------------------------------------------------------------------------------------------------------
//

NTSYSAPI PVOID NTAPI
PsGetProcessPeb(
    _In_ PEPROCESS Process
);

#endif  // XPF_PLATFORM_WIN_KM


EXTERN_C_END
#endif  // XPF_PLATFORM_WIN_UM || XPF_PLATFORM_WIN_KM
