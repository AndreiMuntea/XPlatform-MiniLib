/**
 * @file        xpf_lib/private/Communication/Sockets/win_km/WskApi.cpp
 *
 * @brief       This contains the windows - kernel mode header to abstract away
 *              the WSK API interactions. It is intended to be used only in cpp files.
 *              So this header is in private folder, unaccessible for others
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

/*******************************************************************************************
 *                              SIMPLE SOCKET HELPERS                                      *
 *******************************************************************************************/

//
// The code is available only for windows kernel mode.
// It is guarded by these.
//
#if defined XPF_PLATFORM_WIN_KM

//
// The completion routine must be declared in non-paged code.
//
XPF_SECTION_DEFAULT;

NTSTATUS NTAPI
XpfWskCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID CompletionEvent
) noexcept(true)
{
    XPF_MAX_DISPATCH_LEVEL();

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    (VOID) ::KeSetEvent(static_cast<PKEVENT>(CompletionEvent),
                        IO_NO_INCREMENT,
                        FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
XPF_SECTION_PAGED;

static NTSTATUS XPF_API
XpfWskGetCompletionStatus(
    _In_ NTSTATUS ReturnedStatus,
    _In_ xpf::WskCompletionContext* Context
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    if (STATUS_PENDING == ReturnedStatus)
    {
        LARGE_INTEGER socketWaitTimeout = { 0 };
        socketWaitTimeout.QuadPart = -10000ll * 5 * 1000;

        NTSTATUS status = ::KeWaitForSingleObject(&Context->CompletionEvent,
                                                  KWAIT_REASON::Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  &socketWaitTimeout);
        if (status == STATUS_TIMEOUT)
        {
            ::IoCancelIrp(Context->Irp);
            ::KeWaitForSingleObject(&Context->CompletionEvent,
                                    KWAIT_REASON::Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);
        }

        return NT_SUCCESS(status) ? Context->Irp->IoStatus.Status
                                  : status;
    }

    return NT_SUCCESS(ReturnedStatus) ? Context->Irp->IoStatus.Status
                                      : ReturnedStatus;
}

_Must_inspect_result_
static NTSTATUS XPF_API
XpfWskAddrinfoExWtoAddrinfo(
    _In_ const ADDRINFOEXW* Input,
    _Out_ addrinfo** Output
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity check.
    //
    if (nullptr == Output)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Preinit output.
    //
    *Output = nullptr;

    addrinfo* prevOutput = nullptr;
    for (const ADDRINFOEXW* crtInput = Input; crtInput != nullptr; crtInput = crtInput->ai_next)
    {
        //
        // Allocate the new structure.
        //
        addrinfo* crtOutput = static_cast<addrinfo*>(xpf::MemoryAllocator::AllocateMemory(sizeof(addrinfo)));
        if (nullptr == crtOutput)
        {
            break;
        }
        xpf::ApiZeroMemory(crtOutput, sizeof(addrinfo));

        //
        // Fill what we can.
        //
        crtOutput->ai_flags = crtInput->ai_flags;
        crtOutput->ai_family = crtInput->ai_family;
        crtOutput->ai_socktype = crtInput->ai_socktype;
        crtOutput->ai_protocol = crtInput->ai_protocol;

        //
        // Now allocate space for ai_addr.
        //
        crtOutput->ai_addr = static_cast<struct sockaddr*>(xpf::MemoryAllocator::AllocateMemory(crtInput->ai_addrlen));
        if (nullptr != crtOutput->ai_addr)
        {
            crtOutput->ai_addrlen = crtInput->ai_addrlen;
            xpf::ApiCopyMemory(crtOutput->ai_addr,
                               crtInput->ai_addr,
                               crtOutput->ai_addrlen);
        }

        //
        // And now convert the name from wide to ansi.
        //
        UNICODE_STRING wideName = { 0 };
        ANSI_STRING ansiName = { 0 };
        NTSTATUS status = ::RtlInitUnicodeStringEx(&wideName,
                                                   crtInput->ai_canonname);
        if (NT_SUCCESS(status))
        {
            status = ::RtlUnicodeStringToAnsiString(&ansiName,
                                                    &wideName,
                                                    TRUE);
            if (NT_SUCCESS(status))
            {
                crtOutput->ai_canonname = ansiName.Buffer;
            }
        }

        //
        // Now link the pointers.
        //
        crtOutput->ai_next = prevOutput;
        prevOutput = crtOutput;
    }

    *Output = prevOutput;
    return STATUS_SUCCESS;
}

static void XPF_API
XpfWskFreeAddrInfo(
    _Inout_ addrinfo** Output
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity check.
    //
    if (nullptr == Output)
    {
        return;
    }

    //
    // Free structures one by one.
    //
    addrinfo* crtStructure = *Output;
    while (crtStructure != nullptr)
    {
        //
        // Advance the pointer as we'll free this current structure.
        // So this is the first thing we want to do.
        //
        addrinfo* toFree = crtStructure;
        crtStructure = crtStructure->ai_next;

        //
        // Free the name if any.
        //
        if (nullptr != toFree->ai_canonname)
        {
            ANSI_STRING ansiName;
            ::RtlInitAnsiString(&ansiName, toFree->ai_canonname);

            ::RtlFreeAnsiString(&ansiName);
            toFree->ai_canonname = nullptr;
        }

        //
        // Now the ai_addr.
        //
        if (nullptr != toFree->ai_addr)
        {
            xpf::MemoryAllocator::FreeMemory(toFree->ai_addr);
            toFree->ai_addr = nullptr;
        }
        toFree->ai_addrlen = 0;

        //
        // And now the structure itself.
        //
        xpf::MemoryAllocator::FreeMemory(toFree);
        toFree = nullptr;
    }

    //
    // Don't leave garbage.
    //
    *Output = nullptr;
}

/*******************************************************************************************
 *                              SIMPLE SOCKET INTERFACE                                    *
 *******************************************************************************************/

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskInitializeProvider(
    _Inout_ WskSocketProvider* Provider
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // Sanity check.
    //
    if (nullptr == Provider)
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    //
    // Setup the information.
    //
    Provider->WskClientDispatch.Version = MAKE_WSK_VERSION(1, 0);
    Provider->WskClientDispatch.Reserved = 0;
    Provider->WskClientDispatch.WskClientEvent = NULL;

    Provider->WskClientNpi.ClientContext = NULL;
    Provider->WskClientNpi.Dispatch = &Provider->WskClientDispatch;

    //
    // Register the WSK client.
    //
    status = ::WskRegister(&Provider->WskClientNpi,
                           &Provider->WskRegistration);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    Provider->IsProviderRegistered = TRUE;

    //
    // Capture the NPI provider.
    //
    status = ::WskCaptureProviderNPI(&Provider->WskRegistration,
                                     WSK_INFINITE_WAIT,
                                     &Provider->WskProviderNpi);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    Provider->IsNPIProviderCaptured = TRUE;

    //
    // Initialize the security interface for tls sockets.
    // If this will NULL, it is non critical, but the
    // TLS sockets functionality will not be enabled.
    //
    Provider->WskSecurityFunctionTable = ::InitSecurityInterfaceW();

    //
    // All good.
    //
    status = STATUS_SUCCESS;

CleanUp:
    if (!NT_SUCCESS(status))
    {
        xpf::WskDeinitializeProvider(Provider);
    }
    return status;
}

void
XPF_API
xpf::WskDeinitializeProvider(
    _Inout_ WskSocketProvider* Provider
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Nothing to free. Quick Exit.
    //
    if (nullptr == Provider)
    {
        return;
    }

    //
    // Now we release the NPI provider.
    //
    if (FALSE != Provider->IsNPIProviderCaptured)
    {
        ::WskReleaseProviderNPI(&Provider->WskRegistration);
        Provider->IsNPIProviderCaptured = FALSE;
    }

    //
    // Now we unregister the WSK registration.
    //
    if (FALSE != Provider->IsProviderRegistered)
    {
        ::WskDeregister(&Provider->WskRegistration);
        Provider->IsProviderRegistered = FALSE;
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskInitializeCompletionContext(
    _Inout_ WskCompletionContext* Context
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // Sanity check.
    //
    if (nullptr == Context)
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    //
    // Theoretically a filter could be below us, but legacy filters are no longer officially supported.
    // On MSDN we also see it using 1 for the number of stack locations.
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/network/using-irps-with-winsock-kernel-functions
    //
    Context->Irp = ::IoAllocateIrp(1,
                                   FALSE);
    if (nullptr == Context->Irp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }

    //
    // Now create the completion event.
    //
    ::KeInitializeEvent(&Context->CompletionEvent,
                        EVENT_TYPE::SynchronizationEvent,
                        FALSE);

    //
    // And set the completion routine.
    //
    ::IoSetCompletionRoutine(Context->Irp,
                             (PIO_COMPLETION_ROUTINE)XpfWskCompletionRoutine,
                             &Context->CompletionEvent,
                             TRUE,
                             TRUE,
                             TRUE);

    //
    // Everything was ok.
    //
    status = STATUS_SUCCESS;

CleanUp:
    if (!NT_SUCCESS(status))
    {
        xpf::WskDeinitializeCompletionContext(Context);
    }
    return status;
}

void
XPF_API
xpf::WskDeinitializeCompletionContext(
    _Inout_ WskCompletionContext* Context
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Nothing to free. Quick Exit.
    //
    if (nullptr == Context)
    {
        return;
    }

    //
    // Release the IRP now.
    //
    if (nullptr != Context->Irp)
    {
        ::IoFreeIrp(Context->Irp);
        Context->Irp = nullptr;
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskInitializeWskBuffer(
    _Inout_ WskBuffer* Buffer,
    _In_ LOCK_OPERATION Operation,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // Sanity check.
    //
    if (nullptr == Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Preinit output.
    //
    Buffer->WskBuf.Length = 0;;
    Buffer->WskBuf.Mdl = NULL;
    Buffer->WskBuf.Offset = 0;
    Buffer->ArePagesResident = FALSE;

    //
    // We'll copy the bytes.
    //
    status = Buffer->RawBuffer.Resize(NumberOfBytes);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    xpf::ApiCopyMemory(Buffer->RawBuffer.GetBuffer(),
                       Bytes,
                       NumberOfBytes);

    //
    // Now we'll allocate a WSK_BUFFER properly.
    //
    Buffer->WskBuf.Length = Buffer->RawBuffer.GetSize();
    Buffer->WskBuf.Offset = 0;
    Buffer->WskBuf.Mdl = ::IoAllocateMdl(Buffer->RawBuffer.GetBuffer(),
                                         static_cast<ULONG>(Buffer->RawBuffer.GetSize()),
                                         FALSE,
                                         FALSE,
                                         NULL);
    if (NULL == Buffer->WskBuf.Mdl)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }

    //
    // Now let's make the pages resident.
    //
    __try
    {
        ::MmProbeAndLockPages(Buffer->WskBuf.Mdl,
                              KernelMode,
                              Operation);

        status = STATUS_SUCCESS;
        Buffer->ArePagesResident = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_ACCESS_VIOLATION;
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        xpf::WskDeinitializeWskBuffer(Buffer);
    }
    return status;
}

void
XPF_API
xpf::WskDeinitializeWskBuffer(
    _Inout_ WskBuffer* Buffer
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Nothing to free.
    //
    if (nullptr == Buffer)
    {
        return;
    }

    //
    // First we unlock the pages.
    //
    if (FALSE != Buffer->ArePagesResident)
    {
        ::MmUnlockPages(Buffer->WskBuf.Mdl);
        Buffer->ArePagesResident = FALSE;
    }

    //
    // Now we clean the mdl.
    //
    if (nullptr != Buffer->WskBuf.Mdl)
    {
        ::IoFreeMdl(Buffer->WskBuf.Mdl);
        Buffer->WskBuf.Mdl = nullptr;
    }
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskGetAddrInfo(
    _In_ WskSocketProvider* SocketApiProvider,
    _In_ _Const_ const xpf::StringView<char>& NodeName,
    _In_ _Const_ const xpf::StringView<char>& ServiceName,
    _Out_ addrinfo** AddrInfo
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    xpf::String<wchar_t> nodeNameWide;
    UNICODE_STRING nodeNameUstr = { 0 };

    xpf::String<wchar_t> serviceNameWide;
    UNICODE_STRING serviceNameUstr = { 0 };

    xpf::WskCompletionContext context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PADDRINFOEXW result = nullptr;

    //
    // Sanity check.
    //
    if ((nullptr == AddrInfo) || (nullptr == SocketApiProvider))
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    //
    // Preinit output variables.
    //
    *AddrInfo = nullptr;

    //
    // First we convert the ansi string to wide.
    //
    status = xpf::StringConversion::UTF8ToWide(NodeName,
                                               nodeNameWide);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    status = xpf::StringConversion::UTF8ToWide(ServiceName,
                                               serviceNameWide);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now we grab an UNICODE_STRING.
    //
    status = ::RtlInitUnicodeStringEx(&nodeNameUstr,
                                      &nodeNameWide[0]);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    status = ::RtlInitUnicodeStringEx(&serviceNameUstr,
                                      &serviceNameWide[0]);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Call GetAddressInfo and wait for IRP if required.
    //
    status = SocketApiProvider->WskProviderNpi.Dispatch->WskGetAddressInfo(SocketApiProvider->WskProviderNpi.Client,
                                                                           &nodeNameUstr,
                                                                           &serviceNameUstr,
                                                                           0,
                                                                           NULL,
                                                                           NULL,
                                                                           &result,
                                                                           NULL,
                                                                           NULL,
                                                                           context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // And a bit of extra leg-work in order to convert addrinfoexw to addrinfoA
    //
    status = XpfWskAddrinfoExWtoAddrinfo(result,
                                         AddrInfo);
    //
    // Free result before returning.
    //
    SocketApiProvider->WskProviderNpi.Dispatch->WskFreeAddressInfo(SocketApiProvider->WskProviderNpi.Client,
                                                                   result);

CleanUp:
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskFreeAddrInfo(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ addrinfo** AddrInfo
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    if (nullptr == SocketApiProvider)
    {
        return STATUS_INVALID_PARAMETER;
    }

    XpfWskFreeAddrInfo(AddrInfo);
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskCreateSocket(
    _In_ WskSocketProvider* SocketApiProvider,
    _In_ int AddressFamily,
    _In_ int Type,
    _In_ int Protocol,
    _In_ bool IsListeningSocket,
    _Out_ WskSocket* CreatedSocket
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG socketFlags = (IsListeningSocket) ? WSK_FLAG_LISTEN_SOCKET
                                            : WSK_FLAG_CONNECTION_SOCKET;
    xpf::WskCompletionContext context;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == CreatedSocket))
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    CreatedSocket->Socket = nullptr;
    CreatedSocket->IsListeningSocket = (IsListeningSocket) ? TRUE
                                                           : FALSE;
    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Call WskSocket and wait for IRP if required.
    //
    status = SocketApiProvider->WskProviderNpi.Dispatch->WskSocket(SocketApiProvider->WskProviderNpi.Client,
                                                                   static_cast<ADDRESS_FAMILY>(AddressFamily),
                                                                   static_cast<USHORT>(Type),
                                                                   Protocol,
                                                                   socketFlags,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // All good. Check that we got a valid pointer.
    //
    CreatedSocket->Socket = (PWSK_SOCKET)(context.Irp->IoStatus.Information);
    if (nullptr == CreatedSocket->Socket)
    {
        status = STATUS_INVALID_ADDRESS;
        goto CleanUp;
    }

    CreatedSocket->DispatchTable.Dispatch = CreatedSocket->Socket->Dispatch;
    XPF_DEATH_ON_FAILURE(nullptr != CreatedSocket->DispatchTable.Dispatch);

    status = STATUS_SUCCESS;

CleanUp:
    if (!NT_SUCCESS(status))
    {
        if ((nullptr != CreatedSocket) && (nullptr != CreatedSocket->Socket))
        {
            (void) xpf::WskShutdownSocket(SocketApiProvider,
                                          CreatedSocket);
        }
    }
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskShutdownSocket(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;
    PFN_WSK_CLOSE_SOCKET closeSocket = nullptr;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket))
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    //
    // The socket was not properly created.
    // Nothing to do.
    //
    if (TargetSocket->Socket == nullptr)
    {
        status = STATUS_SUCCESS;
        goto CleanUp;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Handle differently depending on socket type.
    //
    closeSocket = (TargetSocket->IsListeningSocket) ? TargetSocket->DispatchTable.ListenDispatch->Basic.WskCloseSocket
                                                    : TargetSocket->DispatchTable.ConnectionDispatch->Basic.WskCloseSocket;
    XPF_DEATH_ON_FAILURE(nullptr != closeSocket);
    _Analysis_assume_(nullptr != closeSocket);

    status = closeSocket(TargetSocket->Socket,
                         context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

CleanUp:
    if (nullptr != TargetSocket)
    {
        TargetSocket->DispatchTable.Dispatch = nullptr;
        TargetSocket->Socket = nullptr;
    }
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskBind(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ _Const_ const sockaddr* LocalAddress,
    _In_ int Length
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;
    PFN_WSK_BIND bindSocket = nullptr;

    SOCKADDR localAddress;
    xpf::ApiZeroMemory(&localAddress, sizeof(localAddress));

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == LocalAddress) || (Length != static_cast<int>(sizeof(sockaddr))))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Bind call requires a non-const pointer. So do a copy here.
    //
    xpf::ApiCopyMemory(&localAddress, LocalAddress, Length);

    //
    // Handle differently depending on socket type.
    //
    bindSocket = (TargetSocket->IsListeningSocket) ? TargetSocket->DispatchTable.ListenDispatch->WskBind
                                                   : TargetSocket->DispatchTable.ConnectionDispatch->WskBind;
    XPF_DEATH_ON_FAILURE(nullptr != bindSocket);
    _Analysis_assume_(nullptr != bindSocket);

    status = bindSocket(TargetSocket->Socket,
                        &localAddress,
                        0,
                        context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

CleanUp:
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskListen(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Socket was already created in listening mode. Nothing left to do.
    //
    return (TargetSocket->IsListeningSocket) ? STATUS_SUCCESS
                                             : STATUS_NOT_SUPPORTED;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskConnect(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ _Const_ const sockaddr* Address,
    _In_ int Length
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    SOCKADDR remoteAddress;
    xpf::ApiZeroMemory(&remoteAddress, sizeof(remoteAddress));

    SOCKADDR localAddress;
    xpf::ApiZeroMemory(&localAddress, sizeof(localAddress));

    SOCKADDR_IN localAddressIn;
    xpf::ApiZeroMemory(&localAddressIn, sizeof(localAddressIn));

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Address) || (Length != static_cast<int>(sizeof(sockaddr))))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (FALSE != TargetSocket->IsListeningSocket)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    // If the socket is not bounded to a local address, we'll fail with DEVICE_NOT_READY.
    //
    localAddressIn.sin_port = 0;
    localAddressIn.sin_addr.s_addr = INADDR_ANY;
    localAddressIn.sin_family = Address->sa_family;
    static_assert(sizeof(localAddress) == sizeof(localAddressIn), "Invariant!");

    xpf::ApiCopyMemory(&localAddress, &localAddressIn, sizeof(localAddress));
    status = xpf::WskBind(SocketApiProvider,
                          TargetSocket,
                          &localAddress,
                          sizeof(localAddress));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    xpf::ApiCopyMemory(&remoteAddress, Address, Length);
    status = TargetSocket->DispatchTable.ConnectionDispatch->WskConnect(TargetSocket->Socket,
                                                                        &remoteAddress,
                                                                        0,
                                                                        context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }


CleanUp:
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskAccept(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _Out_ WskSocket* NewSocket
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (nullptr == NewSocket)
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (FALSE == TargetSocket->IsListeningSocket)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    // Preinit output.
    //
    NewSocket->Socket = nullptr;
    NewSocket->DispatchTable.Dispatch = nullptr;
    NewSocket->IsListeningSocket = FALSE;

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    status = TargetSocket->DispatchTable.ListenDispatch->WskAccept(TargetSocket->Socket,
                                                                   0,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // All good. Check that we got a valid pointer.
    //
    NewSocket->Socket = (PWSK_SOCKET)(context.Irp->IoStatus.Information);
    if (nullptr == NewSocket->Socket)
    {
        status = STATUS_INVALID_ADDRESS;
        goto CleanUp;
    }

    NewSocket->DispatchTable.Dispatch = NewSocket->Socket->Dispatch;
    XPF_DEATH_ON_FAILURE(nullptr != NewSocket->DispatchTable.Dispatch);

CleanUp:
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskSend(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;
    xpf::WskBuffer wskBuffer;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (FALSE != TargetSocket->IsListeningSocket)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    status = xpf::WskInitializeWskBuffer(&wskBuffer,
                                         IoReadAccess,
                                         NumberOfBytes,
                                         Bytes);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now send data.
    //
    status = TargetSocket->DispatchTable.ConnectionDispatch->WskSend(TargetSocket->Socket,
                                                                     &wskBuffer.WskBuf,
                                                                     0,
                                                                     context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Check that everything was sent.
    //
    if (static_cast<ULONG>(context.Irp->IoStatus.Information) != NumberOfBytes)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto CleanUp;
    }

CleanUp:
    xpf::WskDeinitializeWskBuffer(&wskBuffer);
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskReceive(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true)
{
    //
    // We don't expect this to be called at any other IRQL.
    //
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::WskCompletionContext context;
    xpf::WskBuffer wskBuffer;

    //
    // Sanity check.
    //
    if ((nullptr == SocketApiProvider) || (nullptr == TargetSocket) || (nullptr == TargetSocket->Socket))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if ((nullptr == Bytes) || (0 == *NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < *NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }
    if (FALSE != TargetSocket->IsListeningSocket)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    //
    // Now we prepare for calling the API.
    //
    status = xpf::WskInitializeCompletionContext(&context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }
    status = xpf::WskInitializeWskBuffer(&wskBuffer,
                                         IoWriteAccess,
                                         *NumberOfBytes,
                                         Bytes);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // Now send data.
    //
    status = TargetSocket->DispatchTable.ConnectionDispatch->WskReceive(TargetSocket->Socket,
                                                                        &wskBuffer.WskBuf,
                                                                        0,
                                                                        context.Irp);
    status = XpfWskGetCompletionStatus(status, &context);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    *NumberOfBytes = static_cast<ULONG>(context.Irp->IoStatus.Information);
    xpf::ApiCopyMemory(Bytes,
                       wskBuffer.RawBuffer.GetBuffer(),
                       *NumberOfBytes);

CleanUp:
    xpf::WskDeinitializeWskBuffer(&wskBuffer);
    xpf::WskDeinitializeCompletionContext(&context);
    return status;
}

/*******************************************************************************************
 *                              SECURE SOCKET STRUCTURES                                   *
 *******************************************************************************************/

///
/// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ne-schannel-etlsalgorithmusage
///
typedef enum _eTlsAlgorithmUsage
{
    /* Key exchange algorithm. (e.g. RSA, ECDHE, DHE) */
    TlsParametersCngAlgUsageKeyExchange,
    /* Signature algorithm. (e.g. RSA, DSA, ECDSA) */
    TlsParametersCngAlgUsageSignature,
    /* Encryption algorithm. (e.g. AES, DES, RC4) */
    TlsParametersCngAlgUsageCipher,
    /* Digest of cipher suite. (e.g. SHA1, SHA256, SHA384) */
    TlsParametersCngAlgUsageDigest,
    /* Signature and/or hash used to sign certificate. (e.g. RSA, DSA, ECDSA, SHA1, SHA256) */
    TlsParametersCngAlgUsageCertSig
} eTlsAlgorithmUsage;

///
/// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-crypto_settings
///
typedef struct _CRYPTO_SETTINGS
{
    /* The algorithm being used as specified in the eTlsAlgorithmUsage enumeration. */
    eTlsAlgorithmUsage eAlgorithmUsage;

    /* 
     * The CNG algorithm identifier.
     * https://learn.microsoft.com/en-us/windows/win32/seccng/cng-algorithm-identifiers
     *
     * Cryptographic settings are ignored if the specified algorithm is not used by a supported,
     * enabled cipher suite or an available credential.
     */
    UNICODE_STRING     strCngAlgId;

    /*
     * The count of entries in the rgstrChainingModes array.
     * Set to 0 if strCngAlgId does not have a chaining mode
     * (e.g. BCRYPT_SHA384_ALGORITHM). It is an error to specify
     * more than SCH_CRED_MAX_SUPPORTED_CHAINING_MODES. 
     */
    DWORD              cChainingModes;

    /**
     * An array of CNG chaining mode identifiers.
     * https://learn.microsoft.com/en-us/windows/win32/seccng/cng-property-identifiers
     *
     * Set to NULL if strCngAlgId does not have a chaining mode (e.g. BCRYPT_SHA384_ALGORITHM).
     */
    PUNICODE_STRING    rgstrChainingModes;

    /*
     * Minimum bit length for the specified CNG algorithm.
     * If 0, schannel uses system defaults. Set to 0 if the CNG algorithm implies bit length
     * (e.g. BCRYPT_ECDH_P521_ALGORITHM). 
     */
    DWORD              dwMinBitLength;

    /*
     * Maximum bit length for the specified CNG algorithm.
     * If 0, schannel uses system defaults. Set to 0 if the CNG algorithm implies bit length
     * (e.g. BCRYPT_ECDH_P521_ALGORITHM). 
     */
    DWORD              dwMaxBitLength;
} CRYPTO_SETTINGS, * PCRYPTO_SETTINGS;

///
/// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-tls_parameters
///
typedef struct _TLS_PARAMETERS
{
    /**
     * The number of ALPN Ids in rgstrAlpnIds.
     * Set to 0 if the following parameter restrictions apply regardless
     * of the negotiated application protocol. It is an error to specify more than SCH_CRED_MAX_SUPPORTED_ALPN_IDS. 
     */
    DWORD            cAlpnIds;

    /**
     * An array of ALPN IDs that the following parameters apply to.
     * Set to NULL if parameter restrictions apply regardless of the negotiated application protocol.
     */
    PUNICODE_STRING  rgstrAlpnIds;

    /**
     * The bit string that represents the disabled protocols (i.e protocols you DO NOT want negotiated).
     * Set to 0 to use system defaults. Schannel protocol flags are documented here:
     * https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-schannel_cred
     */
    DWORD            grbitDisabledProtocols;

    /**
     * The count of entries in the pDisabledCrypto array.
     * It is an error to specify more than SCH_CRED_MAX_SUPPORTED_CRYPTO_SETTINGS.
     */
    DWORD            cDisabledCrypto;

    /**
     * An array of pointers to the CRYPTO_SETTINGS structures that express disabled cryptographic settings.
     */
    PCRYPTO_SETTINGS pDisabledCrypto;

    /**
     * (optional) The flags to pass.
     *
     * When TLS_PARAMS_OPTIONAL is set, TLS_PARAMETERS will only be honored if
     * they do not cause the server to terminate the handshake.
     * Otherwise, schannel may fail TLS handshakes in order to honor the TLS_PARAMETERS restrictions.
     */
    DWORD            dwFlags;
} TLS_PARAMETERS, * PTLS_PARAMETERS;

///
/// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-sch_credentials
/// The SCH_CREDENTIALS structure contains initialization information for an Schannel credential.
///
typedef struct _SCH_CREDENTIALS
{
    /**
     *  Set to SCH_CREDENTIALS_VERSION.
     */
    DWORD             dwVersion;

    /**
     * Kernel-mode Schannel supports the following values:
     *  (0x1)   SCH_CRED_FORMAT_CERT_HASH       --  The paCred member of the SCH_CREDENTIALS structure
     *                                              passed in must be a pointer to a byte array of length 20
     *                                              that contains the certificate thumbprint. The certificate
     *                                              is assumed to be in the "MY" store of the local computer.
     *  (0x2)   SCH_CRED_FORMAT_CERT_HASH_STORE --  The paCred member of the SCH_CREDENTIALS structure points
     *                                              to a SCHANNEL_CERT_HASH_STORE structure.
     *
     * Windows Server 2008, Windows Vista, Windows Server 2003, Windows XP and Windows XP/2000:
     * This flag is not supported and must be zero.
     */
    DWORD             dwCredFormat;

    /**
     * The number of structures in the paCred array.
     */
    DWORD             cCreds;

    /**
     * An array of pointers to CERT_CONTEXT structures. Each pointer specifies a certificate that contains
     * a private key to be used in authenticating the application. Client applications often pass in an
     * empty list and either depend on Schannel to find an appropriate certificate or create a certificate later if needed.
     */
    PVOID             *paCred;

    /**
     * Optional. Valid for server applications only. Handle to a certificate store that contains self-signed root certificates
     * for certification authorities (CAs) trusted by the application. This member is used only by server-side applications
     * that require client authentication.
     */
    PVOID             hRootStore;

    /**
     * Reserved.
     */
    DWORD             cMappers;

    /**
     * Reserved.
     */
    struct _HMAPPER   **aphMappers;

    /**
     * The number of milliseconds that Schannel keeps the session in its session cache. After this time has passed,
     * any new connections between the client and the server require a new Schannel session.
     * Set the value of this member to zero to use the default value of 36000000 milliseconds (ten hours).
     */
    DWORD             dwSessionLifespan;

    /**
     * Contains bit flags that control the behavior of Schannel.
     */
    DWORD             dwFlags;

    /**
     * The count of entries in the pTlsParameters array.
     * It is an error to specify more than SCH_CRED_MAX_SUPPORTED_PARAMETERS.
     */
    DWORD             cTlsParameters;

    /**
     * Array of pointers to the TLS_PARAMETERS structures that indicate TLS parameter restrictions, if any.
     * If no restrictions are specified, the system defaults are used.
     * It is recommended that applications rely on the system defaults.
     * It is an error to include more than one TLS_PARAMETERS structure with cAlpnIds == 0 and rgstrAlpnIds == NULL.
     */
    PTLS_PARAMETERS   pTlsParameters;
} SCH_CREDENTIALS, *PSCH_CREDENTIALS;

///
/// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-schannel_cred
/// The SCHANNEL_CRED structure is deprecated. You should use SCH_CREDENTIALS instead.
///
typedef struct _SCHANNEL_CRED {
    DWORD             dwVersion;
    DWORD             cCreds;
    PVOID*            paCred;
    void*             hRootStore;
    DWORD             cMappers;
    struct _HMAPPER** aphMappers;
    DWORD             cSupportedAlgs;
    void*             palgSupportedAlgs;
    DWORD             grbitEnabledProtocols;
    DWORD             dwMinimumCipherStrength;
    DWORD             dwMaximumCipherStrength;
    DWORD             dwSessionLifespan;
    DWORD             dwFlags;
    DWORD             dwCredFormat;
} SCHANNEL_CRED, *PSCHANNEL_CRED;

///
/// Definitions in schannel.h
///

#define SCHANNEL_NAME_A             "Schannel"
#define SCHANNEL_NAME_W             L"Schannel"

#define SCH_CRED_V1                 0x00000001
#define SCH_CRED_V2                 0x00000002  // for legacy code
#define SCH_CRED_VERSION            0x00000002  // for legacy code
#define SCH_CRED_V3                 0x00000003  // for legacy code
#define SCHANNEL_CRED_VERSION       0x00000004  // for legacy code
#define SCH_CREDENTIALS_VERSION     0x00000005

#define SCH_CRED_NO_SYSTEM_MAPPER                    0x00000002
#define SCH_CRED_NO_SERVERNAME_CHECK                 0x00000004
#define SCH_CRED_MANUAL_CRED_VALIDATION              0x00000008
#define SCH_CRED_NO_DEFAULT_CREDS                    0x00000010
#define SCH_CRED_AUTO_CRED_VALIDATION                0x00000020
#define SCH_CRED_USE_DEFAULT_CREDS                   0x00000040
#define SCH_CRED_DISABLE_RECONNECTS                  0x00000080
#define SCH_CRED_REVOCATION_CHECK_END_CERT           0x00000100
#define SCH_CRED_REVOCATION_CHECK_CHAIN              0x00000200
#define SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x00000400
#define SCH_CRED_IGNORE_NO_REVOCATION_CHECK          0x00000800
#define SCH_CRED_IGNORE_REVOCATION_OFFLINE           0x00001000
#define SCH_CRED_RESTRICTED_ROOTS                    0x00002000
#define SCH_CRED_REVOCATION_CHECK_CACHE_ONLY         0x00004000
#define SCH_CRED_CACHE_ONLY_URL_RETRIEVAL            0x00008000
#define SCH_CRED_MEMORY_STORE_CERT                   0x00010000
#define SCH_CRED_CACHE_ONLY_URL_RETRIEVAL_ON_CREATE  0x00020000
#define SCH_SEND_ROOT_CERT                           0x00040000
#define SCH_CRED_SNI_CREDENTIAL                      0x00080000
#define SCH_CRED_SNI_ENABLE_OCSP                     0x00100000
#define SCH_SEND_AUX_RECORD                          0x00200000
#define SCH_USE_STRONG_CRYPTO                        0x00400000
#define SCH_USE_PRESHAREDKEY_ONLY                    0x00800000
#define SCH_USE_DTLS_ONLY                            0x01000000
#define SCH_ALLOW_NULL_ENCRYPTION                    0x02000000

#define SECPKG_CRED_INBOUND         0x00000001
#define SECPKG_CRED_OUTBOUND        0x00000002
#define SECPKG_CRED_BOTH            0x00000003
#define SECPKG_CRED_DEFAULT         0x00000004
#define SECPKG_CRED_RESERVED        0xF0000000

#define SEC_E_OK                        ((HRESULT)(0x00000000L))
#define SEC_E_TARGET_UNKNOWN            ((HRESULT)(0x80090303L))
#define SEC_E_INCOMPLETE_MESSAGE        ((HRESULT)(0x80090318L))
#define SEC_I_CONTINUE_NEEDED           ((HRESULT)(0x00090312L))
#define SEC_I_CONTEXT_EXPIRED           ((HRESULT)(0x00090317L))
#define SEC_I_INCOMPLETE_CREDENTIALS    ((HRESULT)(0x00090320L))

#define SP_PROT_TLS1_1_SERVER           0x00000100
#define SP_PROT_TLS1_1_CLIENT           0x00000200
#define SP_PROT_TLS1_1                  (SP_PROT_TLS1_1_SERVER | \
                                         SP_PROT_TLS1_1_CLIENT)

#define SP_PROT_TLS1_2_SERVER           0x00000400
#define SP_PROT_TLS1_2_CLIENT           0x00000800
#define SP_PROT_TLS1_2                  (SP_PROT_TLS1_2_SERVER | \
                                         SP_PROT_TLS1_2_CLIENT)

#define SP_PROT_TLS1_3_SERVER           0x00001000
#define SP_PROT_TLS1_3_CLIENT           0x00002000
#define SP_PROT_TLS1_3                  (SP_PROT_TLS1_3_SERVER | \
                                         SP_PROT_TLS1_3_CLIENT)

/*******************************************************************************************
 *                              SECURE SOCKET HELPERS                                      *
 *******************************************************************************************/

_Function_class_(ACQUIRE_CREDENTIALS_HANDLE_FN_W)
_Success_(return == SEC_E_OK)
_IRQL_requires_max_(PASSIVE_LEVEL)
static SECURITY_STATUS
XpfSecAcquireCredentialsHandle(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _In_opt_ PSECURITY_STRING Principal,
    _In_ PSECURITY_STRING Package,
    _In_ ULONG CredentialUse,
    _In_opt_ PVOID LogonId,
    _In_opt_ PVOID AuthData,
    _In_opt_ SEC_GET_KEY_FN GetKeyFn,
    _In_opt_ PVOID GetKeyArgument,
    _Out_ PCredHandle Credential,
    _Out_opt_ PTimeStamp Expiry
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->AcquireCredentialsHandleW))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->AcquireCredentialsHandleW(Principal,
                                                                                  Package,
                                                                                  CredentialUse,
                                                                                  LogonId,
                                                                                  AuthData,
                                                                                  GetKeyFn,
                                                                                  GetKeyArgument,
                                                                                  Credential,
                                                                                  Expiry);
}

_Function_class_(FREE_CREDENTIALS_HANDLE_FN)
_Success_(return == SEC_E_OK)
_IRQL_requires_max_(PASSIVE_LEVEL)
static SECURITY_STATUS
XpfSecFreeCredentialsHandle(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _In_ PCredHandle Credential
)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->FreeCredentialsHandle))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->FreeCredentialsHandle(Credential);
}

_Function_class_(INITIALIZE_SECURITY_CONTEXT_FN_W)
_Success_(return == SEC_E_OK)
_IRQL_requires_max_(PASSIVE_LEVEL)
static SECURITY_STATUS
XpfSecInitializeSecurityContextW(
    _In_ xpf::WskSocketProvider * SocketApiProvider,        // NOLINT(*)
    _In_opt_ PCredHandle Credential,                        // NOLINT(*)
    _In_opt_ PCtxtHandle Context,                           // NOLINT(*)
    _In_opt_ PSECURITY_STRING TargetName,                   // NOLINT(*)
    _In_ unsigned long ContextReq,                          // NOLINT(*)
    _In_ unsigned long Reserved1,                           // NOLINT(*)
    _In_ unsigned long TargetDataRep,                       // NOLINT(*)
    _In_opt_ PSecBufferDesc Input,                          // NOLINT(*)
    _In_ unsigned long Reserved2,                           // NOLINT(*)
    _Inout_opt_ PCtxtHandle NewContext,                     // NOLINT(*)
    _Inout_opt_ PSecBufferDesc Output,                      // NOLINT(*)
    _Out_ unsigned long* ContextAttr,                       // NOLINT(*)
    _Out_opt_ PTimeStamp Expiry                             // NOLINT(*)
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->FreeCredentialsHandle))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->InitializeSecurityContextW(Credential,
                                                                                   Context,
                                                                                   TargetName,
                                                                                   ContextReq,
                                                                                   Reserved1,
                                                                                   TargetDataRep,
                                                                                   Input,
                                                                                   Reserved2,
                                                                                   NewContext,
                                                                                   Output,
                                                                                   ContextAttr,
                                                                                   Expiry);
}

_Function_class_(FREE_CONTEXT_BUFFER_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
static VOID
XpfSecFreeContextBuffer(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _Inout_opt_ PVOID ContextBuffer
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved - likely an invalid usage.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->FreeContextBuffer))
    {
        XPF_ASSERT(FALSE);
        return;
    }

    //
    // This should always succeed.
    //
    if (ContextBuffer)
    {
        SECURITY_STATUS status = SocketApiProvider->WskSecurityFunctionTable->FreeContextBuffer(ContextBuffer);
        XPF_DEATH_ON_FAILURE(SEC_E_OK == status);
    }
}

_Function_class_(QUERY_CONTEXT_ATTRIBUTES_FN_W)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
static SECURITY_STATUS
XpfSecQueryContextAttributes(
    _In_ xpf::WskSocketProvider * SocketApiProvider,        // NOLINT(*)
    _In_ PCtxtHandle Context,                               // NOLINT(*)
    _In_ unsigned long Attribute,                           // NOLINT(*)
    _Out_ PVOID Buffer                                      // NOLINT(*)
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->QueryContextAttributesW))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->QueryContextAttributesW(Context,
                                                                                Attribute,
                                                                                Buffer);
}

_Function_class_(ENCRYPT_MESSAGE_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
static SECURITY_STATUS
XpfSecEncryptMessage(
    _In_ xpf::WskSocketProvider * SocketApiProvider,
    _In_ PCtxtHandle Context,
    _In_ ULONG QOP,
    _In_ PSecBufferDesc Message,
    _In_ ULONG MessageSeqNo
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->EncryptMessage))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->EncryptMessage(Context,
                                                                       QOP,
                                                                       Message,
                                                                       MessageSeqNo);
}

_Function_class_(DECRYPT_MESSAGE_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
static SECURITY_STATUS
XpfSecDecryptMessage(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _In_ PCtxtHandle Context,
    _In_ PSecBufferDesc Message,
    _In_ ULONG MessageSeqNo,
    _Out_opt_ PULONG QOP
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->DecryptMessage))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->DecryptMessage(Context,
                                                                       Message,
                                                                       MessageSeqNo,
                                                                       QOP);
}

