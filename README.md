# CopyCat Spectral Delay

[English](#english) | [中文](#中文)

---

## English

### Overview

**CopyCat Spectral Delay** is a spectral audio effect plugin that processes audio through STFT (Short-Time Fourier Transform) and applies independent parameters to each frequency bin. Create unique delay, gain, and feedback effects that vary across the frequency spectrum.

**Author:** CopyCat

### Features

| Feature | Description |
|---------|-------------|
| **Per-Bin Delay** | Apply different delay times to different frequency bands |
| **Per-Bin Gain** | Apply different gain values to different frequency bands |
| **Per-Bin Feedback** | Control feedback amount per frequency band |
| **Phase/Delay Mode** | Switch between phase rotation and actual delay modes |
| **Real-time Spectrum Display** | View input or output spectrum in real-time |
| **Multi-point Curve Editor** | Click to add points, drag to move, double-click to delete |
| **FFT Size** | Adjustable from 256 to 8192 |
| **Dry/Wet Mix** | Blend between original and processed signal |
| **Parameter Multipliers** | Separate multiply sliders for Gain, Delay, and Feedback |
| **Feedback Phase Invert** | Invert feedback signal phase |
| **Line/Bar Display** | Toggle between line and bar chart visualization |
| **State Saving** | Full DAW session save/recall support |

### Screenshot

<!-- TODO: Add screenshot -->
*Screenshot coming soon*

### Supported Formats & Platforms

| Platform | Architecture | Format | Status |
|----------|--------------|--------|--------|
| macOS | Universal Binary (Intel + Apple Silicon) | VST3, AU | ✅ Notarized |
| Windows | x64 | VST3 | Unsigned |

### Installation

#### 🍎 macOS
1. Download `CopyCat_Spectral_Delay_macOS_v1.0.0.zip`.
2. Open the `.dmg` file.
3. Drag the **VST3** and **Components (AU)** files to their respective installation folders shown in the window.
4. Restart your DAW.

#### 🪟 Windows
1. Download `CopyCat_Spectral_Delay_Win_v1.0.0.zip`.
2. Run the `CopyCat_Spectral_Delay_Win_v1.0.0.exe` installer.
3. Follow the wizard steps (installed to `Common Files\VST3` by default).
4. Restart your DAW.

#### 🛠️ Manual Installation (If installers fail)
If the automated installers do not work, you can manually copy the plugin files:

**macOS:**
- Copy `CopyCat Spectral Delay.vst3` to: `/Library/Audio/Plug-Ins/VST3/`
- Copy `CopyCat Spectral Delay.component` to: `/Library/Audio/Plug-Ins/Components/`

**Windows:**
- Copy the `CopyCat Spectral Delay.vst3` folder to: `C:\Program Files\Common Files\VST3\`

### Building from Source

#### Prerequisites
- [JUCE 8.0.12](https://juce.com/)
- CMake 3.15+
- C++17 compatible compiler

#### macOS
```bash
# Clone the repository
git clone https://github.com/CopycatMusic/SpectralDelay.git
cd SpectralDelay

# Configure (adjust JUCE path as needed)
cmake -B build -DJUCE_DIR=/Applications/JUCE

# Build
cmake --build build --config Release
```

#### Windows
```powershell
# Clone the repository
git clone https://github.com/CopycatMusic/SpectralDelay.git
cd SpectralDelay

# Configure (adjust JUCE path as needed)
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=C:/JUCE

# Build
cmake --build build --config Release
```

### Usage

The plugin interface is divided into two main sections:

#### Left Panel - Spectrum Display & Curve Editor
- **Spectrum Visualization**: Real-time FFT magnitude display (logarithmic frequency scale)
- **Curve Editing**:
  - **Single-click**: Add a control point
  - **Drag**: Move a control point
  - **Double-click**: Delete a control point

#### Right Panel - Control Panel
- **Parameter Selection**: Choose between Gain, Time (Delay), or Feedback
- **Mode Toggle**: Switch between Phase and Delay modes
- **Multiply Sliders**: Scale the effect of each parameter (0-100%)
- **FFT Size**: Select FFT window size (256-8192)
- **Display**: Toggle between Line and Bar visualization
- **Source**: Switch between Input and Output spectrum display
- **Mix**: Dry/Wet balance control

### References

- **STFT Implementation**: Based on [hollance/fft-juce](https://github.com/hollance/fft-juce)
- **Framework**: [JUCE](https://juce.com/)

### License

**All Rights Reserved** © 2025 CopyCat

This software is provided for personal use only. Redistribution or modification without permission is prohibited.

### Acknowledgements

*Coming soon*

---

## 中文

### 概述

**CopyCat Spectral Delay**（频谱延迟）是一款频谱音频效果器插件，通过 STFT（短时傅里叶变换）处理音频，并为每个频段独立设置参数。可创建在频谱上变化的独特延迟、增益和反馈效果。

**作者：** CopyCat

### 功能特性

| 功能 | 说明 |
|------|------|
| **每频段延迟** | 为不同频段设置不同的延迟时间 |
| **每频段增益** | 为不同频段设置不同的增益值 |
| **每频段反馈** | 为不同频段控制反馈量 |
| **相位/延迟模式** | 在相位旋转和实际延迟模式之间切换 |
| **实时频谱显示** | 实时查看输入或输出频谱 |
| **多点曲线编辑器** | 单击添加点、拖拽移动点、双击删除点 |
| **FFT 窗口大小** | 可调范围 256 - 8192 |
| **干湿混合** | 混合原始信号和处理后信号 |
| **参数倍率** | Gain、Delay、Feedback 各自独立的倍率滑块 |
| **反馈相位反转** | 反转反馈信号相位 |
| **线条/柱状显示** | 在线条图和柱状图之间切换 |
| **状态保存** | 完整支持 DAW 工程保存/加载 |

### 界面截图

<!-- TODO: 添加截图 -->
*截图即将添加*

### 支持格式与平台

| 平台 | 架构 | 格式 | 状态 |
|----------|--------------|--------|--------|
| macOS | Universal Binary（Intel + Apple Silicon） | VST3, AU | ✅ 已公证 |
| Windows | x64 | VST3 | 未签名 |

### 安装说明

#### 🍎 macOS
1. 下载 `CopyCat_Spectral_Delay_macOS_v1.0.0.zip`。
2. 打开 `.dmg` 文件。
3. 将 **VST3** 和 **Components (AU)** 文件拖动到窗口中显示的对应安装文件夹中。
4. 重启你的 DAW。

#### 🪟 Windows
1. 下载 `CopyCat_Spectral_Delay_Win_v1.0.0.zip`。
2. 运行 `CopyCat_Spectral_Delay_Win_v1.0.0.exe` 安装程序名。
3. 按照向导步骤操作（默认安装到 `Common Files\VST3`）。
4. 重启你的 DAW。

#### 🛠️ 手动安装（如果安装程序无法运行）
如果自动安装程序无法工作，你可以手动复制插件文件：

**macOS:**
- 将 `CopyCat Spectral Delay.vst3` 复制到：`/Library/Audio/Plug-Ins/VST3/`
- 将 `CopyCat Spectral Delay.component` 复制到：`/Library/Audio/Plug-Ins/Components/`

**Windows:**
- 将 `CopyCat Spectral Delay.vst3` 文件夹复制到：`C:\Program Files\Common Files\VST3\`

### 从源码构建

#### 前置要求
- [JUCE 8.0.12](https://juce.com/)
- CMake 3.15+
- 支持 C++17 的编译器

#### macOS
```bash
# 克隆仓库
git clone https://github.com/CopycatMusic/SpectralDelay.git
cd SpectralDelay

# 配置（根据需要调整 JUCE 路径）
cmake -B build -DJUCE_DIR=/Applications/JUCE

# 构建
cmake --build build --config Release
```

#### Windows
```powershell
# 克隆仓库
git clone https://github.com/CopycatMusic/SpectralDelay.git
cd SpectralDelay

# 配置（根据需要调整 JUCE 路径）
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=C:/JUCE

# 构建
cmake --build build --config Release
```

### 使用说明

插件界面分为两个主要部分：

#### 左侧面板 - 频谱显示与曲线编辑
- **频谱可视化**：实时 FFT 幅度显示（对数频率刻度）
- **曲线编辑**：
  - **单击**：添加控制点
  - **拖拽**：移动控制点
  - **双击**：删除控制点

#### 右侧面板 - 控制面板
- **参数选择**：在 Gain（增益）、Time（延迟时间）、Feedback（反馈）之间选择
- **模式切换**：在 Phase（相位）和 Delay（延迟）模式之间切换
- **倍率滑块**：缩放各参数的效果（0-100%）
- **FFT Size**：选择 FFT 窗口大小（256-8192）
- **Display**：在线条图和柱状图之间切换
- **Source**：在输入和输出频谱显示之间切换
- **Mix**：干湿混合控制

### 参考资料

- **STFT 实现**：基于 [hollance/fft-juce](https://github.com/hollance/fft-juce)
- **框架**：[JUCE](https://juce.com/)

### 许可证

**保留所有权利** © 2025 CopyCat

本软件仅供个人使用。未经许可不得重新分发或修改。

### 致谢

*即将添加*
