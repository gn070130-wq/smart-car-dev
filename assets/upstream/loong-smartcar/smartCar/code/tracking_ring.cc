#include "tracking_ring.h"

//返回图像xy坐标
struct returnXY {
    int16_t imgx;
    int16_t imgy;
    int16_t index;
};

Ring_State ringstate;

//圆环补线数组+当前索引
int *inRing_X_Arr[5];
int *outRing_X_Arr[5];
int nowRingIndex = 0;

//圆环出环里程
int *outRing_distance[5];
//圆环跟踪前瞻
int *ringTrackAim[5];

//圆环数量
int RingCounts = 0;

//圆环顺序
bool RingDir = true;

//圆环初始化
void Ring_init(void)
{
    inRing_X_Arr[0] = &preprocess.ring1_X_in;
    inRing_X_Arr[1] = &preprocess.ring2_X_in;
    inRing_X_Arr[2] = &preprocess.ring3_X_in;
    inRing_X_Arr[3] = &preprocess.ring4_X_in;
    inRing_X_Arr[4] = &preprocess.ring5_X_in;

    outRing_X_Arr[0] = &preprocess.ring1_X_out;
    outRing_X_Arr[1] = &preprocess.ring2_X_out;
    outRing_X_Arr[2] = &preprocess.ring3_X_out;
    outRing_X_Arr[3] = &preprocess.ring4_X_out;
    outRing_X_Arr[4] = &preprocess.ring5_X_out;

    outRing_distance[0] = &preprocess.ring1_out_dis;
    outRing_distance[1] = &preprocess.ring2_out_dis;
    outRing_distance[2] = &preprocess.ring3_out_dis;
    outRing_distance[3] = &preprocess.ring4_out_dis;
    outRing_distance[4] = &preprocess.ring5_out_dis;

    ringTrackAim[0] = &preprocess.ring1_trackAim;
    ringTrackAim[1] = &preprocess.ring2_trackAim;
    ringTrackAim[2] = &preprocess.ring3_trackAim;
    ringTrackAim[3] = &preprocess.ring4_trackAim;
    ringTrackAim[4] = &preprocess.ring5_trackAim;
}

bool check_Right_intoRing_weak(void)
{
    //有拐点识别成功
    bool right_flag = 0;

    if(right_points_turnUp[2] && right_up_mutation[2] 
    &&  abs(right_points_turnUp[0] - right_up_mutation[0]) < 10
    &&  abs(right_points_turnUp[1] - right_up_mutation[1]) < 10
    &&  !left_up_mutation[2]
    &&  right_points_turnUp[0] > preprocess.Ring_into_Size && right_points_turnUp[0] < preprocess.Ring_into_Size_Max)
    {
        right_flag = 1;
    }

    if(!right_flag) {
        ringstate = No_Ring;
        return false;
    }

    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        //过滤底部丢线
        if(left_Line.edgelinepoint[i][1] <= 2 && i == 0) {
            i++;
            for(i; i < left_Line.pointsNum; i++)
            {
                if(left_Line.edgelinepoint[i][1] > 2)
                {
                    break;
                }
            }

            if( left_Line.edgelinepoint[i][0] <= right_points_turnUp[0]
            ||  i > 30)
            {
                ringstate = No_Ring;
                return false;
            }
        } 
        else 
        {
            i++;
        }

        if( left_Line.edgelinepoint[i][1] - left_Line.edgelinepoint[i-1][1] < 0)
        {
            ringstate = No_Ring;
            return false;
        }

        if( left_Line.edgelinepoint[i][0] > right_points_turnUp[0]
        &&  left_Line.edgelinepoint[i][1] <= 2) 
        {
            ringstate = No_Ring;
            return false;
        }

        if( left_Line.edgelinepoint[i][0] <= right_points_turnUp[0])
        {
            ringstate = Right_intoWeak;
            return true;
        }

    }

    ringstate = No_Ring;
    return false;
}

