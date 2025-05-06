// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "yolo_det_overlay_node.hpp"

#include "cv_bridge/cv_bridge.hpp"

namespace qrb_ros::yolo_process
{
YoloDetOverlayNode::YoloDetOverlayNode(const rclcpp::NodeOptions & options)
  : Node("yolo_det_overlay_node", options)
{
  this->declare_parameter<int>("resize_width", 0);
  this->declare_parameter<int>("resize_height", 0);

  this->get_parameter("resize_width", resize_width_);
  this->get_parameter("resize_height", resize_height_);

  if (resize_width_ <= 0 || resize_height_ <= 0) {
    throw std::invalid_argument(
        "YoloDetOverlayNode: Invalid resize value: " + std::to_string(resize_width_) + "," +
        std::to_string(resize_height_));
  }

  img_sub_.subscribe(this, "resized_image");
  yolo_det_sub_.subscribe(this, "yolo_detect_result");
  pub_ = create_publisher<sensor_msgs::msg::Image>("yolo_detect_overlay", 10);

  exact_sync_.reset(new ExactSync(ExactPolicy(10), img_sub_, yolo_det_sub_));
  exact_sync_->registerCallback(std::bind(&YoloDetOverlayNode::msg_callback, this, _1, _2));

  RCLCPP_INFO(this->get_logger(), "init done~");
}

void YoloDetOverlayNode::draw_inplace(std::vector<YoloInstance> & instances, cv::Mat & img)
{
  for (auto & instance : instances) {
    // TLWH format
    BBoxCoords tlwh_box = instance.bbox.to_tlwh_coords();
    float x = tlwh_box[0];
    float y = tlwh_box[1];
    float w = tlwh_box[2];
    float h = tlwh_box[3];

    cv::rectangle(img, cv::Rect(x, y, w, h), cv::Scalar(0, 0, 255), 1);

    std::stringstream text;
    text << instance.label << ": " << std::fixed << std::setprecision(2) << instance.score;
    int baseline = 0;
    double fontScale = 0.5;
    int thickness = 1;

    cv::Size text_size =
        cv::getTextSize(text.str(), cv::FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);
    cv::Rect background(
        x, y - text_size.height + baseline, text_size.width, text_size.height + baseline);
    cv::rectangle(img, background, cv::Scalar(0, 0, 255), cv::FILLED);
    cv::putText(img, text.str(), cv::Point(x, y + baseline), cv::FONT_HERSHEY_SIMPLEX, fontScale,
        cv::Scalar(255, 255, 255), thickness);
  }
}

void YoloDetOverlayNode::msg_callback(sensor_msgs::msg::Image::ConstSharedPtr img_msg,
    vision_msgs::msg::Detection2DArray::ConstSharedPtr yolo_msg)
{
  RCLCPP_DEBUG(this->get_logger(), ">>> YOLO DET overlay begin");
  if (img_msg->width != resize_width_ || img_msg->height != resize_height_) {
    RCLCPP_ERROR(this->get_logger(), "Image size mismatch: expect %dx%d but got %dx%d",
        resize_width_, resize_height_, img_msg->width, img_msg->height);
    return;
  }

  if (yolo_msg->detections.empty()) {
    RCLCPP_DEBUG(this->get_logger(), ">>> YOLO DET post-process end, no detection");
    pub_->publish(*img_msg);
    return;
  }

  cv_bridge::CvImagePtr cv_ptr;
  try {
    cv_ptr = cv_bridge::toCvCopy(img_msg, sensor_msgs::image_encodings::BGR8);
  } catch (cv_bridge::Exception & e) {
    RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    return;
  }

  std::vector<YoloInstance> instances;

  for (const auto & it : yolo_msg->detections) {
    float x = it.bbox.center.position.x;
    float y = it.bbox.center.position.y;
    float w = it.bbox.size_x;
    float h = it.bbox.size_y;
    float score = it.results[0].hypothesis.score;

    std::string label = it.results[0].hypothesis.class_id;
    YoloInstance instance(x, y, w, h, BoundingBox::BoxFmt::CXYWH, score, label);
    instances.push_back(instance);
  }
  draw_inplace(instances, cv_ptr->image);

  pub_->publish(*(cv_ptr->toImageMsg()));
  RCLCPP_DEBUG(this->get_logger(), ">>> YOLO DET post-process end");
}

}  // namespace qrb_ros::yolo_process

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(qrb_ros::yolo_process::YoloDetOverlayNode)
