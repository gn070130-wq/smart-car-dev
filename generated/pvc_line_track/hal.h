#ifndef PVC_HAL_H_
#define PVC_HAL_H_

#include <stdint.h>
#include "track_pvc.h"

#ifdef __cplusplus
extern "C" {
#endif

const uint8_t *hal_camera_gray(void);
uint32_t hal_millis(void);
float hal_yaw_deg(void);
void hal_motor_set(float left, float right);
void hal_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
