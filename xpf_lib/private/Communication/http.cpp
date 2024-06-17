/**
 * @file        xpf_lib/private/Communication/http.cpp
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

#include "xpf_lib/xpf.hpp"

/**
 * @brief   All code is paged out as we don't expect it to be run at higher irqls.
 */
XPF_SECTION_PAGED;

/**
 * @brief   The specification is explicit about CRLF being the standard.
 */
static constexpr const char gHttpHeaderLineEnding[] = "\r\n";

/**
 * @brief   The specification is explicit about : being the separator
 */
static constexpr const char gHttpHeaderSeparator[] = ":";

/**
 * @brief   The textual representation of supported HTTP versions.
 */
static constexpr const xpf::http::HttpVersionMap gHttpSupportedVersions[] =
{
    { xpf::http::HttpVersion::Http1_0, "HTTP/1.0"},
    { xpf::http::HttpVersion::Http1_1, "HTTP/1.1"},
};

/**
 * @brief   The textual representation of supported HTTP error codes.
 *          See https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
 */
static constexpr const xpf::http::HttpStatusMap gHttpStatusCodes[] =
{
    // Informational
    { 100,      "100" },     // Continue
    { 101,      "101" },     // Switching Protocols
    { 102,      "102" },     // Processing
    { 103,      "103" },     // Early Hints

    // Success
    { 200,      "200" },     // OK
    { 201,      "201" },     // Created
    { 202,      "202" },     // Accepted
    { 203,      "203" },     // Non-Authoritative Information
    { 204,      "204" },     // No Content
    { 205,      "205" },     // Reset Content
    { 206,      "206" },     // Partial Content
    { 207,      "207" },     // Multi Status
    { 208,      "208" },     // Already Reported
    { 226,      "226" },     // IM Used

    // Redirection
    { 300,      "300" },     // Multiple Choices
    { 301,      "301" },     // Moved Permanently
    { 302,      "302" },     // Found
    { 303,      "303" },     // See Other
    { 304,      "304" },     // Not Modified
    { 305,      "305" },     // Use Proxy
    { 306,      "306" },     // Reserved
    { 307,      "307" },     // Temporary Redirect
    { 308,      "308" },     // Permanent Redirect

    // Client Errors
    { 400,      "400" },     // Bad Request
    { 401,      "401" },     // Unauthorised
    { 402,      "402" },     // Payment Required
    { 403,      "403" },     // Forbidden
    { 404,      "404" },     // Not Found
    { 405,      "405" },     // Method Not Allowed
    { 406,      "406" },     // Not Acceptable
    { 407,      "407" },     // Proxy Authentication Required
    { 408,      "408" },     // Request Timeout
    { 409,      "409" },     // Conflict
    { 410,      "410" },     // Gone
    { 411,      "411" },     // Length Required
    { 412,      "412" },     // Precondition Failed
    { 413,      "413" },     // Payload Too Large
    { 414,      "414" },     // URI Too Long
    { 415,      "415" },     // Unsupported Media Type
    { 416,      "416" },     // Range Not Satisfiable
    { 417,      "417" },     // Expectation Failed
    { 418,      "418" },     // I'm a teapot
    { 421,      "421" },     // Misdirected Request
    { 422,      "422" },     // Unprocessable Content
    { 423,      "423" },     // Locked
    { 424,      "424" },     // Failed Dependency
    { 425,      "425" },     // Too Early
    { 426,      "426" },     // Upgrade Required
    { 428,      "428" },     // Precondition Required
    { 429,      "429" },     // Too Many Requests
    { 431,      "431" },     // Request Header Fields Too Large
    { 451,      "451" },     // Unavailable For Legal Reasons

    // Server Error
    { 500,      "500" },     // Internal Server Error
    { 501,      "501" },     // Not Implemented
    { 502,      "502" },     // Bad Gateway
    { 503,      "503" },     // Service Unavailable
    { 504,      "504" },     // Gateway Timeout
    { 505,      "505" },     // HTTP Version Not Supported
    { 506,      "506" },     // Variant Also Negotiates
    { 507,      "507" },     // Insufficient Storage
    { 508,      "508" },     // Loop Detected
    { 510,      "510" },     // Not Extended
    { 511,      "511" },     // Network Authentication Required
};

/**
 * @brief       Checks if a character is whitespace (' ' or '\t')
 *
 * @param[in]   Character to be checked if it's whitespace.
 *
 * @return      true if Character is considered whitespace,
 *              false otherwise.
 */
