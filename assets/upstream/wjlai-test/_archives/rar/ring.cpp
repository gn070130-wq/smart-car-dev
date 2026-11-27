#include "ring.h"
#include "imu.h"
#include "image.h"
#include "preprocess.h"

#define count_max 100 //初见圆环和圆环入口的时间
#define t1max 100
extern float ring_exit_distance;
extern int count,t1;
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
int RingCounts = 1;
int ringcount=0;
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




//-----------------------------------右圆环检测部分-----------------------------------------

int a=0;
//初见右圆环
bool check_Right_intoRing_weak(void)
{
    //有拐点识别成功
    bool right_flag = 0;
    
    //右突变点和右下拐点存在,并且y轴距离大于30，左拐点不存在
    if(right_points_turnDown[2] && right_up_mutation[2] 
    &&  right_points_turnDown[0] - right_up_mutation[0] > 30
    &&  !left_up_mutation[2])
    {
        right_flag = 1;
    }

    if(!right_flag) 
    {
        ringstate = No_Ring;
        return false;
    }
    if(left_Line.lossPoint>5)
    {
        ringstate = No_Ring;
        return false;
    }
    //右突变点下方丢线
    int tmpCount = 0;
    int j = 10;
    for(int i = right_up_mutation[3]; i > 0 && j > 0; i--, j--)
    {
        if(right_Line.edgelinepoint[i][1] > WIDTH-5)
        {
            tmpCount++;
        }
    }

    if(tmpCount < 1)
    {
        ringstate = No_Ring;
        return false;
    }
 
    return true;
}


