#ifndef __TRACKINGBASE_H_
#define __TRACKINGBASE_H_

#include "headfile.h"

#define MAX_POINT 250

enum RoadState {
    Normal_Road = 0,
    Cross_Road,
    Ring_into_weak,
    Ring_Road,
    Barrier_Road,
    Bridge_Road,
};

typedef enum Normal_Road_State {
    Normal_longway = 0,
    Normal_longway_weak,
    Normal_curve
}Normal_Road_State;

typedef enum Barrier_dir {
    no_Barrier,
    Barrier_L,
    Barrier_R
}Barrier_dir;

typedef enum Bridge_state {
    no_Bridge,
    Bridge_up,
    Bridge_down,
    Bridge_out
}Bridge_state;

extern Bridge_state bridge_state;

//最长白列信息
struct maxWhiteLine {
    uint16_t lineNum;   //最长白列个数
    uint16_t maxNum;    //最长白列值
    uint16_t maxy;      //最高点y值
    uint16_t starlineX; //最长白列最左x值   （从左往右遍历）
    uint16_t endlineX;  //最长白列最右x值   （从左往右遍历）
};

struct Edgeline {
    int16_t pointsNum;
    int16_t edgelinepoint[MAX_POINT][3];    //0:y   1:x    2:dir
    int16_t lossPoint;
};

struct Edgeline_Data {
    int16_t pointsNum;
    int16_t edgelinepoint[MAX_POINT][3];    //0:y   1:x    2:dir
    int16_t lossPoint;
    int16_t firstLossY;
    bool is_weak_alone;
    char    state;
};
extern uint16_t yend;                                      //Y循环最大值
extern int Otsu_Threshold ;                                /* 大津法阈值 */
extern bool ReCalculation_OTSU_Enable;                     /* 大津法重计算使能 */
extern struct maxWhiteLine     maxWhite;                   /* 最长白列信息 */
extern uint16_t leftbase[2];                               /* 左边基点 */
extern uint16_t rightbase[2];                              /* 右边基点 */
extern struct Edgeline left_Line;                          /* 最长白列左边线 */
extern struct Edgeline right_Line;                         /* 最长白列右边线 */
extern struct Edgeline_Data left_edgeline;                 /* 迷宫左边线 */
extern struct Edgeline_Data right_edgeline;                /* 迷宫有边线 */
extern struct Edgeline midline;                            /* 中线 */

extern int LedgeLine_OtherState_num;                       /* 左边线折角点数量 */
extern int RedgeLine_OtherState_num;                       /* 右边线折角点数量 */

extern enum RoadState roadstate;                           /* 道路状态 */
extern Normal_Road_State normalstate;                      /* 常规道路状态*/


extern float ImgErr;
extern int Redcount;
//斑马线屏蔽
extern bool shieldZebra;


extern int left_points_turnDown[4];     //0:y 1:x 2:是否存在 3:index
extern int right_points_turnUp[4];      //0:y 1:x 2:是否存在 3:index
extern int right_points_turnDown[4];    //0:y 1:x 2:是否存在 3:index
extern int left_points_turnUp[4];       //0:y 1:x 2:是否存在 3:index
//左扫线上突变点
extern int left_up_mutation[4];         //0:y 1:x 2:是否存在 3:index
//右扫线上突变点
extern int right_up_mutation[4];        //0:y 1:x 2:是否存在 3:index

//跟踪初始化
void tracking_base_init(uint16_t cutup, uint16_t cutdown);

//出界检测
bool outCrossing_checking (cv::Mat img);
//斑马线检测
bool zebraCrossing_checking(cv::Mat img, int line);
//获取跟踪基本信息
void get_TrackingBase(cv::Mat imgG);
//左+右/2 -> 中线
void left_plus_right_2midline(void);

//左边线+赛道半半宽
void leftLine_plus_HOWhalfWide(int how);
//右边线+赛道how半宽
void rightLine_plus_HOWhalfWide(int how);

//左边线+赛道半宽
void leftLine_plus_halfWide(void);
//右边线+赛道半宽
void rightLine_plus_halfWide(void);
//提取中线误差
float get_roadError(int aim_d);
//检查是否为常规直道
bool checking_Normal_straightaway(void);
//大津法重装计数器
void ReOTSU_Enable_timer(int counts);
//弯道改判直道
bool checking_curve_to_straightaway(void);
//重新画直道
void redraw_long_road(void);
//常规路段决策
void normalRoad_decusion(void);
void drawEdgeLines(cv::Mat &img);
void drawRoadLines(cv::Mat &img);

//障碍检测
bool check_Barrier_Road(cv::Mat imgYellow);
//障碍处理
void Barrier_Road_preprocess(void);

//坡道检测
bool check_Bridge(cv::Mat imgRed);
//坡道处理
void Bridge_Road_preprocess(void);

//状态复位
void reset_ALL_state(void);

#endif

