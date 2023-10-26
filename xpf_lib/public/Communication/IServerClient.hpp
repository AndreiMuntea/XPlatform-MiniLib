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
#include "xpf_lib/public/Containers/Stream.hpp"

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

/**
 * @brief Copy constructor - delete.
 * 
 * @param[in] Other - The other object to construct from.
 */
IClientCookie(
    _In_ _Const_ const IClientCookie& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IClientCookie(
    _Inout_ IClientCookie&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - delete.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IClientCookie&
operator=(
    _In_ _Const_ const IClientCookie& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IClientCookie&
operator=(
    _Inout_ IClientCookie&& Other
) noexcept(true) = delete;
private:
};

/**
 * @brief   This is a server interface. All other server implementatations
 *          must inherit this one.
 */
class IServer
{
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
Start(
    void
) noexcept(true) = 0;

/**
 * @brief Stop the server gracefully.
 *        This method stops the server and releases any allocated resources.
 */
virtual void
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
AcceptClient(
    _Out_ IClientCookie& ClientCookie
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
DisconnectClient(
    _Inout_ IClientCookie& ClientCookie
) noexcept(true) = 0;

/**
 * @brief Send data to a client. If the client is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] DataStream    - The data stream to be sent to the client.
 *
 * @param[in,out] ClientCookie  - Uniquely identifies the newly connected client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
SendData(
    _Inout_ xpf::IStreamReader& DataStream,
    _Inout_ IClientCookie& ClientCookie
) noexcept(true) = 0;

/**
 * @brief Recieves data from a client. If the client is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] DataStream    - Newly received data will be appended to this stream.
 *
 * @param[in,out] ClientCookie  - Uniquely identifies the newly connected client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
ReceiveData(
    _Inout_ xpf::IStreamWriter& DataStream,
    _Inout_ IClientCookie& ClientCookie
) noexcept(true) = 0;

/**
 * @brief Copy constructor - delete.
 * 
 * @param[in] Other - The other object to construct from.
 */
IServer(
    _In_ _Const_ const IServer& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IServer(
    _Inout_ IServer&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - delete.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IServer&
operator=(
    _In_ _Const_ const IServer& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IServer&
operator=(
    _Inout_ IServer&& Other
) noexcept(true) = delete;
};  // class IServer

//
// ************************************************************************************************
// This is the section containing the interfaces for the client - side.
// ************************************************************************************************
//

/**
 * @brief   This is a server cookie interface.
 *          It can be specialized by each protocol so it uniquely identifies server-associated data
 *          where a client is connected to. The client uses this to send / receive data and communicate with the server.
 */
class IServerCookie
{
public:
/**
 * @brief IServerCookie constructor - default.
 */
IServerCookie(
    void
) noexcept(true) = default;

/**
 * @brief IServerCookie destructor - default.
 */
virtual ~IServerCookie(
    void
) noexcept(true) = default;

/**
 * @brief Copy constructor - delete.
 * 
 * @param[in] Other - The other object to construct from.
 */
IServerCookie(
    _In_ _Const_ const IServerCookie& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IServerCookie(
    _Inout_ IServerCookie&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - delete.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IServerCookie&
operator=(
    _In_ _Const_ const IServerCookie& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IServerCookie&
operator=(
    _Inout_ IServerCookie&& Other
) noexcept(true) = delete;
private:
};

/**
 * @brief   This is a client interface. All other client implementatations
 *          must inherit this one.
 */
class IClient
{
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
 * @param[out] ServerCookie - Uniquely identifies the newly connected client in this server.
 *                            Can be further used to send or receive data from this particular connection.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
Connect(
    _Out_ IServerCookie& ServerCookie
) noexcept(true) = 0;

/**
 * @brief Disconnect a client from the server.
 *        This method gracefully disconnects a client from the server.
 *        It waits for all outstanding communications with the server to end before terminating the connection.
 *
 * @param[in,out] ServerCookie - Uniquely identifies the connected client with this server.
 *                               Is retrieved via Connect method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
Disconnect(
    _Inout_ IServerCookie& ServerCookie
) noexcept(true) = 0;

/**
 * @brief Send data to a server. If the server is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] DataStream    - The data stream to be sent to the client.
 *
 * @param[in,out] ServerCookie  - Uniquely identifies the client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
SendData(
    _Inout_ xpf::IStreamReader& DataStream,
    _Inout_ IServerCookie& ServerCookie
) noexcept(true) = 0;

/**
 * @brief Recieves data from server. If the server is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in,out] DataStream    - Newly received data will be appended to this stream.
 *
 * @param[in,out] ServerCookie  - Uniquely identifies the client in this server.
 *                                Is retrieved via AcceptClient method.
 *
 * @return A proper NTSTATUS error code to indicate success or failure.
 */
_Must_inspect_result_
virtual NTSTATUS
ReceiveData(
    _Inout_ xpf::IStreamWriter& DataStream,
    _Inout_ IServerCookie& ServerCookie
) noexcept(true) = 0;

/**
 * @brief Copy constructor - delete.
 * 
 * @param[in] Other - The other object to construct from.
 */
IClient(
    _In_ _Const_ const IClient& Other
) noexcept(true) = delete;

/**
 * @brief Move constructor - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 */
IClient(
    _Inout_ IClient&& Other
) noexcept(true) = delete;

/**
 * @brief Copy assignment - delete.
 * 
 * @param[in] Other - The other object to construct from.
 * 
 * @return A reference to *this object after copy.
 */
IClient&
operator=(
    _In_ _Const_ const IClient& Other
) noexcept(true) = delete;

/**
 * @brief Move assignment - delete.
 * 
 * @param[in,out] Other - The other object to construct from.
 *                        Will be invalidated after this call.
 * 
 * @return A reference to *this object after move.
 */
IClient&
operator=(
    _Inout_ IClient&& Other
) noexcept(true) = delete;
};  // class IClient
};  // namespace xpf
