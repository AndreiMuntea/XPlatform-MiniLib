/**
 * @file        xpf_tests/tests/Communication/TestSocketClientServer.cpp
 *
 * @brief       This contains tests for client-server implementation using sockets.
 *
 * @author      Andrei-Marius MUNTEA (munteaandrei17@gmail.com)
 *
 * @copyright   Copyright � Andrei-Marius MUNTEA 2020-2023.
 *              All rights reserved.
 *
 * @license     See top-level directory LICENSE file.
 */


#include "xpf_tests/XPF-TestIncludes.hpp"


struct MockServerThreadContext
{
    xpf::Optional<xpf::Signal> RunningEvent;
    xpf::SharedPointer<xpf::IServer> Server;

    NTSTATUS ReturnStatus = STATUS_UNSUCCESSFUL;
};

struct MockClientThreadContext
{
    xpf::Optional<xpf::Signal> RunningEvent;
    xpf::SharedPointer<xpf::IClient> Client;

    NTSTATUS ReturnStatus = STATUS_UNSUCCESSFUL;
};


/**
 * @brief       This is a mock callback used for testing server side.
 *
 * @param[in] Context - a MockServerThreadContext pointer.
 */
static void XPF_API
MockServerCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockServerThreadContext*>(Context);
    if (nullptr == mockContext)
    {
        return;
    }

    /* Start the server. */
    mockContext->ReturnStatus = (*mockContext->Server).Start();
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* The server thread is up & running. */
    (*(mockContext->RunningEvent)).Set();

    /* Accept a new client. */
    xpf::SharedPointer<xpf::IClientCookie> newClient;
    mockContext->ReturnStatus = (*mockContext->Server).AcceptClient(newClient);
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* Send a dummy buffer. */
    xpf::StringView<char> hello = "Hello there!";
    mockContext->ReturnStatus = (*mockContext->Server).SendData(hello.BufferSize(),
                                                                reinterpret_cast<const uint8_t*>(hello.Buffer()),
                                                                newClient);
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* Receive a dummy response. */
    char response[256];
    xpf::ApiZeroMemory(response, sizeof(response));

    size_t recvDataSize = sizeof(response);
    mockContext->ReturnStatus = (*mockContext->Server).ReceiveData(&recvDataSize,
                                                                   reinterpret_cast<uint8_t*>(response),
                                                                   newClient);
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* Validate the response. */
    xpf::StringView<char> responseData(response, recvDataSize);
    mockContext->ReturnStatus = responseData.Equals("General Kenobi!", true) ? STATUS_SUCCESS
                                                                             : STATUS_DATA_ERROR;

    /* All good. */
    mockContext->ReturnStatus = STATUS_SUCCESS;
}

/**
 * @brief       This is a mock callback used for testing client side.
 *
 * @param[in] Context - a MockClientThreadContext pointer.
 */
static void XPF_API
MockClientCallback(
    _In_opt_ xpf::thread::CallbackArgument Context
) noexcept(true)
{
    auto mockContext = reinterpret_cast<MockClientThreadContext*>(Context);
    if (nullptr == mockContext)
    {
        return;
    }

    /* The client thread is up & running. */
    (*(mockContext->RunningEvent)).Set();

    /* Connect to the server. */
    mockContext->ReturnStatus = (*mockContext->Client).Connect();
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* Receive a dummy buffer. */
    char hello[256];
    xpf::ApiZeroMemory(hello, sizeof(hello));

    size_t recvDataSize = sizeof(hello);
    mockContext->ReturnStatus = (*mockContext->Client).ReceiveData(&recvDataSize,
                                                                   reinterpret_cast<uint8_t*>(hello));
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* Validate the hello buffer. */
    xpf::StringView<char> helloData(hello, recvDataSize);
    mockContext->ReturnStatus = helloData.Equals("Hello There!", true) ? STATUS_SUCCESS
                                                                       : STATUS_DATA_ERROR;

    /* Send a dummy buffer. */
    xpf::StringView<char> response = "General Kenobi!";
    mockContext->ReturnStatus = (*mockContext->Client).SendData(response.BufferSize(),
                                                                reinterpret_cast<const uint8_t*>(response.Buffer()));
    if (!NT_SUCCESS(mockContext->ReturnStatus))
    {
        return;
    }

    /* All good. */
    mockContext->ReturnStatus = STATUS_SUCCESS;
}

