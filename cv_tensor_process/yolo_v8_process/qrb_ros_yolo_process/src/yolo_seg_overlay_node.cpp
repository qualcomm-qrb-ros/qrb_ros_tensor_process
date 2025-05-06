// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "yolo_seg_overlay_node.hpp"

#include "cv_bridge/cv_bridge.hpp"

namespace qrb_ros::yolo_process
{
YoloSegOverlayNode::YoloSegOverlayNode(const rclcpp::NodeOptions & options)
  : Node("yolo_seg_overlay_node", options)
{
  this->declare_parameter("resize_width", 0);
  this->declare_parameter("resize_height", 0);

  this->get_parameter("resize_width", resize_width_);
  this->get_parameter("resize_height", resize_height_);

  if (resize_width_ <= 0 || resize_height_ <= 0) {
    throw std::invalid_argument(
        "YoloSegOverlayNode: Invalid resize value: " + std::to_string(resize_width_) + "," +
        std::to_string(resize_height_));
  }

  img_sub_.subscribe(this, "resized_image");
  yolo_seg_sub_.subscribe(this, "yolo_segment_result");
  pub_ = create_publisher<sensor_msgs::msg::Image>("yolo_segment_overlay", 10);

  exact_sync_.reset(new ExactSync(ExactPolicy(10), img_sub_, yolo_seg_sub_));
  exact_sync_->registerCallback(std::bind(&YoloSegOverlayNode::msg_callback, this, _1, _2));

  RCLCPP_INFO(this->get_logger(), "init done~");
}

void YoloSegOverlayNode::draw_inplace(std::vector<YoloInstance> & detections, cv::Mat & img)
{
  // overlay bounding box info
  for (auto & det : detections) {
    // TLWH format
    BBoxCoords tlwh_box = det.bbox.to_tlwh_coords();
    float x = tlwh_box[0];
    float y = tlwh_box[1];
    float w = tlwh_box[2];
    float h = tlwh_box[3];

    cv::rectangle(img, cv::Rect(x, y, w, h), cv::Scalar(0, 0, 255), 1);

    std::stringstream text;
    text << det.label << ": " << std::fixed << std::setprecision(2) << det.score;
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

  // overlay mask

  // merge all mask together
  cv::Mat merged_mask(160, 160, CV_8UC1, cv::Scalar(0));
  for (auto & det : detections) {
    cv::Mat mask(160, 160, CV_8UC1, det.mask.data());
    cv::add(mask, merged_mask, merged_mask);
  }

  // scale mask to 640x640
  cv::Mat resized_image;
  cv::resize(merged_mask, resized_image, cv::Size(), 4, 4, cv::INTER_LINEAR);

  // create a color mask
  cv::Mat colorMask = cv::Mat::zeros(resized_image.size(), CV_8UC3);
  cv::Vec3b color(0, 0, 255);  // red
  for (int i = 0; i < resized_image.rows; ++i) {
    for (int j = 0; j < resized_image.cols; ++j) {
      if (resized_image.at<uchar>(i, j) != 0) {
        colorMask.at<cv::Vec3b>(i, j) = color;
      }
    }
  }
  double beta = 0.5;
  cv::addWeighted(img, 1.0, colorMask, beta, 0.0, img);
}

void YoloSegOverlayNode::msg_callback(sensor_msgs::msg::Image::ConstSharedPtr img_msg,
    qrb_ros_vision_msgs::msg::Detection2DWithMaskArray::ConstSharedPtr yolo_msg)
{
  RCLCPP_DEBUG(this->get_logger(), ">>> YOLO SEG overlay begin");
  if (img_msg->width != resize_width_ || img_msg->height != resize_height_) {
    RCLCPP_ERROR(this->get_logger(), "Image size mismatch: expect %dx%d but got %dx%d",
        resize_width_, resize_height_, img_msg->width, img_msg->height);
    return;
  }

  if (yolo_msg->array.empty()) {
    RCLCPP_DEBUG(this->get_logger(), ">>> YOLO SEG overlay end, no detection");
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

  for (const auto & it : yolo_msg->array) {
    float x = it.detection.bbox.center.position.x;
    float y = it.detection.bbox.center.position.y;
    float w = it.detection.bbox.size_x;
    float h = it.detection.bbox.size_y;
    float score = it.detection.results[0].hypothesis.score;
    std::string label = it.detection.results[0].hypothesis.class_id;

    YoloInstance instance(
        x, y, w, h, BoundingBox::BoxFmt::CXYWH, score, label, it.instance_mask.data);
    instances.push_back(instance);
  }

  draw_inplace(instances, cv_ptr->image);
  pub_->publish(*(cv_ptr->toImageMsg()));
  RCLCPP_DEBUG(this->get_logger(), ">>> YOLO SEG overlay end");
}

}  // namespace qrb_ros::yolo_process

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(qrb_ros::yolo_process::YoloSegOverlayNode)
