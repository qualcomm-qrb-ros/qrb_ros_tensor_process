// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QRB_YOLO_PROCESS_BOUNDING_BOX_CPP_
#define _QRB_YOLO_PROCESS_BOUNDING_BOX_CPP_

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace qrb::yolo_process
{

using BBoxCoords = std::vector<float>;
/**
 * \brief Class to describe bounding box, and provide methods to operate box.
 */
class BoundingBox
{
public:
  enum class BoxFmt
  {
    TLWH,
    CXYWH,
    TLBR,
    MAX
  };

  BoundingBox(const std::vector<float> & box, const BoxFmt fmt);
  ~BoundingBox() = default;

  BBoxCoords to_tlwh_coords() const;
  BBoxCoords to_cxywh_coords() const;
  BBoxCoords to_tlbr_coords() const;

private:
  // store as tlbr bbox;
  BBoxCoords box_arr_;
};

}  // namespace qrb::yolo_process

#endif  // _QRB_YOLO_PROCESS_BOUNDING_BOX_CPP_