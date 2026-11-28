#ifndef VISION_IPC_H_
#define VISION_IPC_H_

#include <stdint.h>

typedef struct {
  float err;
  uint8_t element;
  uint8_t valid;
  uint32_t stamp_ms;
} vision_to_ctrl_t;

void vision_ipc_init(void);
void vision_ipc_publish(float err, uint8_t element, uint32_t now_ms);
int vision_ipc_fetch(vision_to_ctrl_t *out, uint32_t now_ms, uint32_t max_age_ms);

#endif
