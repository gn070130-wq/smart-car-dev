#ifndef __DISPLAY_IMAGE_HPP__
#define __DISPLAY_IMAGE_HPP__

#include "vision.hpp"

void display_image_init(void);
void display_image_show(const uint8_t *gray, const VisionResult *vision);
zf_device_ips200 *display_image_get_device(void);

#endif
