/*******************************************************************************************************************************
 *  代码用途: pd型模糊控制器
 *  作者: WwuSama
 *  邮箱: 1635202242@qq.com
 *  创建日期: 2025-3-20
 *  版本: 1.0.0
 *  
 *  输入介绍：
 *  er      为pid控制器输出值与pid控制器输入目标值的差值      注：是pid控制器不是当前模糊控制器（pid有输出限制幅,当输出达到限幅及为最大差值）
 *  de/dt   为差值变化率，原式为(er-laster) / Ertime       注：使用周期控制可以将Ertime离散成1,简化计算为er-laster
 *  
 *  内部参数介绍
 *  隶属值  NB NM NS Z0 PS PM PB    可以根据实际情况调整
 *  ABS_LUNYU_MAX                   根据论域需求修改
 *  UNIT_ER                         根据论域需求修改
 * 
 ******************************************************************************************************************************/
/*
//                       .::::.
//                     .::::::::.
//                    :::::::::::
//                 ..:::::::::::'
//              '::::::::::::'
//                .::::::::::
//           '::::::::::::::..
//                ..::::::::::::.
//              ``::::::::::::::::
//               ::::``:::::::::'        .:::.
//              ::::'   ':::::'       .::::::::.
//            .::::'      ::::     .:::::::'::::.
//           .:::'       :::::  .:::::::::' ':::::.
//          .::'        :::::.:::::::::'      ':::::.
//         .::'         ::::::::::::::'         ``::::.
//     ...:::           ::::::::::::'              ``::.
//    ````':.          ':::::::::'                  ::::..
//                       '.:::::'                    ':'````..
*/
#include "control_FuzzyCtrl.h"

FuzzyCtrl fuzzy;

FuzzyCtrl::FuzzyCtrl()
{
    
}

FuzzyCtrl::~FuzzyCtrl()
{
    
}

/* 函数用处：   模糊PD控制器
 * 函数名：     get_updataPD_pid (float er, float dedt, float *kp, float *kd)
 * 
 * 传入参数：   float er        差值
 * 传入参数：   float dedt      差值变化率
 * 传入参数：   float *kp       传递pid->kp用于更新
 * 传入参数：   float *kd       传递pid->kd用于更新
 * 函数介绍：   
 * 实现模糊控制，同时更新最新的pid->kp 和 pid->kd
 */
void FuzzyCtrl::get_updataPD_pid (float er, float dedt, float *kp, float *kd)
{
    get_degree_of_membership(er, dedt);
    upPIDdata_pd(kp, kd);
}

/* 函数用处：   获取er与de/dt的隶属度
 * 函数名：     get_degree_of_membership (float er, float dedt)
 * 
 * 传入参数：   float er        差值
 * 传入参数：   float dedt      差值变化率
 * 函数介绍：   传入输出与输入目标值的差值作为er, 传入前一次输出值与当前输出值的变化作为dedt（离散化）
 */
void FuzzyCtrl::get_degree_of_membership (float er, float dedt)
{
    float erRemap = er / error_max * ABS_LUNYU_MAX;     /* 对er映射 */
    float dedtRemap = dedt / dedt_max * ABS_LUNYU_MAX;  /* 对dedt映射 */

    calculateMembership(erRemap, er_liShu, er_liShuDu, lunYu);
    calculateMembership(dedtRemap, dedt_liShu, dedt_liShuDu, lunYu);
    
}

/* 函数用处：   隶属度获取
 * 函数名：     calculateMembership (float value, int* liShu, float* liShuDu, const int* lunYu)
 *
 * 传入参数：   float value         映射到论域上的值
 * 传入参数：   int* liShu          映射值的隶属
 * 传入参数：   float* liShuDu      映射值的隶属度
 * 传入参数：   const int* lunYu    论域
 * 函数介绍：   统一隶属度获取接口，由于er和dedt的隶属逻辑相同，故封装统一接口便于代码阅读
 */
void FuzzyCtrl::calculateMembership (float value, int* liShu, float* liShuDu, const int* lunYu)
{
    if (value > lunYu[0] && value < lunYu[6]) {
        for (int i = 0; i < 6; i++) {
            if (lunYu[i] < value && value < lunYu[i + 1]) {         /* 判断在哪个区间内 */
                /* 三角隶属度 */
                liShu[0] = i;                                       /* 记录前隶属 */
                liShu[1] = i + 1;                                   /* 记录后隶属 */
                liShuDu[0] = 1 - (value - lunYu[i]) ;               /* 记录前隶属度 */
                liShuDu[1] = 1 - (lunYu[i + 1] - value) ;           /* 记录后隶属度 */
                break;
            }
        }
    } else if (value <= lunYu[0]) {
        /* 三角隶属度 */
        liShu[0] = 0;                   /* 记录前隶属 */
        liShu[1] = 1;                   /* 记录后隶属 */
        liShuDu[0] = 1.0;               /* 记录前隶属度(100%) */
        liShuDu[1] = 0;                 /* 记录后隶属度 */
    } else if (value >= lunYu[6]) {
        /* 三角隶属度 */
        liShu[0] = 5;                   /* 记录前隶属 */
        liShu[1] = 6;                   /* 记录后隶属 */
        liShuDu[0] = 0;                 /* 记录前隶属度 */
        liShuDu[1] = 1.0;               /* 记录后隶属度(100%) */
    }
}

/* 函数用处：   更新pd参数
 * 函数名：     upPIDdata_pd(float *kp, float *kd)
 *
 * 传入参数：   float *kp           传递pid->kp用于更新
 * 传入参数：   float *kd           传递pid->kd用于更新
 * 函数介绍：   通过隶属规则更新pid中pd的参数
 */
void FuzzyCtrl::upPIDdata_pd(float* kp, float* kd)
{
    float kptmp = er_liShuDu[0] * dedt_liShuDu[0] * (float)KP_Rule[er_liShu[0]][dedt_liShu[0]]
                + er_liShuDu[0] * dedt_liShuDu[1] * (float)KP_Rule[er_liShu[0]][dedt_liShu[1]]
                + er_liShuDu[1] * dedt_liShuDu[0] * (float)KP_Rule[er_liShu[1]][dedt_liShu[0]]
                + er_liShuDu[1] * dedt_liShuDu[1] * (float)KP_Rule[er_liShu[1]][dedt_liShu[1]];

    float kdtmp = er_liShuDu[0] * dedt_liShuDu[0] * (float)KD_Rule[er_liShu[0]][dedt_liShu[0]]
                + er_liShuDu[0] * dedt_liShuDu[1] * (float)KD_Rule[er_liShu[0]][dedt_liShu[1]]
                + er_liShuDu[1] * dedt_liShuDu[0] * (float)KD_Rule[er_liShu[1]][dedt_liShu[0]]
                + er_liShuDu[1] * dedt_liShuDu[1] * (float)KD_Rule[er_liShu[1]][dedt_liShu[1]];

    kptmp *= kp_buff;
    kdtmp *= kd_buff;

    *kp = (kptmp<0) ? -kptmp : kptmp;
    *kd = (kdtmp<0) ? -kdtmp : kdtmp;
}

