# LightCorrespondence

基于 OpenCV 的“屏幕发光 + 摄像头接收”简化光通信实验。

> 文档与源码统一使用 UTF-8 编码。

本项目对应课程题目：

- 题 1：黑白两色表示二进制位（黑=1，白=0）
- 题 2：黑、白、红、蓝、绿、紫、黄、青表示八进制位（0~7）

## 快速开始

### 1. 配置与编译（Visual Studio 生成器，方案A）
```bash
cmake -S . -B build
cmake --build build --config Debug
```

可执行文件通常在：

```bash
build/Debug/LightCorrespondence.exe
```

> 如果你想使用 Ninja，请改用单独目录（如 `build-ninja`），避免与 `build` 目录冲突。

### 2. 运行方式

- 命令行模式：直接带参数运行。
- 交互模式：不带参数运行，程序会提示输入运行方式和传输内容。

```bash
.\build\Debug\LightCorrespondence.exe
```

### 3. 命令示例（PowerShell）
```bash
# 题1：二进制发送（黑=1，白=0）
.\build\Debug\LightCorrespondence.exe send binary 1

# 题1：二进制接收（摄像头采样后输出0/1）
.\build\Debug\LightCorrespondence.exe receive binary

# 题2：八进制发送（0~7）
.\build\Debug\LightCorrespondence.exe send octal 6

# 图像模拟：生成纯色图用于离线验证
.\build\Debug\LightCorrespondence.exe simulate octal 3 frame.png

# 图像解码：从图像读取颜色并还原消息
.\build\Debug\LightCorrespondence.exe decode-image octal frame.png
```

## 接口说明

- 题1接口：`binary_comm::encode(int msg)`、`binary_comm::decode(cv::Scalar color)`
- 题2接口：`octal_comm::encode(int msg)`、`octal_comm::decode(cv::Scalar color)`
- 公共接口：`Channel::send(int msg)`、`Channel::receive()`

## 接收端验证建议

1. 先做离线验证（不依赖摄像头）：

```bash
.\build\Debug\LightCorrespondence.exe simulate octal 3 frame.png
.\build\Debug\LightCorrespondence.exe decode-image octal frame.png
```

输出 `3` 说明解码逻辑正常。

2. 再做两机联调：

- 发送端执行 `send`
- 接收端执行 `receive`
- 对比发送值与接收值是否一致
