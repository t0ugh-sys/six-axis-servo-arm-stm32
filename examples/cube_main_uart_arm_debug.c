/*
 * UART command debug entry for STM32CubeIDE projects.
 *
 * 目标：
 * - 通过串口命令（U/J/T）调试 6 轴舵机机械臂（PCA9685 + I2C）
 * - 修复常见“丢字节/回显缺字符”问题：中断里不阻塞发送，使用 RX ring buffer
 * - 兼容 PuTTY 的方向键/退格：过滤 ANSI ESC 序列、处理 Backspace/DEL
 * - 尽量自愈 Reset 后偶发的 arm_init FAIL：I2C 外设复位 + 总线恢复 + 重试
 *
 * 使用方法：
 * 1) CubeMX：启用 I2C1 + USART2，并生成代码
 *    - USART2：115200 8N1，勾选 “USART2 global interrupt”
 *    - I2C1：按你的板子选择引脚（常见 PB6/PB7 或 PB8/PB9）
 * 2) 将本文件加入你的工程并参与编译（和 firmware/Inc firmware/Src 一起）
 * 3) 在 CubeMX 生成的 main.c 的 USER CODE 区调用：
 *      app_main_uart_arm_debug();
 *
 * 串口命令（ASCII 文本发送，末尾必须 CRLF 或 LF）：
 * - U0=1500              直接设置 PCA9685 CH0 脉宽（us）
 * - U0=1500 U1=1600      一行设置多个通道
 * - J0=60 T=800          角度模式：关节0到60°，插补时间800ms
 * - PING                 返回 PONG（用于验证串口通路）
 */

#include "arm.h"
#include "arm_cmd.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

// ---- 可按需要修改：I2C1 引脚（用于 BUS recovery）----
// 默认按 CubeMX 常见配置：PB6=SCL PB7=SDA
#ifndef APP_I2C1_GPIO_PORT
#define APP_I2C1_GPIO_PORT GPIOB
#endif
#ifndef APP_I2C1_SCL_PIN
#define APP_I2C1_SCL_PIN GPIO_PIN_6
#endif
#ifndef APP_I2C1_SDA_PIN
#define APP_I2C1_SDA_PIN GPIO_PIN_7
#endif

// ---- UART RX ring buffer ----
#define APP_RX_BUF_SIZE 512u
static volatile uint8_t g_rx_buf[APP_RX_BUF_SIZE];
static volatile uint16_t g_rx_head = 0;
static volatile uint16_t g_rx_tail = 0;
static uint8_t g_rx_byte = 0;

static void rx_push(uint8_t b) {
  uint16_t next = (uint16_t)((g_rx_head + 1u) % APP_RX_BUF_SIZE);
  if (next == g_rx_tail) return;  // overflow: drop
  g_rx_buf[g_rx_head] = b;
  g_rx_head = next;
}

static bool rx_pop(uint8_t* out) {
  if (g_rx_tail == g_rx_head) return false;
  *out = g_rx_buf[g_rx_tail];
  g_rx_tail = (uint16_t)((g_rx_tail + 1u) % APP_RX_BUF_SIZE);
  return true;
}

static void uart_start_rx_it(void) {
  (void)HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
}

static void uart_send_str(const char* s) {
  if (s == NULL) return;
  (void)HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), 200);
}

// ---- arm / command state ----
static Arm g_arm;
static ArmCmd g_cmd;

static uint32_t g_arm_last_ms = 0;

// 手动脉宽模式（收到 U... 后进入），暂停 arm_update，避免被角度刷新覆盖
static volatile uint8_t g_manual_mode = 0;

// ---- I2C recovery helpers ----
static void i2c1_force_reset_periph(void) {
  __HAL_RCC_I2C1_FORCE_RESET();
  HAL_Delay(1);
  __HAL_RCC_I2C1_RELEASE_RESET();
  HAL_Delay(1);
}

static void i2c1_bus_recovery(void) {
  // 关 I2C1 时钟，避免外设抢占引脚
  __HAL_RCC_I2C1_CLK_DISABLE();

  // 把 SCL/SDA 改为 GPIO 开漏输出，上拉
  if (APP_I2C1_GPIO_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
#ifdef GPIOA
  if (APP_I2C1_GPIO_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
#endif

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = APP_I2C1_SCL_PIN | APP_I2C1_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(APP_I2C1_GPIO_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN | APP_I2C1_SDA_PIN, GPIO_PIN_SET);
  HAL_Delay(2);

  // 给 SCL 打 9 个脉冲，尝试释放 SDA
  for (int i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
  }

  // 发送 STOP：SDA low -> SCL high -> SDA high
  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SDA_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SDA_PIN, GPIO_PIN_SET);
  HAL_Delay(1);

  // 恢复 I2C：重新 Init 会调用 HAL_I2C_MspInit() 配置 AF，所以这里不手工设 AF
}

static HAL_StatusTypeDef arm_init_with_retries(void) {
  // Reset 场景经常需要更久的电源稳定时间（舵机上电电流冲击 / PCA 上电慢）
  HAL_Delay(800);

  for (int attempt = 1; attempt <= 10; attempt++) {
    (void)HAL_I2C_DeInit(&hi2c1);
    i2c1_force_reset_periph();
    (void)HAL_I2C_Init(&hi2c1);

    // 先探测 PCA9685 默认地址 0x40 是否 ACK（避免直接 init 一路失败不明原因）
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x40u << 1), 2, 80) != HAL_OK) {
      uart_send_str("0x40 NACK -> recover\r\n");
      i2c1_bus_recovery();
      HAL_Delay(80);
      continue;
    }

    HAL_StatusTypeDef st = arm_init(&g_arm, &hi2c1);
    if (st == HAL_OK) {
      uart_send_str("arm_init OK\r\n");
      return HAL_OK;
    }

    uart_send_str("arm_init FAIL -> recover\r\n");
    i2c1_bus_recovery();
    HAL_Delay(120);
  }

  return HAL_ERROR;
}

