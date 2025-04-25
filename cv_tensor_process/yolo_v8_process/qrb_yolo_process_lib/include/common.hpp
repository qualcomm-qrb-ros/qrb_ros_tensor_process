// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QRB_YOLO_PROCESS_COMMON_HPP_
#define _QRB_YOLO_PROCESS_COMMON_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include "bounding_box.hpp"

namespace qrb::yolo_process
{
/**
 * \brief Tensor data type enumeration
 */
enum class TensorDataType
{
  UINT8,
  INT8,
  FLOAT32,
  FLOAT64,
};

/**
 * \brief get size of basic type represented by given datatype enumeration
 * \param dataType: enum value of specific data type
 */
std::size_t get_size_of_type(TensorDataType dtype);

/**
 * \brief struct to describe a tensor
 */
struct Tensor
{
  std::vector<uint8_t> * p_vec;  // pointer to vector that stores tensor byte stream
  std::vector<uint32_t> shape;   // shape of tensor
  std::string name;              // name of tensor
  TensorDataType dtype;          // data type of tensor
};
std::string get_tensor_shape_str(const Tensor & tensor);

/**
 * \brief YOLO instance info structure
 */
struct YoloInstance
{
  BoundingBox bbox;
  float score;
  std::string label;
  std::vector<uint8_t> mask;

  // instance for yolo object detection
  YoloInstance(float x,
      float y,
      float w,
      float h,
      BoundingBox::BoxFmt box_fmt,
      float score,
      const std::string & label)
    : bbox({ x, y, w, h }, box_fmt), score(score), label(label)
  {
  }

  // instance for yolo image segmentation
  YoloInstance(float x,
      float y,
      float w,
      float h,
      BoundingBox::BoxFmt box_fmt,
      float score,
      const std::string & label,
      const std::vector<uint8_t> & mask)
    : bbox({ x, y, w, h }, box_fmt), score(score), label(label), mask(mask)
  {
  }
};

/**
 * \brief make CV_TYPE as per TensorDataType::dtype and channel number
 * \param dtype data type
 * \param channel channel number
 */
int make_cvtype(TensorDataType dtype, int channel);

}  // namespace qrb::yolo_process

#endif  // _QRB_YOLO_PROCESS_COMMON_HPP_
