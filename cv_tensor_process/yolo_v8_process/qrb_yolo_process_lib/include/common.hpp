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
struct TensorSpec
{
  std::string name;
  TensorDataType dtype;
  std::vector<uint32_t> shape;
};

struct Tensor : public TensorSpec
{
  std::vector<uint8_t> * p_vec;
  Tensor() : p_vec(nullptr) {}

  Tensor(const TensorSpec & spec, std::vector<uint8_t> * data_ptr)
    : TensorSpec(spec), p_vec(data_ptr)
  {
  }

  Tensor(const std::string & name,
      TensorDataType dtype,
      const std::vector<uint32_t> & shape,
      std::vector<uint8_t> * data_ptr)
    : TensorSpec{ name, dtype, shape }, p_vec(data_ptr)
  {
  }
};

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

  // instance for yolo image segmentation (right value reference)
  YoloInstance(float x,
      float y,
      float w,
      float h,
      BoundingBox::BoxFmt box_fmt,
      float score,
      const std::string & label,
      std::vector<uint8_t> && mask)
    : bbox({ x, y, w, h }, box_fmt), score(score), label(label), mask(mask)
  {
  }
};

/**
 * \brief Validates a list of tensors against their expected specifications.
 *
 * This function checks whether the provided tensors match the expected specifications
 * in terms of data type and shape. If any mismatch is found, an exception is thrown
 * with a detailed error message.
 *
 * \param tensors A vector of Tensor objects to validate.
 * \param specs A vector of TensorSpec objects specifying the expected properties of the tensors.
 *
 * \throws std::invalid_argument If the number of tensors does not match the number of
 * specifications, or if any tensor's data type or shape does not match the expected specification.
 */
void validate_tensors(const std::vector<Tensor> & tensors, const std::vector<TensorSpec> & specs);

std::string get_tensor_shape_str(const TensorSpec & tensor);
/**
 * \brief make CV_TYPE as per TensorDataType::dtype and channel number
 * \param dtype data type
 * \param channel channel number
 */
int make_cvtype(TensorDataType dtype, int channel);

/**
 * \brief Performs non-maximum-suppression to filter out valid object.
 * \param tensors Tensors output from YOLO segmentation model.
 * \param score_thres Object with score higher than threshold will be kept
 * \param iou_thres iou(intersection over union) threshold to filter overlapping bbox
 * \param indices (Output) Indices of valid object after NMS.
 */
void non_maximum_suppression(const std::vector<Tensor> & tensors,
    const float score_thres,
    const float iou_thres,
    std::vector<int> & indices,
    const float eta,
    const int top_k);

}  // namespace qrb::yolo_process

#endif  // _QRB_YOLO_PROCESS_COMMON_HPP_
