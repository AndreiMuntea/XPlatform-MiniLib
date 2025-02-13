/**
 * @file        xpf_lib/public/Communication/http.hpp
 *
 * @brief       This contains a mini http parser implementation.
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
#include "xpf_lib/public/Containers/String.hpp"

namespace xpf
{
namespace http
{

/**
 * @brief   The supported http versions.
 *          We support 1.0 and 1.1 as they are the most basic.
 *          for 1.2 and 1.3 there is significat work required, so we're leaving this
 *          for future.
 */
enum class HttpVersion : uint32_t
{
    /**
     * @brief   See the RFC https://www.rfc-editor.org/rfc/rfc1945
     */
    Http1_0 = 0,

    /**
     * @brief   See the RFC https://www.rfc-editor.org/rfc/rfc2616
     */
    Http1_1 = 1,

    /**
     * @brief   To know how many versions are supported.
     *          Do not add anything after this field.
     */
    MaxHttpVersion
};  // enum class HttpVersion

/**
 * @brief   This contains mapping between the http version and its textual representation.
 */
struct HttpVersionMap
{
    /**
     * @brief   The supported http version as enum.
     */
    xpf::http::HttpVersion Version;

    /**
     * @brief   The supported http version as text.
     */
    const char Text[9];
};  // struct HttpVersionText

/**
 * @brief   This contains mapping between the http status codes and their textual representation.
 */
struct HttpStatusMap
{
    /**
     * @brief   The numerical value of the status.
     */
    size_t  Status;

    /**
     * @brief   The textual representation of status.
     */
    const char Text[4];
};

/**
 * @brief   In a http request, a header item is like:
 *          key : value and CR LF
 *          See https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers
 *
 * @note    Host: developer.mozilla.org
 *          User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.9; rv:50.0) Gecko/20100101 Firefox/50.0
 *          Accept-Language: en-US,en;q=0.5
 *          Accept-Encoding: gzip, deflate, br
 */
struct HeaderItem
{
    /**
     * @brief   The value before ':'
     */
    xpf::StringView<char> Key;

    /**
     * @brief   The value after ':'
     */
    xpf::StringView<char> Value;
};

/**
 * @brief   This will be a parsed response from an HTTP request.
 */
struct HttpResponse
{
    /**
     * @brief   The response buffer is referenced counted as the HttpResponse contains data which
     *          points inside of this buffer, so it needs to remain valid, to ensure we don't have
     *          invalid memory. So the http response will reference the original response buffer,
     *          to ensure its lifespan won't end before the response goes out.
     */
    xpf::SharedPointer<xpf::Buffer> ResponseBuffer;

    /**
     * @brief   The received http version as enum.
     */
    xpf::http::HttpVersion Version = xpf::http::HttpVersion::MaxHttpVersion;

    /**
     * @brief   The received response code.
     */
    size_t HttpStatusCode = 0;

    /**
     * @brief   The status message. Shallow copy from inside ResponseBuffer.
     */
    xpf::StringView<char> HttpStatusMessage;

    /**
     * @brief   The identified headers inside the  ResponseBuffer.
     */
    xpf::Vector<xpf::http::HeaderItem> Headers;

    /**
     * @brief   The body of the response.
     *          We use StringView but really it is just a byte array.
     *          We just wrap it inside StringView but should be used as byte*
     */
    xpf::StringView<char> Body;
};

/**
 * @brief   A structure to hold information about an URL.
 *          See https://developer.mozilla.org/en-US/docs/Learn/Common_questions/Web_mechanics/What_is_a_URL
 *
 *          http://www.example.com:80/path/to/myfile.html?key1=value1&key2=value2#SomewhereInTheDocument
 *          |---|  |--------------||-||------------------||----------------------||--------------------|
 *          Scheme   Domain        Port      Path               Parameters              Anchor
 *                 |-----------------|
 *                     Authority
 */
struct UrlInfo
{
    /**
     * @brief   The original URL.
     */
    xpf::String<char> Url;

    /**
     * @brief The first part of the URL is the scheme, which indicates the protocol that
     *        the browser must use to request the resource.
     */
    xpf::String<char> Scheme;

    /**
     * @brief Next follows the authority, which is separated from the scheme by the character pattern ://.
     *        If present the authority includes both the domain (e.g. www.example.com) and the port
     *        (80), separated by a colon:
     */
    xpf::String<char> Authority;

    /**
     * @brief The domain indicates which Web server is being requested.
     */
    xpf::String<char> Domain;

    /**
     * @brief The port indicates the technical "gate" used to access the resources on the web server.
     *        It is usually omitted if the web server uses the standard ports of the HTTP protocol
     *        (80 for HTTP and 443 for HTTPS) to grant access to its resources. Otherwise it is mandatory.
     */
    xpf::String<char> Port;

    /**
     * @brief  is the path to the resource on the Web server. In the early days of the Web, a path
     *         like this represented a physical file location on the Web server.
     *         Nowadays, it is mostly an abstraction handled by Web servers without any physical reality.
     */
    xpf::String<char> Path;

    /**
     * @brief Extra parameters provided to the Web server. 
     */
    xpf::String<char> Parameters;

    /**
     * @brief  An anchor represents a sort of "bookmark" inside the resource, giving the browser the
     *         directions to show the content located at that "bookmarked" spot.
     */
    xpf::String<char> Anchor;
};  // struct UrlInfo

