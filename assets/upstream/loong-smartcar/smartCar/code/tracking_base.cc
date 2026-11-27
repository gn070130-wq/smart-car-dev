#include "tracking_base.h"

/* 前进方向定义：
*   0
* 3   1
*   2
*/
const int dir_front[4][2] = {{0,  -1},
                            {1,  0},
                            {0,  1},
                            {-1, 0}};
const int dir_frontleft[4][2] = {{-1, -1},
                                {1,  -1},
                                {1,  1},
                                {-1, 1}};
const int dir_frontright[4][2] = {{1,  -1},
                                {1,  1},
                                {-1, 1},
                                {-1, -1}};

const uint16_t halfRoad[100] = {15, 15, 15, 15, 15, 15,
    15 ,16 ,16 ,17 ,17 ,18 ,18 ,19 ,19 
   ,20 ,20 ,21 ,21 ,22 ,22 ,23 ,23 ,24 
   ,24 ,25 ,25 ,26 ,26 ,27 ,27 ,28 ,28 
   ,29 ,29 ,30 ,30 ,31 ,31 ,32 ,32 ,33 
   ,33 ,34 ,35 ,35 ,36 ,36 ,37 ,37 ,38 
   ,38 ,39 ,39 ,40 ,40 ,41 ,41 ,42 ,42 
   ,43 ,43 ,44 ,44 ,45 ,45 ,46 ,46 ,47 
   ,47 ,48 ,48 ,49 ,49 ,50 ,50 ,51 ,51 
   ,52 ,53 ,53 ,54 ,54 ,55 ,55 ,56 ,57 
   ,57 ,57 ,58 ,58 ,59 ,60 ,60 ,60 ,61 
   ,62 ,62, 63, 63};
//----------------------------------------------------------------------------------------------------------

maxWhiteLine max_White_Line(cv::Mat img);
/* 寻找左基点 */
bool l_basepoint(cv::Mat img, int16_t find_x);
/* 寻找右基点 */
bool r_basepoint(cv::Mat img, int16_t find_x);
/* 使用最长白列获取左边线 */
void use_Whiteline_find_leftline(cv::Mat img);
/* 使用最长白列获取右边线 */
void use_Whiteline_find_rightline(cv::Mat img);
/* 上交迷宫法左边线 */
void findline_lefthand_adaptive(cv::Mat img);
/* 上交迷宫法右边线 */
void findline_righthand_adaptive(cv::Mat img);

/* 最长白列串道 */
void reMaxWhite(cv::Mat img);

//处理生长算法
void dispose_Edgeline(void);
//处理扫线算法
void dispose_line(void);

//左下拐点
int left_points_turnDown[4];
//右上拐点
int right_points_turnUp[4];
//右下拐点
int right_points_turnDown[4];
//左上拐点
int left_points_turnUp[4];

//左扫线上突变点
int left_up_mutation[4];
//右扫线上突变点
int right_up_mutation[4];
//障碍方向枚举
Barrier_dir barrier_dir = no_Barrier;
//坡道枚举
Bridge_state bridge_state = no_Bridge;


/* 私有变量 */
uint16_t yend;                  //Y循环最大值
uint16_t imgcutup = 0;          //图像上裁剪
uint16_t imgcutdown = 0;        //图像下裁剪
uint16_t roadMid;               //赛道中心点

/* 公有变量 */
int Otsu_Threshold = 127;                           /* 大津法阈值 */
bool ReCalculation_OTSU_Enable = true;              /* 大津法重计算使能 */
struct maxWhiteLine     maxWhite;                   /* 最长白列信息 */
uint16_t leftbase[2];                               /* 左边基点0:y 1:x */
uint16_t rightbase[2];                              /* 右边基点0:y 1:x */
struct Edgeline left_Line;                          /* 最长白列左边线 */
struct Edgeline right_Line;                         /* 最长白列右边线 */
struct Edgeline_Data left_edgeline;                 /* 迷宫左边线 */
struct Edgeline_Data right_edgeline;                /* 迷宫有边线 */
struct Edgeline midline;                            /* 中线 */

int LedgeLine_OtherState_num;                       /* 左边线折角点数量 */
int RedgeLine_OtherState_num;                       /* 右边线折角点数量 */

enum RoadState roadstate = Normal_Road;             /* 道路状态 */
Normal_Road_State normalstate = Normal_longway;     /* 常规道路状态*/

//图像误差
float ImgErr;
//HSV红色像素点数
int Redcount = 0;
//HSV黄色像素点数
int Yellowcount = 0;
//斑马线屏蔽
bool shieldZebra = true;

void tracking_base_init(uint16_t cutup, uint16_t cutdown)
{
    imgcutup = cutup;
    imgcutdown = cutdown;
    roadMid = 80;
    yend = ROWSIMAGE - imgcutdown;
}

void get_TrackingBase(cv::Mat imgG)
{
    cv::Mat imgcopy = imgG;
    /* 大津法计算阈值 */
    if(ReCalculation_OTSU_Enable) {
        ReCalculation_OTSU_Enable = false;
        cv::Mat imageBinary;
        Otsu_Threshold = cv::threshold(imgcopy, imageBinary, 0, 255, cv::THRESH_OTSU);
    }
    /* 画左右两条黑边框 */
    cv::line(imgcopy, cv::Point(0, 0), cv::Point(0, imgcopy.rows-1), cv::Scalar(0, 0, 0), 1);
    cv::line(imgcopy, cv::Point(1, 0), cv::Point(1, imgcopy.rows-1), cv::Scalar(0, 0, 0), 1);
    cv::line(imgcopy, cv::Point(imgcopy.cols-1, 0), cv::Point(imgcopy.cols-1, imgcopy.rows-1), cv::Scalar(0, 0, 0), 1);
    cv::line(imgcopy, cv::Point(imgcopy.cols-2, 0), cv::Point(imgcopy.cols-2, imgcopy.rows-1), cv::Scalar(0, 0, 0), 1);

    

    /* 获取最长白列信息 */
    maxWhite = max_White_Line(imgcopy);
    /* 找左基点 */
    l_basepoint(imgcopy, maxWhite.endlineX);
    /* 找右基点 */
    r_basepoint(imgcopy, maxWhite.endlineX);
    /* 局部二值化迷宫法寻左边线（上交源码） */
    findline_lefthand_adaptive(imgcopy);
    /* 局部二值化迷宫法寻右边线（上交源码） */
    findline_righthand_adaptive(imgcopy);

    /* 最长白列串道 */
    reMaxWhite(imgcopy);

    /* 最长白列获取左边线 */
    use_Whiteline_find_leftline(imgcopy);
    /* 最长白列获取右边线 */
    use_Whiteline_find_rightline(imgcopy);
    
    //处理生长算法
    dispose_Edgeline();
    //处理扫线算法
    dispose_line();

}

