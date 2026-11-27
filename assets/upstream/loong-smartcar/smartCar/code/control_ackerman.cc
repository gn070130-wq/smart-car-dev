#include "control_ackerman.h"

/*  根据舵机的阿克曼转向控制
*
*  参数    float turningRadius = 0;    转向半径
*  函数    calculate_turnR             计算turnR
*  函数    ackerman_Lmotor             根据turnR和质心速度返回左轮速度
*  函数    ackerman_Rmotor             根据turnR和质心速度返回右轮速度
*/
/*WWWWWWWWWWWWWWWWWWWWWWWWWWWWW智能车物理参数WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW*/
#define ICAR_L              20.0f                               //车身长度cm
#define SERVO_DUTY2RAD      (0.13338f * 3.1415926535f / 180.0f)   //角度转弧度的系数
/*WWWWWWWWWWWWWWWWWWWWWWWWWWWWW阿克曼差速部分WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW*/

float turningRadius = 0;

void get_turningRadius(float output, int threshold)
{
   if(abs(output) > threshold) {
    turningRadius = ICAR_L / tanf(output * SERVO_DUTY2RAD);
   } else {
    turningRadius = 0;
   }
}

int ackerman_Lmotor(int target, float half_w)
{
    float nowSpeed;
    if(turningRadius < 0) {
        nowSpeed = target * (fabsf(turningRadius) + half_w) / fabsf(turningRadius);
    } else {
        nowSpeed = target * (fabsf(turningRadius) - half_w) / fabsf(turningRadius);
    }

    return (int)nowSpeed;
}

int ackerman_Rmotor(int target, float half_w)
{
    {
        float nowSpeed;
        if(turningRadius < 0) {
            nowSpeed = target * (fabsf(turningRadius) - half_w) / fabsf(turningRadius);
        } else {
            nowSpeed = target * (fabsf(turningRadius) + half_w) / fabsf(turningRadius);
        }
    
        return (int)nowSpeed;
    }
}