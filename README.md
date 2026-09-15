<div align="center">
  <h1>QRB ROS Tensor Process</h1>
  <p>ROS2 package for processing input and output of AI model.</p>
  <a href="https://ubuntu.com/download/qualcomm-iot" target="_blank"><img src="https://img.shields.io/badge/Qualcomm%20Ubuntu-E95420?style=for-the-badge&logo=ubuntu&logoColor=white" alt="Qualcomm Ubuntu"></a>
  <a href="https://docs.ros.org/en/jazzy/" target="_blank"><img src="https://img.shields.io/badge/ROS%20Jazzy-1c428a?style=for-the-badge&logo=ros&logoColor=white" alt="Jazzy"></a>
</div>

> [!NOTE]
> This repository is currently migrating to ROS 2 Lyrical. Documentation may still reference Jazzy, and some features may not be fully supported yet.

---

## 👋 Overview

**qrb_ros_tensor_process** is a ROS2 package for:
- Transform the input data into the input tensor required by the AI model.
- Handle the output tensor produced by the AI model.

<div align="center">
  <img src="./docs/assets/workflow.png" alt="workflow">
</div>

<br>

**qrb_ros_tensor_process** currently only provide foucs on image data processing, you can access more details in [cv_tensor_common_process](./cv_tensor_common_process).

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