// ---- manual U parsing ----
static const char* skip_spaces(const char* s) {
  while (s != NULL && *s != '\0' && isspace((unsigned char)*s)) s++;
  return s;
}

static bool parse_u_token(const char* token, uint8_t* out_ch, uint16_t* out_us) {
  if (token == NULL || out_ch == NULL || out_us == NULL) return false;
  if (token[0] != 'U' && token[0] != 'u') return false;

  char* end = NULL;
  long ch = strtol(token + 1, &end, 10);
  if (end == token + 1) return false;
  if (ch < 0 || ch > 15) return false;
  if (*end != '=') return false;

  long us = strtol(end + 1, &end, 10);
  (void)end;
  if (us < 0) us = 0;
  if (us > 3000) us = 3000;

  *out_ch = (uint8_t)ch;
  *out_us = (uint16_t)us;
  return true;
}

static bool handle_manual_u_line(const char* line) {
  bool any = false;
  const char* p = line;

  while (p != NULL && *p != '\0') {
    p = skip_spaces(p);
    if (*p == '\0') break;

    const char* start = p;
    while (*p != '\0' && !isspace((unsigned char)*p)) p++;

    char token[32];
    size_t n = (size_t)(p - start);
    if (n >= sizeof(token)) n = sizeof(token) - 1;
    memcpy(token, start, n);
    token[n] = '\0';

    uint8_t ch = 0;
    uint16_t us = 0;
    if (parse_u_token(token, &ch, &us)) {
      (void)pca9685_set_pulse_us(&g_arm.pca, ch, us);
      any = true;
    }
  }

  if (any) g_manual_mode = 1;
  return any;
}

// ---- UART callbacks ----
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART2) {
    rx_push(g_rx_byte);
    uart_start_rx_it();
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART2) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    uart_start_rx_it();
  }
}

void app_main_uart_arm_debug(void) {
  uart_send_str("UART ready\r\n");

  uart_start_rx_it();

  if (arm_init_with_retries() != HAL_OK) {
    uart_send_str("arm_init FINAL FAIL\r\n");
    while (1) HAL_Delay(200);
  }

  arm_cmd_init(&g_cmd, 800);
  uart_send_str("arm ready\r\n");
  uart_send_str("CMD: U0=1500 | J0=60 T=800 | PING\r\n");

  g_arm_last_ms = HAL_GetTick();

  // 过滤 PuTTY 控制序列（方向键等）：ESC [ ... <final>
  uint8_t esc_state = 0;  // 0=none, 1=ESC seen, 2=CSI

  char line[128];
  uint32_t line_len = 0;

  while (1) {
    uint32_t now = HAL_GetTick();

    if (!g_manual_mode) {
      if (now - g_arm_last_ms >= ARM_UPDATE_PERIOD_MS) {
        g_arm_last_ms = now;
        (void)arm_update(&g_arm);
      }
    }

    uint8_t b;
    while (rx_pop(&b)) {
      // ANSI ESC filtering
      if (esc_state == 0 && b == 0x1B) {
        esc_state = 1;
        continue;
      }
      if (esc_state == 1) {
        esc_state = (b == '[') ? 2 : 0;
        continue;
      }
      if (esc_state == 2) {
        // CSI sequence ends with 0x40..0x7E
        if (b >= 0x40 && b <= 0x7E) esc_state = 0;
        continue;
      }

      // backspace / del: line editor
      if (b == 0x08 || b == 0x7F) {
        if (line_len > 0) line_len--;
        continue;
      }

      // ignore other control chars (except CR/LF)
      if (b < 0x20 && b != '\r' && b != '\n') continue;

      // echo accepted bytes in main loop (avoid ISR transmit)
      (void)HAL_UART_Transmit(&huart2, &b, 1, 10);

      char c = (char)b;
      if (c == '\r' || c == '\n') {
        if (line_len == 0) continue;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        line[line_len] = '\0';

        const char* cmd = skip_spaces(line);
        if (strcmp(cmd, "PING") == 0 || strcmp(cmd, "ping") == 0 || strcmp(cmd, "Ping") == 0) {
          uart_send_str("\r\nPONG\r\n");
          line_len = 0;
          continue;
        }

        if (cmd[0] == 'U' || cmd[0] == 'u') {
          bool ok = handle_manual_u_line(cmd);
          uart_send_str(ok ? "\r\nOK\r\n" : "\r\nERR\r\n");
        } else {
          g_manual_mode = 0;
          bool ok = arm_cmd_execute_line(&g_cmd, &g_arm, cmd);
          uart_send_str(ok ? "\r\nOK\r\n" : "\r\nERR\r\n");
        }

        line_len = 0;
      } else {
        if (line_len < (sizeof(line) - 1)) {
          line[line_len++] = c;
        } else {
          line_len = 0;
        }
      }
    }
  }
}

