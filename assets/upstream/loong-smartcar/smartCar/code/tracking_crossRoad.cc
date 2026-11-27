#include "tracking_crossRoad.h"

CrossRoad_State cross_state = no_crossRoad;

int crose_test = 0;                 //状态反馈测试

//正入十字检测
static int check_Zheng_CrossRoad(void)
{
    if( maxWhite.maxNum >= ROWSIMAGE - preprocess.rowCutUp - preprocess.rowCutBottom
    &&  maxWhite.lineNum > 10)
    {
        if( left_points_turnDown[2] && right_points_turnDown[2]
        &&  abs(left_points_turnDown[0] - right_points_turnDown[0]) < 30
        &&  left_points_turnDown[0] > 30 && right_points_turnDown[0] > 30)
        {
            return 1;
        }

        if( (left_edgeline.firstLossY > 80 && right_points_turnDown[2] 
        &&  right_up_mutation[2] && right_edgeline.firstLossY < right_points_turnDown[0]) 
        &&  left_edgeline.lossPoint != 0 
        &&  (!left_edgeline.is_weak_alone || (left_edgeline.is_weak_alone && left_edgeline.lossPoint > 30))
        )
        {
            return 2;
        }

        if( (right_edgeline.firstLossY > 80 && left_points_turnDown[2] 
        &&  left_up_mutation[2] && left_edgeline.firstLossY < left_points_turnDown[0])
        &&  right_edgeline.lossPoint != 0 
        &&  (!right_edgeline.is_weak_alone || (right_edgeline.is_weak_alone && right_edgeline.lossPoint > 30))
        )
        {
            return 3;
        }
    }
    

    return 0;
}

//十字中央
static int check_In_CrossRoad(void)
{
    if( left_edgeline.firstLossY > 80 && right_edgeline.firstLossY > 80
    &&  left_up_mutation[2] && right_up_mutation[2])
    {
        return 1;
    }

    return 0;
}

//十字元素判断
CrossRoad_State isCrossRoad(cv::Mat imgG)
/* 返回0表示非十字 返回1表示正十字 返回2表示斜十字 */
{
    int check = 0;

    check = check_In_CrossRoad();
    if(check > 0) {
        crose_test = check;
        switch (check)
        {
        case 1:
            return in_crossRoad_1;
            break;
        
        default:
            break;
        }
    }

    //检查正入十字
    check = check_Zheng_CrossRoad();
    if(check > 0) {
        crose_test = check;
        switch (check)
        {
        case 1:
            return front_crossRoad_1;
            break;

        case 2:
            return front_crossRoad_2;
            break;

        case 3:
            return front_crossRoad_3;
            break;
        
        default:
            break;
        }
        
    }

    crose_test = 0;
    return no_crossRoad;
    
}

//十字正中间补线
void front_in_crossRoad_Line(void);
//斜入十字左连线
void front_in_Lup_LDown_Link(void);
//斜入十字右连线
void front_in_Rup_RDown_Link(void);
//十字中央拉线
void in_crossRoad_pullLine(void);

//十字所有状态处理
void crossRoad_processing(void)
{
    switch (cross_state)
    {
    case front_crossRoad_1:
        front_in_crossRoad_Line();
        left_plus_right_2midline();
        break;
    
    case front_crossRoad_2:
        front_in_Rup_RDown_Link();
        rightLine_plus_halfWide();
        break;
    
    case front_crossRoad_3:
        front_in_Lup_LDown_Link();
        leftLine_plus_halfWide();
        break;

    case in_crossRoad_1:
        in_crossRoad_pullLine();
        left_plus_right_2midline();
        break;

    default:
        break;
    }
}

//正入十字
void front_in_crossRoad_Line(void)
{
    int L_target_y = 0;
    int L_target_x = maxWhite.starlineX;
    int L_base_y = left_points_turnDown[0];
    int L_base_x = left_points_turnDown[1];
    float lk = preprocess.calculate_slopeK(L_target_y, L_base_y, L_target_x, L_base_x);
    float lb = preprocess.calculate_slopeB(lk, L_base_y, L_base_x);
    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = 99 - i;
        left_Line.edgelinepoint[i][1] = ((99 - i) - lb) / lk;
    }

    int R_target_y = 0;
    int R_target_x = maxWhite.endlineX;
    int R_base_y = right_points_turnDown[0];
    int R_base_x = right_points_turnDown[1];
    float rk = preprocess.calculate_slopeK(R_target_y, R_base_y, R_target_x, R_base_x);
    float rb = preprocess.calculate_slopeB(rk, R_base_y, R_base_x);
    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = 99 - i;
        right_Line.edgelinepoint[i][1] = ((99 - i) - rb) / rk;
    }
}

//十字中央拉线
void in_crossRoad_pullLine(void)
{
    int L_target_y = left_Line.edgelinepoint[left_up_mutation[3]+10][0];
    int L_target_x = left_Line.edgelinepoint[left_up_mutation[3]+10][1];
    int L_base_y = left_up_mutation[0];
    int L_base_x = left_up_mutation[1];
    float lk = preprocess.calculate_slopeK(L_target_y, L_base_y, L_target_x, L_base_x);
    float lb = preprocess.calculate_slopeB(lk, L_base_y, L_base_x);
    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = 99 - i;
        left_Line.edgelinepoint[i][1] = ((99 - i) - lb) / lk;
    }

    int R_target_y = right_Line.edgelinepoint[right_up_mutation[3]+10][0];
    int R_target_x = right_Line.edgelinepoint[right_up_mutation[3]+10][1];
    int R_base_y = right_up_mutation[0];
    int R_base_x = right_up_mutation[1];
    float rk = preprocess.calculate_slopeK(R_target_y, R_base_y, R_target_x, R_base_x);
    float rb = preprocess.calculate_slopeB(rk, R_base_y, R_base_x);
    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = 99 - i;
        right_Line.edgelinepoint[i][1] = ((99 - i) - rb) / rk;
    }
}

//斜入十字左连线
void front_in_Lup_LDown_Link(void)
{
    int L_target_y = left_up_mutation[0];
    int L_target_x = left_up_mutation[1];
    int L_base_y = left_points_turnDown[0];
    int L_base_x = left_points_turnDown[1];
    float lk = preprocess.calculate_slopeK(L_target_y, L_base_y, L_target_x, L_base_x);
    float lb = preprocess.calculate_slopeB(lk, L_base_y, L_base_x);
    for(int i = 0; i < left_Line.pointsNum; i++)
    {
        left_Line.edgelinepoint[i][0] = 99 - i;
        left_Line.edgelinepoint[i][1] = ((99 - i) - lb) / lk;
    }
}

//斜入十字右连线
void front_in_Rup_RDown_Link(void)
{
    int R_target_y = right_up_mutation[0];
    int R_target_x = right_up_mutation[1];
    int R_base_y = right_points_turnDown[0];
    int R_base_x = right_points_turnDown[1];
    float rk = preprocess.calculate_slopeK(R_target_y, R_base_y, R_target_x, R_base_x);
    float rb = preprocess.calculate_slopeB(rk, R_base_y, R_base_x);
    for(int i = 0; i < right_Line.pointsNum; i++)
    {
        right_Line.edgelinepoint[i][0] = 99 - i;
        right_Line.edgelinepoint[i][1] = ((99 - i) - rb) / rk;
    }
}