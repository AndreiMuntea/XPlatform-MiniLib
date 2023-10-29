/**
 * @file        xpf_lib/private/Communication/ClientSocket.cpp
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

    #if defined XPF_PLATFORM_WIN_UM
        WSAData WsaLibData = { 0 };
        SOCKET ServerSocket = INVALID_SOCKET;
        struct addrinfo* AddressInfo = nullptr;
    #else
        #error Unknown Platform
    #endif
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
    // Sanity checks of parameters.
    //
    if (Ip.IsEmpty() || Port.IsEmpty())
    {
        return nullptr;
    }

    //
    // First we allocate and construct the ClientSocketData.
    //
    data = reinterpret_cast<xpf::ClientSocketData*>(xpf::MemoryAllocator::AllocateMemory(sizeof(xpf::ClientSocketData)));
    if (nullptr == data)
    {
        return nullptr;
    }
    xpf::MemoryAllocator::Construct(data);

    //
    // Now we preinitialize the platform specific data.
    //
    data->IsConnected = false;

    #if defined XPF_PLATFORM_WIN_UM
        xpf::ApiZeroMemory(&data->WsaLibData,
                           sizeof(data->WsaLibData));
        data->ServerSocket = INVALID_SOCKET;
        data->AddressInfo = nullptr;
    #else
        #error Unknown Platform
    #endif

    //
    // And finally initialize the socket.
    //
    #if defined XPF_PLATFORM_WIN_UM
        int gleResult = 0;

        struct addrinfo hints;
        xpf::ApiZeroMemory(&hints, sizeof(hints));

        /* Default to TCP and IPv4 always. */
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

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
        gleResult = ::getaddrinfo(Ip.Buffer(), Port.Buffer(), &hints, &data->AddressInfo);
        if (0 != gleResult)
        {
            data->AddressInfo = nullptr;
            status = STATUS_CONNECTION_INVALID;
            goto CleanUp;
        }

        /* All good. */
        status = STATUS_SUCCESS;
    #else
        #error Unknown Platform
    #endif

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

    //
    // First clean platform specific data.
    //
    #if defined XPF_PLATFORM_WIN_UM
        if (INVALID_SOCKET != data->ServerSocket)
        {
            /* The shutdown can fail if the server already closed the socket. Just move on. */
            (void) ::shutdown(data->ServerSocket, SD_BOTH);
            (void) ::closesocket(data->ServerSocket);

            data->ServerSocket = INVALID_SOCKET;
        }
        if (nullptr != data->AddressInfo)
        {
            ::freeaddrinfo(data->AddressInfo);
            data->AddressInfo = nullptr;
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

    /* Connection is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM

        /* We are in an invalid state. We can't connect. */
        if (INVALID_SOCKET != data->ServerSocket)
        {
            return STATUS_INVALID_STATE_TRANSITION;
        }

        /* Attempt to connect to one of the endpoints that getaddrinfo returned. */
        for (struct addrinfo* crt = data->AddressInfo; nullptr != crt; crt = crt->ai_next)
        {
            /* This shouldn't be the case. */
            if (static_cast<uint64_t>(crt->ai_addrlen) > xpf::NumericLimits<int>::MaxValue())
            {
                continue;
            }

            /* Instantiate the socket. */
            data->ServerSocket = ::socket(crt->ai_family, crt->ai_socktype, crt->ai_protocol);
            if (INVALID_SOCKET == data->ServerSocket)
            {
                continue;
            }

            /* Connect to the socket. */
            int gleResult = ::connect(data->ServerSocket, crt->ai_addr, static_cast<int>(crt->ai_addrlen));
            if (0 != gleResult)
            {
                (void) ::closesocket(data->ServerSocket);
                data->ServerSocket = INVALID_SOCKET;

                continue;
            }

            /* We're connected. */
            break;
        }

        /* We didn't manage to get a connection. */
        if (INVALID_SOCKET == data->ServerSocket)
        {
            return STATUS_CONNECTION_REFUSED;
        }

    #else
        #error Unknown Platform
    #endif

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

    /* Disconnect is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        if (INVALID_SOCKET != data->ServerSocket)
        {
            /* The shutdown can fail if the server already closed the socket. Just move on. */
            (void) ::shutdown(data->ServerSocket, SD_BOTH);
            (void) ::closesocket(data->ServerSocket);

            data->ServerSocket = INVALID_SOCKET;
        }
    #else
        #error Unknown Platform
    #endif

    /* We managed to disconnect. */
    data->IsConnected = false;
    return STATUS_SUCCESS;
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

        /* Now send the data to this client connection. */
        status = this->SendDataToServerConnection(NumberOfBytes,
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

        /* Now send the data to this client connection. */
        status = this->ReceiveDataFromServerConnection(NumberOfBytes,
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
xpf::ClientSocket::SendDataToServerConnection(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Validate the parameters. Enforce 64 kb limit. */
    if ((nullptr == Bytes) || (0 == NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Then we get to the underlying data. */
    xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (!data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* And now do the actual send. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        int bytesSent = ::send(data->ServerSocket,
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
xpf::ClientSocket::ReceiveDataFromServerConnection(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    /* Validate the parameters. Enforce 64 kb limit. */
    if ((nullptr == Bytes) || (nullptr == NumberOfBytes) ||
        (0 == *NumberOfBytes) || (xpf::NumericLimits<uint16_t>::MaxValue() < *NumberOfBytes))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Then we get to the underlying data. */
    xpf::ClientSocketData* data = reinterpret_cast<xpf::ClientSocketData*>(this->m_ClientSocketData);
    if ((nullptr == data) || (!data->IsConnected))
    {
        return STATUS_INVALID_STATE_TRANSITION;
    }

    /* And now do the actual receive. This is platform specific. */
    #if defined XPF_PLATFORM_WIN_UM
        int bytesReceived = ::recv(data->ServerSocket,
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
