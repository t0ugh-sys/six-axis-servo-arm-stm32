#include "arm_cmd.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* skip_spaces(const char* p) {
  while (*p && isspace((unsigned char)*p)) p++;
  return p;
}

static bool parse_keyval(const char* p, char* key, size_t key_cap, long* out_val, const char** out_next) {
  // key: [A-Za-z][0-9]*  then '=' then integer
  p = skip_spaces(p);
  if (!isalpha((unsigned char)*p)) return false;

  size_t ki = 0;
  while (*p && (isalpha((unsigned char)*p) || isdigit((unsigned char)*p))) {
    if (ki + 1 < key_cap) key[ki++] = *p;
    p++;
  }
  key[ki] = '\0';

  p = skip_spaces(p);
  if (*p != '=') return false;
  p++;
  p = skip_spaces(p);

  char* endptr = NULL;
  long v = strtol(p, &endptr, 10);
  if (endptr == p) return false;

  *out_val = v;
  *out_next = endptr;
  return true;
}

void arm_cmd_init(ArmCmd* cmd, uint32_t default_duration_ms) {
  cmd->default_duration_ms = default_duration_ms;
}

bool arm_cmd_execute_line(ArmCmd* cmd, Arm* arm, const char* line) {
  float target[ARM_JOINT_COUNT];
  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) target[i] = arm->target_deg[i];

  uint32_t duration_ms = cmd->default_duration_ms;
  bool any_joint = false;

  const char* p = line;
  while (1) {
    p = skip_spaces(p);
    if (*p == '\0') break;

    char key[8];
    long val = 0;
    const char* next = NULL;
    if (!parse_keyval(p, key, sizeof(key), &val, &next)) return false;

    // Jn=deg
    if (key[0] == 'J' && isdigit((unsigned char)key[1])) {
      int idx = atoi(&key[1]);
      if (idx < 0 || idx >= (int)ARM_JOINT_COUNT) return false;
      target[idx] = (float)val;
      any_joint = true;
    }

    // T=ms
    else if (strcmp(key, "T") == 0) {
      if (val < 0) val = 0;
      duration_ms = (uint32_t)val;
    }

    // Un=us (标定用：直接写脉宽)
    else if (key[0] == 'U' && isdigit((unsigned char)key[1])) {
      int idx = atoi(&key[1]);
      if (idx < 0 || idx >= (int)ARM_JOINT_COUNT) return false;
      if (val < 0) val = 0;
      if (val > 3000) val = 3000;  // 先做保守保护
      (void)servo_write_us(&arm->pca, &arm->joints[idx], (uint16_t)val);
    } else {
      return false;
    }

    p = next;
  }

  if (any_joint) {
    arm_set_target(arm, target, duration_ms);
  }
  return true;
}

