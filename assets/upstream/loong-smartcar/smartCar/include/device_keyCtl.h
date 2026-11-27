#ifndef DEVICE_KEY_H
#define DEVICE_KEY_H

#include "headfile.h"

#define KEY_FILE_ADD "/dev/input/event0"

int Key_init(void);
void keythread_Star(void);
void key_listener(void);

#endif