_Function_class_(DELETE_SECURITY_CONTEXT_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
static SECURITY_STATUS
XpfSecDeleteSecurityContext(
    _In_ xpf::WskSocketProvider * SocketApiProvider,
    _In_ PCtxtHandle Context
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->DeleteSecurityContext))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->DeleteSecurityContext(Context);
}

_Function_class_(APPLY_CONTROL_TOKEN_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
static SECURITY_STATUS
XpfSecApplyControlToken(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _In_ PCtxtHandle Context,
    _In_ PSecBufferDesc Input
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    //
    // Api was not resolved.
    //
    if ((nullptr == SocketApiProvider) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable) ||
        (nullptr == SocketApiProvider->WskSecurityFunctionTable->ApplyControlToken))
    {
        return SEC_E_TARGET_UNKNOWN;
    }

    return SocketApiProvider->WskSecurityFunctionTable->ApplyControlToken(Context,
                                                                          Input);
}

_Must_inspect_result_
static NTSTATUS
XpfFinalizeHandshake(
    _In_ xpf::WskSocketProvider* SocketApiProvider,
    _Inout_ xpf::WskSocketTlsContext* TlsContext
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Should always be in system process.
    //
    XPF_DEATH_ON_FAILURE(PsGetCurrentProcess() == PsInitialSystemProcess);

    XPF_DEATH_ON_FAILURE(nullptr != TlsContext);

    SECURITY_STATUS secStatus = SEC_E_OK;
    uint32_t maxMessageSize = 0;

    //
    // We need to find the maximum negotiated message size.
    //
    secStatus = XpfSecQueryContextAttributes(SocketApiProvider,
                                             &TlsContext->ContextHandle,
                                             SECPKG_ATTR_STREAM_SIZES,
                                             &TlsContext->StreamSizes);
    if (SEC_E_OK != secStatus)
    {
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    if (!xpf::ApiNumbersSafeAdd(maxMessageSize, uint32_t{ TlsContext->StreamSizes.cbHeader }, &maxMessageSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (!xpf::ApiNumbersSafeAdd(maxMessageSize, uint32_t{ TlsContext->StreamSizes.cbMaximumMessage }, &maxMessageSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    if (!xpf::ApiNumbersSafeAdd(maxMessageSize, uint32_t{ TlsContext->StreamSizes.cbTrailer }, &maxMessageSize))
    {
        return STATUS_INTEGER_OVERFLOW;
    }
    return TlsContext->TlsBuffer.Resize(maxMessageSize);
}

/*******************************************************************************************
 *                              SECURE SOCKET INTERFACE                                    *
 *******************************************************************************************/

_Must_inspect_result_
NTSTATUS XPF_API
xpf::WskCreateTlsSocketContext(
    _In_ WskSocketProvider* SocketApiProvider,
    _In_ bool TlsSkipCertificateValidation,
    _Out_ WskSocketTlsContext** TlsContext
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(nullptr != TlsContext);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    SECURITY_STATUS securityStatus = SEC_E_OK;
    bool isAttached = false;
    KAPC_STATE state = { 0 };
    RTL_OSVERSIONINFOW version = { 0 };

    UNICODE_STRING schannelName = RTL_CONSTANT_STRING(SCHANNEL_NAME_W);
    WskSocketTlsContext* tlsContext = nullptr;

    PVOID usedCredentials = nullptr;
    SCHANNEL_CRED legacyCredentials = { 0 };

    SCH_CREDENTIALS credentials = { 0 };
    TLS_PARAMETERS tlsParameters[1] = { 0 };

    *TlsContext = nullptr;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    tlsContext = static_cast<WskSocketTlsContext*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::WskSocketTlsContext)));
    if (nullptr == tlsContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanUp;
    }

    xpf::MemoryAllocator::Construct(tlsContext);
    SecInvalidateHandle(&tlsContext->CredentialsHandle);
    SecInvalidateHandle(&tlsContext->ContextHandle);

    tlsParameters[0].cAlpnIds = 0;                          /* Apply restrictions regardless of negotiated protocol. */
    tlsParameters[0].rgstrAlpnIds = NULL;                   /* Apply restrictions regardless of negotiated protocol. */
    tlsParameters[0].grbitDisabledProtocols = 0;            /* Use system defaults. */
    tlsParameters[0].cDisabledCrypto = 0;                   /* The count of entries in the pDisabledCrypto array. */
    tlsParameters[0].pDisabledCrypto = NULL;                /* No disabled entries. */
    tlsParameters[0].dwFlags = 0;                           /* Optional; pass 0. */

    credentials.dwVersion = SCH_CREDENTIALS_VERSION;        /* Required to be set on this. */
    credentials.dwCredFormat = 0;                           /* Not using certs. */
    credentials.cCreds = 0;                                 /* 0 certificates. */
    credentials.paCred = NULL;                              /* No certificates. */
    credentials.hRootStore = NULL;                          /* Valid only for servers. */
    credentials.cMappers = 0;                               /* Reserved. */
    credentials.aphMappers = NULL;                          /* Reserved. */
    credentials.dwSessionLifespan = 0;                      /* Default cache lifespan (10 hours). */
    credentials.dwFlags = SCH_USE_STRONG_CRYPTO |           /* Disable known weak cryptographic algorithms*/
                          SCH_CRED_AUTO_CRED_VALIDATION |   /* Client only. Less work for us :D */
                          SCH_CRED_NO_DEFAULT_CREDS |       /* Client only. Don't supply certificate chain. */
                          SCH_CRED_REVOCATION_CHECK_CHAIN;  /* Check all certificates if they were revoked.*/
    credentials.cTlsParameters = ARRAYSIZE(tlsParameters);  /* Number of tls parameters. */
    credentials.pTlsParameters = tlsParameters;             /* TlsParameters. */

    legacyCredentials.dwVersion = SCHANNEL_CRED_VERSION;
    legacyCredentials.grbitEnabledProtocols = SP_PROT_TLS1_2;        /* Disallow newer TLS versions - might not have KBs. */
    legacyCredentials.dwFlags = SCH_USE_STRONG_CRYPTO |              /* Disable known weak cryptographic algorithms*/
                                SCH_CRED_AUTO_CRED_VALIDATION |      /* Client only. Less work for us :D */
                                SCH_CRED_NO_DEFAULT_CREDS;           /* Client only. Don't supply certificate chain. */

    /* If we need to skip the certificate validation, we elminate the flags. */
    if (TlsSkipCertificateValidation)
    {
        credentials.dwFlags = SCH_USE_STRONG_CRYPTO                     |
                              SCH_CRED_MANUAL_CRED_VALIDATION           |
                              SCH_CRED_NO_SERVERNAME_CHECK              |
                              SCH_CRED_NO_DEFAULT_CREDS;
        legacyCredentials.dwFlags = SCH_USE_STRONG_CRYPTO               |
                                    SCH_CRED_MANUAL_CRED_VALIDATION     |
                                    SCH_CRED_NO_SERVERNAME_CHECK        |
                                    SCH_CRED_NO_DEFAULT_CREDS;
    }

    /* By default we prefer the newer variant of credentials. */
    usedCredentials = &credentials;
    tlsContext->UsesOlderTls = false;

    /* On older os'es we need to use legacy variant of representing credentials. */
    version.dwOSVersionInfoSize = sizeof(version);
    status = ::RtlGetVersion(&version);
    if (!NT_SUCCESS(status) || version.dwMajorVersion < 10)
    {
        usedCredentials = &legacyCredentials;
        tlsContext->UsesOlderTls = true;
    }

    securityStatus = XpfSecAcquireCredentialsHandle(SocketApiProvider,
                                                    NULL,                           /* We're using Schannel - must be NULL  */                      // NOLINT(*)
                                                    &schannelName,                  /* Kernel mode callers must specify SCHANNEL_NAME. */           // NOLINT(*)
                                                    SECPKG_CRED_OUTBOUND,           /* Local client preparing outgoing token. */                    // NOLINT(*)
                                                    NULL,                           /* When using Schannel SSP this must be NULL. */                // NOLINT(*)
                                                    usedCredentials,                /* Package-specific data SCH_CREDENTIALS */                     // NOLINT(*)
                                                    NULL,                           /* This parameter is not used and should be set to NULL. */     // NOLINT(*)
                                                    NULL,                           /* This parameter is not used and should be set to NULL. */     // NOLINT(*)
                                                    &tlsContext->CredentialsHandle, /* Receive the credential handle. */                            // NOLINT(*)
                                                    NULL);                          /* When using the Schannel SSP, this parameter is optional */   // NOLINT(*)
    if (securityStatus != SEC_E_OK)
    {
        status = STATUS_INVALID_NETWORK_RESPONSE;
        goto CleanUp;
    }

    //
    // And allocate a large enough buffer to hold the data.
    //
    status = tlsContext->TlsBuffer.Resize(PAGE_SIZE * 5);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    //
    // All good.
    //
    *TlsContext = tlsContext;
    tlsContext = nullptr;

    status = STATUS_SUCCESS;

CleanUp:
    if (nullptr != tlsContext)
    {
        xpf::WskDestroyTlsSocketContext(SocketApiProvider,
                                        &tlsContext);
    }

    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }

    return status;
}

VOID XPF_API
xpf::WskDestroyTlsSocketContext(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocketTlsContext** TlsContext
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    bool isAttached = false;
    KAPC_STATE state = { 0 };

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    if ((nullptr != TlsContext) && (nullptr != (*TlsContext)))
    {
        WskSocketTlsContext* tlsContext = *TlsContext;
        *TlsContext = nullptr;

        /* Should have been properly shut down before! */
        XPF_DEATH_ON_FAILURE(!SecIsValidHandle(&tlsContext->ContextHandle));

        if (SecIsValidHandle(&tlsContext->CredentialsHandle))
        {
            SECURITY_STATUS cleanupStatus = XpfSecFreeCredentialsHandle(SocketApiProvider,
                                                                        &tlsContext->CredentialsHandle);
            XPF_DEATH_ON_FAILURE(cleanupStatus == SEC_E_OK);

            SecInvalidateHandle(&tlsContext->CredentialsHandle);
        }

        xpf::MemoryAllocator::Destruct(tlsContext);
        xpf::MemoryAllocator::FreeMemory(tlsContext);
    }

    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }
}


