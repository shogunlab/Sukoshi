# Output of thing name
output "implant_thing_name" {
    value = "${var.implant_name}"
}

# Command lines to connect the implant to the AWS IoT endpoint
output "python_implant_command_line" {
    value = "python ./python/implant.py --endpoint ${data.aws_iot_endpoint.endpoint.endpoint_address} --cert terraform/certs/${var.implant_name}.cert.pem --key terraform/certs/${var.implant_name}.private.key --client-id ${var.implant_client_id} --port 443"
}

output "cpp_implant_command_line" {
    value = "./cpp/Sukoshi/x64/Release/Sukoshi.exe --endpoint ${data.aws_iot_endpoint.endpoint.endpoint_address} --cert terraform/certs/${var.implant_name}.cert.pem --key terraform/certs/${var.implant_name}.private.key --client-id ${var.implant_client_id} --port 443"
}
