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
#ifndef __CONTROL_FUZZYPID_H_
#define __CONTROL_FUZZYPID_H_

/* WWWWWWWWWWWWWWWWWWW以下内容根据实际情况修改WWWWWWWWWWWWWWWWWWWWWWW */
#define ERROR_MAX                    (27)                   //输出误差最大值
#define DEDT_MAX                     (7)                    //差值变化率最大值
#define KP_BUFF                      (3.0f)                 //kp增益buff
#define KD_BUFF                      (50.0f)                //kd增益buff

/* WWWWWWWWWWWWWWWWWWW以上内容根据实际情况修改WWWWWWWWWWWWWWWWWWWWWWW */


class FuzzyCtrl
{
public:
    int error_max = ERROR_MAX;
    int dedt_max = DEDT_MAX;
    float kp_buff = KP_BUFF;
    float kd_buff = KD_BUFF;

    FuzzyCtrl();
    ~FuzzyCtrl();
    void get_updataPD_pid (float er, float dedt, float *kp, float *kd);

private:
    /* WWWWWWWWWWWWW 论域————同等于kp与kd的取值范围（可调）WWWWWWWWWWWWWWWWWW */
    #define ABS_LUNYU_MAX                (3.0f)                 //单方向论域最大值  ----根据论域需求修改
    const int NB = -3, NM = -2, NS = -1, Z0 = 0, PS = 1, PM = 2, PB = 3;
    const int lunYu[7] = {NB, NM, NS, Z0, PS, PM, PB};
    /* WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW */
    int er_liShu[2];                //er隶属
    int dedt_liShu[2];              //dedt隶属
    float er_liShuDu[2];            //er隶属度
    float dedt_liShuDu[2];          //dedt隶属度

    const int KP_Rule[7][7] = {
/*  er/dedt      NB  NM  NS  Z0  PS  PM  PB*/
    /*NB*/      {PB, PB, PM, PM, PS, Z0, Z0},
    /*NM*/      {PB, PB, PM, PS, PS, Z0, NS},
    /*NS*/      {PM, PM, PM, PS, Z0, NS, NS},
    /*Z0*/      {PS, PS, PS, Z0, NS, NS, NS},
    /*PS*/      {PS, PS, Z0, NS, NS, NM, NM},
    /*PM*/      {PS, Z0, NS, NM, NM, NM, NB},
    /*PB*/      {Z0, Z0, NM, NM, NM, NB, NB},
    };

    const int KD_Rule[7][7] = {
/*  er/dedt      NB  NM  NS  Z0  PS  PM  PB*/
    /*NB*/      {PS, NS, NB, NB, NB, NM, PS},
    /*NM*/      {PS, NS, NB, NM, NM, NS, Z0},
    /*NS*/      {Z0, NS, NM, NM, NS, NS, Z0},
    /*Z0*/      {Z0, NS, NS, NS, NS, NS, Z0},
    /*PS*/      {Z0, Z0, Z0, Z0, Z0, Z0, Z0},
    /*PM*/      {PB, NS, PS, PS, PS, PS, PB},
    /*PB*/      {PB, PM, PM, PM, PS, PS, PB},
    };
    
    void get_degree_of_membership (float er, float dedt);
    void calculateMembership(float value, int* liShu, float* liShuDu, const int* lunYu);
    void upPIDdata_pd(float *kp, float *kd);
};

extern FuzzyCtrl fuzzy;


#endif
