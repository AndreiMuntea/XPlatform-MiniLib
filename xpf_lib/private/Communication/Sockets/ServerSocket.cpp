/**
 * @file        xpf_lib/private/Communication/Sockets/ServerSocket.cpp
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
    xpf::BerkeleySocket::AddressInfo* AddressInfo = nullptr;
    xpf::BerkeleySocket::Socket ServerSocket = nullptr;
    xpf::BerkeleySocket::SocketApiProvider ApiProvider = nullptr;
};  // struct ServerSocketData

struct ServerSocketClientData : public xpf::IClientCookie
{
 public:
XPF_CLASS_COPY_MOVE_BEHAVIOR(ServerSocketClientData, delete);

ServerSocketClientData(void) noexcept(true) = default;
virtual ~ServerSocketClientData(void) noexcept(true) = default;

 public:
    uuid_t UniqueId = { 0 };
    xpf::RundownProtection ClientRundown;
    xpf::BerkeleySocket::Socket ClientSocket = nullptr;
};  // struct ServerSocketClientData
};  // namespace xpf

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ServerSocket::Start(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

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
    XPF_MAX_PASSIVE_LEVEL();

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
    // First we allocate and construct the ServerSocketData.
    //
    data = reinterpret_cast<xpf::ServerSocketData*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::ServerSocketData)));
    if (nullptr == data)
    {
        return nullptr;
    }
    xpf::MemoryAllocator::Construct(data);

    status = xpf::BerkeleySocket::InitializeSocketApiProvider(&data->ApiProvider);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    /* Resolve the server address and port. */
    status = xpf::BerkeleySocket::GetAddressInformation(data->ApiProvider,
                                                        Ip,
                                                        Port,
                                                        &data->AddressInfo);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    for (xpf::BerkeleySocket::AddressInfo* crt = data->AddressInfo; nullptr != crt; crt = crt->ai_next)
    {
        /* Create a SOCKET for the server to listen for client connections. */
        status = xpf::BerkeleySocket::CreateSocket(data->ApiProvider,
                                                   crt->ai_family,
                                                   crt->ai_socktype,
                                                   crt->ai_protocol,
                                                   &data->ServerSocket);
        if (!NT_SUCCESS(status))
        {
            continue;
        }

        /* Setup the TCP listening socket. */
        status = xpf::BerkeleySocket::Bind(data->ApiProvider,
                                           data->ServerSocket,
                                           crt->ai_addr,
                                           static_cast<int>(crt->ai_addrlen));
        if (!NT_SUCCESS(status))
        {
            NTSTATUS shutdownStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                                          &data->ServerSocket);
            XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));
            continue;
        }

        /* We're binded. */
        break;
    }

    if (nullptr == data->ServerSocket)
    {
        status = STATUS_CONNECTION_REFUSED;
        goto CleanUp;
    }

    /* We no longer need the address info. */
    if (nullptr != data->AddressInfo)
    {
        status = xpf::BerkeleySocket::FreeAddressInformation(data->ApiProvider,
                                                             &data->AddressInfo);
        data->AddressInfo = nullptr;
        if (!NT_SUCCESS(status))
        {
            goto CleanUp;
        }
    }

    /* Put the socket in a listening state. */
    status = xpf::BerkeleySocket::Listen(data->ApiProvider,
                                         data->ServerSocket);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

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
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Can't free null pointer.
    //
    if ((nullptr == ServerSocketData) || (nullptr == (*ServerSocketData)))
    {
        return;
    }
    xpf::ServerSocketData* data = reinterpret_cast<xpf::ServerSocketData*>(*ServerSocketData);

    if (nullptr != data->ServerSocket)
    {
        NTSTATUS shutdownStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                                      &data->ServerSocket);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));

        data->ServerSocket = nullptr;
    }
    if (nullptr != data->AddressInfo)
    {
        NTSTATUS freeStatus = xpf::BerkeleySocket::FreeAddressInformation(data->ApiProvider,
                                                                          &data->AddressInfo);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(freeStatus));

        data->AddressInfo = nullptr;
    }
    if (nullptr != data->ApiProvider)
    {
        xpf::BerkeleySocket::DeInitializeSocketApiProvider(&data->ApiProvider);
        data->ApiProvider = nullptr;
    }

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
    XPF_MAX_PASSIVE_LEVEL();

    /* We need the socket to listen to. */
    xpf::ServerSocketData* serverSocketData = reinterpret_cast<xpf::ServerSocketData*>(this->m_ServerSocketData);

    /* First we get to the underlying data. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientCookie.IsEmpty() || (nullptr == serverSocketData))
    {
        return STATUS_INVALID_CONNECTION;
    }
    ServerSocketClientData& newClient = (*clientCookie);

    /* Assign an unique uuid to this client. */
    xpf::ApiRandomUuid(&newClient.UniqueId);

    return xpf::BerkeleySocket::Accept(serverSocketData->ApiProvider,
                                       serverSocketData->ServerSocket,
                                       &newClient.ClientSocket);
}

