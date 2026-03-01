#pragma once

#include <stdint.h>

#include "stm32f4xx_hal.h"

// 默认 I2C 7-bit 地址通常为 0x40（A0~A5 可改）
typedef struct {
  I2C_HandleTypeDef* hi2c;
  uint8_t addr_7bit;
  float pwm_freq_hz;
} Pca9685;

HAL_StatusTypeDef pca9685_init(Pca9685* dev, I2C_HandleTypeDef* hi2c, uint8_t addr_7bit, float pwm_freq_hz);
HAL_StatusTypeDef pca9685_set_pwm_freq(Pca9685* dev, float pwm_freq_hz);
HAL_StatusTypeDef pca9685_set_pwm(Pca9685* dev, uint8_t channel, uint16_t on_tick, uint16_t off_tick);

// 便于舵机：以“脉宽 us”设置占空（on=0, off=tick）
HAL_StatusTypeDef pca9685_set_pulse_us(Pca9685* dev, uint8_t channel, uint16_t pulse_us);

