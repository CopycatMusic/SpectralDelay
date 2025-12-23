# CopyCat Spectral Delay v0.1beta

## 安装步骤

### macOS

1. **解压文件**
   - 解压 `CopyCat_Spectral_Delay_v0.1beta.zip`

2. **安装 VST3 插件**
   - 将 `CopyCat Spectral Delay.vst3` 文件夹拷贝到：
   ```
   ~/Library/Audio/Plug-Ins/VST3/
   ```
   > 💡 在 Finder 中按 `Cmd + Shift + G`，粘贴上述路径即可快速跳转

3. **重新扫描插件**
   - 重启你的 DAW，或在 DAW 中执行"重新扫描插件"

4. **加载插件**
   - 在效果器列表中找到 "CopyCat Spectral Delay"

---

## 使用说明

### 基本操作
- **左侧**：频谱显示和曲线编辑区
- **右侧**：参数控制面板

### 参数类型
| 按钮 | 功能 | 范围 |
|------|------|------|
| Gain | 每个频段的增益 | 0-200% |
| Time | 每个频段的延迟（Phase/Delay 模式可切换）| Phase: 0-360° / Delay: 0-2000ms |
| Feedback | 每个频段的反馈量 | 0-100% |

### 曲线编辑
- 在左侧频谱图上拖动黄色控制点调整曲线
- 曲线会根据当前选择的参数类型影响对应的音频处理

---

## 系统要求
- macOS 10.13 或更高版本
- 支持 VST3 的 DAW（如 Ableton Live, Logic Pro, REAPER 等）

## 已知问题
- 这是 Beta 测试版本，可能存在未知 bug
- 如发现问题，请反馈给开发者

## 版本信息
- 版本: v0.1beta
- 作者: CopyCat
 