void
XPF_API
xpf::ServerSocket::CloseClientConnection(
    _Inout_ xpf::SharedPointer<xpf::IClientCookie>& ClientConnection
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* First we get to the underlying data. */
    auto clientCookie = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(ClientConnection);
    if (clientCookie.IsEmpty())
    {
        return;
    }
    ServerSocketClientData& clientData = (*clientCookie);

    xpf::ServerSocketData* serverSocketData = reinterpret_cast<xpf::ServerSocketData*>(this->m_ServerSocketData);

    if (nullptr != clientData.ClientSocket)
    {
        (void) xpf::BerkeleySocket::ShutdownSocket(serverSocketData->ApiProvider,
                                                   &clientData.ClientSocket);
    }

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
    XPF_MAX_PASSIVE_LEVEL();

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
xpf::ServerSocket::AcceptClient(
    _Out_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

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
    XPF_MAX_PASSIVE_LEVEL();

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
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::SharedPointer<IClientCookie> clientConnection = this->FindClientConnection(ClientCookie);

    for (size_t retries = 0; retries < 5; ++retries)
    {
        /* Then we get to the underlying data. */
        auto connection = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(clientConnection);
        if (connection.IsEmpty())
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* If the connection is tearing down, we bail. */
        xpf::RundownGuard connectionGuard((*connection).ClientRundown);
        if (!connectionGuard.IsRundownAcquired())
        {
            return STATUS_TOO_LATE;
        }

        /* We need the api provider. */
        xpf::ServerSocketData* serverSocketData = reinterpret_cast<xpf::ServerSocketData*>(this->m_ServerSocketData);

        /* Now send the data to this client connection. */
        status = xpf::BerkeleySocket::Send(serverSocketData->ApiProvider,
                                           (*connection).ClientSocket,
                                           NumberOfBytes,
                                           Bytes);
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
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    xpf::SharedPointer<IClientCookie> clientConnection = this->FindClientConnection(ClientCookie);

    for (size_t retries = 0; retries < 5; ++retries)
    {
        /* Then we get to the underlying data. */
        auto connection = xpf::DynamicSharedPointerCast<xpf::ServerSocketClientData>(clientConnection);
        if (connection.IsEmpty())
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* If the connection is tearing down, we bail. */
        xpf::RundownGuard connectionGuard((*connection).ClientRundown);
        if (!connectionGuard.IsRundownAcquired())
        {
            return STATUS_TOO_LATE;
        }

        /* We need the api provider. */
        xpf::ServerSocketData* serverSocketData = reinterpret_cast<xpf::ServerSocketData*>(this->m_ServerSocketData);

        /* Now send the data to this client connection. */
        status = xpf::BerkeleySocket::Receive(serverSocketData->ApiProvider,
                                              (*connection).ClientSocket,
                                              NumberOfBytes,
                                              Bytes);
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
