/**
 * @file        xpf_lib/public/Communication/Sockets/ClientSocket.hpp
 *
 * @brief       This contains the client implementation using sockets.
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
 * @brief   This class provides the client functionality using sockets.
 *          It contains platform-specific implementation.
 */
class ClientSocket final : public xpf::IClient
{
 public:
/**
 * @brief Copy and move semantics are deleted.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(ClientSocket, delete);

/**
 * @brief ClientSocket constructor.
 *        Currently this uses only IPv4 and TCP.
 *        It can be extended later to support other types as well.
 *        For now keep in mind that there is this limitation!
 *
 * @param[in] Ip - The IPv4 of the socket.
 *
 * @param[in] Port - The port of the socket.
 *
 */
ClientSocket(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true) : xpf::IClient()
{
    if (NT_SUCCESS(xpf::ReadWriteLock::Create(&this->m_ClientLock)))
    {
        this->m_ClientSocketData = this->CreateClientSocketData(Ip, Port);
    }
}

/**
 * @brief ClientSocket destructor.
 */
virtual ~ClientSocket(
    void
) noexcept(true)
{
    (void) this->Disconnect();
    this->DestroyClientSocketData(&this->m_ClientSocketData);
}

/**
 * @brief This method will connect to the server.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Connect(
    void
) noexcept(true) override;

/**
 * @brief Disconnect a client from the server.
 *        This method gracefully disconnects a client from the server.
 *        It waits for all outstanding communications with the server to end before terminating the connection.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Disconnect(
    void
) noexcept(true) override;

/**
 * @brief Send data to a server. If the server is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in] NumberOfBytes - The number of bytes to write to the socket
 *
 * @param[in] Bytes - The bytes to be written.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true) override;

/**
 * @brief Recieves data from server. If the server is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] NumberOfBytes - The number of bytes to read from the server.
 *                                On return this contains the actual number of bytes read.
 *
 * @param[in,out] Bytes - The read bytes.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
NTSTATUS
XPF_API
ReceiveData(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true) override;

 private:
/**
 * @brief Creates the platform specific client socket data.
 *        This socket is used to connect to the server-end.
 *
 * @param[in] Ip - The IPv4 of the socket.
 *
 * @param[in] Port - The port of the socket.
 *
 * @return The client Socket Data, or nullptr on failure.
 */
void*
XPF_API
CreateClientSocketData(
    _In_ _Const_ const xpf::StringView<char>& Ip,
    _In_ _Const_ const xpf::StringView<char>& Port
) noexcept(true);

/**
 * @brief Destroys a previously created server socket data
 *
 * @param[in,out] ClientSocketData - Socket data created by CreateServerSocketData.
 *
 * @return void
 */
void
XPF_API
DestroyClientSocketData(
    _Inout_ void** ClientSocketData
) noexcept(true);

 private:
     void* m_ClientSocketData = nullptr;
     xpf::Optional<xpf::ReadWriteLock> m_ClientLock;
};  // class ClientSocket
};  // namespace xpf