bool check_Left_intoRing_weak(void)
{
    //有拐点识别成功
    bool left_flag = 0;

    if(left_points_turnUp[2] && left_up_mutation[2] 
    &&  abs(left_points_turnUp[0] - left_up_mutation[0]) < 10
    &&  abs(left_points_turnUp[1] - left_up_mutation[1]) < 10
    &&  !right_up_mutation[2]
    &&  left_points_turnUp[0] > preprocess.Ring_into_Size && left_points_turnUp[0] < preprocess.Ring_into_Size_Max)
    {
        left_flag = 1;
    }

    if(!left_flag) {
        ringstate = No_Ring;
        return false;
    }

    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        //过滤底部丢线
        if(right_Line.edgelinepoint[i][1] >= COLSIMAGE - 3 && i == 0) {
            i++;
            for(i; i < right_Line.pointsNum; i++)
            {
                if(right_Line.edgelinepoint[i][1] < COLSIMAGE - 3)
                {
                    break;
                }
            }

            if( right_Line.edgelinepoint[i][0] <= left_points_turnUp[0]
            ||  i > 30)
            {
                ringstate = No_Ring;
                return false;
            }
        }
        else
        {
            i++;
        }

        if( right_Line.edgelinepoint[i-1][1] - right_Line.edgelinepoint[i][1] < 0)
        {
            ringstate = No_Ring;
            return false;
        }

        if( right_Line.edgelinepoint[i][0] > left_points_turnUp[0]
        &&  right_Line.edgelinepoint[i][1] >= COLSIMAGE - 3) 
        {
            ringstate = No_Ring;
            return false;
        }

        if( right_Line.edgelinepoint[i][0] <= left_points_turnUp[0])
        {
            ringstate = Left_intoWeak;
            return true;
        }

    }

    ringstate = No_Ring;
    return false;
}

//-----------------------------------右圆环检测部分-----------------------------------------

//判断右圆环入口
bool check_Right_intoRing(cv::Mat imgG, int size)
{
    
    if(right_points_turnUp[2] && right_up_mutation[2] 
    &&  abs(right_points_turnUp[0] - right_up_mutation[0]) < 10
    &&  abs(right_points_turnUp[1] - right_up_mutation[1]) < 10
    &&  right_points_turnUp[0] > size && right_points_turnUp[0] < preprocess.Ring_into_Size_Max
    )
    {
        if(right_edgeline.pointsNum - right_points_turnUp[3] > 50) {
            return false;
        }

        int tmpCount = 0;
        int j = 10;
        for(int i = right_up_mutation[3]; i > 0 && j > 0; i--, j--)
        {
            if(right_Line.edgelinepoint[i][1] > COLSIMAGE - 6)
            {
                tmpCount++;
            }
        }

        if(tmpCount < 1)
        {
            return false;
        }

        return true;
    }
    return false;
}

//检测是否在圆内
bool check_Right_inRing(void)
{
    if(gyro_z_integration <= -INRING) {
        return true;
    }

    return false;
}

//检测右即将出环
bool check_Right_ReadyoutRing(void)
{
    if(gyro_z_integration > -READYOUTRING)
    {
        return false;
    }

    return true;
}


//检测右出环状态
bool check_Right_outRing(void)
{
    if(gyro_z_integration > -OUTRING) {
        return false;
    }

    return true;
}

//检测退出右圆环状态
bool check_Out_RightRing(void)
{
    if(motor_distance < *outRing_distance[nowRingIndex]) {
        return false;
    }

    return true;
}

//右圆环状态检测
void RightRing_State_checking(cv::Mat imgG)
{
    //Right_RingFront检测(只在直道的时候检测)
    if(
        ( (roadstate == Normal_Road && normalstate == Normal_longway_weak) 
    || (roadstate == Ring_into_weak && ringstate == Right_intoWeak) )
    &&  check_Right_intoRing(imgG, preprocess.Ring_into_Size)
    )
    {
        roadstate = Ring_Road;
        ringstate = Right_intoRing;
        gyro_integration_clean();
    }
    //进圆环检测
    if(roadstate == Ring_Road && ringstate == Right_intoRing)
    {
        if(check_Right_inRing()) {
            ringstate = Right_inRing;
        }
        
    }
    //环内检测
    if(roadstate == Ring_Road && ringstate == Right_inRing)
    {
        if(check_Right_ReadyoutRing()) {
            ringstate = Right_ReadyoutRing;
        }
    }
    //出圆环检测
    if(roadstate == Ring_Road && ringstate == Right_ReadyoutRing)
    {
        if(check_Right_outRing()) {
            ringstate = Right_outRing;
            motor_distance_cleam();
        }   
    }
    //退出圆环检测
    if(roadstate == Ring_Road && ringstate == Right_outRing)
    {
        if(check_Out_RightRing()) {
            roadstate = Normal_Road;
            ringstate = No_Ring;
            if(RingDir) {
                nowRingIndex = (nowRingIndex + 1) % 5;
            } else {
                nowRingIndex = (nowRingIndex + 5 - 1) % 5;
            }
            RingCounts--;
        }
    }
}
//---------------------------------左圆环检测部分--------------------------------------

