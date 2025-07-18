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

int get_tensor_idx(const std::vector<Tensor> & tensors, const std::string & name)
{
  for (size_t i = 0; i < tensors.size(); ++i) {
    if (tensors[i].name == name) {
      return static_cast<int>(i);
    }
  }
  throw std::invalid_argument("Tensor with name '" + name + "' not found.");
}

void validate_tensors(const std::vector<Tensor> & tensors, const std::vector<TensorSpec> & specs)
{
  std::unordered_map<std::string, TensorSpec> spec_map;
  for (const auto & spec : specs) {
    spec_map[spec.name] = spec;
  }

  bool is_valid = true;
  std::ostringstream oss;
  if (tensors.size() != specs.size()) {
    oss << "Expected " << specs.size() << " tensors, but got " << tensors.size();
    throw std::invalid_argument(oss.str());
  }

  for (const auto & tensor : tensors) {
    auto it = spec_map.find(tensor.name);
    if (it == spec_map.end()) {
      oss << "Unexpected tensor: " << tensor.name << "\n";
      is_valid = false;
      continue;
    }

    const TensorSpec & expected = it->second;
    if (tensor.dtype != expected.dtype || tensor.shape != expected.shape) {
      oss << "Tensor '" << tensor.name << "' mismatch: expected " << get_tensor_shape_str(expected)
          << ", but got " << get_tensor_shape_str(tensor) << "\n";
      is_valid = false;
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

void non_maximum_suppression(const std::vector<Tensor> & tensors,
    const float score_thres,
    const float iou_thres,
    std::vector<int> & indices,
    const float eta,
    const int top_k)
{
  const int idx_boxes = get_tensor_idx(tensors, "boxes");
  const int idx_scores = get_tensor_idx(tensors, "scores");
  const Tensor & tensor_bbox = tensors[idx_boxes];
  const Tensor & tensor_score = tensors[idx_scores];

  std::vector<int> indices_1st;
  std::vector<cv::Rect> bboxes;
  std::vector<float> scores;

  // allocate mem for perf, assuming the valid objects is around NMS_RESERVE_CNT
  const int NMS_RESERVE_CNT = 16;
  indices_1st.reserve(NMS_RESERVE_CNT);
  bboxes.reserve(NMS_RESERVE_CNT);
  scores.reserve(NMS_RESERVE_CNT);

  float(*ptr_bbox)[4] = reinterpret_cast<float(*)[4]>(tensor_bbox.p_vec->data());
  float * ptr_score = reinterpret_cast<float *>(tensor_score.p_vec->data());

  for (uint32_t i = 0; i < tensor_bbox.shape[1]; ++i) {
    if (ptr_score[i] < score_thres) {
      continue;
    }
    indices_1st.emplace_back(i);

    // model returns TLBR bbox, convert to TLWH
    std::vector<float> box =
        BoundingBox({ ptr_bbox[i][0], ptr_bbox[i][1], ptr_bbox[i][2], ptr_bbox[i][3] },
            BoundingBox::BoxFmt::TLBR)
            .to_tlwh_coords();

    bboxes.emplace_back(cv::Rect(box[0], box[1], box[2], box[3]));
    scores.emplace_back(ptr_score[i]);
  }

  // cv::NMS processing
  std::vector<int> indices_2nd;
  cv::dnn::NMSBoxes(bboxes, scores, score_thres, iou_thres, indices_2nd, eta, top_k);

  // get final indices
  indices.reserve(indices_2nd.size());
  for (auto & idx : indices_2nd) {
    indices.emplace_back(indices_1st[idx]);
  }
}

}  // namespace qrb::yolo_process
