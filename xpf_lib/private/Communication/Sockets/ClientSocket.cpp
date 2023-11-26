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

    xpf::BerkeleySocket::AddressInfo* AddressInfo = nullptr;
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
    data = static_cast<xpf::ClientSocketData*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::ClientSocketData)));
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

    //
    // Resolve the server address and port.
    //
    status = xpf::BerkeleySocket::GetAddressInformation(data->ApiProvider, Ip, Port, &data->AddressInfo);
    if (!NT_SUCCESS(status))
    {
        goto CleanUp;
    }

CleanUp:
    if (!NT_SUCCESS(status))
    {
        this->DestroyClientSocketData(data);
        data = nullptr;
    }
    return data;
}

void
XPF_API
xpf::ClientSocket::DestroyClientSocketData(
    _Inout_ void* ClientSocketData
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // Can't free null pointer.
    //
    if (nullptr == ClientSocketData)
    {
        return;
    }
    xpf::ClientSocketData* data = static_cast<xpf::ClientSocketData*>(ClientSocketData);

    //
    // Shutdown the server socket, if any.
    // If we created a socket, we don't expect the shutdown to fail.
    //
    if (nullptr != data->ServerSocket)
    {
        NTSTATUS shutdownStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                                      &data->ServerSocket);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(shutdownStatus));

        data->ServerSocket = nullptr;
    }

    //
    // Clean addrinfo, if we have some.
    // If we managed to get the addrinfo, we don't expect the free to fail.
    //
    if (nullptr != data->AddressInfo)
    {
        NTSTATUS freeStatus = xpf::BerkeleySocket::FreeAddressInformation(data->ApiProvider,
                                                                          &data->AddressInfo);
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(freeStatus));

        data->AddressInfo = nullptr;
    }

    //
    // And close the socket api provider.
    //
    if (nullptr != data->ApiProvider)
    {
        xpf::BerkeleySocket::DeInitializeSocketApiProvider(&data->ApiProvider);
        data->ApiProvider = nullptr;
    }

    //
    // Mark it not connected. Be a good citizen.
    //
    data->IsConnected = false;

    //
    // And now destroy the object.
    //
    xpf::MemoryAllocator::Destruct(data);
    xpf::MemoryAllocator::FreeMemory(data);

    data = nullptr;
}

_Must_inspect_result_
NTSTATUS
XPF_API
xpf::ClientSocket::Connect(
    void
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    //
    // If the client was not properly initialized, we can't do much.
    //
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    xpf::ExclusiveLockGuard guard{ *this->m_ClientLock };

    //
    // If the client is already connected, we bail.
    //
    xpf::ClientSocketData* data = static_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    for (xpf::BerkeleySocket::AddressInfo* crt = data->AddressInfo; nullptr != crt; crt = crt->ai_next)
    {
        //
        // Ensure we have valid protocol and type.
        //
        crt->ai_protocol = (crt->ai_protocol == 0) ? IPPROTO_TCP
                                                   : crt->ai_protocol;
        crt->ai_socktype = (crt->ai_socktype == 0) ? SOCK_STREAM
                                                   : crt->ai_socktype;

        //
        // Create the connection socket.
        // On fail, we simply retry the next address.
        //
        NTSTATUS status = xpf::BerkeleySocket::CreateSocket(data->ApiProvider,
                                                            crt->ai_family,
                                                            crt->ai_socktype,
                                                            crt->ai_protocol,
                                                            false,
                                                            &data->ServerSocket);
        if (!NT_SUCCESS(status))
        {
            continue;
        }

        //
        // Connect to the other end.
        // On fail, we simply retry the next address.
        //
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

        //
        // We're connected.
        //
        break;
    }

    //
    // After we moved through all addresses, we need a valid one,
    // otherwise this is a failure.
    //
    if (nullptr == data->ServerSocket)
    {
        return STATUS_CONNECTION_REFUSED;
    }

    //
    // We no longer need the address info.
    //
    if (nullptr != data->AddressInfo)
    {
        NTSTATUS freeStatus = xpf::BerkeleySocket::FreeAddressInformation(data->ApiProvider,
                                                                          &data->AddressInfo);
        data->AddressInfo = nullptr;
        XPF_DEATH_ON_FAILURE(NT_SUCCESS(freeStatus));
    }

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

    //
    // If the client was not properly initialized, we can't do much.
    //
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    xpf::ExclusiveLockGuard guard{ *this->m_ClientLock };

    //
    // If the client is already disconnected, we bail.
    //
    xpf::ClientSocketData* data = static_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (!data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    NTSTATUS retStatus = xpf::BerkeleySocket::ShutdownSocket(data->ApiProvider,
                                                             &data->ServerSocket);
    data->IsConnected = false;
    data->ServerSocket = nullptr;

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

    //
    // If the client was not properly initialized, we can't do much.
    //
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    for (size_t retries = 0; retries < 5; ++retries)
    {
        xpf::SharedLockGuard guard{ *this->m_ClientLock };

        //
        // We get to the underlying data.
        //
        xpf::ClientSocketData* data = static_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
        if ((nullptr == data) || (!data->IsConnected))
        {
            return STATUS_INVALID_STATE_TRANSITION;
        }

        //
        // Now send the data over the connection.
        // If the network was busy, we will retry.
        //
        status = xpf::BerkeleySocket::Send(data->ApiProvider,
                                           data->ServerSocket,
                                           NumberOfBytes,
                                           Bytes);
        if (STATUS_NETWORK_BUSY != status)
        {
            break;
        }
        xpf::ApiSleep(20);
    }

    //
    // If the connection was aborted, we disconnect on our end as well.
    //
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

    //
    // If the client was not properly initialized, we can't do much.
    //
    if ((!this->m_ClientLock.HasValue()) || (nullptr == this->m_ClientSocketData))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    for (size_t retries = 0; retries < 5; ++retries)
    {
        xpf::SharedLockGuard guard{ *this->m_ClientLock };

        //
        // We get to the underlying data.
        //
        xpf::ClientSocketData* data = static_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
        if ((nullptr == data) || (!data->IsConnected))
        {
            return STATUS_INVALID_STATE_TRANSITION;
        }

        //
        // Now send the data over the connection.
        // If the network was busy, we will retry.
        //
        status = xpf::BerkeleySocket::Receive(data->ApiProvider,
                                              data->ServerSocket,
                                              NumberOfBytes,
                                              Bytes);
        if (STATUS_NETWORK_BUSY != status)
        {
            break;
        }
        xpf::ApiSleep(20);
    }

    //
    // If the connection was aborted, we disconnect on our end as well.
    //
    if (STATUS_CONNECTION_ABORTED == status)
    {
        (void) this->Disconnect();
    }
    return status;
}
