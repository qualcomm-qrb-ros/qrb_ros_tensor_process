// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QRB_ROS_CV_TENSOR_COMMON_PROCESS_HPP_
#define _QRB_ROS_CV_TENSOR_COMMON_PROCESS_HPP_

#include <map>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <string>

#include "qrb_ros_tensor_list_msgs/msg/tensor.hpp"
#include "qrb_ros_tensor_list_msgs/msg/tensor_list.hpp"
#include "qrb_ros_transport_image_type/image.hpp"

namespace qrb_ros::cv_tensor_common_process
{
class CvTensorCommonProcessNode : public rclcpp::Node
{
public:
  CvTensorCommonProcessNode(const rclcpp::NodeOptions & options);
  ~CvTensorCommonProcessNode() = default;
  void msg_callback(const std::shared_ptr<qrb_ros::transport::type::Image> msg);

  void process_core(const std::shared_ptr<qrb_ros::transport::type::Image> & msg);

private:
  enum class EnumTensorFmt
  {
    FMT_NHWC = 1,
    FMT_NCHW
  };

  rclcpp::Subscription<qrb_ros::transport::type::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<qrb_ros_tensor_list_msgs::msg::TensorList>::SharedPtr tensor_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr resize_pub_;

  // ros node parameters
  int resize_width_;
  int resize_height_;
  bool normalize_;
  std::string tensor_fmt_;
  std::string data_type_;

  std::map<std::string, int> data_type_map_;
  std::map<std::string, EnumTensorFmt> tensor_fmt_map_;
  int data_type_val_;  // used to construct tensor list msg
};

}  // namespace qrb_ros::cv_tensor_common_process

#endif  //_QRB_ROS_CV_TENSOR_COMMON_PROCESS_HPP_