_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskTlsSocketHandshake(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* Socket,
    _Inout_ WskSocketTlsContext* TlsContext,
    _In_ PSECURITY_STRING TargetName
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(Socket != nullptr);
    XPF_DEATH_ON_FAILURE(TlsContext != nullptr);
    XPF_DEATH_ON_FAILURE(TargetName != nullptr);

    XPF_DEATH_ON_FAILURE(SecIsValidHandle(&TlsContext->CredentialsHandle));
    XPF_DEATH_ON_FAILURE(!SecIsValidHandle(&TlsContext->ContextHandle));


    bool isAttached = false;
    KAPC_STATE state = { 0 };
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    //
    // Tls handshake is a bit of a tedious process:
    //  -> Call InitializeSecurityContext to create schannel context.
    //          -> SEC_I_CONTINUE_NEEDED        - The client must send the output token to the server and
    //                                            wait for a return token. The returned token is then
    //                                            passed in another call to InitializeSecurityContext.
    //          -> SEC_I_INCOMPLETE_CREDENTIALS - The server has requested client authentication, and
    //                                            the supplied credentials either do not include a
    //                                            certificate or the certificate was not issued by a
    //                                            certification authority that is trusted by the server.
    //                                            !!! THIS IS NOT SUPPORTED FOR NOW AND TREATED AS ERROR !!!
    //          -> SEC_E_INCOMPLETE_MESSAGE     - Data for the whole message was not read from the wire.
    //                                            When this value is returned, the pInput buffer contains a
    //                                            SecBuffer structure with a BufferType member of
    //                                            SECBUFFER_MISSING.The cbBuffer member of SecBuffer contains
    //                                            a value that indicates the number of additional bytes that
    //                                            the function must read from the client before this
    //                                            function succeeds.
    //          -> SEC_E_OK                     - The security context was successfully initialized.
    //                                            There is no need for another InitializeSecurityContext call.
    //                                            If the function returns an output token, that is, if the
    //                                            SECBUFFER_TOKEN in pOutput is of nonzero length, that
    //                                            token must be sent to the server.
    //

    CtxtHandle* context = nullptr;
    uint32_t receivedSize = 0;
    SECURITY_STATUS secStatus = SEC_E_OK;

    SecBuffer inbuffers[2] = { 0 };
    SecBuffer outbuffers[1] = { 0 };

    SecBufferDesc indesc =
    {
        .ulVersion = SECBUFFER_VERSION,
        .cBuffers  = ARRAYSIZE(inbuffers),
        .pBuffers  = inbuffers
    };
    SecBufferDesc outdesc =
    {
        .ulVersion = SECBUFFER_VERSION,
        .cBuffers  = ARRAYSIZE(outbuffers),
        .pBuffers  = outbuffers
    };


    while (true)
    {
        unsigned long contextRequest = ISC_REQ_ALLOCATE_MEMORY      |   // The security package allocates output buffers for you.       // NOLINT(*)
                                                                        // When you have finished using the output buffers,
                                                                        // free them by calling the FreeContextBuffer function.
                                       ISC_REQ_CONFIDENTIALITY      |   // Encrypt messages by using the EncryptMessage function.
                                       ISC_REQ_USE_SUPPLIED_CREDS   |   // Schannel must not attempt to supply credentials
                                                                        // for the client automatically.
                                       ISC_REQ_REPLAY_DETECT        |   // Detect replayed messages that have been encoded by
                                                                        // using the EncryptMessage or MakeSignature functions.
                                       ISC_REQ_INTEGRITY            |   // Sign messages and verify signatures by using the
                                                                        // EncryptMessage and MakeSignature functions.
                                       ISC_REQ_SEQUENCE_DETECT      |   // Detect messages received out of sequence.
                                       ISC_REQ_EXTENDED_ERROR       |   // When errors occur, the remote party will be notified.
                                       ISC_REQ_STREAM;                  // Support a stream-oriented connection.

        inbuffers[0].BufferType = SECBUFFER_TOKEN;
        inbuffers[0].pvBuffer = TlsContext->TlsBuffer.GetBuffer();
        inbuffers[0].cbBuffer = receivedSize;

        inbuffers[1].BufferType = SECBUFFER_EMPTY;
        inbuffers[1].pvBuffer = NULL;
        inbuffers[1].cbBuffer = 0;

        XpfSecFreeContextBuffer(SocketApiProvider,
                                outbuffers[0].pvBuffer);

        outbuffers[0].BufferType = SECBUFFER_TOKEN;
        outbuffers[0].cbBuffer = 0;
        outbuffers[0].pvBuffer = NULL;

        /* This needs to be passed only on the first call. */
        /* On subsequenc calls, context is a handle to a partially initialized context. */
        PSECURITY_STRING targetName = (context != nullptr) ? nullptr
                                                           : TargetName;

        secStatus = XpfSecInitializeSecurityContextW(SocketApiProvider,
                                                     &TlsContext->CredentialsHandle,
                                                     context,
                                                     targetName,
                                                     contextRequest,
                                                     0,
                                                     SECURITY_NETWORK_DREP,
                                                     (nullptr == context) ? nullptr : &indesc,
                                                     0,
                                                     &TlsContext->ContextHandle,
                                                     &outdesc,
                                                     &contextRequest,
                                                     nullptr);
        context = &TlsContext->ContextHandle;

        //
        // See RandyGaul's comment on https://gist.github.com/mmozeiko/c0dfcc8fec527a90a02145d2cc0bfb6d
        //
        // "Sometimes during the handshake SEC_E_INCOMPLETE_MESSAGE can be encountered,
        //  meaning a decrypt failed as the full record was not present.
        //  In this case we need to call recv and append more data, then try again."
        //
        if (inbuffers[1].BufferType == SECBUFFER_EXTRA &&
            inbuffers[1].cbBuffer > 0)
        {
            //
            // SECBUFFER_EXTRA means that there are some bytes that has not
            // been processed. This can happen if we are negotiating a
            // connection and this extra data is part of the
            // handshake, usually due to the initial buffer being insufficient.
            //
            // This data needs to be moved at the front of the buffer.
            //
            uint32_t offset = 0;
            uint32_t end = 0;
            if (!xpf::ApiNumbersSafeSub(receivedSize, static_cast<uint32_t>(inbuffers[1].cbBuffer), &offset))
            {
                status = STATUS_INTEGER_OVERFLOW;
                break;
            }
            if (!xpf::ApiNumbersSafeAdd(offset, static_cast<uint32_t>(inbuffers[1].cbBuffer), &end))
            {
                status = STATUS_INTEGER_OVERFLOW;
                break;
            }
            if (end > TlsContext->TlsBuffer.GetSize())
            {
                status = STATUS_BUFFER_OVERFLOW;
                break;
            }
            xpf::ApiCopyMemory(TlsContext->TlsBuffer.GetBuffer(),
                               xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(), offset),
                               inbuffers[1].cbBuffer);
            receivedSize = inbuffers[1].cbBuffer;

            //
            // It is not documented, but it seems in practice we are hitting this case.
            // We can get SEC_I_CONTINUE_NEEDED but we have an output buffer, hence we
            // are forced to make a retrieve to complete the token.
            //
            if (secStatus == SEC_I_CONTINUE_NEEDED &&
                outbuffers[0].BufferType != SECBUFFER_MISSING)
            {
                secStatus = SEC_E_INCOMPLETE_MESSAGE;
            }
        }
        else if (inbuffers[1].BufferType != SECBUFFER_MISSING)
        {
            receivedSize = 0;
        }

        if (secStatus == SEC_E_OK)
        {
            status = XpfFinalizeHandshake(SocketApiProvider,
                                          TlsContext);
            break;
        }
        else if (secStatus == SEC_I_CONTINUE_NEEDED)
        {
            status = STATUS_INVALID_STATE_TRANSITION;

            if (outbuffers[0].BufferType != SECBUFFER_MISSING &&
                outbuffers[0].pvBuffer != NULL &&
                outbuffers[0].cbBuffer != 0)
            {
                status = WskSend(SocketApiProvider,
                                 Socket,
                                 outbuffers[0].cbBuffer,
                                 static_cast<uint8_t*>(outbuffers[0].pvBuffer));
            }
            if (!NT_SUCCESS(status))
            {
                break;
            }
        }
        else if (secStatus == SEC_I_INCOMPLETE_CREDENTIALS)
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }
        else if (secStatus == SEC_E_INCOMPLETE_MESSAGE)
        {
            //
            // Realloc TLS buffer if needed.
            //
            if (receivedSize >= TlsContext->TlsBuffer.GetSize())
            {
                size_t finalSize = 0;
                if (!xpf::ApiNumbersSafeAdd(TlsContext->TlsBuffer.GetSize(), static_cast<size_t>(PAGE_SIZE * 5), &finalSize))
                {
                    status = STATUS_INTEGER_OVERFLOW;
                    break;
                }

                status = TlsContext->TlsBuffer.Resize(finalSize);
                if (!NT_SUCCESS(status))
                {
                    break;
                }
            }

            //
            // Receive extra data.
            //
            size_t toReceive = TlsContext->TlsBuffer.GetSize() - receivedSize;
            status = WskReceive(SocketApiProvider,
                                Socket,
                                &toReceive,
                                static_cast<uint8_t*>(xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(),
                                                                            receivedSize)));
            if (!NT_SUCCESS(status))
            {
                break;
            }

            //
            // Should fit in uint32_t
            //
            if (toReceive > xpf::NumericLimits<uint32_t>::MaxValue())
            {
                status = STATUS_DATA_ERROR;
                break;
            }

            receivedSize += static_cast<uint32_t>(toReceive);
            status = STATUS_SUCCESS;
        }
        else
        {
            XPF_ASSERT(FALSE);
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    XpfSecFreeContextBuffer(SocketApiProvider,
                            outbuffers[0].pvBuffer);

    if (!NT_SUCCESS(status) && SecIsValidHandle(&TlsContext->ContextHandle))
    {
        SECURITY_STATUS cleanupStatus = XpfSecDeleteSecurityContext(SocketApiProvider,
                                                                    &TlsContext->ContextHandle);
        SecInvalidateHandle(&TlsContext->ContextHandle);
        XPF_DEATH_ON_FAILURE(SEC_E_OK == cleanupStatus);
    }

    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }
    return status;
}

