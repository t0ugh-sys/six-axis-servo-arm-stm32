#include "pca9685.h"

#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_PRESCALE 0xFE
#define PCA9685_LED0_ON_L 0x06

#define PCA9685_MODE1_SLEEP (1u << 4)
#define PCA9685_MODE1_AI (1u << 5)  // auto-increment

#define PCA9685_MODE2_OUTDRV (1u << 2)  // totem pole

static HAL_StatusTypeDef pca9685_write_u8(Pca9685* dev, uint8_t reg, uint8_t val) {
  uint8_t buf[2] = {reg, val};
  return HAL_I2C_Master_Transmit(dev->hi2c, (uint16_t)(dev->addr_7bit << 1), buf, sizeof(buf), 100);
}

static HAL_StatusTypeDef pca9685_read_u8(Pca9685* dev, uint8_t reg, uint8_t* out) {
  HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(dev->hi2c, (uint16_t)(dev->addr_7bit << 1), &reg, 1, 100);
  if (st != HAL_OK) return st;
  return HAL_I2C_Master_Receive(dev->hi2c, (uint16_t)(dev->addr_7bit << 1), out, 1, 100);
}

HAL_StatusTypeDef pca9685_set_pwm_freq(Pca9685* dev, float pwm_freq_hz) {
  // datasheet formula: prescale = round(25MHz / (4096 * freq)) - 1
  if (pwm_freq_hz < 1.0f) pwm_freq_hz = 1.0f;
  if (pwm_freq_hz > 1526.0f) pwm_freq_hz = 1526.0f;

  const float prescale_f = (25000000.0f / (4096.0f * pwm_freq_hz)) - 1.0f;
  uint8_t prescale = (uint8_t)(prescale_f + 0.5f);

  uint8_t old_mode1 = 0;
  HAL_StatusTypeDef st = pca9685_read_u8(dev, PCA9685_MODE1, &old_mode1);
  if (st != HAL_OK) return st;

  // go to sleep to set prescale
  st = pca9685_write_u8(dev, PCA9685_MODE1, (uint8_t)((old_mode1 & ~PCA9685_MODE1_AI) | PCA9685_MODE1_SLEEP));
  if (st != HAL_OK) return st;

  st = pca9685_write_u8(dev, PCA9685_PRESCALE, prescale);
  if (st != HAL_OK) return st;

  // wake up + enable auto-increment
  st = pca9685_write_u8(dev, PCA9685_MODE1, (uint8_t)((old_mode1 | PCA9685_MODE1_AI) & ~PCA9685_MODE1_SLEEP));
  if (st != HAL_OK) return st;

  HAL_Delay(1);
  dev->pwm_freq_hz = pwm_freq_hz;
  return HAL_OK;
}

HAL_StatusTypeDef pca9685_init(Pca9685* dev, I2C_HandleTypeDef* hi2c, uint8_t addr_7bit, float pwm_freq_hz) {
  dev->hi2c = hi2c;
  dev->addr_7bit = addr_7bit;
  dev->pwm_freq_hz = 0.0f;

  // MODE2: totem-pole output
  HAL_StatusTypeDef st = pca9685_write_u8(dev, PCA9685_MODE2, PCA9685_MODE2_OUTDRV);
  if (st != HAL_OK) return st;

  // MODE1: auto-increment enabled; normal mode
  st = pca9685_write_u8(dev, PCA9685_MODE1, PCA9685_MODE1_AI);
  if (st != HAL_OK) return st;

  HAL_Delay(1);
  return pca9685_set_pwm_freq(dev, pwm_freq_hz);
}

HAL_StatusTypeDef pca9685_set_pwm(Pca9685* dev, uint8_t channel, uint16_t on_tick, uint16_t off_tick) {
  if (channel > 15) return HAL_ERROR;
  on_tick &= 0x0FFF;
  off_tick &= 0x0FFF;

  uint8_t reg = (uint8_t)(PCA9685_LED0_ON_L + 4u * channel);
  uint8_t buf[5] = {reg, (uint8_t)(on_tick & 0xFF), (uint8_t)(on_tick >> 8), (uint8_t)(off_tick & 0xFF),
                    (uint8_t)(off_tick >> 8)};
  return HAL_I2C_Master_Transmit(dev->hi2c, (uint16_t)(dev->addr_7bit << 1), buf, sizeof(buf), 100);
}

HAL_StatusTypeDef pca9685_set_pulse_us(Pca9685* dev, uint8_t channel, uint16_t pulse_us) {
  // tick_us = 1e6 / (freq * 4096)
  // off = pulse_us / tick_us = pulse_us * freq * 4096 / 1e6
  if (dev->pwm_freq_hz <= 0.1f) return HAL_ERROR;

  uint32_t off = (uint32_t)(((uint64_t)pulse_us * (uint64_t)(dev->pwm_freq_hz * 4096.0f)) / 1000000ull);
  if (off > 4095u) off = 4095u;
  return pca9685_set_pwm(dev, channel, 0u, (uint16_t)off);
}

