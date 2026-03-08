# 常见问题排查（FAQ）

## 1) 舵机抽动 / 乱转 / 慢慢漂移

优先按“供电问题”排查（90% 都在这里）：
- 舵机电源必须独立：`5~6V`、足够大电流（别用开发板 5V 或 ST-Link 供电带舵机）
- 必须共地：舵机电源 GND / PCA9685 GND / STM32 GND 三者连在一起
- PCA9685 两路电：
  - `VCC=3.3V`（逻辑）
  - `V+=5~6V`（舵机）
- `OE` 推荐直接接 `GND`（高电平会禁用输出）
- 在 PCA9685 的 `V+` 端并联 `1000uF~2200uF` 电解电容（耐压 >= 10V）

软件侧建议：
- 先跑 `examples/cube_main_pca_single_servo.c` 固定输出 `1500us`，排除插补/映射问题
- I2C 先用 `100kHz`，线尽量短

## 2) 舵机不动

- PCA9685 是否正确供电：
  - `VCC=3.3V`
  - `V+=5~6V`
- 是否共地
- 舵机是否插在正确通道（CH0/CH1…）
- 舵机插头方向是否正确（GND/V+/SIG 别插反）
- I2C 扫描是否能找到 PCA9685（常见 `0x40`）

## 3) `arm_init FAIL`

含义：
- 机械臂初始化阶段访问 PCA9685 失败（本质是 I2C 不通或总线卡住）

常见原因：
- 供电掉压（特别是接上机械臂后、舵机上电冲击大）
- Reset 时 PCA9685 还没稳定
- I2C 总线被拉低（SDA/SCL 短路、线材问题、模块损坏）

建议措施（从易到难）：
- 上电/复位后延时更久再 init（`300~1500ms`）
- I2C1 外设强制复位（`FORCE_RESET/RELEASE_RESET`）
- I2C 总线恢复：SCL 打 9 脉冲 + STOP
- 用 `examples/cube_main_uart_arm_debug.c`（已带“延时 + I2C recovery + 重试”）

## 4) 串口回显缺字符 / 发送字母缺失

典型表现：
- 发送 `ABCDEFGHIJKLMN` 回显缺字

根因：
- 在 `HAL_UART_RxCpltCallback()`（接收中断）里做阻塞 `HAL_UART_Transmit()` 回显/打印，导致 RX overrun 丢字节

解决：
- 中断里只收 1 字节并放入 ring buffer
- 主循环里再回显/拼行/解析
- 参考 `examples/cube_main_uart_arm_debug.c`

## 5) 命令没执行（没有 `OK/ERR`，命令粘成一串）

根因：
- 串口工具没发送真正的行尾（`CR`/`LF`），解析器一直等不到“行结束”

解决：
- 串口工具选择 ASCII 文本发送
- 勾选“发送行尾”：`CRLF` 或 `LF`
- 不要手打 `\\r\\n` 字符串

