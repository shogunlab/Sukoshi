provider "aws" {
  profile    = "default"
  region     = "us-east-1"
}

# The AWS IoT thing
resource "aws_iot_thing" "sukoshi_iot_thing" {
  name = "${var.implant_name}"
}

# A policy for the implant that allows it to publish results/heartbeats, subscribe to and receive tasking.
# Only allows the specified client ID to connect.
resource "aws_iot_policy" "sukoshi_implant_policy" {
  name = "SukoshiImplantPolicy"

  policy = <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:RetainPublish"
      ],
      "Resource": [
        "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topic/c2/results",
        "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topic/c2/heartbeat"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Receive"
      ],
      "Resource": [
        "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topic/c2/tasking"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Subscribe"
      ],
      "Resource": [
        "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:topicfilter/c2/tasking"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
        "iot:Connect"
      ],
      "Resource": [
        "arn:aws:iot:${data.aws_region.current.name}:${data.aws_caller_identity.current.account_id}:client/${var.implant_client_id}"
      ]
    }
  ]
}
EOF
}

# Create an AWS IoT certificate
resource "aws_iot_certificate" "sukoshi_cert" {
  active = true
}

# Attach policy generated above to the AWS IoT thing
resource "aws_iot_policy_attachment" "sukoshi_policy_att" {
  policy = aws_iot_policy.sukoshi_implant_policy.name
  target = aws_iot_certificate.sukoshi_cert.arn
}

# Output certificate to /certs
resource "local_file" "sukoshi_cert_pem" {
  content     = aws_iot_certificate.sukoshi_cert.certificate_pem
  filename = "${path.module}/certs/${var.implant_name}.cert.pem"
}

# Output private key to /certs
resource "local_file" "sukoshi_private_key" {
  content     = aws_iot_certificate.sukoshi_cert.private_key
  filename = "${path.module}/certs/${var.implant_name}.private.key"
}

# Output public key to /certs
resource "local_file" "sukoshi_public_key" {
  content     = aws_iot_certificate.sukoshi_cert.public_key
  filename = "${path.module}/certs/${var.implant_name}.public.key"
}

# Attach AWS IoT cert generated above to the AWS IoT thing
resource "aws_iot_thing_principal_attachment" "sukoshi_principal_att" {
  principal = aws_iot_certificate.sukoshi_cert.arn
  thing     = aws_iot_thing.sukoshi_iot_thing.name
}

# Get the AWS IoT endpoint to print out for reference
data "aws_iot_endpoint" "endpoint" {
    endpoint_type = "iot:Data-ATS"
}
