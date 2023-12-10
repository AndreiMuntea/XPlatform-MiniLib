/**
 * @file        xpf_lib/public/Communication/Sockets/BerkeleySocket.hpp
 *
 * @brief       This contains the interface for berkeley socket.
 *              It abstracts away the platform-specific data.
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
#include "xpf_lib/public/Containers/String.hpp"

#include "xpf_lib/public/Communication/Sockets/win_km/WskApi.hpp"


namespace xpf
{
namespace BerkeleySocket
{
//
// ************************************************************************************************
// This is the section containing the initialization support.
// ************************************************************************************************
//

/**
 * @brief On some platforms (eg: windows), we need to pre-initialize the library
 *        to be able to use sockets. We abstract this data away in this call.
 */
using SocketApiProvider = void*;

/**
 * @brief This initiates the support for using sockets.
 *        It is safe to call this function multiple times.
 *        But each call must be matched by a call to DeInitializeSocketApiProvider
 *
 * @param[in,out] SocketApiProvider - An opaque pointer which will be used to store data
 *                                    required by each platform to correctly function with sockets.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
InitializeSocketApiProvider(
    _Inout_ xpf::BerkeleySocket::SocketApiProvider* SocketApiProvider
) noexcept(true);

/**
 * @brief This deinitializes the support for using sockets.
 *        It is safe to call this function multiple times.
 *        Only the final function call performs the actual cleanup
 *
 * @param[in,out] SocketApiProvider - An opaque pointer which will be used to store data
 *                                    required by each platform to correctly function with sockets.
 *
 * @return void
 *
 */
void
XPF_API
DeInitializeSocketApiProvider(
    _Inout_ xpf::BerkeleySocket::SocketApiProvider* SocketApiProvider
) noexcept(true);

//
// ************************************************************************************************
// This is the section containing the getaddrinfo support.
// ************************************************************************************************
//

/**
 * @brief The GetAddressInformation function provides protocol-independent translation from an ANSI host name to an address.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in] NodeName - Contains a host (node) name or a numeric host address string.
 *                       For the Internet protocol, the numeric host address string is
 *                       a dotted-decimal IPv4 address or an IPv6 hex address.
 *
 * @param[in] ServiceName - A service name is a string alias for a port number.
 *                          For example, “http” is an alias for port 80 defined by the Internet Engineering Task Force (IETF)
 *                          as the default port used by web servers for the HTTP protocol.
 *                          Possible values for the pServiceName parameter when a port number is not specified
 *                          are listed in the following file: %WINDIR%/system32/drivers/etc/services
 *
 * @param[out] AddrInfo - A pointer containing information about the host.
 *                        Must be freed with FreeAddressInformation.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
GetAddressInformation(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _In_ _Const_ const xpf::StringView<char>& NodeName,
    _In_ _Const_ const xpf::StringView<char>& ServiceName,
    _Out_ addrinfo** AddrInfo
) noexcept(true);

/**
 * @brief The FreeAddressInformation function frees address information that the GetAddressInformation
 *        function dynamically allocates in AddressInformation structure.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[out] AddrInfo - A pointer containing information about the host.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
FreeAddressInformation(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ addrinfo** AddrInfo
) noexcept(true);

//
// ************************************************************************************************
// This is the section containing the berkeley socket support.
// ************************************************************************************************
//

/**
 * @brief This is an opaque pointer to hide the berkeley socket implementation of SOCKET.
 */
using Socket = void*;

/**
 * @brief Creates a socket that is bound to a specific transport service provider.
 *        See https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in] AddressFamily     - AF_INET for IPv4 or AF_INET6 for IPv6.
 *
 * @param[in] Type              - SOCK_STREAM for TCP.
 *
 * @param[in] Protocol          - IPPROTO_TCP for TCP.
 *
 * @param[in] IsListeningSocket - If true, the socket will be use in listening calls,
 *                                if false it will be a connection socket.
 *
 * @param[out] CreatedSocket    - The newly created socket.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
CreateSocket(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _In_ int AddressFamily,
    _In_ int Type,
    _In_ int Protocol,
    _In_ bool IsListeningSocket,
    _Out_ xpf::BerkeleySocket::Socket* CreatedSocket
) noexcept(true);

/**
 * @brief Closes an existing socket.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket  - The socket created with CreateSocket() function.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
ShutdownSocket(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket* TargetSocket
) noexcept(true);

/**
 * @brief Associates a local address with a socket.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket  - The socket created with CreateSocket() function.
 *
 * @param[in] LocalAddress      - A pointer to a sockaddr structure of the local address to assign to the bound socket.
 *
 * @param[in] Length            - The length, in bytes, of the value pointed to by Address.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Bind(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ _Const_ const sockaddr* LocalAddress,
    _In_ int Length
) noexcept(true);

/**
 * @brief Places a socket in a state in which it is listening for an incoming connection.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket  - The socket created with CreateSocket() function.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Listen(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket
) noexcept(true);

/**
 * @brief The connect function establishes a connection to a specified socket.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket  - The socket created with CreateSocket() function.
 *
 * @param[in] Address           - A pointer to the sockaddr structure to which the connection should be established.
 *
 * @param[in] Length            - The length, in bytes, of the value pointed to by Address.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Connect(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ _Const_ const sockaddr* Address,
    _In_ int Length
) noexcept(true);

/**
 * @brief The accept function permits an incoming connection attempt on a socket.
 *
 * @param[in] SocketApiProvider - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket  - The socket created with CreateSocket() function.
 *
 * @param[out] NewSocket        - The Socket where the connection is actually made.
 *
 * @return a proper NTSTATUS error code.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Accept(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _Out_ xpf::BerkeleySocket::Socket* NewSocket
) noexcept(true);

/**
 * @brief Send data to a socket connection. If the other end is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in] SocketApiProvider     - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket      - The socket where to send data to
 *
 * @param[in] NumberOfBytes         - The number of bytes to write to the socket
 *
 * @param[in] Bytes                 - The bytes to be written.
 *
 * @return STATUS_SUCCESS if the data was succesfully sent.
 *         STATUS_CONNECTION_ABORTED if the connection was terminated on client end.
 *         STATUS_NETWORK_BUSY if the connection is still valid, but we failed to send data over network.
 *         Another error status to describe non-network related errors.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Send(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _In_ size_t NumberOfBytes,
    _In_ _Const_ const uint8_t* Bytes
) noexcept(true);

/**
 * @brief Recieves data from a socket connection. If the other end is disconnecting or was disconnected,
 *        this method will return a failure status.
 *
 * @param[in] SocketApiProvider     - An opaque pointer which was returned by InitializeSocketApiProvider.
 *
 * @param[in,out] TargetSocket      - The socket where to send data to
 *
 * @param[in,out] NumberOfBytes     - The number of bytes to read from the server.
 *                                    On return this contains the actual number of bytes read.
 *
 * @param[in,out] Bytes             - The read bytes.
 *
 * @return STATUS_SUCCESS if the data was succesfully sent.
 *         STATUS_CONNECTION_ABORTED if the connection was terminated on client end.
 *         STATUS_NETWORK_BUSY if the connection is still valid, but we failed to send data over network.
 *         Another error status to describe non-network related errors.
 *
 */
_Must_inspect_result_
NTSTATUS
XPF_API
Receive(
    _In_ xpf::BerkeleySocket::SocketApiProvider SocketApiProvider,
    _Inout_ xpf::BerkeleySocket::Socket TargetSocket,
    _Inout_ size_t* NumberOfBytes,
    _Inout_ uint8_t* Bytes
) noexcept(true);
};  // namespace BerkeleySocket
};  // namespace xpf
