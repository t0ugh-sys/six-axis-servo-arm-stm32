# 六轴舵机机械臂（STM32F411 + PCA9685）

本目录用于搭建/记录一套 **6 轴舵机机械臂** 的可落地方案：硬件接线、供电注意事项、标定流程，以及一份可直接拷贝进 CubeMX 工程的固件代码骨架（HAL）。

## 你当前的硬件配置

- MCU：STM32F411CEU6
- 开发板：BlackPill（STM32F411CE）
- 舵机：MG996R ×3（建议用于底座/肩/肘），MG90S ×3（建议用于腕/夹爪）
- PWM 驱动：PCA9685（I2C，16 路）

## 目录结构

- `docs/`：接线/供电/标定/串口协议/CubeMX 配置说明
- `firmware/`：可移植的 C 源码（放进你自己的 STM32CubeMX 工程里即可）
  - `firmware/Inc/`：头文件
  - `firmware/Src/`：源文件

## 快速开始（推荐顺序）

0. 按步骤走（从 0 到能动）：
   - `docs/steps.md`
1. 先看供电与接线：
   - `docs/power.md`
   - `docs/wiring.md`
2. 在 CubeMX 配好 I2C + UART（可选）：
   - `docs/cubemx_notes.md`
3. 把 `firmware/Inc/*`、`firmware/Src/*` 拷贝进你的工程，并在 `arm_config.h` 填好通道/限位：
   - `firmware/Inc/arm_config.h`
4. 跑标定流程，记录每个关节的安全范围与中心偏置：
   - `docs/calibration.md`

## 重要提醒（别跳过）

- **舵机电源必须独立**（6.0V 大电流），不要用开发板 5V 供舵机。
- **必须共地**：舵机电源 GND、PCA9685 GND、STM32 GND 全部连接在一起。
- PCA9685 的逻辑电源 `VCC` 用 **3.3V**（与 STM32 一致）。
- 在 PCA9685 的 `V+` 端子旁边加 **1000uF~2200uF** 电解电容，能显著减少抖动/复位。