static inline bool
HttpIsWhitespace(
    _In_ const char Character
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();
    return (Character == ' ') || (Character == '\t');
}

/**
 * @brief           This is used to remove whitespaces from the beginning and
 *                  from the end of the string. Makes the parsing easier.
 *                  This can be used to remove the line ending as well (gHttpHeaderLineEnding).
 *
 * @param[in,out]   Line to remove the whitespaces from.
 *
 * @return          Nothing.
 */
static inline void
HttpTrimWhitespaces(
    _Inout_ xpf::StringView<char>& String
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    if (String.EndsWith(gHttpHeaderLineEnding, false))
    {
        String.RemoveSuffix(xpf::StringView<char>(gHttpHeaderLineEnding).BufferSize());
    }

    while (!String.IsEmpty() && HttpIsWhitespace(String[0]))
    {
        String.RemovePrefix(1);
    }

    while (!String.IsEmpty() && HttpIsWhitespace(String[String.BufferSize() - 1]))
    {
        String.RemoveSuffix(1);
    }
}

/**
 * @brief           This will retrieve the next line from the response.
 *                  Basically checks for gHttpHeaderLineEnding (CR LF).
 *
 * @param[in,out]   Response - The current response buffer.
 *                             On output this skips over the current line, so the
 *                             cursor is advanced.
 *
 * @return          A view over Response which contains the current line.
 *                  It's empty if no CR LF is found. On output it contains
 *                  the line ending.
 */
static inline xpf::StringView<char>
HttpNextLine(
    _Inout_ xpf::StringView<char>& Response
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    size_t index = 0;
    xpf::StringView<char> line;

    if (Response.Substring(gHttpHeaderLineEnding, false, &index))
    {
        line = xpf::StringView<char>{ Response.Buffer(),
                                      index + xpf::StringView<char>(gHttpHeaderLineEnding).BufferSize() };
        Response.RemovePrefix(line.BufferSize());
    }
    return line;
}

/**
 * @brief           This parses the first line of the http response, also called the status line.
 *                  A typical status line looks like: HTTP/1.1 404 Not Found.
 *
 * @param[in]       StatusLine      - The first line of the http response
 * @param[in,out]   ParsedResponse  - Will be filled with data from the Status line.
 *
 * @return          A proper NTSTATUS error code.
 */
static NTSTATUS
HttpParseStatusLine(
    _In_ _Const_ const xpf::StringView<char>& StatusLine,
    _Inout_ xpf::http::HttpResponse& ParsedResponse
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    xpf::StringView<char> line = StatusLine;

    /* First the http version. */
    HttpTrimWhitespaces(line);
    status = STATUS_NOT_FOUND;
    for (const auto& supportedHttpVersion : gHttpSupportedVersions)
    {
        if (line.StartsWith(supportedHttpVersion.Text, true))
        {
            ParsedResponse.Version = supportedHttpVersion.Version;
            line.RemovePrefix(xpf::StringView<char>(supportedHttpVersion.Text).BufferSize());

            status = STATUS_SUCCESS;
            break;
        }
    }
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Now the status code. */
    HttpTrimWhitespaces(line);
    status = STATUS_NOT_FOUND;
    for (const auto& code : gHttpStatusCodes)
    {
        if (line.StartsWith(code.Text, true))
        {
            ParsedResponse.HttpStatusCode = code.Status;
            line.RemovePrefix(xpf::StringView<char>(code.Text).BufferSize());

            status = STATUS_SUCCESS;
            break;
        }
    }

    /* Now the tesxt error. This is the leftover. */
    HttpTrimWhitespaces(line);
    ParsedResponse.HttpStatusMessage = line;

    return STATUS_SUCCESS;
}

/**
 * @brief           This parses a typical line containing a header.
 *                  Headers lines contain <key> : <value>.
 *                  This will split the line on ':' and update the ParsedResponse.
 *
 * @param[in]       HeaderLine      - A line containing a header.
 * @param[in,out]   ParsedResponse  - Will be filled with data from the Header line.
 *
 * @return          A proper NTSTATUS error code.
 */
