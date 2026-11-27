#ifndef __TRACKING_RING_H_
#define __TRACKING_RING_H_
#include "headfile.h"

#define INRING 60
#define READYOUTRING 240
#define OUTRING 350

typedef enum Ring_State {
    No_Ring = 0,
    Right_intoWeak,
    Right_intoRing,
    Right_inRing,
    Right_ReadyoutRing,
    Right_outRing,
    Left_intoWeak,
    Left_intoRing,
    Left_inRing,
    Left_ReadyoutRing,
    Left_outRing,
}Ring_State;

extern Ring_State ringstate;

extern int *inRing_X_Arr[5];
extern int *outRing_X_Arr[5];
extern int nowRingIndex;

extern int RingCounts;
//圆环跟踪前瞻
extern int *ringTrackAim[5];
//圆环顺序
extern bool RingDir;

//圆环初始化
void Ring_init(void);

bool check_Right_intoRing_weak(void);

bool check_Right_intoRing(cv::Mat imgG, int size);
bool check_Right_inRing(void);
bool check_Right_ReadyoutRing(void);
bool check_Right_outRing(void);
bool check_Out_RightRing(void);
void RightRing_State_checking(cv::Mat imgG);

bool check_Left_intoRing_weak(void);

bool check_Left_intoRing(cv::Mat imgG, int size);
bool check_Left_inRing(void);
bool check_Left_ReadyoutRing(void);
bool check_Left_outRing(void);
bool check_Out_LeftRing(void);
void LeftRing_State_checking(cv::Mat imgG);

void ring_processing(cv::Mat img, int lefty, int leftx, int outleftx);

#endif
