# CubeMX 配置建议（STM32F411）

你只需要把本目录的 `firmware/Inc` 和 `firmware/Src` 拷进 CubeMX 生成的工程，然后做以下外设配置即可。

## I2C

- 开启一个 I2C（I2C1/I2C2 都行）
- 速率建议先用 100kHz（距离短也可 400kHz）
- 确保 SDA/SCL 配置为 AF 开漏（Open Drain）+ 上拉（板上通常已有）

### BlackPill（STM32F411CE）推荐配置

- 推荐用 `I2C1`：
  - `PB8 = I2C1_SCL`（板子丝印一般写 `B8`）
  - `PB9 = I2C1_SDA`（板子丝印一般写 `B9`）
- 备选用 `PB6/PB7` 作为 I2C1（看你板子引脚占用情况）

## UART（可选）

如果你要用串口命令控制：
- 开一个 UART（例如 USART1/USART2）
- 115200 8N1
- 建议用中断接收或 DMA（先简单也可轮询）

### BlackPill 常见 UART 引脚（任选其一）

- `USART1`：`PA9=TX`，`PA10=RX`
- `USART2`：`PA2=TX`，`PA3=RX`

## SysTick / 定时调用

机械臂的插补更新建议 20ms 一次（与 50Hz 舵机刷新一致）：
- 你可以用 `HAL_GetTick()` 在主循环里做 20ms 任务
- 或开一个 TIM 周期中断 20ms 调 `arm_update(...)`