//判断左圆环入口
bool check_Left_intoRing(cv::Mat imgG, int size)
{
    if(left_points_turnUp[2] && left_up_mutation[2] 
    &&  abs(left_points_turnUp[0] - left_up_mutation[0]) < 10
    &&  abs(left_points_turnUp[1] - left_up_mutation[1]) < 10
    &&  left_points_turnUp[0] > size && left_points_turnUp[0] < preprocess.Ring_into_Size_Max
    )
    {
        if(left_edgeline.pointsNum - left_points_turnUp[3] > 50) {
            return false;
        }

        int tmpCount = 0;
        int j = 10;
        for(int i = left_up_mutation[3]; i > 0 && j > 0; i--, j--)
        {
            if(left_Line.edgelinepoint[i][1] < 5)
            {
                tmpCount++;
            }
        }

        if(tmpCount < 1)
        {
            return false;
        }

        return true;
    }

    return false;

}

// 检测是否在左圆环内
bool check_Left_inRing(void)
{
    if(gyro_z_integration >= INRING) {  // 左转为负值
        return true;
    }
    return false;
}

// 检测左即将出环
bool check_Left_ReadyoutRing(void)
{
    if(gyro_z_integration < READYOUTRING)  // 左转角度绝对值比较
    {
        return false;
    }
    return true;
}

// 检测左出环状态
bool check_Left_outRing(void)
{
    if(gyro_z_integration < OUTRING) {  // 左转360度完成
        return false;
    }
    return true;
}

// 检测退出左圆环状态（保持原距离判断逻辑）
bool check_Out_LeftRing(void)
{
    if(motor_distance < *outRing_distance[nowRingIndex]) {
        return false;
    }
    return true;
}

//左圆环状态检测
void LeftRing_State_checking(cv::Mat imgG)
{
    //Right_RingFront检测(只在直道的时候检测)
    if(
        ( ( roadstate == Normal_Road && normalstate == Normal_longway_weak) 
    ||  (roadstate == Ring_into_weak && ringstate == Left_intoWeak)
    )
    &&  check_Left_intoRing(imgG, preprocess.Ring_into_Size)
    )
    {
        roadstate = Ring_Road;
        ringstate = Left_intoRing;
        gyro_integration_clean();
    }
    //进圆环检测
    if(roadstate == Ring_Road && ringstate == Left_intoRing)
    {
        if(check_Left_inRing()) {
            ringstate = Left_inRing;
        }
        
    }
    //环内检测
    if(roadstate == Ring_Road && ringstate == Left_inRing)
    {
        if(check_Left_ReadyoutRing()) {
            ringstate = Left_ReadyoutRing;
        }
    }
    //出圆环检测
    if(roadstate == Ring_Road && ringstate == Left_ReadyoutRing)
    {
        if(check_Left_outRing()) {
            ringstate = Left_outRing;
            motor_distance_cleam();
        }   
    }
    //退出圆环检测
    if(roadstate == Ring_Road && ringstate == Left_outRing)
    {
        if(check_Out_LeftRing()) {
            roadstate = Normal_Road;
            ringstate = No_Ring;

            if(RingDir) {
                nowRingIndex = (nowRingIndex + 1) % 5;
            } else {
                nowRingIndex = (nowRingIndex + 5 - 1) % 5;
            }
                RingCounts--;  
        }
            
    }
}
//---------------------------------左圆环检测结束--------------------------------------


