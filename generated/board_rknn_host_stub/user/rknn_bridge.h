#ifndef RKNN_BRIDGE_H_
#define RKNN_BRIDGE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float err;
  int type_id;
  uint32_t stamp_ms;
  int fresh;
} RknnResult;

void rknn_bridge_init(void);
/* 模拟异步推理：每隔 period 帧更新一次结果 */
void rknn_bridge_poll(uint32_t now_ms, RknnResult *out);

#ifdef __cplusplus
}
#endif

#endif
