variable "implant_name" {
  type = string
  default = "sukoshi_implant"
}

variable "implant_client_id" {
  type = string
  default = "sukoshi_client_id"
}

data "aws_region" "current" {}

data "aws_caller_identity" "current" {}