//---------------------------------右圆环处理部分---------------------------------------

//左固定拉线
void creat_fixedLeftLine(int leftx, int lefty)
{
    int tar_lefty = lefty;
    int tar_leftx = 80 + leftx;
    int Lstary = leftbase[0];
    int Lstarx = COLSIMAGE/ 2 - 60;
    
    float lk = preprocess.calculate_slopeK(tar_lefty, Lstary, tar_leftx, Lstarx);
    float lb = preprocess.calculate_slopeB(lk, tar_lefty, tar_leftx);

    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = Lstary - i;
        left_Line.edgelinepoint[i][1] = (Lstary - i - lb) / lk;

        if(left_Line.edgelinepoint[i][0] == tar_lefty)
        {
            left_Line.pointsNum = i+1;
            break;
        }
    }

    right_edgeline.pointsNum = left_Line.pointsNum;
    for(int i = 0; i < right_edgeline.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = left_Line.edgelinepoint[i][0];
        right_Line.edgelinepoint[i][1] = COLSIMAGE - 3;
    }

    left_plus_right_2midline();
}

//入环处理
void Right_intoRing_processing(int leftx, int lefty)
{
    //环外拉线
    creat_fixedLeftLine(leftx, lefty);
}

//环内处理
void Right_inRing_processing()
{
    //左右合成中线
    left_plus_right_2midline();
}

//准备出环处理
void Right_ReadyoutRing_processing(int leftx, int lefty)
{
    //环外拉线
    creat_fixedLeftLine(leftx, lefty);
}

//出环单边
void Right_outRing_processing(void)
{
    leftLine_plus_halfWide();
}
//-----------------------右圆环处理结束-----------------------------------


//-----------------------左圆环处理开始----------------------------------

//右固定拉线
void creat_fixedRightLine(int rightx, int righty)
{
    int tar_righty = righty;
    int tar_rightx = 80 - rightx;
    int Rstary = leftbase[0];
    int Rstarx = COLSIMAGE/ 2 + 60;

    float rk = preprocess.calculate_slopeK(tar_righty, Rstary, tar_rightx, Rstarx);
    float rb = preprocess.calculate_slopeB(rk, tar_righty, tar_rightx);

    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = Rstary - i;
        right_Line.edgelinepoint[i][1] = (Rstary - i - rb) / rk;

        if(right_Line.edgelinepoint[i][0] == tar_righty)
        {
            right_Line.pointsNum = i+1;
            break;
        }
    }

    left_edgeline.pointsNum = right_Line.pointsNum;
    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = right_Line.edgelinepoint[i][0];
        left_Line.edgelinepoint[i][1] = 2;
    }

    left_plus_right_2midline();
}

//入环处理
void Left_intoRing_processing(int rightx, int righty)
{
    //环外拉线
    creat_fixedRightLine(rightx, righty);
}

//环内处理
void Left_inRing_processing()
{
    //左右合成中线
    left_plus_right_2midline();
}

//准备出环处理
void Left_ReadyoutRing_processing(int rightx, int righty)
{
    creat_fixedRightLine(rightx, righty);
}

//出环单边
void Left_outRing_processing(void)
{
    rightLine_plus_halfWide();
}


//圆环处理
//用法:ring_processing(imageGray, preprocess.rightRing_leftY, inRing_X_Arr[nowRingIndex], outRing_X_Arr[nowRingIndex]);
void ring_processing(cv::Mat img, int lefty, int leftx, int outleftx)
{
    switch (ringstate)
    {
    case Right_intoRing:
        Right_intoRing_processing(leftx, lefty);
        break;

    case Right_inRing:
        Right_inRing_processing();
        break;

    case Right_ReadyoutRing:
        Right_ReadyoutRing_processing(outleftx, lefty);
        break;

    case Right_outRing:
        Right_outRing_processing();
        break;

    case Left_intoRing:
        Left_intoRing_processing(leftx, lefty);
        break;

    case Left_inRing:
        Left_inRing_processing();
        break;

    case Left_ReadyoutRing:
        Left_ReadyoutRing_processing(outleftx, lefty);
        break;

    case Left_outRing:
        Left_outRing_processing();
        break;

    default:
        break;
    }
}