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

std::string get_tensor_shape_str(const Tensor & tensor)
{
  std::ostringstream oss;
  oss << tensor.name << ": dtype=" << static_cast<int>(tensor.dtype) << ", shape=[";

  for (size_t i = 0; i < tensor.shape.size(); ++i) {
    oss << tensor.shape[i];
    oss << ",";
  }
  oss << "]";
  std::cout << oss.str() << std::endl;
  return oss.str();
}

}  // namespace qrb::yolo_process
