// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "bounding_box.hpp"

#include <iostream>
#include <iterator>
#include <sstream>

namespace qrb::yolo_process
{
BoundingBox::BoundingBox(const BBoxCoords & bbox, BoxFmt fmt)
{
  const int box_size = 4;
  if (bbox.size() != box_size) {
    std::ostringstream oss;
    oss << "Invalid bounding box, expected 4 elements but got " << bbox.size() << " elements."
        << std::endl;
    throw std::invalid_argument(oss.str());
  }
  BBoxCoords box = bbox;
  for (int n = 0; n < box_size; ++n) {
    if (box[n] < 0.0) {
      box[n] = 0.0;
    }
  }

  float tl_x, tl_y, br_x, br_y;
  switch (fmt) {
    case BoxFmt::TLBR:
      tl_x = box[0];
      tl_y = box[1];
      br_x = box[2];
      br_y = box[3];
      break;

    case BoxFmt::TLWH:
      tl_x = box[0];
      tl_y = box[1];
      br_x = box[0] + box[2];
      br_y = box[1] + box[3];
      break;

    case BoxFmt::CXYWH:
      tl_x = box[0] - box[2] / 2.0;
      tl_y = box[1] - box[3] / 2.0;
      br_x = box[0] + box[2] / 2.0;
      br_y = box[1] + box[3] / 2.0;
      break;

    default:
      throw std::invalid_argument("Invalid bounding box format");
      break;
  }
  box_arr_ = { tl_x, tl_y, br_x, br_y };

  if (tl_x < 0.0f || tl_y < 0.0f || br_x < tl_x || br_y < tl_y) {
    std::ostringstream oss;
    oss << "Invalid bounding box(TLBR): (";
    std::copy(box_arr_.begin(), box_arr_.end(), std::ostream_iterator<float>(oss, ","));
    oss << ")" << std::endl;
    throw std::invalid_argument(oss.str());
  }
}

BBoxCoords BoundingBox::to_tlbr_coords() const
{
  return box_arr_;
}

BBoxCoords BoundingBox::to_tlwh_coords() const
{
  float tl_x = box_arr_[0];
  float tl_y = box_arr_[1];
  float w = box_arr_[2] - box_arr_[0];
  float h = box_arr_[3] - box_arr_[1];
  return { tl_x, tl_y, w, h };
}

BBoxCoords BoundingBox::to_cxywh_coords() const
{
  float w = box_arr_[2] - box_arr_[0];
  float h = box_arr_[3] - box_arr_[1];
  float c_x = box_arr_[0] + 0.5 * w;
  float c_y = box_arr_[1] + 0.5 * h;
  return { c_x, c_y, w, h };
}

}  // namespace qrb::yolo_process
