<div align="center">
  <h1>QRB ROS Tensor Process</h1>
  <p>ROS2 package for processing input and output of AI model.</p>
  <a href="https://ubuntu.com/download/qualcomm-iot" target="_blank"><img src="https://img.shields.io/badge/Qualcomm%20Ubuntu-E95420?style=for-the-badge&logo=ubuntu&logoColor=white" alt="Qualcomm Ubuntu"></a>
  <a href="https://docs.ros.org/en/jazzy/" target="_blank"><img src="https://img.shields.io/badge/ROS%20Jazzy-1c428a?style=for-the-badge&logo=ros&logoColor=white" alt="Jazzy"></a>
</div>

---

## 👋 Overview

**qrb_ros_tensor_process** is a ROS2 package designed to bridge the gap between conventional data formats and AI model requirements:

- **Input Processing**: Transforms conventional data (images, text, audio, etc.) into the specific tensor format required by AI models。
- **Output Processing**: Handles the output tensors produced by AI models and converts them into usable results。

For input processing, corrently this package focuses primarily on image data preprocessing capabilities, you can see [cv_tensor_common_process](./cv_tensor_process/cv_tensor_common_process).<br>

For output processing, the package provides **model-specific** libraries to handle the unique requirements of different AI architectures and only YOLOv8 output tensor post-processing is supported now, you can get more details in [yolov8_process](./cv_tensor_process/yolo_v8_process).

<div align="center">
  <img src="./docs/assets/workflow.png" alt="workflow">
</div>

---

## 🔎 Table of Contents
  * [Supported Targets](#-supported-targets)
  * [Contributing](#-contributing)
  * [License](#-license)

---

## 🎯 Supported Targets

<table >
  <tr>
    <th>Development Hardware</th>
    <th>Hardware Overview</th>
  </tr>
  <tr>
    <td>Qualcomm Dragonwing™ RB3 Gen2</td>
    <th><a href="https://www.qualcomm.com/developer/hardware/rb3-gen-2-development-kit"><img src="https://s7d1.scene7.com/is/image/dmqualcommprod/rb3-gen2-carousel?fmt=webp-alpha&qlt=85" width="180"/></a></th>
  </tr>
    <tr>
    <td>Qualcomm Dragonwing™ IQ-9075 EVK</td>
    <th><a href="https://www.qualcomm.com/products/internet-of-things/industrial-processors/iq9-series/iq-9075"><img src="https://s7d1.scene7.com/is/image/dmqualcommprod/dragonwing-IQ-9075-EVK?$QC_Responsive$&fmt=png-alpha" width="160"></a></th>
  </tr>
</table>

---

## 🤝 Contributing

We love community contributions! Get started by reading our [CONTRIBUTING.md](CONTRIBUTING.md).
Feel free to create an issue for bug report, feature requests or any discussion.

---

## 📜 License

Project is licensed under the [BSD-3-Clause](https://spdx.org/licenses/BSD-3-Clause.html) License. See [LICENSE](./LICENSE) for the full license text.
