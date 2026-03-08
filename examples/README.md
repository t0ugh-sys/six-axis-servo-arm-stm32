# examples 说明（可复制到 CubeIDE 工程）

这里的文件是“示例入口/片段”，目的是让你把 `firmware/` 这套可移植代码快速接入到 STM32CubeIDE（CubeMX 生成的 HAL 工程）里。

## 使用方式（推荐）

1. 在 CubeMX 里启用外设并生成代码
   - `I2C1`：连接 PCA9685（引脚按你的板子选择）
   - `USART2`：串口调试（`115200 8N1`），勾选 `USART2 global interrupt`
2. 把仓库里的 `firmware/Inc`、`firmware/Src` 加入你的 CubeIDE 工程并参与编译
3. 把需要的 `examples/*.c` 也加入工程，并在 `main.c` 的 USER CODE 区调用对应入口函数

## 推荐示例

- `cube_main_uart_arm_debug.c`
  - 入口：`app_main_uart_arm_debug()`
  - 功能：
    - 串口命令（ASCII 行命令，必须以 CRLF/LF 结束）：
      - `U0=1500`：直接设置 PCA9685 `CH0` 输出 `1500us`
      - `J0=60 T=800`：角度模式（关节 0 到 60°，用时 800ms）
      - `PING`：返回 `PONG`
    - 修复“串口丢字节/回显缺字符”：RX 中断只收字节，主循环处理（ring buffer）
    - 兼容 PuTTY 方向键/退格：过滤 ANSI ESC 序列、处理 DEL/Backspace
    - 尽量自愈 Reset 后偶发 `arm_init FAIL`：I2C 外设复位 + 总线恢复 + 重试

## I2C 引脚说明（用于 BUS recovery）

`cube_main_uart_arm_debug.c` 里默认假设 I2C1 是：
- `PB6=SCL`
- `PB7=SDA`

如果你实际使用的是 `PB8/PB9`（BlackPill 常见丝印 B8/B9），在你的工程编译选项里加宏即可覆盖：

- `APP_I2C1_GPIO_PORT=GPIOB`
- `APP_I2C1_SCL_PIN=GPIO_PIN_8`
- `APP_I2C1_SDA_PIN=GPIO_PIN_9`

（也可以直接在 `cube_main_uart_arm_debug.c` 顶部改默认宏。）

