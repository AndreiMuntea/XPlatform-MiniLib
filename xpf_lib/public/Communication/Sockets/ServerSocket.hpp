/**
 * @file        xpf_lib/public/Communication/Sockets/ServerSocket.hpp
 *
 * @brief       This contains the server implementation using sockets.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#pragma once


#include "xpf_lib/public/core/Core.hpp"
#include "xpf_lib/public/core/TypeTraits.hpp"

#include "xpf_lib/public/Locks/ReadWriteLock.hpp"
#include "xpf_lib/public/Memory/SharedPointer.hpp"

#include "xpf_lib/public/Containers/Stream.hpp"
#include "xpf_lib/public/Containers/String.hpp"
#include "xpf_lib/public/Containers/Vector.hpp"

#include "xpf_lib/public/Communication/Sockets/BerkeleySocket.hpp"
#include "xpf_lib/public/Communication/IServerClient.hpp"


namespace xpf
{

/**
 * @brief   This class provides the server functionality using sockets.
 *          It contains platform-specific implementation.
 */
class ServerSocket final : public virtual xpf::IServer
{
 public:
/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ServerSocket, delete);

/**
 * @brief ServerSocket constructor.
 *        Currently this uses only IPv4 and TCP.
 *        It can be extended later to support other types as well.
 *        For now keep in mind that there is this limitation!
 *
 * @param[in] Ip - The IPv4 of the socket.
 *
 * @param[in] Port - The port of the socket.
 *
 */
ServerSocket(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true) : xpf::IServer()
{
    if (NT_SUCCESS(xpf::ReadWriteLock::Create(&this->m_ServerLock)))
    {
        this->m_ServerSocketData = this->CreateServerSocketData(Ip, Port);
    }
}

/**
 * @brief ServerSocket destructor.
 */
virtual ~ServerSocket(
    void
) noexcept(true)
{
    this->Stop();

    this->DestroyServerSocketData(this->m_ServerSocketData);
    this->m_ServerSocketData = nullptr;
}

/**
 * @brief This method initializes the server and prepares it to accept client connections.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Start(
    void
) noexcept(true) override;

/**
 * @brief Stop the server gracefully.
 *        This method stops the server and releases any allocated resources.
 *
 * @return void
 */
void
XPF_API
Stop(
    void
) noexcept(true) override;

/**
 * @brief Accept client connections.
 *        This method listens for incoming client connections and handles them.
 *
 * @param[out] ClientCookie - Uniquely identifies the newly connected client in this server.
 *                            Can be further used to send or receive data from this particular connection.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
AcceptClient(
    _Out_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) override;

/**
 * @brief Disconnect a client from the server.
 *        This method gracefully disconnects a client from the server.
 *        It waits for all outstanding communications with this client to end before terminating the connection.
 *
 * @param[in,out] ClientCookie - Uniquely identifies the newly connected client in this server.
 *                               Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
DisconnectClient(
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) override;

/**
 * @brief Send data to a client. If the client is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in] NumberOfBytes - The number of bytes to write to the socket
 *
 * @param[in] Bytes - The bytes to be written.
 *
 * @param[in,out] ClientCookie  - Uniquely identifies the newly connected client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) override;

/**
 * @brief Recieves data from a client. If the client is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] NumberOfBytes - The number of bytes to read from the client.
 *                                On return this contains the actual number of bytes read.
 *
 * @param[in,out] Bytes - The read bytes.
 *
 * @param[in,out] ClientCookie  - Uniquely identifies the newly connected client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
ReceiveData(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) override;

 private:
/**
 * @brief Creates the platform specific server socket data.
 *        This socket can be used to listen for new connections.
 *
 * @param[in] Ip - The IPv4 of the socket.
 *
 * @param[in] Port - The port of the socket.
 *
 * @return The server Socket Data, or nullptr on failure.
 */
void*
XPF_API
CreateServerSocketData(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true);

/**
 * @brief Destroys a previously created server socket data
 *
 * @param[in,out] ServerSocketData - Socket data created by CreateServerSocketData.
 *
 * @return void
 */
void
XPF_API
DestroyServerSocketData(
    _Inout_ void* ServerSocketData
) noexcept(true);

/**
 * @brief Initializes a client connection.
 *
 * @param[in, out] ClientConnection - The newly initialized client connection.
 * 
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
EstablishClientConnection(
    _Inout_ xpf::SharedPointer<xpf::IClientCookie>& ClientConnection
) noexcept(true);

/**
 * @brief Properly terminates a client connection.
 *
 * @param[in, out] ClientConnection - The connection to be terminated.
 *
 * @return void.
 */
void
XPF_API
CloseClientConnection(
    _Inout_ xpf::SharedPointer<xpf::IClientCookie>& ClientConnection
) noexcept(true);

 private:
     void* m_ServerSocketData = nullptr;

    xpf::Optional<xpf::ReadWriteLock> m_ServerLock;
    xpf::Vector<xpf::SharedPointer<xpf::IClientCookie>> m_Clients;

    bool m_IsStarted = false;
};  // class ServerSocket
};  // namespace xpf