VOID XPF_API
xpf::WskTlsShutdown(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* Socket,
    _Inout_ WskSocketTlsContext* TlsContext
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    bool isAttached = false;
    KAPC_STATE state = { 0 };
    SECURITY_STATUS secStatus = SEC_E_OK;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    if (!SecIsValidHandle(&TlsContext->ContextHandle))
    {
        goto CleanUp;
    }

    while (true)
    {
        unsigned long contextRequest = ISC_REQ_ALLOCATE_MEMORY      |    // The security package allocates output buffers for you.  // NOLINT(*)
                                                                         // When you have finished using the output buffers,
                                                                         // free them by calling the FreeContextBuffer function.
                                       ISC_REQ_CONFIDENTIALITY      |    // Encrypt messages by using the EncryptMessage function.
                                       ISC_REQ_USE_SUPPLIED_CREDS   |    // Schannel must not attempt to supply credentials
                                                                         // for the client automatically.
                                       ISC_REQ_REPLAY_DETECT        |    // Detect replayed messages that have been encoded by
                                                                         // using the EncryptMessage or MakeSignature functions.
                                       ISC_REQ_INTEGRITY            |    // Sign messagesand verify signatures by using the
                                                                         // EncryptMessageand MakeSignature functions.
                                       ISC_REQ_SEQUENCE_DETECT      |    // Detect messages received out of sequence.
                                       ISC_REQ_EXTENDED_ERROR       |    // When errors occur, the remote party will be notified.
                                       ISC_REQ_STREAM;                   // Support a stream-oriented connection.

        SecBuffer inbuffers[1] = { 0 };
        inbuffers[0].BufferType = SECBUFFER_EMPTY;
        inbuffers[0].pvBuffer = NULL;
        inbuffers[0].cbBuffer = 0;

        SecBuffer outbuffers[1] = { 0 };
        outbuffers[0].BufferType = SECBUFFER_TOKEN;
        outbuffers[0].cbBuffer = 0;
        outbuffers[0].pvBuffer = NULL;

        SecBufferDesc indesc =
        {
            .ulVersion = SECBUFFER_VERSION,
            .cBuffers  = ARRAYSIZE(inbuffers),
            .pBuffers  = inbuffers
        };
        SecBufferDesc outdesc =
        {
            .ulVersion = SECBUFFER_VERSION,
            .cBuffers  = ARRAYSIZE(outbuffers),
            .pBuffers  = outbuffers
        };

        unsigned long contextAttr = 0;  // NOLINT(*)

        secStatus = XpfSecApplyControlToken(SocketApiProvider,
                                            &TlsContext->ContextHandle,
                                            &indesc);
        if (secStatus != SEC_E_OK)
        {
            break;
        }

        secStatus = XpfSecInitializeSecurityContextW(SocketApiProvider,
                                                     &TlsContext->CredentialsHandle,
                                                     &TlsContext->ContextHandle,
                                                     NULL,
                                                     contextRequest,
                                                     0,
                                                     SECURITY_NETWORK_DREP,
                                                     &indesc,
                                                     0,
                                                     &TlsContext->ContextHandle,
                                                     &outdesc,
                                                     &contextAttr,
                                                     nullptr);
        if (secStatus == SEC_I_CONTINUE_NEEDED)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            if (outbuffers[0].BufferType != SECBUFFER_MISSING &&
                outbuffers[0].pvBuffer != NULL &&
                outbuffers[0].cbBuffer != 0)
            {
                status = WskSend(SocketApiProvider,
                                 Socket,
                                 outbuffers[0].cbBuffer,
                                 static_cast<uint8_t*>(outbuffers[0].pvBuffer));
            }

            XpfSecFreeContextBuffer(SocketApiProvider,
                                    outbuffers[0].pvBuffer);
            outbuffers[0].cbBuffer = 0;
            outbuffers[0].BufferType = SECBUFFER_MISSING;

            if (!NT_SUCCESS(status))
            {
                break;
            }
        }
        else
        {
            XpfSecFreeContextBuffer(SocketApiProvider,
                                    outbuffers[0].pvBuffer);
            outbuffers[0].cbBuffer = 0;
            outbuffers[0].BufferType = SECBUFFER_MISSING;

            break;
        }
    }

    secStatus = XpfSecDeleteSecurityContext(SocketApiProvider,
                                            &TlsContext->ContextHandle);
    XPF_DEATH_ON_FAILURE(SEC_E_OK == secStatus);

