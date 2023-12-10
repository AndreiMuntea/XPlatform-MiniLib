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
        if (nullptr != prevOutput)
        {
            prevOutput->ai_next = crtOutput;
            prevOutput = crtOutput;
        }
        else
        {
            /* First structure. We don't want to lose it. */
            *Output = crtOutput;
        }
    }

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
    Context->WasIrpUsed = FALSE;

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
                             XpfWskCompletionRoutine,
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
    Context->WasIrpUsed = FALSE;
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

#endif  // XPF_PLATFORM_WIN_KM
