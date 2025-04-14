// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cv_bridge/cv_bridge.hpp>
#include <cv_tensor_common_process.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <vector>

namespace qrb_ros::cv_tensor_common_process
{
CvTensorCommonProcessNode::CvTensorCommonProcessNode(const rclcpp::NodeOptions & options)
  : Node("cv_tensor_common_process", options)
{
  // declare & get input ros parameter
  this->declare_parameter<bool>("normalize", true);
  this->declare_parameter<std::string>("tensor_fmt", "nhwc");
  this->declare_parameter<std::string>("data_type", "float32");
  this->declare_parameter<std::string>("target_res", "0x0");

  this->get_parameter("normalize", normalize_);
  this->get_parameter("tensor_fmt", tensor_fmt_);
  this->get_parameter("data_type", data_type_);
  std::string target_res = this->get_parameter("target_res").as_string();

  // param check for data_type
  data_type_map_["uint8"] = CV_8UC3;
  data_type_map_["float32"] = CV_32FC3;
  data_type_map_["float64"] = CV_64FC3;

  if (data_type_map_.find(data_type_) == data_type_map_.end()) {
    std::ostringstream oss;
    oss << this->get_name() << ": Invalid data_type: " << data_type_ << ", ";
    oss << "support type: uint8, float32, float64" << std::endl;
    RCLCPP_ERROR_STREAM(this->get_logger(), oss.str());
    throw std::invalid_argument("Invalid data_type");
  }

  switch (data_type_map_[data_type_]) {
    case CV_8UC3:
      data_type_val_ = 0;  // tensor msg: "uint8"
      break;
    case CV_32FC3:
      data_type_val_ = 2;  // tensor msg: "float32"
      break;
    case CV_64FC3:
      data_type_val_ = 3;  // tensor msg: "float64"
      break;
  }

  // param check for tensor_fmt
  tensor_fmt_map_["nhwc"] = EnumTensorFmt::FMT_NHWC;
  tensor_fmt_map_["nchw"] = EnumTensorFmt::FMT_NCHW;

  if (tensor_fmt_map_.find(tensor_fmt_) == tensor_fmt_map_.end()) {
    std::ostringstream oss;
    oss << this->get_name() << ": Invalid tensor_fmt: " << tensor_fmt_ << ", ";
    oss << "support fmt: nhwc, nchw" << std::endl;
    RCLCPP_ERROR_STREAM(this->get_logger(), oss.str());
    throw std::invalid_argument(oss.str());
  }

  // params check for resize_*
  std::transform(target_res.begin(), target_res.end(), target_res.begin(), ::tolower);
  size_t split_pos = target_res.find('x');
  if (split_pos == std::string::npos) {
    std::ostringstream oss;
    oss << this->get_name() << ": Invalid target_res format: " << target_res << ", ";
    oss << "support format: <width>x<height>" << std::endl;
    RCLCPP_ERROR_STREAM(this->get_logger(), oss.str());
    throw std::invalid_argument(oss.str());
  }
  resize_width_ = std::stoi(target_res.substr(0, split_pos));
  resize_height_ = std::stoi(target_res.substr(split_pos + 1));

  if (resize_width_ <= 0 || resize_height_ <= 0) {
    std::ostringstream oss;
    oss << this->get_name() << ": Invalid target resolution: " << resize_width_ << "x"
        << resize_height_;
    oss << std::endl;
    RCLCPP_ERROR_STREAM(this->get_logger(), oss.str());
    throw std::invalid_argument(oss.str());
  }

  RCLCPP_INFO_STREAM(this->get_logger(),
      "params list -> resoltuion: " << target_res << ", tensor format: " << tensor_fmt_
                                    << ", dtype: " << data_type_ << ", normalize: " << normalize_);

  // create publisher & subscription
  image_sub_ = this->create_subscription<qrb_ros::transport::type::Image>("input_image", 10,
      std::bind(&CvTensorCommonProcessNode::msg_callback, this, std::placeholders::_1));

  tensor_pub_ =
      this->create_publisher<qrb_ros_tensor_list_msgs::msg::TensorList>("encoded_image", 10);

  resize_pub_ = this->create_publisher<sensor_msgs::msg::Image>("resized_image", 10);
}

void CvTensorCommonProcessNode::msg_callback(
    const std::shared_ptr<qrb_ros::transport::type::Image> msg)
{
  RCLCPP_DEBUG_STREAM(this->get_logger(), ">>> Got image with encoding: " << msg->encoding);

  try {
    process_core(msg);
  } catch (std::invalid_argument & e) {
    RCLCPP_ERROR_STREAM(this->get_logger(), "Error:" << e.what());
  } catch (const cv::Exception & e) {
    RCLCPP_ERROR_STREAM(this->get_logger(), "Error:" << e.what());
  } catch (const std::exception & e) {
    RCLCPP_ERROR_STREAM(this->get_logger(), "Error:" << e.what());
  } catch (...) {
    RCLCPP_ERROR_STREAM(this->get_logger(), "Error:unexpected exception");
  }

  RCLCPP_DEBUG_STREAM(this->get_logger(), "<<< Process tensor done.");
}

void CvTensorCommonProcessNode::process_core(
    const std::shared_ptr<qrb_ros::transport::type::Image> & msg)
{
  RCLCPP_DEBUG_STREAM(this->get_logger(), "get_image_align_size of dmabuf.");
  // read out image data from dmabuf
  auto size =
      qrb_ros::transport::image_utils::get_image_align_size(msg->width, msg->height, msg->encoding);
  RCLCPP_DEBUG_STREAM(this->get_logger(), "alloc buffer for image.");
  auto img_data = std::make_unique<char[]>(size);

  qrb_ros::transport::image_utils::read_image_from_dmabuf(msg->dmabuf, img_data.get(), msg->width,
      msg->height, qrb_ros::transport::image_utils::get_image_stride(msg->width, msg->encoding),
      msg->encoding, true);

  // color space convert, get cv::Mat of rgb8 format
  cv::Mat image;
  if (msg->encoding == "nv12") {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "color space convert begin");
    int stride = qrb_ros::transport::image_utils::align_width(msg->width);
    int slice = qrb_ros::transport::image_utils::align_height(msg->height);

    cv::Mat img_nv12_tmp(
        slice + slice / 2, stride, CV_8UC1, reinterpret_cast<uchar *>(img_data.get()));
    cv::cvtColor(img_nv12_tmp, image, cv::COLOR_YUV2RGB_NV12);

  } else if (msg->encoding == "rgb8" || msg->encoding == "bgr8") {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "color space convert skipped");
    cv::Mat img_rgb_tmp(
        msg->height, msg->width, CV_8UC3, reinterpret_cast<uchar *>(img_data.get()));
    image = img_rgb_tmp;
  } else {
    RCLCPP_ERROR_STREAM(this->get_logger(), "Unsupported input encoding: " << msg->encoding);
    throw std::invalid_argument("Unsupported input encoding: " + msg->encoding);
  }

  // resize & send resize image
  cv::Mat resized_image;
  if (image.cols != resize_width_ || image.rows != resize_height_) {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "resize image.");
    cv::resize(image, resized_image, cv::Size(resize_width_, resize_height_));
  } else {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "resize image skipped.");
    resized_image = image;
  }

  // send resize image
  RCLCPP_DEBUG_STREAM(this->get_logger(), "send resize image");
  {
    cv_bridge::CvImage msg_cv_image;
    msg_cv_image.header = msg->header;
    msg_cv_image.encoding = "rgb8";
    msg_cv_image.image = resized_image;
    resize_pub_->publish(*(msg_cv_image.toImageMsg()));
  }

  // data type convert
  cv::Mat converted_image;
  if (data_type_map_[data_type_] != resized_image.type()) {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "convert data type to " << data_type_);
    resized_image.convertTo(converted_image, data_type_map_[data_type_]);
  } else {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "convert data type skipped");
    converted_image = resized_image;
  }

  // normalize
  if (normalize_) {
    RCLCPP_DEBUG_STREAM(this->get_logger(), "normalize data");
    double scale_factor = 255.0;
    converted_image /= scale_factor;
  }

  // HWC <--> CHW
  RCLCPP_DEBUG_STREAM(this->get_logger(), "populate tensor data");
  qrb_ros_tensor_list_msgs::msg::Tensor msg_tensor;
  size_t payload_size = converted_image.total() * converted_image.elemSize();
  msg_tensor.data.resize(payload_size);  // alloc tensor payload
  msg_tensor.name = "image_tensor";
  msg_tensor.data_type = data_type_val_;

  switch (tensor_fmt_map_[tensor_fmt_]) {
    case EnumTensorFmt::FMT_NHWC: {
      // [batch_size, height, width, channels]
      msg_tensor.shape.push_back(1);
      msg_tensor.shape.push_back(converted_image.rows);
      msg_tensor.shape.push_back(converted_image.cols);
      msg_tensor.shape.push_back(converted_image.channels());
      std::memcpy(msg_tensor.data.data(), converted_image.data, payload_size);
      break;
    }

    case EnumTensorFmt::FMT_NCHW: {
      // [batch_size, channels, height, width]
      msg_tensor.shape.push_back(1);
      msg_tensor.shape.push_back(converted_image.channels());
      msg_tensor.shape.push_back(converted_image.rows);
      msg_tensor.shape.push_back(converted_image.cols);

      // split rgb channel to single
      std::vector<cv::Mat> split_channels;
      cv::split(converted_image, split_channels);

      size_t copied_size = 0;
      for (const auto & channel : split_channels) {
        size_t channel_size = channel.total() * channel.elemSize();
        std::memcpy(msg_tensor.data.data() + copied_size, channel.data, channel_size);
        copied_size += channel_size;
      }
      break;
    }
    default: {
      std::ostringstream oss;
      oss << "Unsupported tensor format: " << tensor_fmt_;
      throw std::invalid_argument(oss.str());
    }
  }

  RCLCPP_DEBUG_STREAM(this->get_logger(), "publish tensor_list");
  qrb_ros_tensor_list_msgs::msg::TensorList msg_tensor_list;
  msg_tensor_list.tensor_list.push_back(msg_tensor);
  msg_tensor_list.header = msg->header;
  tensor_pub_->publish(msg_tensor_list);
}

}  // namespace qrb_ros::cv_tensor_common_process

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(qrb_ros::cv_tensor_common_process::CvTensorCommonProcessNode)
