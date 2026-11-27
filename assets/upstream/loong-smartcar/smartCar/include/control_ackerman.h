#ifndef __CONTROL_ACKERMAN_H_
#define __CONTROL_ACKERMAN_H_

#include "headfile.h"

extern float turningRadius;

void get_turningRadius(float output, int threshold);
int ackerman_Lmotor(int target, float half_w);
int ackerman_Rmotor(int target, float half_w);

#endif