/* 左右边线合成中线 */
void left_plus_right_2midline(void)
{
    int max = (left_Line.pointsNum < right_Line.pointsNum) ? left_Line.pointsNum : right_Line.pointsNum;
    midline.pointsNum = 0;
    for(int i = 0; i < max; i++)
    {
        midline.edgelinepoint[i][1] = (left_Line.edgelinepoint[i][1] + right_Line.edgelinepoint[i][1]) / 2;
        midline.edgelinepoint[i][0] = (left_Line.edgelinepoint[i][0] + right_Line.edgelinepoint[i][0]) / 2;
        midline.pointsNum++;
    }
    // 滑动中值滤波（窗口大小=3）
    for (int i = 1; i < max - 1; i++) {
        // 对Y坐标滤波
        float windowY[3] = {(float)midline.edgelinepoint[i-1][1], (float)midline.edgelinepoint[i][1], (float)midline.edgelinepoint[i+1][1]};
        std::sort(windowY, windowY + 3);
        midline.edgelinepoint[i][1] = windowY[1];  // 取中值

        // 对X坐标滤波
        float windowX[3] = {(float)midline.edgelinepoint[i-1][0], (float)midline.edgelinepoint[i][0], (float)midline.edgelinepoint[i+1][0]};
        std::sort(windowX, windowX + 3);
        midline.edgelinepoint[i][0] = windowX[1];  // 取中值
    }
}

//右边线+赛道半宽
void rightLine_plus_halfWide(void)
{
    struct Edgeline edge = right_Line;
    struct Edgeline other = left_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] - halfRoad[edge.edgelinepoint[i][0]];
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//右边线+赛道how半宽
void rightLine_plus_HOWhalfWide(int how)
{
    struct Edgeline edge = right_Line;
    struct Edgeline other = left_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] - halfRoad[edge.edgelinepoint[i][0]] / how;
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//左边线+赛道半半宽
void leftLine_plus_HOWhalfWide(int how)
{
    struct Edgeline edge = left_Line;
    struct Edgeline other = right_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] + halfRoad[edge.edgelinepoint[i][0]] / how;
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//左边线+赛道半宽
void leftLine_plus_halfWide(void)
{
    struct Edgeline edge = left_Line;
    struct Edgeline other = right_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] + halfRoad[edge.edgelinepoint[i][0]];
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

/* 出界检测 */
bool outCrossing_checking (cv::Mat img)
{
    bool out = false;
    int16_t y = ROWSIMAGE-1;
    int whiteCounts = 0;
    for(int i = 0; i < COLSIMAGE; i++) {
        if(img.at<uchar>(y, i) > Otsu_Threshold) 
        whiteCounts++;
    }
    if(whiteCounts < 20)
        out = true;

    return out;
}

/* 斑马线检测 */
bool zebraCrossing_checking(cv::Mat img, int line)
{
    //距离屏蔽斑马线
    if(shieldZebra == true) {
        if(motor_distance > preprocess.zebraOUT)
            shieldZebra = false;
        return false;
    }

    bool zebra = false;
    int16_t y = ROWSIMAGE-1 - line;
    int zebraCounts = 0;
    for(int i = 0; i < COLSIMAGE-1; i++) {
        if(img.at<uchar>(y, i) < Otsu_Threshold && img.at<uchar>(y, i+1) >= Otsu_Threshold)
        zebraCounts++;
    }
    if(zebraCounts > 6)
    zebra = true;

    //左边没扫
    if(!zebra)
    {
        zebraCounts = 0;
        for(int i = COLSIMAGE-1; i > 0; i--) {
            if(img.at<uchar>(y, i) < Otsu_Threshold && img.at<uchar>(y, i-1) >= Otsu_Threshold)
            zebraCounts++;
        }
        if(zebraCounts > 6)
        zebra = true;
    }

    return zebra;
}

#define WEIGHTARR           10              /* 跟踪权重点集数 */
const   int16_t     weight[WEIGHTARR] = {3,3,5,5,5,6,6,7,9,11};     /* 跟踪位置权重 */
//图像误差提取
float get_roadError(int aim_d)
{
    float err = 0;
    if(aim_d >= midline.pointsNum) {
        if(midline.pointsNum < WEIGHTARR+5) {
            err = midline.edgelinepoint[midline.pointsNum-1][1] - roadMid;
            return err;
        }
        float counts = 0;
        for(int i = 0; i < WEIGHTARR; i++) {
            err += (midline.edgelinepoint[midline.pointsNum-1-i][1] - roadMid) * weight[WEIGHTARR-1-i];
            counts += weight[WEIGHTARR-1-i];
        }
        err /= counts;
    }  
    else {
        uint16_t counts = 0;
        for(int i = 0; i < WEIGHTARR; i++) {
            err += (midline.edgelinepoint[aim_d-i][1] - roadMid) * weight[WEIGHTARR-1-i];
            counts += weight[WEIGHTARR-1-i];
        }
        err /= counts;
    }
    return (err > preprocess.imglimiter) ? preprocess.imglimiter : (err < -preprocess.imglimiter) ? -preprocess.imglimiter : err;
}

//大津法重装计数器
void ReOTSU_Enable_timer(int counts)
{
    static int ReOTSU_timers = counts;

    ReOTSU_timers--;

    if(ReOTSU_timers == 0) {
        ReOTSU_timers = counts;
        ReCalculation_OTSU_Enable = true;
    }
}

//---------------------------------------------------------------------------------------
//
//                                 以下为内部调用函数
//
//---------------------------------------------------------------------------------------

