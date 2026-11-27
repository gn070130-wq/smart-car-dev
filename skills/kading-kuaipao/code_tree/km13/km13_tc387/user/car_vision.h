#ifndef _car_vision_h_
#define _car_vision_h_

#include "car_config.h"
#include "../code/vision_tag_tracker.h"

extern volatile uint8 car_vision_runtime_ready;
extern volatile uint8 car_vision_runtime_valid;
extern volatile uint8 car_vision_runtime_detected;
extern volatile uint8 car_vision_runtime_predicted;
extern volatile int16 car_vision_runtime_error_x_px;
extern volatile int16 car_vision_runtime_error_y_px;
extern volatile int16 car_vision_runtime_error_x_q15;
extern volatile uint16 car_vision_runtime_size_px;
extern volatile uint16 car_vision_runtime_confidence;
extern volatile uint16 car_vision_runtime_distance_mm;
extern volatile uint32 car_vision_runtime_frame_id;
extern volatile uint32 car_vision_runtime_process_us;
extern volatile uint32 car_vision_runtime_fps_x10;
extern volatile int16 car_vision_runtime_bbox_x;
extern volatile int16 car_vision_runtime_bbox_y;
extern volatile int16 car_vision_runtime_bbox_w;
extern volatile int16 car_vision_runtime_bbox_h;

void car_vision_mark_camera_ready(uint8 ready);
void car_vision_publish_result(const vision_tag_result_t *result, uint32 process_us, uint32 fps_x10);

#endif
