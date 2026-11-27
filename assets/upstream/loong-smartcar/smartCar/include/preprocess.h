#ifndef __PREPROCESS_H_
#define __PREPROCESS_H_

#include "headfile.h"

using namespace cv;

class Preprocess
{
public:
    /* 此处可以存放可调试的变量 */
    uint16_t rowCutUp = 0;          /* 图像上切割 */
    uint16_t rowCutBottom = 0;      /* 图像下切割 */
    int     trackAim = 0;           /* 跟踪瞄准 */
    int     L_motor_kp = 0;           /* L电机pid kp */
    int     L_motor_ki = 0;           /* L电机pid ki */
    int     R_motor_kp = 0;           /* R电机pid kp */
    int     R_motor_ki = 0;           /* R电机pid ki */
    
    int Ring_into_Size = 0;         /* 圆环入环口尺寸 */
    int Ring_into_Size_Max = 0;     /* 圆环入环口尺寸最大值 */
    int Ring_Y = 0;                 /* 右圆环左边拉线Y坐标 */
    int Ring_Count = 0;             /* 圆环数量 */

    int ring1_X_in = 0;
    int ring1_X_out = 0;
    int ring1_out_dis = 0;
    int ring1_trackAim = 0;

    int ring2_X_in = 0;
    int ring2_X_out = 0;
    int ring2_out_dis = 0;
    int ring2_trackAim = 0;

    int ring3_X_in = 0;
    int ring3_X_out = 0;
    int ring3_out_dis = 0;
    int ring3_trackAim = 0;

    int ring4_X_in = 0;
    int ring4_X_out = 0;
    int ring4_out_dis = 0;
    int ring4_trackAim = 0;

    int ring5_X_in = 0;
    int ring5_X_out = 0;
    int ring5_out_dis = 0;
    int ring5_trackAim = 0;
    

    int motorspeed_low = 0;         /* 电机中心低度 */
    int motorspeed_high = 0;        /* 电机中心高速 */
    int motorspeed_weakhigh = 0;    /* 电机中心弱高速 */

    int ring_intoSpeed = 0;         /* 圆环入环速度 */
    int ringSpeed = 0;              /* 圆环内速度 */
    int crossSpeed_h = 0;           /* 十字高速度 */
    int crossSpeed_l = 0;           /* 十字低速度 */
    int barrier_speed = 0;          /* 障碍速度 */
    int bridge_speed = 0;           /* 坡道速度 */

    bool showImg = false;           /* 是否开启图像 */
    bool weakRing = false;          /* 弱圆环检测开关 */
    float ICAR_HALFW = 0;           /* 阿克曼转向轮半距 */
    float LEFT_HALFW_BUF = 0;       /* 阿克曼转向左转增益（由于左右不对称） */

    float Gryo_Zkd =0;              /* 陀螺仪抑制kd */
    int akmThreshold = 0;           /* 阿克曼阈值 */
    int fans_Maxduty = 0;           /* 负压最大占空比 */
    float long_restrain = 0;        /* 直道抑制比例 */
    float long_weak_restrain = 0;   /* 弱直道抑制比例 */
    float barrier_buffer = 0;       /* 障碍增益 */

    int imglimiter = 0;             /* 图像限幅 */

    bool Bridge_Enable = false;     /* 坡道使能 */

    int zebraOUT = 0;               /* 退出斑马线里程 */
    int barrierOUT = 0;             /* 退出障碍里程 */

    int servoMID = 0;

    /* 色彩空间 */
    int h_l = 0;                    
    int s_l = 0;
    int v_l = 0;
    int h_h = 0;
    int s_h = 0;
    int v_h = 0;

    Preprocess();
    ~Preprocess();

    int Preprocess_init();
    int Preprocess_save();

    Mat Cut_and_imageGrayING(Mat &img, int start_y, int len_y);

    Mat binaryzation(Mat img);

    /* 计算两点斜率k */
    float calculate_slopeK(int y1, int y2, int x1, int x2);

    /* 通过斜率k和一个点坐标得到所在直线的偏移b */
    float calculate_slopeB(float k, int y, int x);
    

private:
};

extern Preprocess preprocess;

#endif