//处理生长算法
void dispose_Edgeline(void)
{
    //计算丢线数
    int firstflag = 1;
    for(int i = 0; i < right_edgeline.pointsNum; i++) {
        if(right_edgeline.edgelinepoint[i][1] >= COLSIMAGE - 3) {
            if(firstflag) {
                firstflag = 0;
                right_edgeline.firstLossY = right_edgeline.edgelinepoint[i][0];
            }
            right_edgeline.lossPoint++;
        }
    }
    firstflag = 1;
    for(int i = 0; i < left_edgeline.pointsNum; i++) {
        if(left_edgeline.edgelinepoint[i][1] <= 2) {
            if(firstflag) {
                firstflag = 0;
                left_edgeline.firstLossY = left_edgeline.edgelinepoint[i][0];
            }
            left_edgeline.lossPoint++;
        }
    }

    if(left_edgeline.state == 'D' && left_edgeline.lossPoint < 10)
    {
        right_edgeline.pointsNum = 0;
        //右转
    }
    if(right_edgeline.state == 'A' && right_edgeline.lossPoint < 10)
    {
        left_edgeline.pointsNum = 0;
        //左转
    }

    //检测左线弱连通
    left_edgeline.is_weak_alone = false;
    if(left_edgeline.edgelinepoint[0][1] <= 2)
    {
        int counts = 0;
        for(int i = 0; i < left_edgeline.pointsNum; i++)
        {
            switch (counts)
            {
            case 0:
                if(left_edgeline.edgelinepoint[i][1] <= 2)
                {
                    counts = 1;
                }
                break;
            case 1:
                if(left_edgeline.edgelinepoint[i][1] > 2)
                {
                    counts = 2;
                }
                break;
            case 2:
                if(left_edgeline.edgelinepoint[i][1] <= 2)
                {
                    counts = 3;
                    left_edgeline.is_weak_alone = false;
                }
                break;
            
            default:
                break;
            }
        }

        if(counts == 2) {
            left_edgeline.is_weak_alone = true;
        }
    }

    //检测右线弱连通
    right_edgeline.is_weak_alone = false;
    if(right_edgeline.edgelinepoint[0][1] >= COLSIMAGE - 3)
    {
        int counts = 0;
        for(int i = 0; i < right_edgeline.pointsNum; i++)
        {
            switch (counts)
            {
            case 0:
                if(right_edgeline.edgelinepoint[i][1] >= COLSIMAGE - 3)
                {
                    counts = 1;
                }
                break;
            case 1:
                if(right_edgeline.edgelinepoint[i][1] < COLSIMAGE - 3)
                {
                    counts = 2;
                }
                break;
            case 2:
                if(right_edgeline.edgelinepoint[i][1] >= COLSIMAGE - 3)
                {
                    counts = 3;
                    right_edgeline.is_weak_alone = false;
                }
                break;
            
            default:
                break;
            }
        }

        if(counts == 2) {
            right_edgeline.is_weak_alone = true;
        }
    }

    //左下拐点
    left_points_turnDown[2] = 0;
    if( right_edgeline.pointsNum != 0)
    {
        for(int i = 5; i < left_edgeline.pointsNum - 5; i++)
        {
            //左下拐点特征  上左
            if( left_edgeline.edgelinepoint[i-0][2] == 0
            &&  (left_edgeline.edgelinepoint[i+1][2] == 3 || left_edgeline.edgelinepoint[i+1][2] == 2)
            )
            {
                //检测下面dir为0的状态数量是否不够
                int dir0conts = 0;
                for(int j = 2; j <= 5; j++) {
                    if( left_edgeline.edgelinepoint[i-j][2] == 0
                    ||  left_edgeline.edgelinepoint[i-j][2] == 1) {
                        dir0conts++;
                    }
                }
                if(dir0conts < 3) {
                    continue;
                }

                //检测上面dir为3的状态数量是否不够
                int dir3conts = 0;
                for(int j = 2; j <= 5; j++) {
                    if( left_edgeline.edgelinepoint[i+j][2] == 3
                    ||  left_edgeline.edgelinepoint[i+j][2] == 2) {
                        dir3conts++;
                    }
                }
                if(dir3conts < 3) {
                    continue;
                }

                left_points_turnDown[0] = left_edgeline.edgelinepoint[i][0];
                left_points_turnDown[1] = left_edgeline.edgelinepoint[i][1];
                left_points_turnDown[2] = 1;
                left_points_turnDown[3] = i;
                break;
            }
        }
    }

    //右下拐点
    right_points_turnDown[2] = 0;
    if( left_edgeline.pointsNum != 0)
    {
        for(int i = 5; i < right_edgeline.pointsNum - 5; i++)
        {
            //右下拐点特征  上右
            if( right_edgeline.edgelinepoint[i+0][2] == 0
            &&  (right_edgeline.edgelinepoint[i+1][2] == 1 || right_edgeline.edgelinepoint[i+1][2] == 2)
            )
            {
                //检测下面dir为0的状态数量是否不够
                int dir0conts = 0;
                for(int j = 2; j <= 5; j++) {
                    if( right_edgeline.edgelinepoint[i-j][2] == 0
                    ||  right_edgeline.edgelinepoint[i-j][2] == 3) {
                        dir0conts++;
                    }
                }
                if(dir0conts < 3) {
                    continue;
                }

                //检测上面dir为1的状态数量是否不够
                int dir1conts = 0;
                for(int j = 2; j <= 5; j++) {
                    if( right_edgeline.edgelinepoint[i+j][2] == 1
                    ||  right_edgeline.edgelinepoint[i+j][2] == 2) {
                        dir1conts++;
                    }
                }
                if(dir1conts < 3) {
                    continue;
                }

                right_points_turnDown[0] = right_edgeline.edgelinepoint[i][0];
                right_points_turnDown[1] = right_edgeline.edgelinepoint[i][1];
                right_points_turnDown[2] = 1;
                right_points_turnDown[3] = i;
                break;
            }
        }
    }

    //右上拐点
    right_points_turnUp[2] = 0;
    if( left_edgeline.pointsNum != 0)
    {
        for(int i = 10; i < right_edgeline.pointsNum - 10; i++)
        {
            //右上拐点特征  左上
            if( (right_edgeline.edgelinepoint[i-1][2] == 3)
            &&  right_edgeline.edgelinepoint[i-0][2] == 0
            &&  right_edgeline.edgelinepoint[i+1][2] == 0
            &&  right_edgeline.edgelinepoint[i+2][2] == 0
            &&  right_edgeline.edgelinepoint[i+3][2] == 0
            &&  right_edgeline.edgelinepoint[i+4][2] == 0
            )
            {
                //检测下面dir为3的状态数量是否不够
                int dir3conts = 0;
                for(int j = 2; j <= 10; j++) {
                    if(right_edgeline.edgelinepoint[i-j][2] == 3
                    || right_edgeline.edgelinepoint[i-j][2] == 2) {
                        dir3conts++;
                    }
                }
                if(dir3conts < 4) {
                    continue;;
                }
                right_points_turnUp[0] = right_edgeline.edgelinepoint[i][0];
                right_points_turnUp[1] = right_edgeline.edgelinepoint[i][1];
                right_points_turnUp[2] = 1;
                right_points_turnUp[3] = i;
                break;
            }
        }
    }

    //左上拐点
    left_points_turnUp[2] = 0;
    if( right_edgeline.pointsNum != 0)
    {
        for(int i = 10; i < left_edgeline.pointsNum - 10; i++)
        {
            //左上拐点特征  右上
            if( (left_edgeline.edgelinepoint[i-1][2] == 1)
            &&  left_edgeline.edgelinepoint[i-0][2] == 0
            &&  left_edgeline.edgelinepoint[i+1][2] == 0
            &&  left_edgeline.edgelinepoint[i+2][2] == 0
            &&  left_edgeline.edgelinepoint[i+3][2] == 0
            &&  left_edgeline.edgelinepoint[i+4][2] == 0
            )
            {
                //检测下面dir为1的状态数量是否不够
                int dir1conts = 0;
                for(int j = 2; j <= 10; j++) {
                    if(left_edgeline.edgelinepoint[i-j][2] == 1
                    || left_edgeline.edgelinepoint[i-j][2] == 2) {
                        dir1conts++;
                    }
                }
                if(dir1conts < 4) {
                    continue;
                }

                left_points_turnUp[0] = left_edgeline.edgelinepoint[i][0];
                left_points_turnUp[1] = left_edgeline.edgelinepoint[i][1];
                left_points_turnUp[2] = 1;
                left_points_turnUp[3] = i;
                break;
            }
        }
    }

}

