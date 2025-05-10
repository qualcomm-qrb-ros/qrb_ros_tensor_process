// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "common.hpp"

#include "opencv2/opencv.hpp"

namespace qrb::yolo_process
{
std::size_t get_size_of_type(TensorDataType dtype)
{
  switch (dtype) {
    case TensorDataType::INT8:
      return sizeof(int8_t);
    case TensorDataType::UINT8:
      return sizeof(uint8_t);
    case TensorDataType::FLOAT32:
      return sizeof(float);
    case TensorDataType::FLOAT64:
      return sizeof(double);
    default:
      throw std::invalid_argument(
          "Unknown TensorDataType: " + std::to_string(static_cast<int>(dtype)));
  }
}

std::string tensor_dtype_to_string(TensorDataType dtype)
{
  switch (dtype) {
    case TensorDataType::INT8:
      return "INT8";
    case TensorDataType::UINT8:
      return "UINT8";
    case TensorDataType::FLOAT32:
      return "FLOAT32";
    case TensorDataType::FLOAT64:
      return "FLOAT64";
    default:
      return "UNKNOWN";
  }
}

int make_cvtype(TensorDataType dtype, int channel)
{
  int cvType;
  switch (dtype) {
    case TensorDataType::INT8:
      cvType = CV_MAKETYPE(CV_8S, channel);
      break;
    case TensorDataType::UINT8:
      cvType = CV_MAKETYPE(CV_8U, channel);
      break;
    case TensorDataType::FLOAT32:
      cvType = CV_MAKETYPE(CV_32F, channel);
      break;
    case TensorDataType::FLOAT64:
      cvType = CV_MAKETYPE(CV_64F, channel);
      break;
    default:
      std::cerr << "data type " << static_cast<int>(dtype) << "not supported." << std::endl;
      return -1;
  }

  return cvType;
}

void validate_tensors(const std::vector<Tensor> & tensors, const std::vector<TensorSpec> & specs)
{
  bool is_valid = true;
  std::ostringstream oss;
  if (tensors.size() != specs.size()) {
    oss << "Expected " << specs.size() << " tensors, but got " << tensors.size();
    throw std::invalid_argument(oss.str());
  }

  for (size_t i = 0; i < specs.size(); ++i) {
    const Tensor & tensor = tensors[i];
    const TensorSpec & spec = specs[i];

    // Check data type
    if (tensor.dtype != spec.dtype || tensor.shape != spec.shape) {
      oss << "Tensor spec mismatch,"
          << " expected " << get_tensor_shape_str(spec) << ", but got "
          << get_tensor_shape_str(tensor);
      is_valid = false;
      break;
    }
  }
  if (!is_valid) {
    throw std::invalid_argument(oss.str());
  }
}

std::string get_tensor_shape_str(const TensorSpec & spec)
{
  std::ostringstream oss;
  oss << "<" << spec.name << ">:" << tensor_dtype_to_string(spec.dtype) << "[";

  for (size_t i = 0; i < spec.shape.size(); ++i) {
    oss << spec.shape[i];
    oss << ",";
  }
  oss << "]";
  return oss.str();
}

}  // namespace qrb::yolo_process
