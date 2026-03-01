#include "servo.h"

static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void servo_init(Servo* s,
                uint8_t channel,
                bool inverted,
                float angle_min_deg,
                float angle_max_deg,
                float offset_deg,
                uint16_t us_min,
                uint16_t us_max) {
  s->channel = channel;
  s->inverted = inverted;
  s->angle_min_deg = angle_min_deg;
  s->angle_max_deg = angle_max_deg;
  s->offset_deg = offset_deg;
  s->us_min = us_min;
  s->us_max = us_max;
}

HAL_StatusTypeDef servo_write_us(Pca9685* dev, const Servo* s, uint16_t pulse_us) {
  (void)s;
  return pca9685_set_pulse_us(dev, s->channel, pulse_us);
}

HAL_StatusTypeDef servo_write_angle(Pca9685* dev, const Servo* s, float angle_deg) {
  float a = angle_deg + s->offset_deg;
  a = clampf(a, s->angle_min_deg, s->angle_max_deg);

  const float span_deg = (s->angle_max_deg - s->angle_min_deg);
  if (span_deg <= 0.01f) return HAL_ERROR;

  float t = (a - s->angle_min_deg) / span_deg;  // 0..1
  if (s->inverted) t = 1.0f - t;

  const float us = (float)s->us_min + t * (float)((int32_t)s->us_max - (int32_t)s->us_min);
  uint16_t pulse = (uint16_t)clampf(us, 0.0f, 65535.0f);
  return pca9685_set_pulse_us(dev, s->channel, pulse);
}