//处理扫线算法
void dispose_line(void)
{
    left_up_mutation[2] = 0;
    for(int i = left_Line.pointsNum - 10; i >= 10; i --)
    {
        if( left_Line.edgelinepoint[i+3][1] - left_Line.edgelinepoint[i+2][1] < 5
        &&  left_Line.edgelinepoint[i+2][1] - left_Line.edgelinepoint[i+1][1] < 5
        &&  left_Line.edgelinepoint[i+1][1] - left_Line.edgelinepoint[i+0][1] < 5
        &&  left_Line.edgelinepoint[i+0][1] - left_Line.edgelinepoint[i-5][1] > 20)
        {
            left_up_mutation[0] = left_Line.edgelinepoint[i][0];
            left_up_mutation[1] = left_Line.edgelinepoint[i][1];
            left_up_mutation[2] = 1;
            left_up_mutation[3] = i;
            break;
        }
    }

    right_up_mutation[2] = 0;
    for(int i = right_Line.pointsNum - 10; i >= 10; i --)
    {
        if( right_Line.edgelinepoint[i+2][1] - right_Line.edgelinepoint[i+3][1] < 5
        &&  right_Line.edgelinepoint[i+1][1] - right_Line.edgelinepoint[i+2][1] < 5
        &&  right_Line.edgelinepoint[i+0][1] - right_Line.edgelinepoint[i+1][1] < 5
        &&  right_Line.edgelinepoint[i-5][1] - right_Line.edgelinepoint[i+0][1] > 20)
        {
            right_up_mutation[0] = right_Line.edgelinepoint[i][0];
            right_up_mutation[1] = right_Line.edgelinepoint[i][1];
            right_up_mutation[2] = 1;
            right_up_mutation[3] = i;
            break;
        }
    }
}

//最长白列查找
maxWhiteLine max_White_Line(cv::Mat img)
{
    struct maxWhiteLine White;
    White.lineNum = 0;
    White.maxNum = 0;
    White.endlineX = 0;
    White.maxy = yend-1;
    for(uint16_t col = 0; col < COLSIMAGE; col++)
    {
        if(img.at<uchar>(yend-1, col) > Otsu_Threshold)
        {
            uint16_t tmpRow = yend-2;
            uint16_t Whitetmp = 1;
            while(img.at<uchar>(tmpRow, col) > Otsu_Threshold && tmpRow >= imgcutup) {
                Whitetmp++;
                tmpRow--;   //<------------------------------------|
            }                                                    //|
            //upData                                             //|
            if(Whitetmp > White.maxNum) {                        //|
                White.lineNum = 1;                               //|
                White.starlineX = col;                           //|
                White.endlineX = col;                            //|
                White.maxNum = Whitetmp;                         //|
                White.maxy = (tmpRow + 1);          //原因在这--->---
            } else if (Whitetmp == White.maxNum) {    /* 计数最长白列 */
                White.lineNum++;
                White.endlineX = col;
            }
        }
        else continue;
    }

    if(White.maxNum != ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom) {
        return White;
    }

    if(White.endlineX - White.starlineX + 1 <= White.lineNum) {
        return White;
    }

    int Lconts = 0, start_x = White.starlineX+1;
    while(img.at<uchar>(White.maxy, start_x ++) > Otsu_Threshold && start_x < COLSIMAGE-1) {
        int y = White.maxy;
        int conts = 1;
        while(img.at<uchar>(y, start_x) > Otsu_Threshold && y < ROWSIMAGE-1) {
            y++;
            conts++;
        }
        if(conts >= White.maxNum)
            Lconts++;
        else
            break;
    }

    int Rconts = 0, end_x = White.endlineX-1;
    while(img.at<uchar>(White.maxy, end_x --) > Otsu_Threshold && end_x > 0) {
        int y = White.maxy;
        int conts = 1;
        while(img.at<uchar>(y, end_x) > Otsu_Threshold && y < ROWSIMAGE-1) {
            y++;
            conts++;
        }
        if(conts >= White.maxNum)
            Rconts++;
        else
            break;
    }

    if(Lconts <= 10 && Rconts <= 10) {
        int offset = 5;
        int tmp_Lconts = 0, start_x = White.starlineX+1;
        while(img.at<uchar>(White.maxy+offset, start_x ++) > Otsu_Threshold && start_x < COLSIMAGE-1) {
            int y = White.maxy+offset;
            int conts = 1;
            while(img.at<uchar>(y, start_x) > Otsu_Threshold && y < ROWSIMAGE-1) {
                y++;
                conts++;
            }
            if(conts >= White.maxNum-offset)
                tmp_Lconts++;
            else
                break;
        }

        int tmp_Rconts = 0, end_x = White.endlineX-1;
        while(img.at<uchar>(White.maxy+offset, end_x --) > Otsu_Threshold && end_x > 0) {
            int y = White.maxy+offset;
            int conts = 1;
            while(img.at<uchar>(y, end_x) > Otsu_Threshold && y < ROWSIMAGE-1) {
                y++;
                conts++;
            }
            if(conts >= White.maxNum-offset)
                tmp_Rconts++;
            else
                break;
        }

        if(tmp_Lconts >= tmp_Rconts) {
            White.endlineX = White.starlineX + tmp_Lconts;
            White.lineNum = tmp_Lconts;
            White.maxy+=offset;
            White.maxNum-=offset;
        } else {
            White.starlineX = White.endlineX - tmp_Rconts;
            White.lineNum = tmp_Rconts;
            White.maxy+=offset;
            White.maxNum-=offset;
        }

    }
    else if(Lconts >= Rconts) {
        White.endlineX = White.starlineX + Lconts;
        White.lineNum = Lconts;
    } 
    else if (Lconts < Rconts) {
        White.starlineX = White.endlineX - Rconts;
        White.lineNum = Rconts;
    }

    return White;
}

