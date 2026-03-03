# tools（上位机工具）

## pc_serial_cli.py

一个最小的 PC 串口命令行工具，用来向 STM32 发送命令（配套 `firmware/Src/arm_cmd.c`）。

### 安装依赖

仅需要 PC 端安装 `pyserial`（不影响固件）：

```bash
pip install pyserial
```

### 列出串口

```bash
python tools/pc_serial_cli.py --list
```

### 交互发送（推荐）

```bash
python tools/pc_serial_cli.py -p COM5
```

然后输入命令回车发送，例如：

```text
J0=60 T=800
U0=1500
```

### 单次发送

```bash
python tools/pc_serial_cli.py -p COM5 --send "J0=60 T=800"
```

