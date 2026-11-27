#ifndef __TRACKING_CROSSROAD_H_
#define __TRACKING_CROSSROAD_H_

#include "headfile.h"

typedef enum CrossRoad_State {
    no_crossRoad = 0,
    front_crossRoad_1,
    front_crossRoad_2,
    front_crossRoad_3,
    in_crossRoad_1
}CrossRoad_State;

extern int crose_test;
extern int cross_strchoose; 

extern enum CrossRoad_State cross_state;
CrossRoad_State isCrossRoad(cv::Mat imgG);
void crossRoad_processing(void);

#endif