CleanUp:
    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }
    SecInvalidateHandle(&TlsContext->ContextHandle);
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskTlsSend(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* Socket,
    _Inout_ WskSocketTlsContext* TlsContext,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    bool isAttached = false;
    KAPC_STATE state = { 0 };
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    while (NumberOfBytes > 0)
    {
        //
        // We negotiatated how many bytes we need to send - won't exceed that.
        //
        XPF_ASSERT(TlsContext->TlsBuffer.GetSize() >= (static_cast<size_t>(TlsContext->StreamSizes.cbHeader) +
                                                       static_cast<size_t>(TlsContext->StreamSizes.cbMaximumMessage) +
                                                       static_cast<size_t>(TlsContext->StreamSizes.cbTrailer)));
        uint32_t toSend = min(static_cast<uint32_t>(NumberOfBytes),
                              TlsContext->StreamSizes.cbMaximumMessage);

        SecBuffer buffers[3] = { 0 };

        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].pvBuffer = TlsContext->TlsBuffer.GetBuffer();
        buffers[0].cbBuffer = TlsContext->StreamSizes.cbHeader;

        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(),
                                                    static_cast<size_t>(buffers[0].cbBuffer));
        buffers[1].cbBuffer = toSend;

        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].pvBuffer = xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(),
                                                    static_cast<size_t>(buffers[0].cbBuffer) +
                                                    static_cast<size_t>(buffers[1].cbBuffer));
        buffers[2].cbBuffer = TlsContext->StreamSizes.cbTrailer;

        xpf::ApiCopyMemory(buffers[1].pvBuffer,
                           Bytes,
                           toSend);
        SecBufferDesc desc =
        {
            .ulVersion = SECBUFFER_VERSION,
            .cBuffers  = ARRAYSIZE(buffers),
            .pBuffers  = buffers
        };

        //
        // We don't expect the encryption to fail, but check, just in case.
        //
        SECURITY_STATUS secStatus = XpfSecEncryptMessage(SocketApiProvider,
                                                         &TlsContext->ContextHandle,
                                                         0,
                                                         &desc,
                                                         0);
        if (SEC_E_OK != secStatus)
        {
            XPF_ASSERT(false);
            status = STATUS_INVALID_DEVICE_STATE;
            goto CleanUp;
        }

        //
        // Send the encrypted bytes. We need to account for header and trailer.
        //
        status = xpf::WskSend(SocketApiProvider,
                              Socket,
                              static_cast<size_t>(buffers[0].cbBuffer) +
                              static_cast<size_t>(buffers[1].cbBuffer) +
                              static_cast<size_t>(buffers[2].cbBuffer),
                              static_cast<uint8_t*>(buffers[0].pvBuffer));
        if (!NT_SUCCESS(status))
        {
            goto CleanUp;
        }

        NumberOfBytes -= toSend;
        Bytes += toSend;
    }

