/**
 * @file        xpf_lib/private/Communication/ServerSocket.cpp
 *
 * @brief       Server-Side implementation using sockets.
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

struct ServerSocketData
{
    #if defined XPF_PLATFORM_WIN_UM
        WSAData WsaLibData;
        SOCKET ListenSocket;
    #endif
};  // struct ServerSocketDataForServer
};  // namespace xpf


void*
XPF_API
xpf::ServerSocket::CreateServerSocketData(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::ServerSocketData* data = nullptr;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // Sanity checks of parameters.
    //
    if (Ip.IsEmpty() || Port.IsEmpty())
    {
        return nullptr;
    }

    //
    // First we allocate and construct the ServerSocketDataForServer.
    //
    data = reinterpret_cast<xpf::ServerSocketData*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::ServerSocketData)));
    if (nullptr == data)
    {
        return nullptr;
    }
    xpf::MemoryAllocator::Construct(data);

    //
    // Now we preinitialize the platform specific data.
    //
    #if defined XPF_PLATFORM_WIN_UM
        xpf::ApiZeroMemory(&data->WsaLibData,
                           sizeof(data->WsaLibData));
        data->ListenSocket = INVALID_SOCKET;
    #endif

    //
    // And finally initialize the socket.
    //
    #if defined XPF_PLATFORM_WIN_UM
        struct addrinfo* result = nullptr;
        int gleResult = 0;

        struct addrinfo hints;
        xpf::ApiZeroMemory(&hints, sizeof(hints));

        /* Default to TCP and IPv4 always. */
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        /* Initialize the WinSock library. */
        gleResult = ::WSAStartup(MAKEWORD(2, 2), &data->WsaLibData);
        if (0 != gleResult)
        {
            xpf::ApiZeroMemory(&data->WsaLibData,
                               sizeof(data->WsaLibData));
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }

        /* Resolve the server address and port. */
        gleResult = ::getaddrinfo(Ip.Buffer(), Port.Buffer(), &hints, &result);
        if (0 != gleResult)
        {
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }

        /* Create a SOCKET for the server to listen for client connections. */
        data->ListenSocket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (INVALID_SOCKET == data->ListenSocket)
        {
            ::freeaddrinfo(result);
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }

        /* Setup the TCP listening socket. */
        gleResult = ::bind(data->ListenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen));
        ::freeaddrinfo(result);
        if (0 != gleResult)
        {
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }
    #endif

CleanUp:
    if (!NT_SUCCESS(status))
    {
        this->DestroyServerSocketData(reinterpret_cast<void**>(&data));
        data = nullptr;
    }
    return data;
}

void
XPF_API
xpf::ServerSocket::DestroyServerSocketData(
    _Inout_ void** ServerSocketData
) noexcept(true)
{
    //
    // Can't free null pointer.
    //
    if ((nullptr == ServerSocketData) || (nullptr == (*ServerSocketData)))
    {
        return;
    }
    xpf::ServerSocketData* data = reinterpret_cast<xpf::ServerSocketData*>(ServerSocketData);

    //
    // First clean platform specific data.
    //
    #if defined XPF_PLATFORM_WIN_UM
        if (INVALID_SOCKET != data->ListenSocket)
        {
            int closeResult = ::closesocket(data->ListenSocket);
            XPF_DEATH_ON_FAILURE(0 == closeResult);

            data->ListenSocket = INVALID_SOCKET;
        }
        if (0 != data->WsaLibData.wVersion)
        {
            int cleanUpResult = ::WSACleanup();
            XPF_DEATH_ON_FAILURE(0 == cleanUpResult);

            xpf::ApiZeroMemory(&data->WsaLibData,
                               sizeof(data->WsaLibData));
        }
    #endif

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(data);
    xpf::MemoryAllocator::FreeMemory(ServerSocketData);
    *ServerSocketData = nullptr;
}