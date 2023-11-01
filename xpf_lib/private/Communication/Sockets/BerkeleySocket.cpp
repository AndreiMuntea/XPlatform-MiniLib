/**
 * @file        xpf_lib/private/Communication/Sockets/BerkeleySocket.cpp
 *
 * @brief       Platform-Specific implementation of berkeley.
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
  * @brief   By default all code in here goes into paged section.
  */
XPF_SECTION_PAGED;

namespace xpf
{
namespace BerkeleySocket
{

struct SocketApiProviderInternal
{
    #if defined XPF_PLATFORM_WIN_UM
        WSAData WsaLibData = { 0 };
    #elif defined XPF_PLATFORM_LINUX_UM
        /* Linux has no data specific for now. */
    #elif defined XPF_PLATFORM_WIN_KM
        WSK_REGISTRATION WskRegistration = { 0 };
        WSK_CLIENT_NPI WskClientNpi = { 0 };
        WSK_PROVIDER_NPI WskProviderNpi = { 0 };
        WSK_CLIENT_DISPATCH WskClientDispatch = { 0 };
    #else
        #error Unknown Platform
    #endif
};  // struct SocketApiProviderInternal

struct SocketInternal
{
    #if defined XPF_PLATFORM_WIN_UM
        SOCKET Socket = INVALID_SOCKET;
    #elif defined XPF_PLATFORM_LINUX_UM
        int Socket = -1;
    #elif defined XPF_PLATFORM_WIN_KM
        /* So we can either listen or send data over a socket. */
        /* To mimic the user mode functionality, we'll create two. */
        /* And depending on how they are used, in their specific calls */
        /* we'll use either one or the other. */
        PWSK_SOCKET ListenSocket = nullptr;
        PWSK_SOCKET ConnectionSocket = nullptr;
    #else
        #error Unknown Platform
    #endif
};  // struct SocketInternal


//
// On windows KM we have a little bit of extra leg work to make this work.
// Each request requires an IRP and an event to wait for to obtain a behavior with UM.
//
#if defined XPF_PLATFORM_WIN_KM

typedef struct _WINKM_WSK_CONTEXT
{
    KEVENT CompletionEvent;
    PIRP Irp;
} WINKM_WSK_CONTEXT;

//
// The completion routine must be declared in non-paged code.
//
XPF_SECTION_DEFAULT;
    static NTSTATUS NTAPI
    WskCompletionRoutine(
        _In_ PDEVICE_OBJECT	DeviceObject,
        _In_ PIRP Irp,
        _In_ PKEVENT CompletionEvent
    ) noexcept(true)
    {
        XPF_MAX_DISPATCH_LEVEL();

        UNREFERENCED_PARAMETER(DeviceObject);
        UNREFERENCED_PARAMETER(Irp);

        (VOID) ::KeSetEvent(CompletionEvent, IO_NO_INCREMENT, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
XPF_SECTION_PAGED;

_Must_inspect_result_
static inline NTSTATUS XPF_API
WskCreateUnicodeStringFromStringView(
    _In_ _Const_ const xpf::StringView<char>& View,
    _Out_ UNICODE_STRING* String
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    ANSI_STRING ansiView;
    ::RtlInitEmptyAnsiString(&ansiView, nullptr, 0);

    UNICODE_STRING unicodeString;
    ::RtlInitEmptyUnicodeString(&unicodeString, nullptr, 0);

    /* Empty view is not supported. */
    if ((View.IsEmpty()) || (NULL == String))
    {
        return STATUS_INVALID_PARAMETER;
    }
    ::RtlInitEmptyUnicodeString(String, nullptr, 0);

    /* Get to the underlying ansi view. */
    NTSTATUS status = ::RtlInitAnsiStringEx(&ansiView, View.Buffer());
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Now convert it to ustr. */
    status = ::RtlAnsiStringToUnicodeString(&unicodeString, &ansiView, TRUE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Ensure we got a valid string. */
    status = ::RtlValidateUnicodeString(0, &unicodeString);
    if (!NT_SUCCESS(status))
    {
        ::RtlFreeUnicodeString(&unicodeString);
        return status;
    }

    /* All good. */
    String->Buffer = unicodeString.Buffer;
    String->Length = unicodeString.Length;
    String->MaximumLength = unicodeString.MaximumLength;
    return STATUS_SUCCESS;
}

static inline void XPF_API
WskFreeUnicodeString(
    _Inout_ UNICODE_STRING* String
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    if (NULL != String)
    {
        ::RtlFreeUnicodeString(String);
    }
}

static inline void XPF_API
WskContextCreate(
    _Inout_ xpf::BerkeleySocket::WINKM_WSK_CONTEXT* Context
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Preinit everything with 0. */
    xpf::ApiZeroMemory(Context, sizeof(*Context));

    /* Create the completion event. */
    ::KeInitializeEvent(&Context->CompletionEvent,
                        EVENT_TYPE::SynchronizationEvent,
                        FALSE);

    /* And now the IRP - We can do something smarter here. But for now use max value. */
    /* We can come back to this in future when we notice a real problem. */
    /* Some requests are critical. So we can't fail. Attempt until we succeed! */
    /* It is not ideal, but not expected to fail. However, we can improve in future. For now this is easier. */
    while (NULL == Context->Irp)
    {
        /* Theoretically a filter could be below us, but legacy filters are no longer officially supported. */
        /* On MSDN we also see it using 1 for the number of stack locations. */
        /* https://learn.microsoft.com/en-us/windows-hardware/drivers/network/using-irps-with-winsock-kernel-functions */
        Context->Irp = ::IoAllocateIrp(1, FALSE);
        if (NULL == Context->Irp)
        {
            xpf::ApiSleep(100);
        }
    }

    /* And now set the completion routine. This will be called on all cases error, success, cancel. */
    ::IoSetCompletionRoutine(Context->Irp,
                             reinterpret_cast<PIO_COMPLETION_ROUTINE>(xpf::BerkeleySocket::WskCompletionRoutine),
                             &Context->CompletionEvent,
                             TRUE,
                             TRUE,
                             TRUE);
}

static inline void XPF_API
WskContextDestroy(
    _Inout_ xpf::BerkeleySocket::WINKM_WSK_CONTEXT* Context
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    if (NULL != Context->Irp)
    {
        ::IoFreeIrp(Context->Irp);
        Context->Irp = NULL;
    }
}

_Must_inspect_result_
static inline NTSTATUS XPF_API
WskContextRetrieveCompletionStatus(
    _In_ NTSTATUS ReturnedStatus,
    _Inout_ xpf::BerkeleySocket::WINKM_WSK_CONTEXT* Context
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* If the status is pending, we wait until everything is completed. */
    if (STATUS_PENDING == ReturnedStatus)
    {
        NTSTATUS status = ::KeWaitForSingleObject(&Context->CompletionEvent,
                                                  KWAIT_REASON::Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  NULL);
        return NT_SUCCESS(status) ? Context->Irp->IoStatus.Status
                                  : status;
    }

    return NT_SUCCESS(ReturnedStatus) ? Context->Irp->IoStatus.Status
                                      : ReturnedStatus;
}

template <class WskApi, typename... Arguments>
static inline NTSTATUS XPF_API
WskCallApi(
    const WskApi WskApiFunction,
    PVOID* IoStatusInformation,
    Arguments&& ...WskApiArguments
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::BerkeleySocket::WINKM_WSK_CONTEXT wskContext;
    xpf::ApiZeroMemory(&wskContext, sizeof(wskContext));

    /* Prepare the call context. */
    xpf::BerkeleySocket::WskContextCreate(&wskContext);

    /* Call the actual API. This will require extra processing. */
    NTSTATUS completionStatus = (*WskApiFunction)(xpf::Forward<Arguments>(WskApiArguments)...,
                                                  wskContext.Irp);

    /* Wait for the actual result. */
    NTSTATUS status = xpf::BerkeleySocket::WskContextRetrieveCompletionStatus(completionStatus, &wskContext);

    /* Set the IoStatusInformation if required. */
    if ((nullptr != IoStatusInformation) && (NT_SUCCESS(status)))
    {
        *IoStatusInformation = reinterpret_cast<PVOID>(wskContext.Irp->IoStatus.Information);
    }

    /* Before returning, we need to do some cleanup. */
    xpf::BerkeleySocket::WskContextDestroy(&wskContext);

    /* Get back to the caller with the actual status. */
    return status;
}

#endif  // XPF_PLATFORM_WIN_KM

};  // namespace BerkeleySocket
};  // namespace xpf


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::InitializeSocketApiProvider(
    _Inout_ xpf::BerkeleySocket::SocketApiProvider* SocketApiProvider
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    xpf::BerkeleySocket::SocketApiProviderInternal* apiProvider = nullptr;

    //
    // Sanity check for parameters.
    //
    if (nullptr == SocketApiProvider)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create the api provider.
    //
    apiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProviderInternal*>(
                    xpf::MemoryAllocator::AllocateMemory(sizeof(*apiProvider)));
    if (nullptr == apiProvider)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::MemoryAllocator::Construct(apiProvider);

    //
    // Now preinitialize the data.
    //
    #if defined XPF_PLATFORM_WIN_UM
        xpf::ApiZeroMemory(&apiProvider->WsaLibData,
                           sizeof(apiProvider->WsaLibData));
    #elif defined XPF_PLATFORM_LINUX_UM
        /* Linux has no data specific for now. */
    #elif defined XPF_PLATFORM_WIN_KM
        xpf::ApiZeroMemory(&apiProvider->WskRegistration,
                           sizeof(apiProvider->WskRegistration));
        xpf::ApiZeroMemory(&apiProvider->WskClientNpi,
                           sizeof(apiProvider->WskClientNpi));
        xpf::ApiZeroMemory(&apiProvider->WskProviderNpi,
                           sizeof(apiProvider->WskProviderNpi));
        xpf::ApiZeroMemory(&apiProvider->WskClientDispatch,
                           sizeof(apiProvider->WskClientDispatch));
    #else
        #error Unknown Platform
    #endif

    //
    // And finally initialize the support.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::WSAStartup(MAKEWORD(2, 2), &apiProvider->WsaLibData);
        if (0 != gleResult)
        {
            xpf::MemoryAllocator::Destruct(apiProvider);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&apiProvider));
            return STATUS_CONNECTION_INVALID;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        /* Linux has no data specific for now. */
    #elif defined XPF_PLATFORM_WIN_KM
        apiProvider->WskClientDispatch.Version = MAKE_WSK_VERSION(1, 0);
        apiProvider->WskClientDispatch.Reserved = 0;
        apiProvider->WskClientDispatch.WskClientEvent = NULL;

        apiProvider->WskClientNpi.ClientContext = NULL;
        apiProvider->WskClientNpi.Dispatch = &apiProvider->WskClientDispatch;

        /* First register the WSK client. */
        NTSTATUS status = ::WskRegister(&apiProvider->WskClientNpi,
                                        &apiProvider->WskRegistration);
        if (!NT_SUCCESS(status))
        {
            xpf::MemoryAllocator::Destruct(apiProvider);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&apiProvider));
            return status;
        }