//判断右圆环入口
bool check_Right_intoRing()
{
    
    if(count<count_max) 
    {
        return false;
    }
    if(right_points_turnUp[2] && right_up_mutation[2] 
    &&  abs(right_points_turnUp[0] - right_up_mutation[0]) < 6
    &&  abs(right_points_turnUp[1] - right_up_mutation[1]) < 6
    &&  right_points_turnUp[0] > 10 && right_points_turnUp[0] < 25
    )
    {


        //突变点下方丢线
        int tmpCount = 0;
        int j = 10;
        for(int i = right_up_mutation[3]; i > 0 && j > 0; i--, j--)
        {
            if(right_Line.edgelinepoint[i][1] > WIDTH-5)
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
    if(gyro_int.integration>40)
    {
        return true;
    }
    // if(gyro_z_integration >= INRING) {  // 左转为负值
    //     return true;
    // }
    return false;
}

//检测右即将出环
bool check_Right_ReadyoutRing(void)
{
    // //有拐点识别成功
    // bool right_flag = 0;
    
    // //右下拐点存在&&右下拐点上面和下面的点的x坐标大于拐点x的坐标
    // if(left_points_turnDown[2])
    // {
    //     right_flag = 1;
    // }

    // if(!right_flag) 
    // {
    //     return false;
    // }

    if(gyro_int.integration>190 )  // 左转角度绝对值比较
    {
         return true;
    }

    return false;
}


//检测右出环状态
bool check_Right_outRing(void)
{
    if(gyro_int.integration > 295) {
        return true;
    }
    return false;
}

//检测退出右圆环状态
bool check_Out_RightRing(void)
{
    if(t1 > t1max) 
    {
        return true;
        t1=0;
    }
    return false;
}

//右圆环状态检测
void RightRing_State_checking()
{
    if(roadstate==Normal_Road)
    {
        a=2;
        if(check_Right_intoRing_weak()) 
        {
            roadstate = Ring_Road;
            ringstate = Right_intoWeak;
        }
    }

    if(roadstate == Ring_Road && ringstate == Right_intoWeak)
    {
        if(check_Right_intoRing()) 
        {
            ringstate = Right_intoRing;
        }

    }

    //圆环内检测
    if(roadstate == Ring_Road && ringstate == Right_intoRing)
    {
        if(check_Right_inRing())
        {
            ringstate = Right_inRing;

        }

    }

    //圆环出口检测
    if(roadstate == Ring_Road && ringstate == Right_inRing)
    {
        if(check_Right_ReadyoutRing()) 
        {
            ringstate = Right_ReadyoutRing;
            
        }
    }

    //已经出圆环检测
    if(roadstate == Ring_Road && ringstate == Right_ReadyoutRing)
    {
        if(check_Right_outRing()) 
        {
            ringstate = Right_outRing;
        }
    }

    //退出圆环检测
    if(roadstate == Ring_Road && ringstate == Right_outRing)
    {
        if(check_Out_RightRing()) {
            roadstate = Normal_Road;
            ringstate = No_Ring;
            ringcount++;
        }
            
    }
}


//---------------------------------左圆环检测部分--------------------------------------


bool check_Left_intoRing_weak(void)
{
    //有拐点识别成功
    bool left_flag = 0;
    
    //左突变点和左下拐点存在,并且y轴距离大于30，右拐点不存在
    if(left_points_turnDown[2] && left_up_mutation[2] 
    &&  left_points_turnDown[0] - left_up_mutation[0] > 20
    &&  !right_up_mutation[2]&&!right_points_turnDown[2])
    {
        left_flag = 1;
    }
    if(!left_flag) 
    {
        ringstate = No_Ring;
        return false;
    }

    if(right_Line.lossPoint>5)
    {
        ringstate = No_Ring;
        return false;
    }
    //左突变点下方丢线
    int tmpCount = 0;
    int j =10;
    for(int i = left_up_mutation[3];i > 0 && j > 0;i--,j--)
    {
        if(left_Line.edgelinepoint[i][1] < 5)
        {
            tmpCount++;
        }
    }

    if(tmpCount < 1)
    {
        ringstate = No_Ring;
        return false;
    }

    return true;
}

//左圆环入口
bool check_Left_intoRing()
{
    if(count<count_max) 
    {
        return false;
    }
    if(left_points_turnUp[2] && left_up_mutation[2] 
    &&  abs(left_points_turnUp[0] - left_up_mutation[0]) < 6
    &&  abs(left_points_turnUp[1] - left_up_mutation[1]) < 6
    &&  left_points_turnUp[0] > 10 && left_points_turnUp[0] < 25
    )
    {


        //突变点下方丢线
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
    if(gyro_int.integration<-40)
    {
        return true;
    }
    return false;
}

// 左圆环出口(拉线)
bool check_Left_ReadyoutRing(void)
{
    if(gyro_int.integration<-190 )  // 左转角度绝对值比较
    {
        return true;
    }
    return false;
}


// 左环已经出状态(右-半宽)
bool check_Left_outRing(void)
{   
    if(gyro_int.integration<-295)
    {
        return true;
    }
    return false;
}

// 退出圆环状态（保持原距离判断逻辑）
bool check_Out_LeftRing(void)
{
    if(t1 > t1max) 
    {
        return true;
        t1=0;
    }
    return false;
}

//左圆环状态检测
void LeftRing_State_checking()
{
    if(roadstate==Normal_Road)
    {
        if(check_Left_intoRing_weak()) 
        {
            roadstate = Ring_Road;
            ringstate = Left_intoWeak;
        }

    }

    if(roadstate == Ring_Road && ringstate == Left_intoWeak)
    {
        if(check_Left_intoRing()) 
        {
            ringstate = Left_intoRing;
        }

    }

    //圆环内检测
    if(roadstate == Ring_Road && ringstate == Left_intoRing)
    {
        if(check_Left_inRing())
        {
            ringstate = Left_inRing;

        }

    }

    //左圆环圆环准备出口
    if(roadstate == Ring_Road && ringstate == Left_inRing)
    {
        if(check_Left_ReadyoutRing()) {
            ringstate = Left_ReadyoutRing;
            ring_exit_distance = 0.0f;
        }
    }

    //已经出圆环检测
    if(roadstate == Ring_Road && ringstate == Left_ReadyoutRing)
    {
        if(check_Left_outRing()) 
        {
            ringstate = Left_outRing;
        }
    }

    //退出圆环检测
    if(roadstate == Ring_Road && ringstate == Left_outRing)
    {
        if(check_Out_LeftRing()) {
            roadstate = Normal_Road;
            ringstate = No_Ring;
            ringcount++;
        }
            
    }
}
//---------------------------------左圆环检测结束--------------------------------------



//---------------------------------右圆环处理部分---------------------------------------

//左固定拉线
void creat_fixedLeftLine(int leftx,int lefty)
{
    int tar_lefty = lefty;
    int tar_leftx = 80 + leftx;
    int Lstary = leftbase[0];
    int Lstarx = WIDTH/ 2 - 60;
    
    float lk =calculate_slopeK(tar_lefty, Lstary, tar_leftx, Lstarx);
    float lb =calculate_slopeB(lk, tar_lefty, tar_leftx);

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
        right_Line.edgelinepoint[i][1] = WIDTH - 3;
    }

    left_plus_right_2midline();
}


//入环处理
void Right_intoRing_processing(int leftx, int lefty)
{

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

}

//出环单边
void Right_outRing_processing(void)
{
    leftLine_plus_halfWide();
}



//-----------------------左圆环处理开始----------------------------------

//右固定拉线
void creat_fixedRightLine(int rightx, int righty)
{
    int tar_righty = left_up_mutation[0];
    int tar_rightx = left_up_mutation[1];
    int Rstary = right_Line.edgelinepoint[0][1];
    int Rstarx = right_Line.edgelinepoint[0][0];

    float rk =calculate_slopeK(tar_righty, Rstary, tar_rightx, Rstarx);
    float rb =calculate_slopeB(rk, tar_righty, tar_rightx);

    for(int i = 0; i < left_up_mutation[3] ; i++)
    {
        right_Line.edgelinepoint[i][0] = Rstary -i;
        right_Line.edgelinepoint[i][1] = (Rstary - i - rb) / rk;

        if(right_Line.edgelinepoint[i][0] == tar_righty)
        {
            right_Line.pointsNum = i+1;
            break;
        }
    }
}


//右固定拉线
void creat_fixedRightLine2()
{
    int tar_righty = 0;
    int tar_rightx = 0;
    int Rstary = right_points_turnDown[0];
    int Rstarx = right_points_turnDown[1];

    float rk =calculate_slopeK(tar_righty, Rstary, tar_rightx, Rstarx);
    float rb =calculate_slopeB(rk, tar_righty, tar_rightx);
    int j=0;
    for(int i = right_points_turnDown[3]; i <right_Line.pointsNum && Rstary -j > 0  ; i++)
    {
        
        right_Line.edgelinepoint[i][0] = Rstary -j;
        right_Line.edgelinepoint[i][1] = (Rstary - j - rb) / rk;
        j++;
        if(right_Line.edgelinepoint[i][0] == tar_righty)
        {
            right_Line.pointsNum = i+1;
            break;
        }
    }
}

//入环处理
void Left_intoRing_processing(int rightx, int righty)
{
    //环外拉线
    creat_fixedRightLine(rightx, righty);
    left_plus_right_2midline();
}

//环内处理
void Left_inRing_processing()
{
    // //左右合成中线
    // left_plus_right_2midline();
    rightLine_plus_halfWide();
}

//准备出环处理
void Left_ReadyoutRing_processing()
{
    creat_fixedRightLine2();
    rightLine_plus_halfWide();
}

//出环单边
void Left_outRing_processing(void)
{
    rightLine_plus_halfWide();
}


//圆环处理
void ring_processing()
{
    switch (ringstate)
    {
        //右圆环
        case Right_intoWeak:
            leftLine_plus_halfWide();
            gyro_integration_clean();
            break;

        case Right_intoRing:
            //rightLine_plus_HOWhalfWide();
            rightLine_plus_halfWide();
            break;

        case Right_inRing:
        if(left_Line.lossPoint>right_Line.lossPoint)
        {
            rightLine_plus_halfWide();
        }
        else
        {
            leftLine_plus_halfWide();
        }
        break;

        case Right_ReadyoutRing:
            rightLine_plus_HOWhalfWide();
            //rightLine_plus_halfWide();
            break;

        case Right_outRing:
            leftLine_plus_halfWide();
            break;
        
        //左圆环
        case Left_intoWeak:
            rightLine_plus_halfWide();
            gyro_integration_clean();
            break;
        
        case Left_intoRing:
            leftLine_plus_halfWide();
            break;

        case Left_inRing:
        if(left_Line.lossPoint>right_Line.lossPoint)
        {
            rightLine_plus_halfWide();
        }
        else
        {
            leftLine_plus_halfWide();
        }
            break;

        case Left_ReadyoutRing:
            leftLine_plus_HOWhalfWide();
            break;

        case Left_outRing:
            rightLine_plus_halfWide();
            break;

        default:
            break;
    }
}