static NTSTATUS
HttpParseHeaderLine(
    _In_ _Const_ const xpf::StringView<char>& HeaderLine,
    _Inout_ xpf::http::HttpResponse& ParsedResponse
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    xpf::http::HeaderItem headerItem;
    size_t index = 0;

    if (!HeaderLine.Substring(":", false, &index))
    {
        return STATUS_NOT_FOUND;
    }

    headerItem.Key = { HeaderLine.Buffer(), index };
    HttpTrimWhitespaces(headerItem.Key);

    headerItem.Value = HeaderLine;
    headerItem.Value.RemovePrefix(index + 1);
    HttpTrimWhitespaces(headerItem.Value);

    return ParsedResponse.Headers.Emplace(xpf::Move(headerItem));
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
xpf::http::ParseHttpResponse(
    _In_ _Const_ xpf::SharedPointer<xpf::Buffer<>>& RawResponseBuffer,
    _Inout_ xpf::http::HttpResponse& ParsedResponse
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    xpf::StringView<char> rawResponse{ static_cast<const char*>((*RawResponseBuffer).GetBuffer()),
                                       (*RawResponseBuffer).GetSize(), };

    xpf::StringView<char> currentLine = HttpNextLine(rawResponse);

    ParsedResponse.ResponseBuffer = RawResponseBuffer;

    /* The status line first. */
    if (currentLine.IsEmpty())
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    status = HttpParseStatusLine(currentLine, ParsedResponse);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* Now the headers. */
    ParsedResponse.Headers.Clear();
    while (true)
    {
        /* No more line endings found. */
        currentLine = HttpNextLine(rawResponse);
        if (currentLine.IsEmpty())
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* The current line is just a line ending. So we finished the headers. */
        if (currentLine.Equals(gHttpHeaderLineEnding, false))
        {
            break;
        }

        /* Parse the current line. */
        status = HttpParseHeaderLine(currentLine, ParsedResponse);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    /* And the body is the rest of the message. */
    ParsedResponse.Body = rawResponse;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
xpf::http::BuildHttpRequest(
    _In_ _Const_ const xpf::StringView<char>& Host,
    _In_ _Const_ const xpf::StringView<char>& Method,
    _In_ _Const_ const xpf::StringView<char>& ResourcePath,
    _In_ _Const_ const xpf::StringView<char>& Parameters,
    _In_ _Const_ const xpf::http::HttpVersion& Version,
    _In_opt_ _Const_ const HeaderItem* HeaderItems,
    _In_ _Const_ size_t HeaderItemsCount,
    _Inout_ xpf::String<char>& Request
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Request.Reset();

    /**
     * @brief Helper macro to append data to a request.
     */
    #define HTTP_REQUEST_APPEND(Request, Data)  \
    {                                           \
        status = Request.Append(Data);          \
        if (!NT_SUCCESS(status))                \
        {                                       \
            return status;                      \
        }                                       \
    }

    /* GET */
    HTTP_REQUEST_APPEND(Request, Method);
    HTTP_REQUEST_APPEND(Request, " ");
    /* GET  /foobar/otherbar/somepage */
    HTTP_REQUEST_APPEND(Request, ResourcePath);
    /* GET  /foobar/otherbar/somepage?arg1=val1&arg2=val2 */
    HTTP_REQUEST_APPEND(Request, Parameters);

    /* GET  /foobar/otherbar/somepage?arg1=val1&arg2=val2 HTTP/1.1*/
    status = STATUS_NOT_FOUND;
    for (const auto& item : gHttpSupportedVersions)
    {
        if (item.Version == Version)
        {
            HTTP_REQUEST_APPEND(Request, " ");
            HTTP_REQUEST_APPEND(Request, item.Text);
            break;
        }
    }
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    HTTP_REQUEST_APPEND(Request, gHttpHeaderLineEnding);

    /* Now the header - first the HOST. */
    HTTP_REQUEST_APPEND(Request, "Host:");
    HTTP_REQUEST_APPEND(Request, Host);
    HTTP_REQUEST_APPEND(Request, gHttpHeaderLineEnding);

    /* Now the other header items*/
    if (nullptr != HeaderItems)
    {
        for (size_t i = 0; i < HeaderItemsCount; ++i)
        {
            HTTP_REQUEST_APPEND(Request, HeaderItems[i].Key);
            HTTP_REQUEST_APPEND(Request, ":");
            HTTP_REQUEST_APPEND(Request, HeaderItems[i].Value);
            HTTP_REQUEST_APPEND(Request, gHttpHeaderLineEnding);
        }
    }

    /* End the header. */
    HTTP_REQUEST_APPEND(Request, gHttpHeaderLineEnding);

    #undef HTTP_REQUEST_APPEND
    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
xpf::http::ParseUrlInformation(
    _In_ _Const_ const xpf::StringView<char>& Url,
    _Inout_ xpf::http::UrlInfo& UrlInformation
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /**
     * @brief   Helper macro to assign an url part to url information.
     */
    #define HTTP_URL_PART_ASSIGN(UrlPart, View)         \
    {                                                   \
        UrlPart.Reset();                                \
        status = UrlPart.Append(View);                  \
        if (!NT_SUCCESS(status))                        \
        {                                               \
            return status;                              \
        }                                               \
    }

    /* Save the original URL.*/
    HTTP_URL_PART_ASSIGN(UrlInformation.Url, Url);

    xpf::StringView url = UrlInformation.Url.View();
    size_t index = 0;

    /* Do the parsing in reverse - anchor first. */
    if (url.Substring("#", false, &index))
    {
        xpf::StringView<char> anchor = Url;
        anchor.RemovePrefix(index);
        url.RemoveSuffix(anchor.BufferSize());

        HTTP_URL_PART_ASSIGN(UrlInformation.Anchor, anchor);
    }

    /* Do the parsing in reverse - parameters second. */
    if (url.Substring("?", false, &index))
    {
        xpf::StringView<char> parameters = url;
        parameters.RemovePrefix(index);
        url.RemoveSuffix(parameters.BufferSize());

        HTTP_URL_PART_ASSIGN(UrlInformation.Parameters, parameters);
    }

    /* Do the parsing in reverse - path third . */
    if (url.Substring("://", false, &index))
    {
        xpf::StringView<char> path = url;
        path.RemovePrefix(index);
        url.RemoveSuffix(path.BufferSize());
        path.RemovePrefix(3);    // Skip over "://"

        /* Now we skip over the domain from path. */
        if (path.Substring("/", false, &index))
        {
            xpf::StringView<char> authority = path;
            authority.RemoveSuffix(path.BufferSize() - index);
            path.RemovePrefix(authority.BufferSize());

            HTTP_URL_PART_ASSIGN(UrlInformation.Authority, authority);
        }
        HTTP_URL_PART_ASSIGN(UrlInformation.Path, path);

        /* We also need to separate the domain and the port, if any. */
        xpf::StringView<char> domain = UrlInformation.Authority.View();

        if (domain.Substring(":", false, &index))
        {
            xpf::StringView<char> port = domain;

            port.RemovePrefix(index);
            domain.RemoveSuffix(port.BufferSize());

            port.RemovePrefix(1);    // Skip over ":"
            HTTP_URL_PART_ASSIGN(UrlInformation.Port, port);
        }
        HTTP_URL_PART_ASSIGN(UrlInformation.Domain, domain);
    }

    /* The scheme is what is left. */
    HTTP_URL_PART_ASSIGN(UrlInformation.Scheme, url);

    #undef HTTP_URL_PART_ASSIGN
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
xpf::http::InitiateHttpDownload(
    _In_ _Const_ const xpf::StringView<char>& Url,
    _In_opt_ _Const_ const HeaderItem* HeaderItems,
    _In_ _Const_ size_t HeaderItemsCount,
    _Inout_ xpf::http::HttpResponse& ParsedResponse,
    _Inout_ xpf::SharedPointer<xpf::IClient>& ClientConnection
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* Hardcode the number redirects to follow. Can be changed later if needed. */
    size_t maximumRedirectsAllowed = 5;

    /* Start with the provided URL. Will change if redirects are required. */
    xpf::StringView<char> url = Url;
    xpf::http::UrlInfo urlInfo;

    /* The client socket we get during the connection. */
    xpf::SharedPointer<xpf::ClientSocket> clientSocket;

    while (maximumRedirectsAllowed > 0)
    {
        /* Parsing the URL. */
        status = xpf::http::ParseUrlInformation(url,
                                                urlInfo);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Now select the domain-port based on url info. */
        xpf::StringView<char> domain = urlInfo.Domain.View();
        xpf::StringView<char> port = urlInfo.Port.View();

        /* If port is missing, select it based on scheme.*/
        if (port.IsEmpty())
        {
            port = urlInfo.Scheme.View().StartsWith("https", false) ? "443"
                                                                    : "80";
        }
        const bool isTlsSocket = port.StartsWith("443", true);

        /* Now create the socket. */
        clientSocket = xpf::MakeShared<ClientSocket>(domain,
                                                     port,
                                                     isTlsSocket);
        if (clientSocket.IsEmpty())
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Now build the http request. */
        xpf::String<char> request;
        status = xpf::http::BuildHttpRequest(urlInfo.Domain.View(),
                                             "GET",
                                             urlInfo.Path.View(),
                                             urlInfo.Parameters.View(),
                                             xpf::http::HttpVersion::Http1_1,
                                             HeaderItems,
                                             HeaderItemsCount,
                                             request);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Now connect to the socket. */
        status = (*clientSocket).Connect();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* And send the request. */
        const void* requestBuffer = request.View().Buffer();
        status = (*clientSocket).SendData(request.BufferSize(),
                                          static_cast<const uint8_t*>(requestBuffer));
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Ensure the response buffer is big enough. */
        xpf::SharedPointer<xpf::Buffer<>> sharedBuffer = xpf::MakeShared<xpf::Buffer<>>();
        if (sharedBuffer.IsEmpty())
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        status = (*sharedBuffer).Resize(4096 * 5);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        /* Get the response. */
        void* responseBuffer = (*sharedBuffer).GetBuffer();
        size_t responseBufferSize = (*sharedBuffer).GetSize();
        status = (*clientSocket).ReceiveData(&responseBufferSize,
                                             static_cast<uint8_t*>(responseBuffer));
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        status = (*sharedBuffer).Resize(responseBufferSize);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        status = xpf::http::ParseHttpResponse(sharedBuffer,
                                              ParsedResponse);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (ParsedResponse.HttpStatusCode < 300 || ParsedResponse.HttpStatusCode > 399)
        {
            /* If we don't have a redirect, we're done. */
            break;
        }
        else
        {
            /* Here we go again. Grab the new URL */
            bool foundRedirect = false;
            for (size_t i = 0; i < ParsedResponse.Headers.Size(); ++i)
            {
                if (ParsedResponse.Headers[i].Key.Equals("Location", false))
                {
                    url = ParsedResponse.Headers[i].Value;
                    foundRedirect = true;
                    break;
                }
            }
            if (!foundRedirect)
            {
                return STATUS_INVALID_MESSAGE;
            }
        }

        maximumRedirectsAllowed--;
    }

    if (ParsedResponse.HttpStatusCode != 200)
    {
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    /* Got an original connection. return it. */
    ClientConnection = xpf::DynamicSharedPointerCast<xpf::IClient, xpf::ClientSocket>(clientSocket);
    return (ClientConnection.IsEmpty()) ? STATUS_INSUFFICIENT_RESOURCES
                                        : STATUS_SUCCESS;
}

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
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
xpf::http::HttpContinueDownload(
    _In_ xpf::SharedPointer<xpf::IClient>& ClientConnection,
    _Inout_ xpf::http::HttpResponse& ParsedResponse,
    _Out_ bool* HasMoreData
) noexcept(true)
{
    XPF_MAX_PASSIVE_LEVEL();

    XPF_DEATH_ON_FAILURE(!ClientConnection.IsEmpty());
    XPF_DEATH_ON_FAILURE(!ParsedResponse.ResponseBuffer.IsEmpty());
    XPF_DEATH_ON_FAILURE(nullptr != HasMoreData);

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* Invalidate the response. We are going to recycle the buffer. */
    ParsedResponse.HttpStatusCode = 0;
    ParsedResponse.HttpStatusMessage.Reset();
    ParsedResponse.Headers.Clear();
    ParsedResponse.Body.Reset();

    /* Set output parameters. */
    *HasMoreData = false;

    /* Will be used to check whether we have more data or not. */
    size_t availableBufferSize = (*ParsedResponse.ResponseBuffer).GetSize();
    void* availableBuffer = (*ParsedResponse.ResponseBuffer).GetBuffer();

    /* Receive more data. */
    status = (*ClientConnection).ReceiveData(&availableBufferSize,
                                             static_cast<uint8_t*>(availableBuffer));
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    /* We maxed out. */
    if (availableBufferSize == (*ParsedResponse.ResponseBuffer).GetSize())
    {
        *HasMoreData = true;
    }

    /* Set the body properly. */
    if (availableBufferSize > 0)
    {
        ParsedResponse.Body = xpf::StringView{ static_cast<const char*>(availableBuffer),
                                              availableBufferSize };
    }
    return STATUS_SUCCESS;
}
