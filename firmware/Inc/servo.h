#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pca9685.h"

typedef struct {
  uint8_t channel;       // PCA9685 channel 0..15
  bool inverted;         // true: angle increase -> pulse decrease
  float angle_min_deg;   // software clamp (mechanical safe range)
  float angle_max_deg;
  float offset_deg;      // center offset (calibration)
  uint16_t us_min;       // pulse width at angle_min_deg
  uint16_t us_max;       // pulse width at angle_max_deg
} Servo;

void servo_init(Servo* s,
                uint8_t channel,
                bool inverted,
                float angle_min_deg,
                float angle_max_deg,
                float offset_deg,
                uint16_t us_min,
                uint16_t us_max);

// 将“目标角度（deg）”映射为脉宽（us），并写入 PCA9685
HAL_StatusTypeDef servo_write_angle(Pca9685* dev, const Servo* s, float angle_deg);

// 标定用：直接写入脉宽（us）
HAL_StatusTypeDef servo_write_us(Pca9685* dev, const Servo* s, uint16_t pulse_us);