//左基点查找
bool l_basepoint(cv::Mat img, int16_t find_x)
{
    bool foundpoint = false;
    int16_t y = yend - 4;       //减2是因为索引是ystar~yend-1， 再减1防止八邻域越界
    int16_t x = find_x;
    for(int16_t i = x; i > 3 ; i--)
    {
        if(img.at<uchar>(y, i) > Otsu_Threshold
        && img.at<uchar>(y, i-1) < Otsu_Threshold
        && img.at<uchar>(y, i-2) < Otsu_Threshold
        && img.at<uchar>(y, i-3) < Otsu_Threshold)
        {
            leftbase[1] = i;
            leftbase[0] = y;
            foundpoint = true;
        }
    }

    if(!foundpoint) {
        for(int16_t i = 0; i < 5; i++)
        {
            if(img.at<uchar>(y, i) < Otsu_Threshold
            && img.at<uchar>(y, i+1) > Otsu_Threshold)
            {
                leftbase[1] = i+1;
                leftbase[0] = y;
                foundpoint = true;
                break;
            }
        }
    }

    return foundpoint;
}

//右基点查找
bool r_basepoint(cv::Mat img, int16_t find_x)
{
    bool foundpoint = false;
    int16_t y = yend - 4;
    int16_t x = find_x;
    for(int16_t i = x; i < COLSIMAGE - 3; i++)
    {
        if(img.at<uchar>(y, i) > Otsu_Threshold
        && img.at<uchar>(y, i+1) < Otsu_Threshold
        && img.at<uchar>(y, i+2) < Otsu_Threshold
        && img.at<uchar>(y, i+3) < Otsu_Threshold)
        {
            rightbase[1] = i;
            rightbase[0] = y;
            foundpoint = true;
        }
    }

    if(!foundpoint) {
        for(int16_t i = COLSIMAGE-1; i > COLSIMAGE-6; i--)
        {
            if(img.at<uchar>(y, i) < Otsu_Threshold
            && img.at<uchar>(y, i-1) > Otsu_Threshold)
            {
                rightbase[1] = i-1;
                rightbase[0] = y;
                foundpoint = true;
                break;
            }
        }
    }

    return foundpoint;
}

//通过最长白列扫左边线
void use_Whiteline_find_leftline(cv::Mat img)
{
    Mat tmpImg = img;
    left_Line.pointsNum = 0;
    left_Line.lossPoint = 0;
    for(int y = yend - 4; y > maxWhite.maxy; y--)
    {
        for(int x = maxWhite.starlineX; x > 1; x--)
        {
            if(tmpImg.at<uchar>(y, x) >= Otsu_Threshold
            && tmpImg.at<uchar>(y, x-1) < Otsu_Threshold)
            {
                left_Line.edgelinepoint[left_Line.pointsNum][0] = y;
                left_Line.edgelinepoint[left_Line.pointsNum][1] = x;
                left_Line.pointsNum++;
                left_Line.lossPoint = (x == 2) ? left_Line.lossPoint+1 : left_Line.lossPoint;
                break;
            }
            if(tmpImg.at<uchar>(y, x) < Otsu_Threshold
            && tmpImg.at<uchar>(y, x-1) < Otsu_Threshold)
            {
                left_Line.edgelinepoint[left_Line.pointsNum][0] = y;
                left_Line.edgelinepoint[left_Line.pointsNum][1] = left_Line.edgelinepoint[left_Line.pointsNum-1][1];
                left_Line.pointsNum++;
                left_Line.lossPoint = (x == 2) ? left_Line.lossPoint+1 : left_Line.lossPoint;
                break;
            }
        }
    }
}

//通过最长白列扫右边线
void use_Whiteline_find_rightline(cv::Mat img)
{
    Mat tmpImg = img;
    right_Line.pointsNum = 0;
    right_Line.lossPoint = 0;
    for(int y = yend - 4; y > maxWhite.maxy; y--)
    {
        for(int x = maxWhite.endlineX; x < COLSIMAGE-2; x++)
        {
            if(tmpImg.at<uchar>(y, x) >= Otsu_Threshold
            && tmpImg.at<uchar>(y, x+1) < Otsu_Threshold)
            {
                right_Line.edgelinepoint[right_Line.pointsNum][0] = y;
                right_Line.edgelinepoint[right_Line.pointsNum][1] = x;
                right_Line.pointsNum++;
                right_Line.lossPoint = (x == 157) ? right_Line.lossPoint+1 : right_Line.lossPoint;
                break;
            }
            if(tmpImg.at<uchar>(y, x) < Otsu_Threshold
            && tmpImg.at<uchar>(y, x+1) < Otsu_Threshold)
            {
                right_Line.edgelinepoint[right_Line.pointsNum][0] = y;
                right_Line.edgelinepoint[right_Line.pointsNum][1] = right_Line.edgelinepoint[right_Line.pointsNum-1][1];
                right_Line.pointsNum++;
                right_Line.lossPoint = (x == 157) ? right_Line.lossPoint+1 : right_Line.lossPoint;
                break;
            }
        }
    }
}

