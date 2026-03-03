# 六轴舵机机械臂（STM32F411 BlackPill + PCA9685）

这个仓库记录一套“能落地”的 6 轴舵机机械臂方案：接线/供电注意事项、CubeMX 配置要点、标定流程，以及一份可直接拷进 STM32CubeIDE 工程的 HAL C 代码（PCA9685 驱动 + 舵机映射 + 机械臂插补）。

## 你的硬件配置（当前）

- MCU：STM32F411CEU6
- 开发板：BlackPill (STM32F411CE)
- 舵机：MG996R × 3、MG90S × 3
- PWM 驱动：PCA9685（I2C，16 路）
- 下载/调试：ST-Link（SWD）

## 目录结构

- `docs/`：接线、供电、CubeMX、标定与调试说明
- `firmware/`：可移植的 C 源码（复制到你的 CubeIDE 工程即可）
  - `firmware/Inc/`：头文件
  - `firmware/Src/`：源文件
- `examples/`：可复制粘贴到 CubeIDE 的 `main.c` 示例（最小化接入/标定）

## 快速开始（推荐顺序）

1) 先把供电与接线搞对（否则再好的程序也会“抽动/乱转”）

- `docs/power.md`
- `docs/wiring.md`
- `docs/bom.md`

2) CubeMX 新建工程并配置 I2C（可选 UART）

- `docs/cubemx_notes.md`

3) 拷贝固件代码到你的工程并编译通过

- `docs/steps.md`

4) 开始标定（不使用按键，直接用 ST-Link 调试器改变量/看脉宽）

- `docs/calibration.md`
- `docs/debug_stlink.md`
- 常见问题：`docs/troubleshooting.md`

5) （可选）PC 串口控制工具

- `tools/README.md`

## 重要提醒（强烈建议读）

- 舵机电源必须独立（5~6V 大电流），不要用开发板 5V 给舵机供电。
- 必须共地：舵机电源 GND、PCA9685 GND、STM32 GND 必须连在一起。
- PCA9685：`VCC` 接 3.3V（逻辑电源），`V+` 接 5~6V（舵机电源）。
- `OE` 引脚高电平会禁止输出；建议接 GND（或接 MCU GPIO 控制），不要接 5V。
- 建议在 PCA9685 的 `V+` 端附近并一个 1000uF~2200uF 电解电容（耐压 >= 10V）抑制电源压降导致的抽动/复位。
