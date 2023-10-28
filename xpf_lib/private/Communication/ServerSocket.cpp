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
        WSAData WsaLibData = { 0 };
        SOCKET ListenSocket = INVALID_SOCKET;
    #else
        #error Unknown Platform
    #endif
};  // struct ServerSocketDataForServer


struct ServerSocketClientData : public xpf::IClientCookie
{
    uuid_t UniqueId = { 0 };
    xpf::RundownProtection ClientRundown;

    #if defined XPF_PLATFORM_WIN_UM
        SOCKET ClientSocket = INVALID_SOCKET;
    #else
        #error Unknown Platform
    #endif
};

};  // namespace xpf

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::Start(
    void
) noexcept(true)
{
    /* If the server was not properly initialized, we can't do much. */
    if ((!this->m_ServerLock.HasValue()) || (nullptr == this->m_ServerSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* The server is already started.*/
    xpf::ExclusiveLockGuard serverGuard{ *this->m_ServerLock };
    if (this->m_IsStarted)
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* All good. Accept new clients.*/
    this->m_IsStarted = true;
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::ServerSocket::Stop(
    void
) noexcept(true)
{
    /* If the server was not properly initialized, we can't do much. */
    if ((!this->m_ServerLock.HasValue()) || (nullptr == this->m_ServerSocketData))
    {
        return;
    }

    /* Stop the server. */
    xpf::ExclusiveLockGuard serverGuard{ *this->m_ServerLock };
    this->m_IsStarted = false;

    /* Close all connections. */
    for (size_t i = 0; i < this->m_Clients.Size(); ++i)
    {
        this->CloseClientConnection(this->m_Clients[i]);
    }

    /* Empty the connections list. */
    this->m_Clients.Clear();
}

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
    #else
        #error Unknown Platform
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

        /* Put the socket in a listening state. */
        gleResult = ::listen(data->ListenSocket, SOMAXCONN);
        if (0 != gleResult)
        {
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }
    #else
        #error Unknown Platform
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
    #else
        #error Unknown Platform
    #endif

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(data);
    xpf::MemoryAllocator::FreeMemory(ServerSocketData);
    *ServerSocketData = nullptr;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::EstablishClientConnection(
    _Inout_ xpf::SharedPointer<xpf::IClientCookie>& ClientConnection
) noexcept(true)
{
    /* We need the socket to listen to. */
    xpf::ServerSocketData* serverSocketData = reinterpret_cast<xpf::ServerSocketData*>(this->m_ServerSocketData);

    /* First we get to the underlying data. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientCookie.IsEmpty() || (nullptr == serverSocketData))
    {
        return STATUS_INVALID_CONNECTION;
    }
    ServerSocketClientData& newClient = (*clientCookie);

    /* Now wait for a new client. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
       newClient.ClientSocket = ::accept(serverSocketData->ListenSocket, nullptr, nullptr);
       if (INVALID_SOCKET == newClient.ClientSocket)
       {
           return STATUS_CONNECTION_REFUSED;
       }
    #else
        #error Unknown Platform
    #endif

    /* Assign an unique uuid to this client. */
    xpf::ApiRandomUuid(&newClient.UniqueId);

    /* Everything went good. We got a new client. */
    return STATUS_SUCCESS;
}

void
XPF_API
xpf::ServerSocket::CloseClientConnection(
    _Inout_ xpf::SharedPointer<xpf::IClientCookie>& ClientConnection
) noexcept(true)
{
    /* First we get to the underlying data. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientCookie.IsEmpty())
    {
        return;
    }
    ServerSocketClientData& clientData = (*clientCookie);

    /* Now close the socket. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        if (INVALID_SOCKET != clientData.ClientSocket)
        {
            /* The shutdown can fail if the client already closed the socket. Just move on. */
            (void) ::shutdown(clientData.ClientSocket, SD_BOTH);

            /* Be a good citizen and clean the resources. */
            int gleResult = ::closesocket(clientData.ClientSocket);
            XPF_DEATH_ON_FAILURE(0 != gleResult);

            clientData.ClientSocket = INVALID_SOCKET;
        }
    #else
        #error Unknown Platform
    #endif

    /*
     * Wait for all send and recv operations to finish.
     * Because we closed the socket, they can fail with errors.
     * We'll expect them in the send/recv handling.
     * And block further operations on this client.
     */
    clientData.ClientRundown.WaitForRelease();
}

xpf::SharedPointer<xpf::IClientCookie>
XPF_API
xpf::ServerSocket::FindClientConnection(
    _In_ _Const_ const xpf::SharedPointer<xpf::IClientCookie>& ClientCookie
) noexcept(true)
{
    /* If the server was not properly initialized, we can't do much. */
    if ((!this->m_ServerLock.HasValue()) || (nullptr == this->m_ServerSocketData))
    {
        return xpf::SharedPointer<xpf::IClientCookie>();
    }

    xpf::SharedPointer<xpf::ServerSocketClientData> clientConnection;
    {
        /* If the server did not start. Can't send data. */
        xpf::SharedLockGuard serverGuard{ *this->m_ServerLock };
        if (!this->m_IsStarted)
        {
            return xpf::SharedPointer<xpf::IClientCookie>();
        }

        /* First we get to the underlying data. */
        auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientCookie);
        if (clientCookie.IsEmpty())
        {
            return xpf::SharedPointer<xpf::IClientCookie>();
        }

        /* Now we search for this client. */
        for (size_t i = 0; i < this->m_Clients.Size(); ++i)
        {
            /* Check if this client has the same UUID as the one we are searching. */
            const auto& client = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(this->m_Clients[i]);
            if (client.IsEmpty() || !xpf::ApiAreUuidsEqual((*clientCookie).UniqueId, (*client).UniqueId))
            {
                continue;
            }

            /* Found the client connection. */
            clientConnection = client;
            break;
        }
    }

    /* Return whatever we found to the caller. */
    return xpf::DynamicSharedPointerCast<xpf::IClientCookie>(clientConnection);
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::SendDataToClientConnection(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientConnection
) noexcept(true)
{
    /* Validate the parameters. Enforce 64 kb limit. */
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Then we get to the underlying data. */
    auto clientConnection = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientConnection.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If the connection is tearing down, we bail. */
    xpf::RundownGuard connectionGuard((*clientConnection).ClientRundown);
    if (!connectionGuard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    /* And now do the actual send. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::send((*clientConnection).ClientSocket,
                                reinterpret_cast<const char*>(Bytes),
                                static_cast<int>(NumberOfBytes),
                                0);
        switch (gleResult)
        {
            case ERROR_SUCCESS:
            {
                return STATUS_SUCCESS;
            }
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
xpf::ServerSocket::ReceiveDataFromClientConnection(
    _In_ size_t NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientConnection
) noexcept(true)
{
    /* Validate the parameters. Enforce 64 kb limit. */
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Then we get to the underlying data. */
    auto clientConnection = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientConnection.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If the connection is tearing down, we bail. */
    xpf::RundownGuard connectionGuard((*clientConnection).ClientRundown);
    if (!connectionGuard.IsRundownAcquired())
    {
        return STATUS_TOO_LATE;
    }

    /* And now do the actual send. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = ::recv((*clientConnection).ClientSocket,
                                reinterpret_cast<char*>(Bytes),
                                static_cast<int>(NumberOfBytes),
                                0);
        switch (gleResult)
        {
            case ERROR_SUCCESS:
            {
                return STATUS_SUCCESS;
            }
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

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::AcceptClient(
    _Out_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    /* If the server was not properly initialized, we can't do much. */
    if ((!this->m_ServerLock.HasValue()) || (nullptr == this->m_ServerSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* The server did not start. Can't accept new clients.*/
    xpf::ExclusiveLockGuard serverGuard{ *this->m_ServerLock };
    if (!this->m_IsStarted)
    {
        return STATUS_CONNECTION_REFUSED;
    }

    /* Allocate memory for ServerSocketClientData. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::IClientCookie>(xpf::MakeShared<xpf::ServerSocketClientData>());
    if (clientCookie.IsEmpty())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Now wait for a new client. */
    NTSTATUS status = this->EstablishClientConnection(clientCookie);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* And finally insert the client to the clients list.*/
    status = this->m_Clients.Emplace(clientCookie);
    if (!NT_SUCCESS(status))
    {
        this->CloseClientConnection(clientCookie);
        return status;
    }

    /* All good. */
    ClientCookie = clientCookie;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::DisconnectClient(
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    /* If the server was not properly initialized, we can't do much. */
    if ((!this->m_ServerLock.HasValue()) || (nullptr == this->m_ServerSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* The server did not start. Can't accept new clients.*/
    xpf::ExclusiveLockGuard serverGuard{ *this->m_ServerLock };
    if (!this->m_IsStarted)
    {
        return STATUS_NOT_SUPPORTED;
    }

    /* First we get to the underlying data. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientCookie);
    if (clientCookie.IsEmpty())
    {
        return STATUS_NOT_SUPPORTED;
    }

    /* Now we search for this client. */
    for (size_t i = 0; i < this->m_Clients.Size(); ++i)
    {
        /* Check if this client has the same UUID as the one we are searching. */
        const auto& client = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(this->m_Clients[i]);
        if (client.IsEmpty() || !xpf::ApiAreUuidsEqual((*clientCookie).UniqueId, (*client).UniqueId))
        {
            continue;
        }

        /* Found the client - close the connection. */
        this->CloseClientConnection(ClientCookie);

        /* And erase it from the clients list. */
        return this->m_Clients.Erase(i);
    }

    /* If we are here, the client is not found. */
    return STATUS_NOT_FOUND;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::SharedPointer<IClientCookie> clientConnection = this->FindClientConnection(ClientCookie);

    for (size_t retries = 0; retries < 5; ++retries)
    {
        /* Now send the data to this client connection. */
        status = this->SendDataToClientConnection(NumberOfBytes,
                                                  Bytes,
                                                  clientConnection);
        /* If the network was busy, we will retry. */
        if (STATUS_NETWORK_BUSY != status)
        {
            break;
        }
        xpf::ApiSleep(20);
    }

    /* If the connection was aborted, we disconnect on our end as well. */
    if (STATUS_CONNECTION_ABORTED == status)
    {
        (void) this->DisconnectClient(ClientCookie);
    }
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::ReceiveData(
    _In_ size_t NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::SharedPointer<IClientCookie> clientConnection = this->FindClientConnection(ClientCookie);

    for (size_t retries = 0; retries < 5; ++retries)
    {
        /* Now send the data to this client connection. */
        status = this->ReceiveDataFromClientConnection(NumberOfBytes,
                                                       Bytes,
                                                       clientConnection);
        /* If the network was busy, we will retry. */
        if (STATUS_NETWORK_BUSY != status)
        {
            break;
        }
        xpf::ApiSleep(20);
    }

    /* If the connection was aborted, we disconnect on our end as well. */
    if (STATUS_CONNECTION_ABORTED == status)
    {
        (void) this->DisconnectClient(ClientCookie);
    }
    return status;
}
