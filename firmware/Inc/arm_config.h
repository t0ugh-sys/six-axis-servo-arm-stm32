#pragma once

// 你需要根据实际机械与标定结果修改这里
//
// 建议通道分配：
// - MG996R: 底座/肩/肘（0/1/2）
// - MG90S : 腕/腕旋/夹爪（3/4/5）
//
// 注意：所有角度/限位最终以你的机械结构为准，先从保守范围开始。

#define ARM_JOINT_COUNT 6

// PCA9685 I2C 地址（7-bit）
#define ARM_PCA9685_ADDR 0x40

// 舵机 PWM 频率
#define ARM_SERVO_FREQ_HZ 50.0f

// 默认标定脉宽范围（起步用，最终请按每个关节标定）
#define ARM_DEFAULT_US_MIN 500
#define ARM_DEFAULT_US_MAX 2500

// 插补更新周期（ms），建议与 50Hz 对齐
#define ARM_UPDATE_PERIOD_MS 20