//上交迷宫法左寻线
void findline_lefthand_adaptive(cv::Mat img)
{
    left_edgeline.state = 0;
    left_edgeline.lossPoint = 0;
    int x = leftbase[1];
    int y = leftbase[0];
    int step = 0, dir = 0, turn = 0;
    int half = 2;
    while (step < MAX_POINT && turn < 4) 
    {
        // if(half >= x) {
        //     left_edgeline.state = 'A';
        //     break;
        if (x >= img.cols - half - 1) {
            left_edgeline.state = 'D';
            break;
        } else if (half >= y) {
            left_edgeline.state = 'W';
            break;
        } else if (y >= img.rows - half - 1) {
            left_edgeline.state = 'S';
            break;
        }

        int local_thres = Otsu_Threshold;

        int current_value = img.at<uchar>(y, x);
        int front_value = img.at<uchar>(y + dir_front[dir][1], x + dir_front[dir][0]);
        int frontleft_value = img.at<uchar>(y + dir_frontleft[dir][1], x + dir_frontleft[dir][0]);
        if (front_value < local_thres) {
            dir = (dir + 1) % 4;
            turn++;
        } else if (frontleft_value < local_thres) {
            x += dir_front[dir][0];
            y += dir_front[dir][1];
            left_edgeline.edgelinepoint[step][2] = dir;
            left_edgeline.edgelinepoint[step][1] = x;
            left_edgeline.edgelinepoint[step][0] = y;
            step++;
            turn = 0;
        } else {
            x += dir_frontleft[dir][0];
            y += dir_frontleft[dir][1];
            dir = (dir + 3) % 4;
            left_edgeline.edgelinepoint[step][2] = dir;
            left_edgeline.edgelinepoint[step][1] = x;
            left_edgeline.edgelinepoint[step][0] = y;
            step++;
            turn = 0;
        }
    }
    left_edgeline.pointsNum = step;
}

//上交迷宫法右寻线
void findline_righthand_adaptive(cv::Mat img)
{
    right_edgeline.state = 0;
    right_edgeline.lossPoint = 0;
    int x = rightbase[1];
    int y = rightbase[0];
    int step = 0, dir = 0, turn = 0;
    int half = 2;
    while (step < MAX_POINT && turn < 4) 
    {
        // if (x >= img.cols - half - 1) {
        //     right_edgeline.state = 'D';
        //     break;
        if(half >= x) {
            right_edgeline.state = 'A';
            break;
        } else if (half >= y) {
            right_edgeline.state = 'W';
            break;
        } else if (y >= img.rows - half - 1) {
            right_edgeline.state = 'S';
            break;
        }

        int local_thres = Otsu_Threshold;

        int current_value = img.at<uchar>(y, x);
        int front_value = img.at<uchar>(y + dir_front[dir][1], x + dir_front[dir][0]);
        int frontright_value = img.at<uchar>(y + dir_frontright[dir][1], x + dir_frontright[dir][0]);
        if (front_value < local_thres) {
            dir = (dir + 3) % 4;
            turn++;
        } else if (frontright_value < local_thres) {
            x += dir_front[dir][0];
            y += dir_front[dir][1];
            right_edgeline.edgelinepoint[step][2] = dir;
            right_edgeline.edgelinepoint[step][1] = x;
            right_edgeline.edgelinepoint[step][0] = y;
            step++;
            turn = 0;
        } else {
            x += dir_frontright[dir][0];
            y += dir_frontright[dir][1];
            dir = (dir + 1) % 4;
            right_edgeline.edgelinepoint[step][2] = dir;
            right_edgeline.edgelinepoint[step][1] = x;
            right_edgeline.edgelinepoint[step][0] = y;
            step++;
            turn = 0;
        }
    }
    right_edgeline.pointsNum = step;
}

//直道检测
bool checking_Normal_straightaway(void)
{
    if( left_edgeline.lossPoint == 0 && left_edgeline.pointsNum > 80
    &&  right_edgeline.lossPoint == 0 && right_edgeline.pointsNum > 80
    &&  abs(left_edgeline.edgelinepoint[left_edgeline.pointsNum - 1][1] - maxWhite.starlineX) < 5
    &&  abs(right_edgeline.edgelinepoint[right_edgeline.pointsNum - 1][1] - maxWhite.endlineX) < 5
    &&  maxWhite.maxNum == ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom )
    {
        return true;
    }

    if( left_edgeline.lossPoint == 0 && left_edgeline.pointsNum > 80
    &&  right_edgeline.lossPoint != 0 
    &&  right_edgeline.is_weak_alone 
    &&  right_edgeline.lossPoint < 30)
    {
        return true;
    }

    if(left_edgeline.lossPoint != 0 
    && right_edgeline.lossPoint == 0 && right_edgeline.pointsNum > 80
    && left_edgeline.is_weak_alone 
    && left_edgeline.lossPoint < 30)
    {
        return true;
    }

    if( shieldZebra == true
    &&  left_edgeline.lossPoint == 0 && left_edgeline.pointsNum > 80
    &&  right_edgeline.lossPoint == 0 && right_edgeline.pointsNum > 80)
    {
        return true;
    }

    return false;
}

//弱直道检测
bool checking_curve_to_straightaway(void)
{
    if(maxWhite.maxNum < ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom) {
        return false;
    }

    if(maxWhite.lineNum < 10) {
        return false;
    }

    if( (left_edgeline.lossPoint == 0 && !left_points_turnDown[2] && !left_points_turnUp[2] && left_edgeline.pointsNum > 80)
    ||  (right_edgeline.lossPoint == 0 && !right_points_turnDown[2] && !right_points_turnUp[2] && right_edgeline.pointsNum > 80)) {
        return true;
    }

    if( (left_edgeline.is_weak_alone && left_edgeline.lossPoint < 30 && !left_points_turnDown[2] && !left_points_turnUp[2])
    ||  (right_edgeline.is_weak_alone && right_edgeline.lossPoint < 30 && !right_points_turnDown[2] && !right_points_turnUp[2]))
    {
        return true;
    }

    return false;

}

#define LONG_K      (1.9375f)