CleanUp:
    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::WskTlsReceive(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* Socket,
    _Inout_ WskSocketTlsContext* TlsContext,
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true)
{
    //
    // Should always be at passive.
    //
    XPF_MAX_PASSIVE_LEVEL();

    bool isAttached = false;
    KAPC_STATE state = { 0 };
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    size_t receivedBytes = 0;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        ::KeStackAttachProcess(PsInitialSystemProcess, &state);
        isAttached = true;
    }

    //
    // The transitions might seem a bit sus at beginning, but the logic is as follows:
    //      -> We are calling WskReceive; this will get us encrypted data.
    //      -> We need to decrypt the data
    //              a) The buffer was large enough to store all data, and to properly decrypt it.
    //              b) The buffer was not large enough so we need to read more data.
    //      -> At this point we have available DECRYPTED data
    //              a) The caller passed a large enough buffer to store the full data
    //              b) The called did not pass a large enough buffer to store the data.
    //      -> We copy as many bytes as we can in the provided caller buffer.
    //              * If the buffer is too small, we'll store the other decrypted bytes inside
    //                the TLS context, so when the caller will invoke this function again,
    //                we'll service with them.
    //
    // The code is structured in a bit of reverse order, as we need to handle the case where
    // the caller is calling TlsReceive() and we already have decrypted data available for use.
    //

    while (*NumberOfBytes > 0)
    {
        if (TlsContext->DecryptedData != nullptr)
        {
            //
            // First we check if this is a WskTlsReceive and we already have decrypted data
            // available for the caller, so we handle this scenario.
            //

            const size_t toCopyToCaller = min(TlsContext->AvailableDecryptedData,
                                              *NumberOfBytes);

            xpf::ApiCopyMemory(xpf::AlgoAddToPointer(Bytes, receivedBytes),
                               TlsContext->DecryptedData,
                               toCopyToCaller);

            receivedBytes  += toCopyToCaller;
            *NumberOfBytes -= toCopyToCaller;

            TlsContext->AvailableDecryptedData -= static_cast<uint32_t>(toCopyToCaller);
            TlsContext->DecryptedData = xpf::AlgoAddToPointer(TlsContext->DecryptedData, toCopyToCaller);

            if (TlsContext->AvailableDecryptedData == 0)
            {
                TlsContext->DecryptedData = nullptr;

                //
                // We finished consuming the received decrypted buffer.
                //  Basycally our buffer looks like: [ddddddddddd][eeeeeeeeeeeeeeeeeee]
                //                                   | decrypted |
                //                                   |            total               |
                // So we need to skip over the decrypted part:
                //  [eeeeeeeeeeeeeeeeeee]
                //  | total - Decrypted |
                //
                XPF_ASSERT(TlsContext->ReceivedDecryptedData <= TlsContext->TlsBuffer.GetSize());
                XPF_ASSERT(TlsContext->ReceivedDecryptedData <= TlsContext->ReceivedTotalData);

                xpf::ApiCopyMemory(TlsContext->TlsBuffer.GetBuffer(),
                                   xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(), TlsContext->ReceivedDecryptedData),
                                   static_cast<size_t>(TlsContext->ReceivedTotalData) - TlsContext->ReceivedDecryptedData);

                TlsContext->ReceivedTotalData -= TlsContext->ReceivedDecryptedData;
                TlsContext->ReceivedDecryptedData = 0;
            }

            continue;
        }
        else if (TlsContext->ReceivedTotalData > 0)
        {
            //
            // We don't have any decrypted data available.
            // But we might have available encrypted data.
            // So we need to decrypt it.
            //

            SecBuffer buffers[4] = { 0 };

            buffers[0].BufferType = SECBUFFER_DATA;
            buffers[0].pvBuffer = TlsContext->TlsBuffer.GetBuffer();
            buffers[0].cbBuffer = TlsContext->ReceivedTotalData;

            buffers[1].BufferType = SECBUFFER_EMPTY;
            buffers[1].pvBuffer = NULL;
            buffers[1].cbBuffer = 0;

            buffers[2].BufferType = SECBUFFER_EMPTY;
            buffers[2].pvBuffer = NULL;
            buffers[2].cbBuffer = 0;

            buffers[3].BufferType = SECBUFFER_EMPTY;
            buffers[3].pvBuffer = NULL;
            buffers[3].cbBuffer = 0;

            SecBufferDesc desc =
            {
                .ulVersion = SECBUFFER_VERSION,
                .cBuffers  = ARRAYSIZE(buffers),
                .pBuffers  = buffers
            };

            SECURITY_STATUS secStatus = XpfSecDecryptMessage(SocketApiProvider,
                                                             &TlsContext->ContextHandle,
                                                             &desc,
                                                             0,
                                                             NULL);
            if (SEC_E_OK == secStatus)
            {
                //
                // This means the decryption was successful. So the buffers will be:
                // [header][data][trailer][extra]
                //
                // Extra means that the there have been [x] extra bytes that could not
                // be decrypted. So they are still encrypted, but RECEIVED.
                //
                if (buffers[0].BufferType != SECBUFFER_STREAM_HEADER ||
                    buffers[1].BufferType != SECBUFFER_DATA ||
                    buffers[2].BufferType != SECBUFFER_STREAM_TRAILER)
                {
                    status = STATUS_UNEXPECTED_NETWORK_ERROR;
                    goto CleanUp;
                }

                //
                // The decryption happens in-place so this should be the tlsbuffer.
                //
                XPF_ASSERT(TlsContext->TlsBuffer.GetBuffer() <= buffers[1].pvBuffer);
                XPF_ASSERT(buffers[1].pvBuffer <= xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(),
                                                                        TlsContext->TlsBuffer.GetSize()));

                TlsContext->DecryptedData = buffers[1].pvBuffer;
                TlsContext->AvailableDecryptedData = buffers[1].cbBuffer;

                TlsContext->ReceivedDecryptedData = TlsContext->ReceivedTotalData;
                if (buffers[3].BufferType == SECBUFFER_EXTRA)
                {
                    //
                    // We didn't consumed the full received buffer.
                    // EXTRA means that there are x bytes that have not
                    // been consumed so we used fewer bytes.
                    //
                    const bool isOk = xpf::ApiNumbersSafeSub(TlsContext->ReceivedDecryptedData,
                                                             static_cast<uint32_t>(buffers[3].cbBuffer),
                                                             &TlsContext->ReceivedDecryptedData);
                    if (!isOk)
                    {
                        status = STATUS_INTEGER_OVERFLOW;
                        goto CleanUp;
                    }
                }

                //
                // We have a bunch of decrypted data, we can pass it to caller.
                //
                continue;
            }
            else if (SEC_E_INCOMPLETE_MESSAGE != secStatus)
            {
                //
                // SEC_E_INCOMPLETE_MESSAGE means we need to actually read more data in order to be able
                // to decrypt our data so we'll fallthrough and go to WskReceive below.
                // In all other cases, we return a network error.
                //
                status = STATUS_UNEXPECTED_NETWORK_ERROR;
                goto CleanUp;
            }
        }

        //
        // TlsContext->ReceivedTotalData might not be 0, so we might have some leftover bytes
        // that could not be decrypted. We don't want to overwrite those.
        //
        XPF_ASSERT(TlsContext->ReceivedTotalData < TlsContext->TlsBuffer.GetSize());

        size_t recv = TlsContext->TlsBuffer.GetSize() - TlsContext->ReceivedTotalData;
        status = WskReceive(SocketApiProvider,
                            Socket,
                            &recv,
                            static_cast<uint8_t*>(xpf::AlgoAddToPointer(TlsContext->TlsBuffer.GetBuffer(),
                                                                        TlsContext->ReceivedTotalData)));
        if (!NT_SUCCESS(status))
        {
            goto CleanUp;
        }

        TlsContext->ReceivedTotalData += static_cast<uint32_t>(recv);
        if (TlsContext->ReceivedTotalData == 0)
        {
            break;
        }
    }

    //
    // Fill the correct received size.
    // At this point we are successful.
    //
    *NumberOfBytes = receivedBytes;
    status = STATUS_SUCCESS;


CleanUp:
    if (isAttached)
    {
        ::KeUnstackDetachProcess(&state);
        isAttached = false;
    }

    if (!NT_SUCCESS(status))
    {
        TlsContext->DecryptedData = NULL;
        TlsContext->AvailableDecryptedData = 0;
        TlsContext->ReceivedDecryptedData = 0;
        TlsContext->ReceivedTotalData = 0;
    }

    return status;
}

#endif  // XPF_PLATFORM_WIN_KM
