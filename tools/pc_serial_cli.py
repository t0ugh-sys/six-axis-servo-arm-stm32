#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
PC 串口命令行工具（可选上位机）

用途：
- 通过 USB-TTL（CH340/CP2102）向 STM32 发送控制命令
- 与本仓库 `firmware/Src/arm_cmd.c` 的命令格式配套

依赖：
- 本脚本使用 `pyserial`（仅用于 PC 端工具，不影响 STM32 固件）。

安装：
  pip install pyserial

用法示例：
  python tools/pc_serial_cli.py --list
  python tools/pc_serial_cli.py -p COM5
  python tools/pc_serial_cli.py -p COM5 --send "J0=60 T=800"

交互模式（默认）：
- 启动后输入一行命令回车发送
- 输入 `exit` 退出

命令示例（末尾会自动加 \\r\\n）：
  J0=90 J1=90 J2=90 J3=90 J4=90 J5=90 T=1000
  J0=60 T=800
  U0=1500
"""

from __future__ import annotations

import argparse
import sys
import time
from typing import Optional


def _print_pyserial_hint() -> int:
  print("未检测到 pyserial。")
  print("")
  print("请先安装：")
  print("  pip install pyserial")
  print("")
  print("然后重试，例如：")
  print("  python tools/pc_serial_cli.py --list")
  print("  python tools/pc_serial_cli.py -p COM5")
  return 2


def _try_import_pyserial():
  try:
    import serial  # type: ignore
    from serial.tools import list_ports  # type: ignore

    return serial, list_ports
  except Exception:
    return None, None


def _list_ports(list_ports_mod) -> int:
  ports = list(list_ports_mod.comports())
  if not ports:
    print("未发现串口。请确认 CH340 已插入并正确安装驱动。")
    return 1

  for p in ports:
    # p.device: COMx；p.description：设备描述
    print(f"{p.device}\t{p.description}")
  return 0


def _open_serial(serial_mod, port: str, baud: int, timeout_s: float):
  # Windows 下建议显式设置 timeout，避免 readline 卡死
  return serial_mod.Serial(port=port, baudrate=baud, timeout=timeout_s)


def _send_line(ser, line: str, line_ending: str) -> None:
  payload = (line + line_ending).encode("utf-8", errors="replace")
  ser.write(payload)
  ser.flush()


def main(argv: Optional[list[str]] = None) -> int:
  serial_mod, list_ports_mod = _try_import_pyserial()
  if serial_mod is None or list_ports_mod is None:
    return _print_pyserial_hint()

  parser = argparse.ArgumentParser(description="PC 串口命令行工具（发送机械臂命令）")
  parser.add_argument("-p", "--port", help="串口号，例如 COM5", default=None)
  parser.add_argument("-b", "--baud", help="波特率（默认 115200）", type=int, default=115200)
  parser.add_argument("--timeout", help="串口读超时（秒）", type=float, default=0.2)
  parser.add_argument(
    "--line-ending",
    help="行尾（默认 CRLF）",
    choices=["crlf", "lf", "cr"],
    default="crlf",
  )
  parser.add_argument("--list", help="列出可用串口", action="store_true")
  parser.add_argument("--send", help="发送一条命令后退出", default=None)
  args = parser.parse_args(argv)

  if args.list:
    return _list_ports(list_ports_mod)

  if args.port is None:
    print("缺少串口号。先运行：python tools/pc_serial_cli.py --list")
    return 1

  if args.line_ending == "crlf":
    line_ending = "\r\n"
  elif args.line_ending == "lf":
    line_ending = "\n"
  else:
    line_ending = "\r"

  try:
    ser = _open_serial(serial_mod, args.port, args.baud, args.timeout)
  except Exception as e:
    print(f"打开串口失败：{e}")
    return 1

  # 给 MCU 一点时间（某些 USB-TTL 插入时会抖一下）
  time.sleep(0.05)

  print(f"已打开：{args.port} @ {args.baud}")
  print("输入一行命令回车发送；输入 exit 退出。")
  print("示例：J0=60 T=800")

  try:
    if args.send is not None:
      _send_line(ser, args.send.strip(), line_ending)
      return 0

    while True:
      try:
        line = sys.stdin.readline()
      except KeyboardInterrupt:
        break

      if not line:
        break

      line = line.strip()
      if not line:
        continue
      if line.lower() in ("exit", "quit"):
        break

      _send_line(ser, line, line_ending)
  finally:
    try:
      ser.close()
    except Exception:
      pass

  return 0


if __name__ == "__main__":
  raise SystemExit(main())

