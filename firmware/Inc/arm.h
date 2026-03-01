#pragma once

#include <stdint.h>

#include "arm_config.h"
#include "servo.h"

typedef struct {
  Pca9685 pca;
  Servo joints[ARM_JOINT_COUNT];

  float current_deg[ARM_JOINT_COUNT];
  float target_deg[ARM_JOINT_COUNT];

  uint32_t move_total_steps;
  uint32_t move_step_idx;
  float step_delta_deg[ARM_JOINT_COUNT];
} Arm;

// 初始化：需要你传入 CubeMX 生成的 I2C 句柄
HAL_StatusTypeDef arm_init(Arm* arm, I2C_HandleTypeDef* hi2c);

// 立即写当前角（不插补）
HAL_StatusTypeDef arm_write_now(Arm* arm);

// 设置目标角并启动插补（duration_ms 取 0 则立即到位）
void arm_set_target(Arm* arm, const float deg[ARM_JOINT_COUNT], uint32_t duration_ms);

// 每 ARM_UPDATE_PERIOD_MS 调一次，做平滑运动并输出到 PCA9685
HAL_StatusTypeDef arm_update(Arm* arm);

