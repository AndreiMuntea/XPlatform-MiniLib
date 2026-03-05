/**
 * @file        xpf_tests/tests/Communication/TestHttp.cpp
 *
 * @brief       This contains tests for HTTP parser implementation.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright © Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"


//
// ************************************************************************************************
// URL Parsing tests
// ************************************************************************************************
//

/**
 * @brief       This tests parsing a full URL with all components.
 */
XPF_TEST_SCENARIO(TestHttp, ParseUrlFull)
{
    xpf::http::UrlInfo urlInfo;
    NTSTATUS status = xpf::http::ParseUrlInformation("https://www.example.com:8080/path/to/resource?key=val#section",
                                                     urlInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(urlInfo.Scheme.View().Equals("https", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Domain.View().Equals("www.example.com", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Port.View().Equals("8080", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Path.View().Equals("/path/to/resource", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Parameters.View().Equals("?key=val", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Anchor.View().Equals("#section", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Authority.View().Equals("www.example.com:8080", true));
}

/**
 * @brief       This tests parsing a minimal URL with no port, no params, no anchor.
 */
XPF_TEST_SCENARIO(TestHttp, ParseUrlMinimal)
{
    xpf::http::UrlInfo urlInfo;
    NTSTATUS status = xpf::http::ParseUrlInformation("http://example.com/index.html",
                                                     urlInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(urlInfo.Scheme.View().Equals("http", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Domain.View().Equals("example.com", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Port.View().IsEmpty());
    XPF_TEST_EXPECT_TRUE(urlInfo.Path.View().Equals("/index.html", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Parameters.View().IsEmpty());
    XPF_TEST_EXPECT_TRUE(urlInfo.Anchor.View().IsEmpty());
}

/**
 * @brief       This tests parsing a URL with only a root path.
 */
XPF_TEST_SCENARIO(TestHttp, ParseUrlNoPath)
{
    xpf::http::UrlInfo urlInfo;
    NTSTATUS status = xpf::http::ParseUrlInformation("https://example.com/",
                                                     urlInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(urlInfo.Scheme.View().Equals("https", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Domain.View().Equals("example.com", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Path.View().Equals("/", true));
}

/**
 * @brief       This tests parsing a URL with parameters but no anchor.
 */
XPF_TEST_SCENARIO(TestHttp, ParseUrlWithParametersNoAnchor)
{
    xpf::http::UrlInfo urlInfo;
    NTSTATUS status = xpf::http::ParseUrlInformation("http://api.example.com/v1/users?page=2&limit=50",
                                                     urlInfo);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(urlInfo.Scheme.View().Equals("http", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Domain.View().Equals("api.example.com", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Path.View().Equals("/v1/users", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Parameters.View().Equals("?page=2&limit=50", true));
    XPF_TEST_EXPECT_TRUE(urlInfo.Anchor.View().IsEmpty());
}


//
// ************************************************************************************************
// Request Building tests
// ************************************************************************************************
//

/**
 * @brief       This tests building a simple GET request.
 */
XPF_TEST_SCENARIO(TestHttp, BuildHttpRequestGet)
{
    xpf::String<char> request;
    NTSTATUS status = xpf::http::BuildHttpRequest("www.example.com",
                                                  "GET",
                                                  "/index.html",
                                                  "",
                                                  xpf::http::HttpVersion::Http1_1,
                                                  nullptr,
                                                  0,
                                                  request);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Verify the request line contains the method, path and version.
    //
    xpf::StringView<char> view = request.View();
    size_t idx = 0;
    XPF_TEST_EXPECT_TRUE(view.Substring("GET /index.html HTTP/1.1\r\n", true, &idx));
    XPF_TEST_EXPECT_TRUE(view.Substring("Host:www.example.com\r\n", true, &idx));

    //
    // Should end with trailing \r\n\r\n (headers end).
    //
    XPF_TEST_EXPECT_TRUE(view.EndsWith("\r\n\r\n", true));
}

/**
 * @brief       This tests building a GET request with custom headers.
 */
XPF_TEST_SCENARIO(TestHttp, BuildHttpRequestWithHeaders)
{
    xpf::http::HeaderItem headers[2];
    headers[0].Key = "Accept";
    headers[0].Value = "text/html";
    headers[1].Key = "User-Agent";
    headers[1].Value = "XPF-Test/1.0";

    xpf::String<char> request;
    NTSTATUS status = xpf::http::BuildHttpRequest("www.example.com",
                                                  "GET",
                                                  "/page",
                                                  "",
                                                  xpf::http::HttpVersion::Http1_1,
                                                  headers,
                                                  2,
                                                  request);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StringView<char> view = request.View();
    size_t idx = 0;
    XPF_TEST_EXPECT_TRUE(view.Substring("Accept:text/html\r\n", true, &idx));
    XPF_TEST_EXPECT_TRUE(view.Substring("User-Agent:XPF-Test/1.0\r\n", true, &idx));
}

/**
 * @brief       This tests building an HTTP/1.0 request.
 */
XPF_TEST_SCENARIO(TestHttp, BuildHttpRequestHttp10)
{
    xpf::String<char> request;
    NTSTATUS status = xpf::http::BuildHttpRequest("www.example.com",
                                                  "GET",
                                                  "/",
                                                  "",
                                                  xpf::http::HttpVersion::Http1_0,
                                                  nullptr,
                                                  0,
                                                  request);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StringView<char> view = request.View();
    size_t idx = 0;
    XPF_TEST_EXPECT_TRUE(view.Substring("HTTP/1.0", true, &idx));
}


//
// ************************************************************************************************
// Response Parsing tests
// ************************************************************************************************
//

/**
 * @brief       This tests parsing a complete 200 OK response with headers and body.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseOk)
{
    const char raw[] = "HTTP/1.1 200 OK\r\n"
                       "Content-Type:text/html\r\n"
                       "Content-Length:13\r\n"
                       "\r\n"
                       "Hello, World!";

    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    NTSTATUS status = (*buf).Resize(sizeof(raw) - 1);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    xpf::ApiCopyMemory((*buf).GetBuffer(), raw, sizeof(raw) - 1);

    xpf::http::HttpResponse response;
    status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(xpf::http::HttpVersion::Http1_1 == response.Version);
    XPF_TEST_EXPECT_TRUE(size_t{ 200 } == response.HttpStatusCode);
    XPF_TEST_EXPECT_TRUE(response.HttpStatusMessage.Equals("OK", true));

    //
    // Verify headers.
    //
    XPF_TEST_EXPECT_TRUE(size_t{ 2 } == response.Headers.Size());

    //
    // Verify body.
    //
    XPF_TEST_EXPECT_TRUE(response.Body.Equals("Hello, World!", true));
}

/**
 * @brief       This tests parsing a 404 response with empty body.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseNotFound)
{
    const char raw[] = "HTTP/1.1 404 Not Found\r\n"
                       "Content-Length:0\r\n"
                       "\r\n";

    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    NTSTATUS status = (*buf).Resize(sizeof(raw) - 1);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    xpf::ApiCopyMemory((*buf).GetBuffer(), raw, sizeof(raw) - 1);

    xpf::http::HttpResponse response;
    status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(size_t{ 404 } == response.HttpStatusCode);
    XPF_TEST_EXPECT_TRUE(response.HttpStatusMessage.Equals("Not Found", true));
}

/**
 * @brief       This tests that an incomplete response (no \r\n\r\n) returns STATUS_MORE_PROCESSING_REQUIRED.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseIncomplete)
{
    const char raw[] = "HTTP/1.1 200 OK\r\n"
                       "Content-Type:text/html\r\n";

    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    NTSTATUS status = (*buf).Resize(sizeof(raw) - 1);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    xpf::ApiCopyMemory((*buf).GetBuffer(), raw, sizeof(raw) - 1);

    xpf::http::HttpResponse response;
    status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(STATUS_MORE_PROCESSING_REQUIRED == status);
}

/**
 * @brief       This tests that an empty buffer returns STATUS_MORE_PROCESSING_REQUIRED.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseEmptyBuffer)
{
    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    xpf::http::HttpResponse response;
    NTSTATUS status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(STATUS_MORE_PROCESSING_REQUIRED == status);
}

/**
 * @brief       This tests parsing an HTTP/1.0 301 response.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseHttp10)
{
    const char raw[] = "HTTP/1.0 301 Moved Permanently\r\n"
                       "Location:https://www.example.com/\r\n"
                       "\r\n";

    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    NTSTATUS status = (*buf).Resize(sizeof(raw) - 1);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    xpf::ApiCopyMemory((*buf).GetBuffer(), raw, sizeof(raw) - 1);

    xpf::http::HttpResponse response;
    status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    XPF_TEST_EXPECT_TRUE(xpf::http::HttpVersion::Http1_0 == response.Version);
    XPF_TEST_EXPECT_TRUE(size_t{ 301 } == response.HttpStatusCode);
}

/**
 * @brief       This tests parsing a response with multiple headers.
 */
XPF_TEST_SCENARIO(TestHttp, ParseResponseMultipleHeaders)
{
    const char raw[] = "HTTP/1.1 200 OK\r\n"
                       "Content-Type:text/html\r\n"
                       "Content-Length:4\r\n"
                       "Server:XPF-Server\r\n"
                       "X-Custom:custom-value\r\n"
                       "\r\n"
                       "test";

    xpf::SharedPointer<xpf::Buffer> buf = xpf::MakeShared<xpf::Buffer>();
    XPF_TEST_EXPECT_TRUE(!buf.IsEmpty());

    NTSTATUS status = (*buf).Resize(sizeof(raw) - 1);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    xpf::ApiCopyMemory((*buf).GetBuffer(), raw, sizeof(raw) - 1);

    xpf::http::HttpResponse response;
    status = xpf::http::ParseHttpResponse(buf, &response);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    //
    // Verify we have 4 headers.
    //
    XPF_TEST_EXPECT_TRUE(size_t{ 4 } == response.Headers.Size());

    //
    // Find each header by key.
    //
    bool foundContentType = false;
    bool foundContentLength = false;
    bool foundServer = false;
    bool foundCustom = false;

    for (size_t i = 0; i < response.Headers.Size(); ++i)
    {
        if (response.Headers[i].Key.Equals("Content-Type", true))
        {
            XPF_TEST_EXPECT_TRUE(response.Headers[i].Value.Equals("text/html", true));
            foundContentType = true;
        }
        else if (response.Headers[i].Key.Equals("Content-Length", true))
        {
            XPF_TEST_EXPECT_TRUE(response.Headers[i].Value.Equals("4", true));
            foundContentLength = true;
        }
        else if (response.Headers[i].Key.Equals("Server", true))
        {
            XPF_TEST_EXPECT_TRUE(response.Headers[i].Value.Equals("XPF-Server", true));
            foundServer = true;
        }
        else if (response.Headers[i].Key.Equals("X-Custom", true))
        {
            XPF_TEST_EXPECT_TRUE(response.Headers[i].Value.Equals("custom-value", true));
            foundCustom = true;
        }
    }

    XPF_TEST_EXPECT_TRUE(foundContentType);
    XPF_TEST_EXPECT_TRUE(foundContentLength);
    XPF_TEST_EXPECT_TRUE(foundServer);
    XPF_TEST_EXPECT_TRUE(foundCustom);
}
