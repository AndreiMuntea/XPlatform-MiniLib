/**
 * @file        xpf_lib/public/Communication/IServerClient.hpp
 *
 * @brief       This contains the server client interface.
 *              Can be use to abstract away any protocols (like pipe / sockets).
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

namespace xpf
{
//
// ************************************************************************************************
// This is the section containing the interfaces for the server - side.
// ************************************************************************************************
//

/**
 * @brief   This is a client cookie interface.
 *          It can be specialized by each protocol so it uniquely identifies a client connected
 *          to a server. The server uses this to send / receive data and communicate with an endpoint.
 */
class IClientCookie
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IClientCookie, default);

 public:
/**
 * @brief IClientCookie constructor - default.
 */
IClientCookie(
    void
) noexcept(true) = default;

/**
 * @brief IServer destructor - default.
 */
virtual ~IClientCookie(
    void
) noexcept(true) = default;
};

/**
 * @brief   This is a server interface. All other server implementatations
 *          must inherit this one.
 */
class IServer
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IServer, default);

 public:
/**
 * @brief IServer constructor - default.
 */
IServer(
    void
) noexcept(true) = default;

/**
 * @brief IServer destructor - default.
 */
virtual ~IServer(
    void
) noexcept(true) = default;

/**
 * @brief This method initializes the server and prepares it to accept client connections.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
XPF_API
Start(
    void
) noexcept(true) = 0;

/**
 * @brief Stop the server gracefully.
 *        This method stops the server and releases any allocated resources.
 *
 * @return void.
 */
virtual void
XPF_API
Stop(
    void
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
AcceptClient(
    _Out_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
DisconnectClient(
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
ReceiveData(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes,
    _Inout_ xpf::SharedPointer<IClientCookie>& ClientCookie
) noexcept(true) = 0;
};  // class IServer

//
// ************************************************************************************************
// This is the section containing the interfaces for the client - side.
// ************************************************************************************************
//

/**
 * @brief   This is a client interface. All other client implementatations
 *          must inherit this one.
 */
class IClient
{
 protected:
/**
 * @brief Copy and move semantics are defaulted.
 *        It's each class responsibility to handle them.
 */
XPF_CLASS_COPY_MOVE_BEHAVIOR(IClient, default);

 public:
/**
 * @brief IClient constructor - default.
 */
IClient(
    void
) noexcept(true) = default;

/**
 * @brief IClient destructor - default.
 */
virtual ~IClient(
    void
) noexcept(true) = default;


/**
 * @brief This method will connect to the server.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
XPF_API
Connect(
    void
) noexcept(true) = 0;

/**
 * @brief Disconnect a client from the server.
 *        This method gracefully disconnects a client from the server.
 *        It waits for all outstanding communications with the server to end before terminating the connection.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
XPF_API
Disconnect(
    void
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
SendData(
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true) = 0;

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
virtual NTSTATUS
XPF_API
ReceiveData(
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true) = 0;
};  // class IClient
};  // namespace xpf