/**
 * @brief           Parses an URL in its components. See the description of UrlInfo structure above.
 *
 * @param[in]       Url             - The url in text format.
 * @param[in,out]   UrlInformation  - The url in token format.
 *
 * @return          A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
ParseUrlInformation(
    _In_ _Const_ const xpf::StringView<char>& Url,
    _Inout_ xpf::http::UrlInfo& UrlInformation
) noexcept(true);

/**
 * @brief   Builds an HTTP request like:
 *
 *          Method               Path            Parameters     HTTP version
 *          |----| |------------------------||-----------------||-----------|
 *           GET   /foobar/otherbar/somepage?arg1=val1&arg2=val2  HTTP/1.1
 *
 *          It also appends the Host header so it is not required to specify in HeaderItems:
 *          Host: www.google.com
 *
 * @param[in]       Host             -   The http host (for example www.google.com)
 * @param[in]       Method           -   The http method (for example GET)
 * @param[in]       ResourcePath     -   The http resource path (for example /foobar/otherbar/somepage)
 * @param[in]       Version          -   The http version (see enum HttpVersion)
 * @param[in]       Parameters       -   The http request parameters (for example ?param1=value1&param2=value2)
 * @param[in]       HeaderItems      -   The header items to be included in the request.
 *                                       This is a variable sized array.
 * @param[in]       HeaderItemsCount -   The number of elements in the HeaderItems array.
 * @param[in,out]   Request          -   Will contain the request string and headers properly formed.
 *
 * @return          A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
BuildHttpRequest(
    _In_ _Const_ const xpf::StringView<char>& Host,
    _In_ _Const_ const xpf::StringView<char>& Method,
    _In_ _Const_ const xpf::StringView<char>& ResourcePath,
    _In_ _Const_ const xpf::StringView<char>& Parameters,
    _In_ _Const_ const xpf::http::HttpVersion& Version,
    _In_opt_ _Const_ const HeaderItem* HeaderItems,
    _In_ _Const_ size_t HeaderItemsCount,
    _Inout_ xpf::String<char>& Request
) noexcept(true);

/**
 * @brief           Parses the response of an http request.
 *
 *                  - The start-line is called the "status line" in response.
 *                    A typical status line looks like: HTTP/1.1 404 Not Found.
 *                  - HTTP headers for responses follow the same structure as any other header:
 *                    a case-insensitive string followed by a colon (':') and a value whose
 *                    structure depends upon the type of the header.
 *                  - The last part of a response is the body. Not all responses have one:
 *                    responses with a status code that sufficiently answers the request without
 *                    the need for corresponding payload (like 201 Created or 204 No Content) usually don't.
 *
 * @param[in]       RawResponseBuffer - The raw bytes of the http response.
 * @param[in,out]   ParsedResponse    - The parsed http response.
 *
 * @return          A proper NTSTATUS error code.
 *
 * @note            If the RawResponseBuffer does not contain all headers, and another receive is required,
 *                  STATUS_MORE_PROCESSING_REQUIRED is returned.
 */
_Must_inspect_result_
NTSTATUS
ParseHttpResponse(
    _In_ _Const_ const xpf::SharedPointer<xpf::Buffer>& RawResponseBuffer,
    _Inout_ xpf::http::HttpResponse* ParsedResponse
) noexcept(true);

/**
 * @brief           Creates an http connection to the specified URL attempting to download a binary.
 *                  Follows the redirects and ensures the connection is established.
 *                  Appends the application/octet-stream header. Uses a GET method.
 *
 *                  It is the caller responsibility to call HttpContinueDownload on the client connection after this method
 *                  to ensure that the binary is completly downloaded.
 *
 * @param[in]       Url              - The url to be used.
 * @param[in]       HeaderItems      - The header items to be included in the request.
 *                                     This is a variable sized array.
 * @param[in]       HeaderItemsCount - The number of elements in the HeaderItems array.
 * @param[in,out]   ParsedResponse   - The initial response containing part of the object and the
 *                                     initial headers.
 * @param[in,out]   ClientConnection - A newly established socket connection.
 *
 * @return          A proper NTSTATUS error code.
 *
 * @note            This preservese the client connection and parsed response allocators.
 */
_Must_inspect_result_
NTSTATUS
InitiateHttpDownload(
    _In_ _Const_ const xpf::StringView<char>& Url,
    _In_opt_ _Const_ const HeaderItem* HeaderItems,
    _In_ _Const_ size_t HeaderItemsCount,
    _Inout_ xpf::http::HttpResponse* ParsedResponse,
    _Inout_ xpf::SharedPointer<xpf::IClient>& ClientConnection
) noexcept(true);

/**
 * @brief           Continues a previously opened download over a connection.
 *
 * @param[in]       ClientConnection - A previously opened connection.
 * @param[in,out]   ParsedResponse -  Updates the buffer and the body. The other headers are discarded.
 * @param[out]      HasMoreData - A boolean indicating that we still need to call HttpContinueDownload.
 *                                If true, subsequent calls are required.
 *
 * @return          A proper NTSTATUS error code.
 */
_Must_inspect_result_
NTSTATUS
HttpContinueDownload(
    _Inout_ xpf::SharedPointer<xpf::IClient>& ClientConnection,     // NOLINT(*)
    _Inout_ xpf::http::HttpResponse* ParsedResponse,                // NOLINT(*)
    _Out_ bool* HasMoreData                                         // NOLINT(*)
) noexcept(true);
};  // namespace http
};  // namespace xpf