/**
 * @brief       This tests the default constructor of client and server.
 */
XPF_TEST_SCENARIO(TestSocketClientServeir, DefaultConstructorDestructor)
{
    xpf::ServerSocket server("localhost", "27015");
    xpf::ClientSocket client("localhost", "27015");
}

/**
 * @brief       This tests the accept client and connect.
 *              Will send a dummy buffer from both client and server.
 */
XPF_TEST_SCENARIO(TestSocketClientServeir, SendReceive)
{
    xpf::thread::Thread serverThread;
    xpf::thread::Thread clientThread;

    MockServerThreadContext serverContext;
    MockClientThreadContext clientContext;

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = xpf::Signal::Create(&serverContext.RunningEvent, true);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    serverContext.Server = xpf::DynamicSharedPointerCast<xpf::IServer>(
                                            xpf::MakeShared<xpf::ServerSocket>("localhost", "27015"));
    XPF_TEST_EXPECT_TRUE(!serverContext.Server.IsEmpty());

    status = xpf::Signal::Create(&clientContext.RunningEvent, true);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    clientContext.Client = xpf::DynamicSharedPointerCast<xpf::IClient>(
                                            xpf::MakeShared<xpf::ClientSocket>("localhost", "27015"));
    XPF_TEST_EXPECT_TRUE(!serverContext.Server.IsEmpty());

    /* Spawn server thread. */
    status = serverThread.Run(MockServerCallback, &serverContext);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    (*serverContext.RunningEvent).Wait();

    /* Spawn client thread. */
    status = clientThread.Run(MockClientCallback, &clientContext);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
    (*clientContext.RunningEvent).Wait();

    /* Wait both. */
    clientThread.Join();
    serverThread.Join();

    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(serverContext.ReturnStatus));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(clientContext.ReturnStatus));
}

/**
 * @brief       This tests the connection to httpbin. An actual site.
 */
XPF_TEST_SCENARIO(TestSocketClientServeir, HttpBinRequest)
{
    //
    // Use https://httpbin.org/#/HTTP_Methods/get_get to connect to a real site.
    // We just want to see that the resolution of ip address works.
    // And we can get an 200 success response code.
    //
    xpf::ClientSocket client("httpbin.org", "80");
    xpf::StringView<char> httpGetRequest = "GET /uuid HTTP/1.1\r\n"
                                           "Host: httpbin.org\r\n"
                                           "Connection: close\r\n"
                                           "\r\n";
    NTSTATUS status = client.Connect();
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    status = client.SendData(httpGetRequest.BufferSize(),
                             reinterpret_cast<const uint8_t*>(httpGetRequest.Buffer()));
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::Buffer response;
    status = response.Resize(4096);
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::ApiZeroMemory(response.GetBuffer(), response.GetSize());
    size_t responseSize = response.GetSize();

    //
    // A sample of expected response:
    //
    // "HTTP/1.1 200 OK\r\n
    // Date: Mon, 30 Oct 2023 14:57:15 GMT\r\n
    // Content-Type: application/json\r\n
    // Content-Length: 53\r\n
    // Connection: close\r\n
    // Server: gunicorn/19.9.0\r\n
    // Access-Control-Allow-Origin: *\r\n
    // Access-Control-Allow-Credentials: true\r\n
    // \r\n
    // {\n  \"uuid\": \"54496741-7c79-4d38-...
    //
    status = client.ReceiveData(&responseSize,
                                response.GetBuffer());
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));

    xpf::StringView<char> actualResponse(reinterpret_cast<const char*>(response.GetBuffer()),
                                         responseSize);
    XPF_TEST_EXPECT_TRUE(actualResponse.Substring("HTTP/1.1 200 OK", true, nullptr));

    status = client.Disconnect();
    XPF_TEST_EXPECT_TRUE(NT_SUCCESS(status));
}