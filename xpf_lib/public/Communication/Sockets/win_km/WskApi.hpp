/**
 * @file        xpf_lib/public/Communication/Sockets/win_km/WskApi.hpp
 *
 * @brief       This contains the windows - kernel mode header to abstract away
 *              the WSK API interactions. It is intended to be used only in cpp files.
 *              So this header is in private folder, unaccessible for others
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */

#pragma once

#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/Containers/Vector.hpp"
#include "xpf_lib/public/Containers/String.hpp"

//
// The code is available only for windows kernel mode.
// It is guarded by these.
//
#if defined XPF_PLATFORM_WIN_KM
namespace xpf
{
struct WskCompletionContext
{
    KEVENT CompletionEvent = { 0 };
    PIRP Irp = nullptr;
    BOOLEAN WasIrpUsed = FALSE;
};

struct WskSocketProvider
{
    WSK_REGISTRATION WskRegistration = { 0 };
    BOOLEAN IsProviderRegistered = FALSE;

    WSK_PROVIDER_NPI WskProviderNpi = { 0 };
    BOOLEAN IsNPIProviderCaptured = FALSE;

    WSK_CLIENT_NPI WskClientNpi = { 0 };
    WSK_CLIENT_DISPATCH WskClientDispatch = { 0 };
};  // struct WskSocketProvider

struct WskSocket
{
    PWSK_SOCKET Socket = nullptr;
    BOOLEAN IsListeningSocket = FALSE;

    union
    {
        const WSK_PROVIDER_CONNECTION_DISPATCH* ConnectionDispatch;
        const WSK_PROVIDER_LISTEN_DISPATCH* ListenDispatch;
        const VOID* Dispatch;
    } DispatchTable;
};  // struct WskSocket

struct WskBuffer
{
    WSK_BUF WskBuf = { 0 };
    BOOLEAN ArePagesResident = FALSE;
    xpf::Buffer<xpf::MemoryAllocator> RawBuffer;
};  // struct WskBuffer

_Must_inspect_result_
NTSTATUS
XPF_API
WskInitializeProvider(
    _Inout_ WskSocketProvider* Provider
) noexcept(true);

void
XPF_API
WskDeinitializeProvider(
    _Inout_ WskSocketProvider* Provider
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskInitializeCompletionContext(
    _Inout_ WskCompletionContext* Context
) noexcept(true);

void
XPF_API
WskDeinitializeCompletionContext(
    _Inout_ WskCompletionContext* Context
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskInitializeWskBuffer(
    _Inout_ WskBuffer* Buffer,
    _In_ LOCK_OPERATION Operation,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true);

void
XPF_API
WskDeinitializeWskBuffer(
    _Inout_ WskBuffer* Buffer
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskGetAddrInfo(
    _In_ WskSocketProvider* SocketApiProvider,
    _In_ _Const_ const xpf::StringView<char>& NodeName,
    _In_ _Const_ const xpf::StringView<char>& ServiceName,
    _Out_ addrinfo** AddrInfo
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskFreeAddrInfo(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ addrinfo** AddrInfo
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskCreateSocket(
    _In_ WskSocketProvider* SocketApiProvider,
    _In_ int AddressFamily,
    _In_ int Type,
    _In_ int Protocol,
    _In_ bool IsListeningSocket,
    _Out_ WskSocket* CreatedSocket
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskShutdownSocket(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskBind(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ _Const_ const sockaddr* LocalAddress,
    _In_ int Length
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskListen(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskConnect(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ _Const_ const sockaddr* Address,
    _In_ int Length
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskAccept(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _Out_ WskSocket* NewSocket
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskSend(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true);

_Must_inspect_result_
NTSTATUS
XPF_API
WskReceive(
    _In_ WskSocketProvider* SocketApiProvider,
    _Inout_ WskSocket* TargetSocket,
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true);
};  // namespace xpf
#endif  // XPF_PLATFORM_WIN_KM