        /* Now capture the NPI provider. */
        status = ::WskCaptureProviderNPI(&apiProvider->WskRegistration,
                                         WSK_INFINITE_WAIT,
                                         &apiProvider->WskProviderNpi);
        if (!NT_SUCCESS(status))
        {
            ::WskDeregister(&apiProvider->WskRegistration);

            xpf::MemoryAllocator::Destruct(apiProvider);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&apiProvider));
            return status;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // Everything went ok.
    //
    *SocketApiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProvider*>(apiProvider);
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::BerkeleySocket::DeInitializeSocketApiProvider(
    _Inout_ xpf::BerkeleySocket::SocketApiProvider* SocketApiProvider
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Can't free null pointer.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == (*SocketApiProvider)))
    {
        return;
    }
    auto apiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProviderInternal*>(*SocketApiProvider);

    //
    // Deinitialize the support provider.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int cleanUpResult = ::WSACleanup();
        XPF_DEATH_ON_FAILURE(0 == cleanUpResult);

        xpf::ApiZeroMemory(&apiProvider->WsaLibData,
                           sizeof(apiProvider->WsaLibData));
    #elif defined XPF_PLATFORM_LINUX_UM
        /* Linux has no data specific for now. */
    #elif defined XPF_PLATFORM_WIN_KM
        /* We have a guarantee that this was captured. Otherwise Initialize would have failed. */
        ::WskReleaseProviderNPI(&apiProvider->WskRegistration);

        /* We have a guarantee that this was registered. Otherwise Initialize would have failed. */
        ::WskDeregister(&apiProvider->WskRegistration);
    #else
        #error Unknown Platform
    #endif

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(apiProvider);
    xpf::MemoryAllocator::FreeMemory(SocketApiProvider);
    *SocketApiProvider = nullptr;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::GetAddressInformation(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _In_ _Const_ const xpf::StringView<char>& NodeName,
    _In_ _Const_ const xpf::StringView<char>& ServiceName,
    _Out_ xpf::BerkeleySocket::AddressInfo** AddrInfo
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == AddrInfo))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (NodeName.IsEmpty() || ServiceName.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }
    auto apiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProviderInternal*>(SocketApiProvider);

    //
    // Platform specific query.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        UNREFERENCED_PARAMETER(apiProvider);
        int result = getaddrinfo(NodeName.Buffer(), ServiceName.Buffer(), nullptr, AddrInfo);
        if (0 != result)
        {
            *AddrInfo = nullptr;

            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        UNICODE_STRING nodeNameUnicode;
        ::RtlInitEmptyUnicodeString(&nodeNameUnicode, nullptr, 0);

        UNICODE_STRING serviceNameUnicode;
        ::RtlInitEmptyUnicodeString(&serviceNameUnicode, nullptr, 0);

        /* Convert the view to unicode string. */
        status = xpf::BerkeleySocket::WskCreateUnicodeStringFromStringView(NodeName, &nodeNameUnicode);
        if (!NT_SUCCESS(status))
        {
            goto CleanUp;
        }
        status = xpf::BerkeleySocket::WskCreateUnicodeStringFromStringView(ServiceName, &serviceNameUnicode);
        if (!NT_SUCCESS(status))
        {
            xpf::BerkeleySocket::WskFreeUnicodeString(&nodeNameUnicode);
            goto CleanUp;
        }

        /* Now do the actual call. */
        status = xpf::BerkeleySocket::WskCallApi(&apiProvider->WskProviderNpi.Dispatch->WskGetAddressInfo,
                                                 NULL,
                                                 apiProvider->WskProviderNpi.Client,
                                                 &nodeNameUnicode,
                                                 &serviceNameUnicode,
                                                 ULONG{0},
                                                 (GUID*)NULL,
                                                 (PADDRINFOEXW)NULL,
                                                 AddrInfo,
                                                 (PEPROCESS)NULL,
                                                 (PETHREAD)NULL);

        /* And before going further, we cleanup the resources. */
        xpf::BerkeleySocket::WskFreeUnicodeString(&nodeNameUnicode);
        xpf::BerkeleySocket::WskFreeUnicodeString(&serviceNameUnicode);

        /* If we failed, we don't return garbage. */
        if (!NT_SUCCESS(status))
        {
            *AddrInfo = nullptr;
            goto CleanUp;
        }

    #else
        #error Unknown Platform
    #endif

    //
    // Everything went good.
    //
    status = STATUS_SUCCESS;

CleanUp:
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::FreeAddressInformation(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::AddressInfo** AddrInfo
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == AddrInfo))
    {
        return STATUS_INVALID_PARAMETER;
    }
    auto apiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProviderInternal*>(SocketApiProvider);

    //
    // Platform specific cleanup.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        UNREFERENCED_PARAMETER(apiProvider);
        freeaddrinfo(*AddrInfo);
 
    #elif defined XPF_PLATFORM_WIN_KM
        apiProvider->WskProviderNpi.Dispatch->WskFreeAddressInfo(apiProvider->WskProviderNpi.Client,
                                                                 *AddrInfo);
    #else
        #error Unknown Platform
    #endif

    //
    // Don't leave uninitialized memory. Be a good citizen.
    //
    *AddrInfo = nullptr;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::CreateSocket(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _In_ int AddressFamily,
    _In_ int Type,
    _In_ int Protocol,
    _Out_ xpf::BerkeleySocket::Socket* CreatedSocket
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    xpf::BerkeleySocket::SocketInternal* newSocket = nullptr;

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == CreatedSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    auto apiProvider = reinterpret_cast<xpf::BerkeleySocket::SocketApiProviderInternal*>(SocketApiProvider);

    //
    // Create the new socket.
    //
    newSocket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(
                        xpf::MemoryAllocator::AllocateMemory(sizeof(*newSocket)));
    if (nullptr == newSocket)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::MemoryAllocator::Construct(newSocket);

    //
    // Now preinitialize the data.
    //
    #if defined XPF_PLATFORM_WIN_UM
        UNREFERENCED_PARAMETER(apiProvider);
        newSocket->Socket = INVALID_SOCKET;
    #elif defined XPF_PLATFORM_LINUX_UM
        UNREFERENCED_PARAMETER(apiProvider);
        newSocket->Socket = -1;
    #elif defined XPF_PLATFORM_WIN_KM
        newSocket->ListenSocket = nullptr;
        newSocket->ConnectionSocket = nullptr;
    #else
        #error Unknown Platform
    #endif

    //
    // And finally create the socket
    //
    #if defined XPF_PLATFORM_WIN_UM
        newSocket->Socket = ::socket(AddressFamily, Type, Protocol);
        if (INVALID_SOCKET == newSocket->Socket)
        {
            xpf::MemoryAllocator::Destruct(newSocket);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&newSocket));
            return STATUS_CONNECTION_INVALID;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        newSocket->Socket = socket(AddressFamily, Type, Protocol);
        if (-1 == newSocket->Socket)
        {
            xpf::MemoryAllocator::Destruct(newSocket);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&newSocket));
            return STATUS_CONNECTION_INVALID;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        /* First the listen socket. */
        NTSTATUS status = xpf::BerkeleySocket::WskCallApi(&apiProvider->WskProviderNpi.Dispatch->WskSocket,
                                                          reinterpret_cast<PVOID*>(&newSocket->ListenSocket),
                                                          apiProvider->WskProviderNpi.Client,
                                                          static_cast<ADDRESS_FAMILY>(AddressFamily),
                                                          static_cast<USHORT>(Type),
                                                          static_cast<ULONG>(Protocol),
                                                          static_cast<ULONG>(WSK_FLAG_LISTEN_SOCKET),
                                                          (PVOID)NULL,
                                                          (CONST VOID*)NULL,
                                                          (PEPROCESS)NULL,
                                                          (PETHREAD)NULL,
                                                          (PSECURITY_DESCRIPTOR)NULL);
        /* Something failed. Clean the resources. */
        if (!NT_SUCCESS(status))
        {
            newSocket->ListenSocket = nullptr;
        
            NTSTATUS shutdownStatus = ShutdownSocket(apiProvider,
                                                     reinterpret_cast<xpf::BerkeleySocket::Socket*>(&newSocket));
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));
            return status;
        }

        /* Then the connection socket. */
        status = xpf::BerkeleySocket::WskCallApi(&apiProvider->WskProviderNpi.Dispatch->WskSocket,
                                                 reinterpret_cast<PVOID*>(&newSocket->ConnectionSocket),
                                                 apiProvider->WskProviderNpi.Client,
                                                 static_cast<ADDRESS_FAMILY>(AddressFamily),
                                                 static_cast<USHORT>(Type),
                                                 static_cast<ULONG>(Protocol),
                                                 static_cast<ULONG>(WSK_FLAG_CONNECTION_SOCKET),
                                                 (PVOID)NULL,
                                                 (CONST VOID*)NULL,
                                                 (PEPROCESS)NULL,
                                                 (PETHREAD)NULL,
                                                 (PSECURITY_DESCRIPTOR)NULL);

        /* Something failed. Clean the resources. */
        if (!NT_SUCCESS(status))
        {
            newSocket->ConnectionSocket = nullptr;
        
            NTSTATUS shutdownStatus = ShutdownSocket(apiProvider,
                                                     reinterpret_cast<xpf::BerkeleySocket::Socket*>(&newSocket));
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));
            return status;
        }

    #else
        #error Unknown Platform
    #endif

    //
    // Everything went ok.
    //
    *CreatedSocket = reinterpret_cast<xpf::BerkeleySocket::Socket*>(newSocket);
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::ShutdownSocket(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket* TargetSocket
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(*TargetSocket);

    //
    // Proper socket shutdown.
    //
    #if defined XPF_PLATFORM_WIN_UM
        if (INVALID_SOCKET != socket->Socket)
        {
            (void) ::shutdown(socket->Socket, SD_BOTH);
            (void) ::closesocket(socket->Socket);
            socket->Socket = INVALID_SOCKET;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        if (-1 != socket->Socket)
        {
            (void) shutdown(socket->Socket, SHUT_RDWR);
            (void) close(socket->Socket);
            socket->Socket = -1;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        if (nullptr != socket->ConnectionSocket)
        {
            /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
            auto socketInterface = reinterpret_cast<const WSK_PROVIDER_CONNECTION_DISPATCH*>(socket->ConnectionSocket->Dispatch);
            XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
            _Analysis_assume_(nullptr != socketInterface);

            /* Close the socket. */
            NTSTATUS status = xpf::BerkeleySocket::WskCallApi(&socketInterface->Basic.WskCloseSocket,
                                                              NULL,
                                                              socket->ConnectionSocket);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(status));

            /* Don't leave garbage. */
            socket->ConnectionSocket = nullptr;
        }

        if (nullptr != socket->ListenSocket)
        {
            /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
            auto socketInterface = reinterpret_cast<const WSK_PROVIDER_LISTEN_DISPATCH*>(socket->ListenSocket->Dispatch);
            XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
            _Analysis_assume_(nullptr != socketInterface);

            /* Close the socket. */
            NTSTATUS status = xpf::BerkeleySocket::WskCallApi(&socketInterface->Basic.WskCloseSocket,
                                                              NULL,
                                                              socket->ListenSocket);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(status));

            /* Don't leave garbage. */
            socket->ListenSocket = nullptr;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(socket);
    xpf::MemoryAllocator::FreeMemory(TargetSocket);
    *TargetSocket = nullptr;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Bind(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ _Const_ const sockaddr* LocalAddress,
    _In_ int Length
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (nullptr == LocalAddress)
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // Platform specific bind
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        int gleResult = bind(socket->Socket, LocalAddress, Length);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        /* WskBind requires a non-const pointer. So we need a local copy. */
        xpf::Buffer localAddressBuffer;
        NTSTATUS status = localAddressBuffer.Resize(Length);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        xpf::ApiCopyMemory(localAddressBuffer.GetBuffer(), LocalAddress, Length);

        /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
        auto socketInterface = reinterpret_cast<const WSK_PROVIDER_LISTEN_DISPATCH*>(socket->ListenSocket->Dispatch);
        XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
        _Analysis_assume_(nullptr != socketInterface);
        
        /* Do the actual call. */
        status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskBind,
                                                 NULL,
                                                 socket->ListenSocket,
                                                 reinterpret_cast<PSOCKADDR>(localAddressBuffer.GetBuffer()),
                                                 ULONG{ 0 });
        /* Something failed. */
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Listen(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // Platform specific listen.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::listen(socket->Socket, SOMAXCONN);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        int gleResult = listen(socket->Socket, 0x7fffffff);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        /* We created the socket in listening mode. No further operation required. */
        XPF_UNREFERENCED_PARAMETER(socket);
    #else
        #error Unknown Platform
    #endif

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Connect(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ _Const_ const sockaddr* Address,
    _In_ int Length
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == Address))
    {
        return STATUS_INVALID_PARAMETER;
    }
    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // Platform specific connect.
    //
    #if defined XPF_PLATFORM_WIN_UM || defined XPF_PLATFORM_LINUX_UM
        int gleResult = connect(socket->Socket, Address, Length);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        /* WskConnect requires a non-const pointer. So we need a local copy. */
        xpf::Buffer remoteAddressBuffer;
        NTSTATUS status = remoteAddressBuffer.Resize(Length);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        xpf::ApiCopyMemory(remoteAddressBuffer.GetBuffer(), Address, Length);

        /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
        auto socketInterface = reinterpret_cast<const WSK_PROVIDER_CONNECTION_DISPATCH*>(socket->ConnectionSocket->Dispatch);
        XPF_DEATH_ON_FAILURE(nullptr != socketInterface);

        /* If the socket is not bounded to a local address, we'll fail with DEVICE_NOT_READY. */
        SOCKADDR_IN localAddress;
        xpf::ApiZeroMemory(&localAddress, sizeof(localAddress));

        localAddress.sin_family = Address->sa_family;

        /* Bind to local address. */
        static_assert(sizeof(localAddress) == sizeof(SOCKADDR), "Invariant!");
        status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskBind,
                                                 NULL,
                                                 socket->ConnectionSocket,
                                                 reinterpret_cast<PSOCKADDR>(&localAddress),
                                                 ULONG{ 0 });
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        
        /* Do the actual call. */
        status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskConnect,
                                                 NULL,
                                                 socket->ConnectionSocket,
                                                 reinterpret_cast<PSOCKADDR>(remoteAddressBuffer.GetBuffer()),
                                                 ULONG{ 0 });
        /* Something failed. */
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // All good.
    //
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Accept(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _Out_ xpf::BerkeleySocket::Socket* NewSocket
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    xpf::BerkeleySocket::SocketInternal* newSocket = nullptr;

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == NewSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create the new socket.
    //
    newSocket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(
                        xpf::MemoryAllocator::AllocateMemory(sizeof(*newSocket)));
    if (nullptr == newSocket)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    xpf::MemoryAllocator::Construct(newSocket);


    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // Platform specific Accept.
    //
    #if defined XPF_PLATFORM_WIN_UM
        newSocket->Socket = ::accept(socket->Socket, nullptr, nullptr);
        if (INVALID_SOCKET == newSocket->Socket)
        {
            xpf::MemoryAllocator::Destruct(newSocket);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&newSocket));
            return STATUS_CONNECTION_REFUSED;
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        newSocket->Socket = accept(socket->Socket, nullptr, nullptr);
        if (-1 == newSocket->Socket)
        {
            xpf::MemoryAllocator::Destruct(newSocket);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&newSocket));
            return STATUS_CONNECTION_REFUSED;
        }
    #elif defined XPF_PLATFORM_WIN_KM
        /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
        auto socketInterface = reinterpret_cast<const WSK_PROVIDER_LISTEN_DISPATCH*>(socket->ListenSocket->Dispatch);
        XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
        _Analysis_assume_(nullptr != socketInterface);
        
        /* We'll get only the connection socket here. The listen socket will not be created. */
        newSocket->ConnectionSocket = nullptr;
        newSocket->ListenSocket = nullptr;

        /* Do the actual call. */
        NTSTATUS status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskAccept,
                                                          reinterpret_cast<PVOID*>(&newSocket->ConnectionSocket),
                                                          socket->ListenSocket,
                                                          ULONG{0},
                                                          (PVOID)NULL,
                                                          (CONST WSK_CLIENT_CONNECTION_DISPATCH *)NULL,
                                                          (PSOCKADDR)NULL,
                                                          (PSOCKADDR)NULL);
        /* Something failed. */
        if (!NT_SUCCESS(status))
        {
            xpf::MemoryAllocator::Destruct(newSocket);
            xpf::MemoryAllocator::FreeMemory(reinterpret_cast<void**>(&newSocket));
            return status;
        }
    #else
        #error Unknown Platform
    #endif

    //
    // All good.
    //
    *NewSocket = reinterpret_cast<xpf::BerkeleySocket::Socket*>(newSocket);
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Send(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // And now do the actual send. This is platform specific.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int bytesSent = ::send(socket->Socket,
                               reinterpret_cast<const char*>(Bytes),
                               static_cast<int>(NumberOfBytes),
                               0);
        if (SOCKET_ERROR != bytesSent)
        {
            return (static_cast<int>(NumberOfBytes) != bytesSent) ? STATUS_INVALID_BUFFER_SIZE
                                                                  : STATUS_SUCCESS;
        }
        switch (::WSAGetLastError())
        {
            case WSAESHUTDOWN:          // The socket has been shut down.
            case WSAENOTCONN:           // The socket is not connected.
            case WSAECONNABORTED:       // The virtual circuit was terminated due to a time-out or other failure.
                                        //     The application should close the socket as it is no longer usable.
            case WSAECONNRESET:         // The virtual circuit was reset by the remote side executing a hard or abortive close.
                                        //     The application should close the socket as it is no longer usable.
            case WSAEHOSTUNREACH:       // The remote host cannot be reached from this host at this time.
            case WSAENETRESET:          // The connection has been broken due to the keep-alive activity
                                        //     detecting a failure while the operation was in progress.
            {
                return STATUS_CONNECTION_ABORTED;
            }
            default:
            {
                return STATUS_NETWORK_BUSY;
            }
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        ssize_t bytesSent = send(socket->Socket,
                                 Bytes,
                                 NumberOfBytes,
                                 MSG_NOSIGNAL);
        if (-1 != bytesSent)
        {
            return (static_cast<int>(NumberOfBytes) != bytesSent) ? STATUS_INVALID_BUFFER_SIZE
                                                                  : STATUS_SUCCESS;
        }
        switch (errno)
        {
            case EPIPE:                 // The local end has been shut down on a connection oriented.
            case ENOTSOCK:              // The file descriptor sockfd does not refer to a socket.
            case ENOTCONN:              // The socket is not connected, and no target has been given.
            {
                return STATUS_CONNECTION_ABORTED;
            }
            default:
            {
                return STATUS_NETWORK_BUSY;
            }
        }
    #elif defined XPF_PLATFORM_WIN_KM
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        
        /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
        auto socketInterface = reinterpret_cast<const WSK_PROVIDER_CONNECTION_DISPATCH*>(socket->ConnectionSocket->Dispatch);
        XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
        _Analysis_assume_(nullptr != socketInterface);

        /* We need to convert the buffer in a wsk buffer. This is a bit more legwork. */
        WSK_BUF wskBuffer;
        xpf::ApiZeroMemory(&wskBuffer, sizeof(wskBuffer));

        wskBuffer.Offset = 0;
        wskBuffer.Length = NumberOfBytes;
        wskBuffer.Mdl = ::IoAllocateMdl((PVOID)Bytes,
                                        static_cast<ULONG>(NumberOfBytes),
                                        FALSE,
                                        FALSE,
                                        NULL);
        if (NULL == wskBuffer.Mdl)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Now we need to make the buffer resident. */
        __try
        {
            ::MmProbeAndLockPages(wskBuffer.Mdl, KernelMode, IoReadAccess);
            status = STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_ACCESS_VIOLATION;
        }
        if (!NT_SUCCESS(status))
        {
            ::IoFreeMdl(wskBuffer.Mdl);
            return status;
        }

        /* Now do the actual call. */
        PVOID sentBytes = 0;
        status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskSend,
                                                 &sentBytes,
                                                 socket->ConnectionSocket,
                                                 &wskBuffer,
                                                 0);
        /* Free the resources. */
        ::MmUnlockPages(wskBuffer.Mdl);
        ::IoFreeMdl(wskBuffer.Mdl);

        /* Now inspect the status. */
        if (NT_SUCCESS(status))
        {
            ULONG actualSentBytes = (ULONG)(ULONG_PTR)(sentBytes);
            return (static_cast<ULONG>(NumberOfBytes) != actualSentBytes) ? STATUS_INVALID_BUFFER_SIZE
                                                                          : STATUS_SUCCESS;
        }
        if (STATUS_FILE_FORCED_CLOSED == status)
        {
            return STATUS_CONNECTION_ABORTED;
        }
        return STATUS_NETWORK_BUSY;

    #else
        #error Unknown Platform
    #endif
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::BerkeleySocket::Receive(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true)
{
    //
    // We can't use socket code at irql != PASSIVE.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity checks for parameters.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Bytes) || (nullptr == NumberOfBytes) ||
        (0 == *NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < *NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    auto socket = reinterpret_cast<xpf::BerkeleySocket::SocketInternal*>(TargetSocket);

    //
    // And now do the actual receive. This is platform specific.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int bytesReceived = ::recv(socket->Socket,
                                   reinterpret_cast<char*>(Bytes),
                                   static_cast<int>(*NumberOfBytes),
                                   0);
        if (SOCKET_ERROR != bytesReceived)
        {
            if (bytesReceived > static_cast<int>(*NumberOfBytes))
            {
                return STATUS_BUFFER_OVERFLOW;
            }

            *NumberOfBytes = static_cast<size_t>(bytesReceived);
            return STATUS_SUCCESS;
        }
        switch (::WSAGetLastError())
        {
            case WSAESHUTDOWN:          // The socket has been shut down.
            case WSAENOTCONN:           // The socket is not connected.
            case WSAECONNABORTED:       // The virtual circuit was terminated due to a time-out or other failure.
                                        //     The application should close the socket as it is no longer usable.
            case WSAECONNRESET:         // The virtual circuit was reset by the remote side executing a hard or abortive close.
                                        //     The application should close the socket as it is no longer usable.
            case WSAENETRESET:          // The connection has been broken due to the keep-alive activity
                                        //     detecting a failure while the operation was in progress.
            {
                return STATUS_CONNECTION_ABORTED;
            }
            default:
            {
                return STATUS_NETWORK_BUSY;
            }
        }
    #elif defined XPF_PLATFORM_LINUX_UM
        ssize_t bytesReceived = ::recv(socket->Socket,
                                       Bytes,
                                       *NumberOfBytes,
                                       0);
        //
        // When a stream socket peer has performed an orderly shutdown, the
        // return value will be 0 (the traditional "end-of-file" return).
        //
        if (0 == bytesReceived)
        {
            return STATUS_CONNECTION_ABORTED;
        }

        //
        // These calls return the number of bytes received, or -1 if an
        // error occurred.  In the event of an error, errno is set to
        // indicate the error.
        //
        if (-1 != bytesReceived)
        {
            if (bytesReceived > static_cast<ssize_t>(*NumberOfBytes))
            {
                return STATUS_BUFFER_OVERFLOW;
            }

            *NumberOfBytes = static_cast<ssize_t>(bytesReceived);
            return STATUS_SUCCESS;
        }

        switch (errno)
        {
            case ENOTSOCK:              // The file descriptor sockfd does not refer to a socket.
            case ENOTCONN:              // The socket is not connected, and no target has been given.
            {
                return STATUS_CONNECTION_ABORTED;
            }
            default:
            {
                return STATUS_NETWORK_BUSY;
            }
        }
    #elif defined XPF_PLATFORM_WIN_KM
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        
        /* This shouldn't be null. It is a logic bug somewhere in code. Die here if this invariant is violated. */
        auto socketInterface = reinterpret_cast<const WSK_PROVIDER_CONNECTION_DISPATCH*>(socket->ConnectionSocket->Dispatch);
        XPF_DEATH_ON_FAILURE(nullptr != socketInterface);
        _Analysis_assume_(nullptr != socketInterface);

        /* We need to convert the buffer in a wsk buffer. This is a bit more legwork. */
        WSK_BUF wskBuffer;
        xpf::ApiZeroMemory(&wskBuffer, sizeof(wskBuffer));

        wskBuffer.Offset = 0;
        wskBuffer.Length = *NumberOfBytes;
        wskBuffer.Mdl = ::IoAllocateMdl((PVOID)Bytes,
                                        static_cast<ULONG>(*NumberOfBytes),
                                        FALSE,
                                        FALSE,
                                        NULL);
        if (NULL == wskBuffer.Mdl)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Now we need to make the buffer resident. */
        __try
        {
            ::MmProbeAndLockPages(wskBuffer.Mdl, KernelMode, IoWriteAccess);
            status = STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_ACCESS_VIOLATION;
        }
        if (!NT_SUCCESS(status))
        {
            ::IoFreeMdl(wskBuffer.Mdl);
            return status;
        }

        /* Now do the actual call. */
        PVOID receivedBytes = 0;
        status = xpf::BerkeleySocket::WskCallApi(&socketInterface->WskReceive,
                                                 &receivedBytes,
                                                 socket->ConnectionSocket,
                                                 &wskBuffer,
                                                 0);
        /* Free the resources. */
        ::MmUnlockPages(wskBuffer.Mdl);
        ::IoFreeMdl(wskBuffer.Mdl);

        /* Now inspect the status. */
        if (NT_SUCCESS(status))
        {
            ULONG actualReceivedBytes = (ULONG)(ULONG_PTR)(receivedBytes);
            if (actualReceivedBytes > static_cast<ULONG>(*NumberOfBytes))
            {
                return STATUS_BUFFER_OVERFLOW;
            }

            *NumberOfBytes = actualReceivedBytes;
            return STATUS_SUCCESS;
        }
        if (STATUS_FILE_FORCED_CLOSED == status)
        {
            return STATUS_CONNECTION_ABORTED;
        }
        return STATUS_NETWORK_BUSY;
    #else
        #error Unknown Platform
    #endif
}