//重新画直道
void redraw_long_road(void)
{
    int left_x;
    int right_x;
    if(maxWhite.starlineX - left_Line.edgelinepoint[left_Line.pointsNum-1][1] < 5) {
        left_x = left_Line.edgelinepoint[left_Line.pointsNum-1][1];
    } else {
        left_x = maxWhite.starlineX;
    }

    if(right_Line.edgelinepoint[right_Line.pointsNum-1][1] - maxWhite.endlineX < 5) {
        right_x = right_Line.edgelinepoint[right_Line.pointsNum-1][1];
    } else {
        right_x = maxWhite.endlineX;
    }

    float lb = preprocess.calculate_slopeB(-LONG_K, left_Line.edgelinepoint[left_Line.pointsNum-1][0], left_x);
    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = leftbase[0] - i;
        left_Line.edgelinepoint[i][1] = ((float)(leftbase[0] - i) - lb) / -LONG_K;
    }

    float rb = preprocess.calculate_slopeB(LONG_K, right_Line.edgelinepoint[right_Line.pointsNum-1][0], right_x);
    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = rightbase[0] - i;
        right_Line.edgelinepoint[i][1] = ((float)(rightbase[0] - i) - rb) / LONG_K;
    }
}

//常规路段决策
void normalRoad_decusion(void)
{
    switch(normalstate)
    {
        case Normal_longway:
        {   
            left_plus_right_2midline();
        }
        break;

        case Normal_curve:
        {
            //重判断直道
            if(checking_curve_to_straightaway())
            {
                normalstate = Normal_longway_weak;
                if( (left_edgeline.is_weak_alone || left_edgeline.lossPoint == 0)
                &&  left_edgeline.pointsNum > 80)
                {
                    leftLine_plus_halfWide();
                    break;
                }

                if( (right_edgeline.is_weak_alone || right_edgeline.lossPoint == 0)
                &&  right_edgeline.pointsNum > 80)
                {
                    rightLine_plus_halfWide();
                    break;
                }

            }
            else //确实是弯道
            {
                left_plus_right_2midline();
            }
        }
        break;
    }
}

/* 最长白列串道 */
void reMaxWhite(cv::Mat img)
{
    if(rightbase[1] - leftbase[1] < 155) {
        return ;
    }

    if(maxWhite.lineNum > 10) {
        return ;
    }

    if(maxWhite.maxNum >= ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom - 5)
    {
        if(right_edgeline.pointsNum == MAX_POINT && left_edgeline.pointsNum != MAX_POINT ) {

            struct maxWhiteLine White;
            White.lineNum = 0;
            White.maxNum = 0;
            White.endlineX = 0;
            White.maxy = yend-1;
            for(uint16_t col = COLSIMAGE - 1; col > COLSIMAGE / 2; col--)
            {
                if(img.at<uchar>(yend-1, col) > Otsu_Threshold)
                {
                    uint16_t tmpRow = yend-2;
                    uint16_t Whitetmp = 1;
                    while(img.at<uchar>(tmpRow, col) > Otsu_Threshold && tmpRow >= imgcutup) {
                        Whitetmp++;
                        tmpRow--;   //<------------------------------------|
                    }                                                    //|
                    //upData                                             //|
                    if(Whitetmp > White.maxNum) {                        //|
                        White.lineNum = 1;                               //|
                        White.starlineX = col;                           //|
                        White.endlineX = col;                            //|
                        White.maxNum = Whitetmp;                         //|
                        White.maxy = (tmpRow + 1);          //原因在这--->---
                    } else if (Whitetmp == White.maxNum) {    /* 计数最长白列 */
                        White.lineNum++;
                        White.endlineX = col;
                    }
                }
                else continue;
            }
            maxWhite = White;
        }

        else if(right_edgeline.pointsNum != MAX_POINT && left_edgeline.pointsNum == MAX_POINT )  {

            struct maxWhiteLine White;
            White.lineNum = 0;
            White.maxNum = 0;
            White.endlineX = 0;
            White.maxy = yend-1;
            for(uint16_t col = 0; col < COLSIMAGE / 2; col++)
            {
                if(img.at<uchar>(yend-1, col) > Otsu_Threshold)
                {
                    uint16_t tmpRow = yend-2;
                    uint16_t Whitetmp = 1;
                    while(img.at<uchar>(tmpRow, col) > Otsu_Threshold && tmpRow >= imgcutup) {
                        Whitetmp++;
                        tmpRow--;   //<------------------------------------|
                    }                                                    //|
                    //upData                                             //|
                    if(Whitetmp > White.maxNum) {                        //|
                        White.lineNum = 1;                               //|
                        White.starlineX = col;                           //|
                        White.endlineX = col;                            //|
                        White.maxNum = Whitetmp;                         //|
                        White.maxy = (tmpRow + 1);          //原因在这--->---
                    } else if (Whitetmp == White.maxNum) {    /* 计数最长白列 */
                        White.lineNum++;
                        White.endlineX = col;
                    }
                }
                else continue;
            }
            maxWhite = White;
        }
    }
}

//画边线
void drawEdgeLines(cv::Mat &img)
{
    for (int i = 0; i < left_edgeline.pointsNum; ++i)
    {
        int x = left_edgeline.edgelinepoint[i][1];
        int y = left_edgeline.edgelinepoint[i][0];
        if(left_edgeline.edgelinepoint[i][2] == 0)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(255, 0, 0), -1);
        else if(left_edgeline.edgelinepoint[i][2] == 1)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 255, 0), -1);
        else if(left_edgeline.edgelinepoint[i][2] == 2)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 0, 255), -1);
        else if(left_edgeline.edgelinepoint[i][2] == 3)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 0, 0), -1);
    }
    // 绘制右边缘线
    for (int i = 0; i < right_edgeline.pointsNum; ++i)
    {
        int x = right_edgeline.edgelinepoint[i][1];
        int y = right_edgeline.edgelinepoint[i][0];
        if(right_edgeline.edgelinepoint[i][2] == 0)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(255, 0, 0), -1);
        else if(right_edgeline.edgelinepoint[i][2] == 1)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 255, 0), -1);
        else if(right_edgeline.edgelinepoint[i][2] == 2)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 0, 255), -1);
        else if(right_edgeline.edgelinepoint[i][2] == 3)
            cv::circle(img, cv::Point(x, y), 2, cv::Scalar(0, 0, 0), -1);
    }
    cv::line(img, cv::Point(maxWhite.starlineX, 0), cv::Point(maxWhite.starlineX, img.rows-1), cv::Scalar(0, 255, 0), 1);
    cv::line(img, cv::Point(maxWhite.endlineX, 0), cv::Point(maxWhite.endlineX, img.rows-1), cv::Scalar(0, 255, 0), 1);

    if(left_points_turnDown[2])
        cv::circle(img, cv::Point(left_points_turnDown[1], left_points_turnDown[0]), 4, cv::Scalar(75, 75, 75), -1);
    if(right_points_turnUp[2])
        cv::circle(img, cv::Point(right_points_turnUp[1], right_points_turnUp[0]), 4, cv::Scalar(255, 255, 255), -1);
    if(left_points_turnUp[2])
        cv::circle(img, cv::Point(left_points_turnUp[1], left_points_turnUp[0]), 4, cv::Scalar(255, 255, 255), -1);
    if(right_points_turnDown[2])
        cv::circle(img, cv::Point(right_points_turnDown[1], right_points_turnDown[0]), 4, cv::Scalar(75, 75, 75), -1);
}

