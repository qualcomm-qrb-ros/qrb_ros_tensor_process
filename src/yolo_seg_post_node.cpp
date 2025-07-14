// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "yolo_seg_post_node.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "sensor_msgs/msg/image.hpp"

namespace qrb_ros::yolo_process
{
YoloSegPostProcessNode::YoloSegPostProcessNode(const rclcpp::NodeOptions & options)
  : Node("yolo_postprocess_node", options)
{
  // params handling
  this->declare_parameter<std::string>("label_file", "");
  this->declare_parameter<double>("score_thres", 0.3);
  this->declare_parameter<double>("iou_thres", 0.5);

  std::string label_file = this->get_parameter("label_file").as_string();
  double iou_thres = this->get_parameter("iou_thres").as_double();
  double score_thres = this->get_parameter("score_thres").as_double();

  RCLCPP_INFO(this->get_logger(), "label file path: %s", label_file.c_str());
  RCLCPP_INFO(this->get_logger(), "iou_thres: %f", iou_thres);
  RCLCPP_INFO(this->get_logger(), "score_thres: %f", score_thres);

  if (label_file.empty()) {
    RCLCPP_ERROR(this->get_logger(), "label file not specified.");
    throw std::invalid_argument("label_file not specified.");
  }

  // topic publisher & subscriber
  sub_ = this->create_subscription<qrb_ros_tensor_list_msgs::msg::TensorList>(
      "yolo_segment_tensor_output", 10,
      std::bind(&YoloSegPostProcessNode::msg_callback, this, std::placeholders::_1));

  pub_ = this->create_publisher<qrb_ros_vision_msgs::msg::Detection2DWithMaskArray>(
      "yolo_segment_result", 10);

  // Yolo processor hal-lib
  processor_ =
      std::make_unique<qrb::yolo_process::YoloSegPostProcessor>(label_file, score_thres, iou_thres);

  RCLCPP_INFO(this->get_logger(), "init done~");
}

void YoloSegPostProcessNode::populate_tensor_from_msg(const custom_msg::TensorList::SharedPtr & msg,
    std::vector<qrb::yolo_process::Tensor> & tensors)
{
  tensors.reserve(msg->tensor_list.size());
  for (uint32_t i = 0; i < msg->tensor_list.size(); i++) {
    tensors.emplace_back(msg->tensor_list[i].name,
        static_cast<qrb::yolo_process::TensorDataType>(msg->tensor_list[i].data_type),
        msg->tensor_list[i].shape, &msg->tensor_list[i].data);
  }
}

void YoloSegPostProcessNode::populate_pub_msg(
    qrb_ros_vision_msgs::msg::Detection2DWithMaskArray & msg,
    std::vector<qrb::yolo_process::YoloInstance> & instances)
{
  int obj_cnt = instances.size();

  // alloc memory & initialize in advance for perf
  msg.array.resize(obj_cnt);

  for (int i = 0; i < obj_cnt; i++) {
    qrb_ros_vision_msgs::msg::Detection2DWithMask & yolo_instance = msg.array[i];
    qrb::yolo_process::YoloInstance & instance = instances[i];

    // BoundingBox2D use CXYWH format
    qrb::yolo_process::BBoxCoords box_cxywh = instance.bbox.to_cxywh_coords();
    yolo_instance.detection.bbox.center.position.x = box_cxywh[0];
    yolo_instance.detection.bbox.center.position.y = box_cxywh[1];
    yolo_instance.detection.bbox.size_x = box_cxywh[2];
    yolo_instance.detection.bbox.size_y = box_cxywh[3];

    vision_msgs::msg::ObjectHypothesisWithPose hypo;
    hypo.hypothesis.score = instance.score;
    hypo.hypothesis.class_id = instance.label;
    yolo_instance.detection.results.push_back(std::move(hypo));

    // populate sensor_msgs manually for sake of performance
    yolo_instance.instance_mask.header = msg.header;
    yolo_instance.instance_mask.height = 160;
    yolo_instance.instance_mask.width = 160;
    yolo_instance.instance_mask.step = 160;
    yolo_instance.instance_mask.encoding = "mono8";
    // **ATTENTION**, don't refer to member "mask" afterward
    yolo_instance.instance_mask.data = std::move(instance.mask);
  }
}

void YoloSegPostProcessNode::msg_callback(
    const qrb_ros_tensor_list_msgs::msg::TensorList::SharedPtr msg)
{
  RCLCPP_DEBUG(this->get_logger(), ">>> YOLO SEG post-process begin");
  uint32_t tensor_size = msg->tensor_list.size();
  if (tensor_size != 5) {
    RCLCPP_ERROR(this->get_logger(), "bad input, expect tensor size %d but got %d", 5, tensor_size);
    return;
  }

  // populate tensor from ros msg
  std::vector<qrb::yolo_process::Tensor> tensors;
  populate_tensor_from_msg(msg, tensors);

  // process tensor
  std::vector<qrb::yolo_process::YoloInstance> instances;
  try {
    processor_->process(tensors, instances);
  } catch (const std::invalid_argument & e) {
    RCLCPP_ERROR(this->get_logger(), "Error: %s", e.what());
    return;
  } catch (const std::exception & e) {
    RCLCPP_ERROR(this->get_logger(), "Error: %s", e.what());
    return;
  } catch (...) {
    RCLCPP_ERROR(this->get_logger(), "Error: unexpected exception");
    return;
  }

  // construct ros msg and publish
  auto pub_msg = std::make_unique<qrb_ros_vision_msgs::msg::Detection2DWithMaskArray>();
  pub_msg->header = msg->header;
  populate_pub_msg(*pub_msg, instances);

  pub_->publish(std::move(pub_msg));
  RCLCPP_DEBUG(this->get_logger(), "<<< YOLO SEG post-process end");
}

}  // namespace qrb_ros::yolo_process

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(qrb_ros::yolo_process::YoloSegPostProcessNode)
