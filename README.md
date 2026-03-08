# six-axis-servo-arm-stm32

STM32F411（BlackPill）+ PCA9685（I2C）+ 6 路舵机（MG996R ×3 + MG90S ×3）的六轴机械臂开源项目。

目标是给出一套“能落地、可复现”的方案：接线/供电注意事项、CubeMX 配置要点、以及可直接拷进 STM32CubeIDE 工程的 HAL C 代码（PCA9685 驱动 + 舵机映射 + 串口命令调试）。

## 目录结构

- `docs/`：接线、供电、CubeMX 配置、调试与排坑
- `firmware/`：可移植的 HAL C 源码（不包含 `main()`）
  - `firmware/Inc/`：头文件
  - `firmware/Src/`：源文件
- `examples/`：示例入口（把文件加入 CubeIDE 工程后调用入口函数）
- `tools/`：PC 端小工具（可选）

## 快速开始（推荐顺序）

1. 看供电与接线（先把“电”搞稳）
   - `docs/power.md`
   - `docs/wiring.md`
2. CubeMX 配置 I2C + UART
   - `docs/cubemx_notes.md`
3. 把 `firmware/` 加入你的 CubeIDE 工程并编译通过
   - `docs/steps.md`
4. 用串口命令调试（推荐）
   - `examples/cube_main_uart_arm_debug.c`
   - 命令说明：`docs/command_protocol.md`
5. 常见问题排查
   - `docs/troubleshooting.md`

## 重要提醒（强烈建议按这个做）

- 舵机电源必须独立：`5~6V`、足够大电流；不要用开发板 5V 或 ST-Link 供电带舵机。
- 必须共地：舵机电源 GND / PCA9685 GND / STM32 GND 三者必须相连。
- PCA9685：
  - `VCC=3.3V`（逻辑）
  - `V+=5~6V`（舵机）
  - `OE` 推荐接 `GND`
- 在 PCA9685 的 `V+` 端并联 `1000uF~2200uF` 电容（耐压 >= 10V）抑制掉压与抽动。

## License

MIT（见 `LICENSE`）。

