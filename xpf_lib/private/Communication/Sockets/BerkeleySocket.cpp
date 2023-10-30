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
    #else
        #error Unknown Platform
    #endif
};  // struct SocketApiProviderInternal

struct SocketInternal
{
    #if defined XPF_PLATFORM_WIN_UM
        SOCKET Socket = INVALID_SOCKET;
    #else
        #error Unknown Platform
    #endif
};  // struct SocketInternal

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
    _Out_ struct addrinfo** AddrInfo
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

    //
    // Platform specific query.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int result = ::getaddrinfo(NodeName.Buffer(), ServiceName.Buffer(), nullptr, AddrInfo);
        if (0 != result)
        {
            *AddrInfo = nullptr;

            status = STATUS_CONNECTION_INVALID;
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
    _Inout_ struct addrinfo** AddrInfo
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

    //
    // Platform specific cleanup.
    //
    #if defined XPF_PLATFORM_WIN_UM
        ::freeaddrinfo(*AddrInfo);
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
        newSocket->Socket = INVALID_SOCKET;
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
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::bind(socket->Socket, LocalAddress, Length);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
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
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::connect(socket->Socket, Address, Length);
        if (0 != gleResult)
        {
            return STATUS_INVALID_CONNECTION;
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
            return (static_cast<int>(NumberOfBytes) != bytesSent) ? STATUS_PARTIAL_COPY
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
    #else
        #error Unknown Platform
    #endif
}