void drawRoadLines(cv::Mat &img)
{
    for (int i = 0; i < left_Line.pointsNum; ++i)
    {
        int x = left_Line.edgelinepoint[i][1];
        int y = left_Line.edgelinepoint[i][0];
        cv::circle(img, cv::Point(x, y), 1, cv::Scalar(0, 255, 0), -1);
    }
    for (int i = 0; i < right_Line.pointsNum; ++i)
    {
        int x = right_Line.edgelinepoint[i][1];
        int y = right_Line.edgelinepoint[i][0];
        cv::circle(img, cv::Point(x, y), 1, cv::Scalar(0, 0, 255), -1);
    }
    for(int i = 0; i < midline.pointsNum; i++)
    {
        int x = midline.edgelinepoint[i][1];
        int y = midline.edgelinepoint[i][0];
        cv::circle(img, cv::Point(x, y), 1, cv::Scalar(255, 0, 0), -1);
    }
    cv::line(img, cv::Point(maxWhite.starlineX, 0), cv::Point(maxWhite.starlineX, img.rows-1), cv::Scalar(0, 255, 0), 1);
    cv::line(img, cv::Point(maxWhite.endlineX, 0), cv::Point(maxWhite.endlineX, img.rows-1), cv::Scalar(255, 0, 0), 1);

    if(left_up_mutation[2])
        cv::circle(img, cv::Point(left_up_mutation[1], left_up_mutation[0]), 4, cv::Scalar(255, 255, 255), -1);
    if(right_up_mutation[2])
        cv::circle(img, cv::Point(right_up_mutation[1], right_up_mutation[0]), 4, cv::Scalar(255, 255, 255), -1);

    cv::line(img, cv::Point(0, img.rows - 1 - trackAim), cv::Point(img.cols-1, img.rows - 1 - trackAim), cv::Scalar(255, 255, 255), 1);
}


//避障判断
bool check_Barrier_Road(cv::Mat imgYellow)
{
    switch(roadstate)
    {
        case Normal_Road:
            Yellowcount = 0;
            for(int i = 120; i < 160; i++)
            {
                for(int j = 65; j < 80; j++)
                {
                    if(imgYellow.at<uchar>(j, i) > 125)
                    {
                        Yellowcount++;
                    }
                }
            }
            if(Yellowcount > 75)
            {
                motor_distance_cleam();
                barrier_dir = Barrier_R;
                return true;
            }

            Yellowcount = 0;
            for(int i = 40; i > 0; i--)
            {
                for(int j = 65; j < 80; j++)
                {
                    if(imgYellow.at<uchar>(j, i) > 125)
                    {
                        Yellowcount++;
                    }
                }
            }
            if(Yellowcount > 75)
            {
                motor_distance_cleam();
                barrier_dir = Barrier_L;
                return true;
            }

            break;

        case Barrier_Road:
            switch (barrier_dir)
            {
            case Barrier_L:
                if(motor_distance > preprocess.barrierOUT)
                    return false;
                else
                    return true;
                break;

            case Barrier_R:
                if(motor_distance > preprocess.barrierOUT)
                    return false;
                else
                    return true;
                break;
            
            default:
                break;
            }
        break;
    }

    

    return false;
    
}

//避障处理
void Barrier_Road_preprocess(void)
{
    switch (barrier_dir)
    {
    case Barrier_L:
        rightLine_plus_HOWhalfWide(2);
        break;

    case Barrier_R:
        leftLine_plus_HOWhalfWide(2);
        break;
    
    default:
        break;
    }
}

//检测坡道
bool check_Bridge(cv::Mat imgRed)
{
    
    switch (roadstate)
    {
    case Normal_Road:
        Redcount = 0;
        if( maxWhite.maxNum >= ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom)
        {
            for(int i = 120; i < 160; i++)
            {
                for(int j = 65; j < 80; j++)
                {
                    if(imgRed.at<uchar>(j, i) > 125)
                    {
                        Redcount++;
                    }
                }
            }
            if(Redcount > 75)
            {
                gyro_y_integration = 0;
                bridge_state = Bridge_up;
                return true;
            }
        }
        break;
    case Bridge_Road:
        switch (bridge_state)
        {
        case Bridge_up:
            if(gyro_y_integration < -5)
            {
                bridge_state = Bridge_down;
                return true;
            }
            else
            {
                return true;
            }
            break;

        case Bridge_down:
            if(gyro_y_integration > 5)
            {
                bridge_state = Bridge_out;
                return true;
            }
            else
            {
                return true;
            }

        case Bridge_out:
            if(gyro_y_integration < 5)
            {
                bridge_state = no_Bridge;
                return false;
            }
            else
            {
                return true;
            }
            break;
        
        default:
            break;
        }
        break;
    }

    return false;
    
}

//坡道处理
void Bridge_Road_preprocess(void)
{
    left_plus_right_2midline();
}

//状态复位
void reset_ALL_state(void)
{
    //斑马线屏蔽
    shieldZebra = true;
    //圆环个数重装载
    RingCounts = preprocess.Ring_Count;
    //当前圆环索引
    if(RingDir)
        nowRingIndex = 0;
    else
        nowRingIndex = RingCounts - 1;
    //道路状态
    roadstate = Normal_Road;
    //圆环状态
    ringstate = No_Ring;
    

    gyro_integration_clean();
    motor_distance_cleam();
}

