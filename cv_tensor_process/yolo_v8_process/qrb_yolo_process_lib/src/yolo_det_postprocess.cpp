// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "yolo_det_postprocess.hpp"

#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"

namespace qrb::yolo_process
{
YoloDetPostProcessor::YoloDetPostProcessor(const std::string & label_file,
    float score_thres,
    float iou_thres,
    float eta,
    int top_k)
  : score_thres_(score_thres), iou_thres_(iou_thres), eta_(eta), top_k_(top_k)
{
  // args range check
  auto out_of_range = [](float value) { return value <= 0.0f || value >= 1.0f; };

  if (out_of_range(score_thres_) || out_of_range(iou_thres_)) {
    throw std::invalid_argument("Err: thres out of range..");
  }

  // init label map from yaml
  try {
    YAML::Node label_yml = YAML::LoadFile(label_file);
    const YAML::Node & nd_name = label_yml["names"];

    for (YAML::const_iterator it = nd_name.begin(); it != nd_name.end(); ++it) {
      label_map_[it->first.as<int>()] = it->second.as<std::string>();
    }

  } catch (const YAML::Exception & e) {
    std::cerr << "YAML Exception: " << e.what() << std::endl;
    label_map_.clear();
  }

  // init tensor specs
  tensor_specs_ = {
    { "boxes", TensorDataType::FLOAT32, { 1, 8400, 4 } },
    { "scores", TensorDataType::FLOAT32, { 1, 8400 } },
    { "class_idx", TensorDataType::UINT8, { 1, 8400 } },
  };
}

void YoloDetPostProcessor::process(const std::vector<Tensor> & tensors,
    std::vector<YoloInstance> & instances)
{
  validate_tensors(tensors, tensor_specs_);
  std::vector<int> indices;
  non_maximum_suppression(tensors, score_thres_, iou_thres_, indices, eta_, top_k_);

  const int idx_boxes = get_tensor_idx(tensors, "boxes");
  const int idx_scores = get_tensor_idx(tensors, "scores");
  const int idx_class = get_tensor_idx(tensors, "class_idx");

  const float(*const ptr_bbox)[4] = reinterpret_cast<float(*)[4]>(tensors[idx_boxes].p_vec->data());
  const float * const ptr_score = reinterpret_cast<float *>(tensors[idx_scores].p_vec->data());
  const uint8_t * const ptr_label = reinterpret_cast<uint8_t *>(tensors[idx_class].p_vec->data());

  for (auto & idx : indices) {
    float score = ptr_score[idx];

    std::string label;
    try {
      label = label_map_.at(ptr_label[idx]);
    } catch (const std::out_of_range & e) {
      label = "unknown";
    }
    YoloInstance instance(ptr_bbox[idx][0], ptr_bbox[idx][1], ptr_bbox[idx][2], ptr_bbox[idx][3],
        BoundingBox::BoxFmt::TLBR, score, label);

    instances.push_back(instance);
  }
}

}  // namespace qrb::yolo_process
