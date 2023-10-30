/**
 * @file        xpf_lib/private/Communication/Sockets/ClientSocket.cpp
 *
 * @brief       Client-Side implementation using sockets.
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

struct ClientSocketData
{
    bool IsConnected = false;

    struct addrinfo* AddressInfo = nullptr;
    xpf::BerkeleySocket::Socket ServerSocket = nullptr;
    xpf::BerkeleySocket::SocketApiProvider ApiProvider = nullptr;
};  // struct ClientSocketData
};  // namespace xpf


void*
XPF_API
xpf::ClientSocket::CreateClientSocketData(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::ClientSocketData* data = nullptr;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    //
    // First we allocate and construct the ClientSocketData.
    //
    data = reinterpret_cast<xpf::ClientSocketData*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::ClientSocketData)));
    if (nullptr == data)
    {
        return nullptr;
    }
    xpf::MemoryAllocator::Construct(data);

    data->IsConnected = false;

    status = xpf::BerkeleySocket::InitializeSocketApiProvider(&data->ApiProvider);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

    status = xpf::BerkeleySocket::GetAddressInformation(data->ApiProvider, Ip, Port, &data->AddressInfo);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        this->DestroyClientSocketData(reinterpret_cast<void**>(&data));
        data = nullptr;
    }
    return data;
}

void
XPF_API
xpf::ClientSocket::DestroyClientSocketData(
    _Inout_ void** ClientSocketData
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Can't free null pointer.
    //
    if ((nullptr == ClientSocketData) || (nullptr == (*ClientSocketData)))
    {
        return;
    }
    xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(*ClientSocketData);

    if (nullptr != data->ServerSocket)
    {
        NTSTATUS shutdownStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                                      &data->ServerSocket);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));
    }
    if (nullptr != data->AddressInfo)
    {
        NTSTATUS freeStatus = xpf::BerkeleySocket::FreeAddressInformation(data->ApiProvider,
                                                                          &data->AddressInfo);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(freeStatus));
    }
    if (nullptr != data->ApiProvider)
    {
        xpf::BerkeleySocket::DeInitializeSocketApiProvider(&data->ApiProvider);
    }

    //
    // Mark it not connected. Be a good citizen.
    //
    data->IsConnected = false;

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(data);
    xpf::MemoryAllocator::FreeMemory(ClientSocketData);
    *ClientSocketData = nullptr;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ClientSocket::Connect(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* If the client was not properly initialized, we can't do much. */
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    xpf::ExclusiveLockGuard guard{ *this->m_ClientLock };

    /* If the client is already connected, we bail. */
    xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }


    /* Attempt to connect to one of the endpoints that getaddrinfo returned. */
    for (struct addrinfo* crt = data->AddressInfo; nullptr != crt; crt = crt->ai_next)
    {
        /* Instantiate the socket. */
        NTSTATUS status = xpf::BerkeleySocket::CreateSocket(data->ApiProvider,
                                                            crt->ai_family,
                                                            crt->ai_socktype,
                                                            crt->ai_protocol,
                                                            &data->ServerSocket);
        if (!NT_SUCCESS(status))
        {
            continue;
        }

        /* Connect to the socket. */
        status = xpf::BerkeleySocket::Connect(data->ApiProvider,
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

        /* We're connected. */
        break;
    }

    if (nullptr == data->ServerSocket)
    {
        return STATUS_CONNECTION_REFUSED;
    }

    /* We managed to connect. */
    data->IsConnected = true;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ClientSocket::Disconnect(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* If the client was not properly initialized, we can't do much. */
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    xpf::ExclusiveLockGuard guard{ *this->m_ClientLock };

    /* If the client is already disconnected, we bail. */
    xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (!data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    NTSTATUS retStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                             &data->ServerSocket);
    data->IsConnected = false;
    return retStatus;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ClientSocket::SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* If the client was not properly initialized, we can't do much. */
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    for (size_t retries = 0; retries < 5; ++retries)
    {
        xpf::SharedLockGuard guard{ *this->m_ClientLock };

        xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
        if ((nullptr == data) || (!data->IsConnected))
        {
            return STATUS_INVALID_STATE_TRANSITION;
        }

        /* Now send the data to this client connection. */
        status = xpf::BerkeleySocket::Send(data->ApiProvider,
                                           data->ServerSocket,
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
        (void) this->Disconnect();
    }
    return status;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ClientSocket::ReceiveData(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* If the client was not properly initialized, we can't do much. */
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    for (size_t retries = 0; retries < 5; ++retries)
    {
        xpf::SharedLockGuard guard{ *this->m_ClientLock };

        xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
        if ((nullptr == data) || (!data->IsConnected))
        {
            return STATUS_INVALID_STATE_TRANSITION;
        }

        /* Now send the data to this client connection. */
        status = xpf::BerkeleySocket::Receive(data->ApiProvider,
                                              data->ServerSocket,
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
        (void) this->Disconnect();
    }
    return status;
}
