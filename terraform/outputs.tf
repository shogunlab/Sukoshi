# Output of thing name
output "implant_thing_name" {
    value = "${var.implant_name}"
}

# Command line to connect the implant to the AWS IoT endpoint
output "implant_command_line" {
    value = "python implant.py --endpoint ${data.aws_iot_endpoint.endpoint.endpoint_address} --cert terraform/certs/${var.implant_name}.cert.pem --key terraform/certs/${var.implant_name}.private.key --client-id ${var.implant_client_id} --port 443"
}
