#include "arm.h"

#include <string.h>

static void arm_load_default_servos(Arm* arm) {
  // 这里给一组“保守默认值”，你标定后应该把 offset / inverted / us_min/us_max / 角度限位改掉
  // angle_min/max 建议先从 10~170 这种保守范围起步，避免顶死。
  servo_init(&arm->joints[0], 0, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
  servo_init(&arm->joints[1], 1, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
  servo_init(&arm->joints[2], 2, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
  servo_init(&arm->joints[3], 3, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
  servo_init(&arm->joints[4], 4, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
  servo_init(&arm->joints[5], 5, false, 10.0f, 170.0f, 0.0f, ARM_DEFAULT_US_MIN, ARM_DEFAULT_US_MAX);
}

HAL_StatusTypeDef arm_init(Arm* arm, I2C_HandleTypeDef* hi2c) {
  memset(arm, 0, sizeof(*arm));
  arm_load_default_servos(arm);

  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
    arm->current_deg[i] = 90.0f;
    arm->target_deg[i] = 90.0f;
  }

  HAL_StatusTypeDef st = pca9685_init(&arm->pca, hi2c, ARM_PCA9685_ADDR, ARM_SERVO_FREQ_HZ);
  if (st != HAL_OK) return st;
  return arm_write_now(arm);
}

HAL_StatusTypeDef arm_write_now(Arm* arm) {
  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
    HAL_StatusTypeDef st = servo_write_angle(&arm->pca, &arm->joints[i], arm->current_deg[i]);
    if (st != HAL_OK) return st;
  }
  return HAL_OK;
}

void arm_set_target(Arm* arm, const float deg[ARM_JOINT_COUNT], uint32_t duration_ms) {
  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
    arm->target_deg[i] = deg[i];
  }

  if (duration_ms == 0) {
    for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
      arm->current_deg[i] = arm->target_deg[i];
      arm->step_delta_deg[i] = 0.0f;
    }
    arm->move_total_steps = 0;
    arm->move_step_idx = 0;
    return;
  }

  const uint32_t steps = duration_ms / ARM_UPDATE_PERIOD_MS;
  arm->move_total_steps = (steps == 0) ? 1 : steps;
  arm->move_step_idx = 0;
  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
    arm->step_delta_deg[i] = (arm->target_deg[i] - arm->current_deg[i]) / (float)arm->move_total_steps;
  }
}

HAL_StatusTypeDef arm_update(Arm* arm) {
  if (arm->move_total_steps == 0) {
    // no interpolation; still allow periodic refresh if你想
    return arm_write_now(arm);
  }

  if (arm->move_step_idx < arm->move_total_steps) {
    for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
      arm->current_deg[i] += arm->step_delta_deg[i];
    }
    arm->move_step_idx++;
  } else {
    for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) {
      arm->current_deg[i] = arm->target_deg[i];
    }
    arm->move_total_steps = 0;
  }

  return arm_write_now(arm);
}

