#include "image.h"
#include "preprocess.h"
#include "cross.h" 
#include "ring.h" 
#include "imu.h" 



extern zf_device_uvc uvc_dev;

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
const uint16_t halfRoad[100] = {
    9, 10, 10, 10, 11, 11, 12, 12, 13, 13,
    14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
    18, 19, 20, 20, 21, 21, 22, 22, 23, 23,
    24, 24, 25, 25, 26, 27, 27, 28, 28, 29,
    29, 30, 30, 31, 31, 32, 33, 33, 34, 34,
    35, 35, 36, 37, 37, 37, 38, 38, 39, 40,
    40, 41, 41, 42, 42, 43, 43, 44, 45, 45,
    46, 46, 47, 47, 48, 48, 49, 49, 50, 50,
    51, 51, 52, 52, 53, 53, 54, 54, 55, 55,
    56, 56, 57, 57, 58, 58, 59, 59, 60, 60
};
//-----------------------------预处理-----------------------------------------
uint8_t original_image[UVC_HEIGHT][UVC_WIDTH] = {0};//灰度图像
uint8_t bin_image[HEIGHT][WIDTH] = {0};//二值化图像
uint8_t image_thereshold=0;


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

bool shieldZebra = true;
enum RoadState roadstate = Normal_Road;             /* 道路状态 */
Normal_Road_State normalstate = Normal_longway;     /* 常规道路状态*/



//---------------------------------------------------------------------------------------
//
//                                 图像预处理
//
//---------------------------------------------------------------------------------------



// 大津法阈值计算
uint8_t otsuThreshold(uint8_t *image, uint16 col, uint16 row) {
    #define GrayScale 256
    uint16 X, Y;
    uint8_t* data = image;
    int HistGram[GrayScale] = {0};
    uint32 Amount = 0;
    uint32 PixelBack = 0;       //像素背面
    uint32 PixelIntegralBack = 0;   
    uint32 PixelIntegral = 0;
    int32 PixelIntegralFore = 0;
    int32 PixelFore = 0;
    double OmegaBack, OmegaFore, MicroBack, MicroFore, SigmaB, Sigma;
    uint8_t MinValue = 0, MaxValue = 0;
    uint8_t Threshold = 0;

    // 统计灰度直方图
    for(Y = 0; Y < row; Y++) {
        for(X = 0; X < col; X++) {
            HistGram[(int)data[Y*col + X]]++;
        }
    }

    // 获取最小和最大灰度值
    for(MinValue = 0; MinValue < 256 && HistGram[MinValue] == 0; MinValue++);
    for(MaxValue = 255; MaxValue > MinValue && HistGram[MaxValue] == 0; MaxValue--);

    if(MaxValue == MinValue) return MaxValue;
    if(MinValue + 1 == MaxValue) return MinValue;

    // 计算像素总数和灰度总值
    for(Y = MinValue; Y <= MaxValue; Y++) {
        Amount += HistGram[Y];
        PixelIntegral += HistGram[Y] * Y;
    }

    SigmaB = -1;
    for(Y = MinValue; Y < MaxValue; Y++) {
        PixelBack += HistGram[Y];
        PixelFore = Amount - PixelBack;
        OmegaBack = (double)PixelBack / Amount;
        OmegaFore = (double)PixelFore / Amount;
        PixelIntegralBack += HistGram[Y] * Y;
        PixelIntegralFore = PixelIntegral - PixelIntegralBack;
        MicroBack = (double)PixelIntegralBack / PixelBack;
        MicroFore = (double)PixelIntegralFore / PixelFore;
        Sigma = OmegaBack * OmegaFore * (MicroBack - MicroFore) * (MicroBack - MicroFore);
        
        if(Sigma > SigmaB) {
            SigmaB = Sigma;
            Threshold = (uint8_t)Y;
        }
    }
    return Threshold;
}




// 绘制图像边框
void image_draw_rectan(uint8_t(*image)[WIDTH]) {
    uint8_t i = 0;
    
    for(i = 0; i < HEIGHT; i++) {
        image[i][0] = 0x0000;
        image[i][1] = 0x0000;
        image[i][WIDTH-1] = 0x0000;
        image[i][WIDTH-2] = 0x0000;
    }
    

}


//---------------------------------------------------------------------------------------
//
//                                 最长白列
//
//---------------------------------------------------------------------------------------
maxWhiteLine max_White_Line(uint8_t bin_image[HEIGHT][WIDTH])
{
    struct maxWhiteLine White;
    White.lineNum = 0;  //最长白列个数
    White.maxNum = 0;   //最长白列白点数
    White.endlineX = 0; 
    White.maxy = yend-1;//白列最到点y坐标
    
    for(uint16_t col = 0; col < WIDTH; col++)
    {
        if(bin_image[yend-1][col] > image_thereshold)
        {
            int16_t tmpRow = yend-2;
            uint16_t Whitetmp = 1;
            while(tmpRow >= imgcutup && bin_image[tmpRow][col] > image_thereshold) {
                Whitetmp++;
                tmpRow--;
            }
            if(Whitetmp > White.maxNum) {                        
                White.lineNum = 1;
                White.starlineX = col;
                White.endlineX = col;
                White.maxNum = Whitetmp;
                White.maxy = (tmpRow + 1);
            } else if (Whitetmp == White.maxNum) {
                White.lineNum++;
                White.endlineX = col;
            }
        }
    }
    //如果最长白列长度不大于裁切后的图像的长度
    if(White.maxNum != HEIGHT - imgcutup) {
        return White;
    }

    //最长白列跨度小于等于列数，说明白列不连续
    if(White.endlineX - White.starlineX + 1 <= White.lineNum) {
        return White;
    }
    
    //从start_x往右，计算该列从 White.maxy 到底部的连续白点长度conts,统计白列列数Lconts
    int Lconts = 0;
    int start_x = White.starlineX + 1;
    while(start_x < WIDTH - 1 && bin_image[White.maxy][start_x] > image_thereshold) {
        int y = White.maxy;
        int conts = 1;
        while(y < HEIGHT - 1 && bin_image[y][start_x] > image_thereshold) {
            y++;
            conts++;
        }
        if(conts >= White.maxNum)
            Lconts++;
        else
            break;
        start_x++;
    }

    // 右边扩展计数
    int Rconts = 0;
    int end_x = White.endlineX - 1;
    while(end_x > 0 && bin_image[White.maxy][end_x] > image_thereshold) {
        int y = White.maxy;
        int conts = 1;
        while(y < HEIGHT - 1 && bin_image[y][end_x] > image_thereshold) {
            y++;
            conts++;
        }
        if(conts >= White.maxNum)
            Rconts++;
        else
            break;
        end_x--;
    }
    
    //左右扩展出的连续列都很少（≤10），但最初的白列又较长
    if(Lconts <= 10 && Rconts <= 10) {
        //向下5格
        int offset = 5;
        int tmp_Lconts = 0;
        int tmp_start_x = White.starlineX + 1;
        while(tmp_start_x < WIDTH - 1 && bin_image[White.maxy + offset][tmp_start_x] > image_thereshold) {
            int y = White.maxy + offset;
            int conts = 1;
            while(y < HEIGHT - 1 && bin_image[y][tmp_start_x] > image_thereshold) {
                y++;
                conts++;
            }
            if(conts >= White.maxNum - offset)
                tmp_Lconts++;
            else
                break;
            tmp_start_x++;
        }

        int tmp_Rconts = 0;
        int tmp_end_x = White.endlineX - 1;
        while(tmp_end_x > 0 && bin_image[White.maxy + offset][tmp_end_x] > image_thereshold) {
            int y = White.maxy + offset;
            int conts = 1;
            while(y < HEIGHT - 1 && bin_image[y][tmp_end_x] > image_thereshold) {
                y++;
                conts++;
            }
            if(conts >= White.maxNum - offset)
                tmp_Rconts++;
            else
                break;
            tmp_end_x--;
        }

        if(tmp_Lconts >= tmp_Rconts) {
            White.endlineX = White.starlineX + tmp_Lconts;
            White.lineNum = tmp_Lconts;
            White.maxy += offset;
            White.maxNum -= offset;
        } else {
            White.starlineX = White.endlineX - tmp_Rconts;
            White.lineNum = tmp_Rconts;
            White.maxy += offset;
            White.maxNum -= offset;
        }
    }
    else if(Lconts >= Rconts) {
        White.endlineX = White.starlineX + Lconts;
        White.lineNum = Lconts;
    } 
    else {
        White.starlineX = White.endlineX - Rconts;
        White.lineNum = Rconts;
    }

    return White;
}



//左基点查找
bool l_basepoint(uint8_t bin_image[HEIGHT][WIDTH], int16_t find_x)
{
    bool foundpoint = false;
    int16_t y = yend - 4;       //减2是因为索引是ystar~yend-1， 再减1防止八邻域越界
    int16_t x = find_x;
    for(int16_t i = x; i > 3 ; i--)
    {   
        if(bin_image[y][i] > image_thereshold
        && bin_image[y][i-1] <= image_thereshold
        && bin_image[y][i-2] <= image_thereshold
        && bin_image[y][i-3] <= image_thereshold)
        {
            leftbase[1] = i;
            leftbase[0] = y;
            foundpoint = true;
        }
    }

    if(!foundpoint) {
        for(int16_t i = 0; i < 5; i++)
        {
            if(bin_image[y][i] <= image_thereshold
            && bin_image[y][i+1] > image_thereshold)
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
bool r_basepoint(uint8_t bin_image[HEIGHT][WIDTH], int16_t find_x)
{
    bool foundpoint = false;
    int16_t y = yend - 4;
    int16_t x = find_x;
    for(int16_t i = x; i < WIDTH - 3; i++)
    {
        if(bin_image[y][i] > image_thereshold
        && bin_image[y][i+1] <= image_thereshold
        && bin_image[y][i+2] <= image_thereshold
        && bin_image[y][i+3]<= image_thereshold)
        {
            rightbase[1] = i;
            rightbase[0] = y;
            foundpoint = true;
        }
    }

    if(!foundpoint) {
        for(int16_t i = WIDTH-1; i > WIDTH-6; i--)
        {
            if(bin_image[y][i] <= image_thereshold
            && bin_image[y][i-1] > image_thereshold)
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


int c=0;
void reMaxWhite(uint8_t bin_image[HEIGHT][WIDTH])
{
    if(roadstate == Ring_Road && (ringstate == Right_intoRing||ringstate == Right_inRing)&&gyro_int.integration<100&&gyro_int.integration>17)
    {
        c=2;
        struct maxWhiteLine White;
        White.lineNum = 0;
        White.maxNum = 0;
        White.endlineX = 0;
        White.maxy = yend-1;
        for(uint16_t col = WIDTH - 1; col > 80; col--)
        {
            if(bin_image[yend-1][col] > image_thereshold)
            {
                uint16_t tmpRow = yend-2;
                uint16_t Whitetmp = 1;
                while(bin_image[tmpRow][col] > image_thereshold && tmpRow >= imgcutup) {
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
        c=1;
    }

    if(roadstate == Ring_Road && (ringstate == Left_intoRing||ringstate == Left_inRing)&&gyro_int.integration>-100&&gyro_int.integration<-10)
    {
        struct maxWhiteLine White;
        White.lineNum = 0;
        White.maxNum = 0;
        White.endlineX = 0;
        White.maxy = yend-1;
        for(uint16_t col = 0; col <60; col++)
        {
            if(bin_image[yend-1][col] > image_thereshold)
            {
                uint16_t tmpRow = yend-2;
                uint16_t Whitetmp = 1;
                while(bin_image[tmpRow][col] > image_thereshold && tmpRow >= imgcutup) {
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

//通过最长白列扫左边线
void use_Whiteline_find_leftline(uint8_t bin_image[HEIGHT][WIDTH])
{

    left_Line.pointsNum = 0;
    left_Line.lossPoint = 0;
    for(int y = yend - 4; y > maxWhite.maxy; y--)
    {
        for(int x = maxWhite.starlineX; x > 1; x--)
        {
            if(bin_image[y][x] > image_thereshold      
            && bin_image[y][x-1] <= image_thereshold)   //从右到左 白-黑
            {
                left_Line.edgelinepoint[left_Line.pointsNum][0] = y;
                left_Line.edgelinepoint[left_Line.pointsNum][1] = x;
                left_Line.pointsNum++;
                left_Line.lossPoint = (x == 2) ? left_Line.lossPoint+1 : left_Line.lossPoint;
                break;
            }
            if(bin_image[y][x] <= image_thereshold
            && bin_image[y][x-1] <= image_thereshold)     //黑-黑     
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
void use_Whiteline_find_rightline(uint8_t bin_image[HEIGHT][WIDTH])
{

    right_Line.pointsNum = 0;
    right_Line.lossPoint = 0;
    for(int y = yend - 4; y > maxWhite.maxy; y--)
    {
        for(int x = maxWhite.endlineX; x < WIDTH-2; x++)
        {
            if(bin_image[y][x] > image_thereshold
            && bin_image[y][x+1] <= image_thereshold )  //白-黑
            {
                right_Line.edgelinepoint[right_Line.pointsNum][0] = y;
                right_Line.edgelinepoint[right_Line.pointsNum][1] = x;
                right_Line.pointsNum++;
                right_Line.lossPoint = (x == 157) ? right_Line.lossPoint+1 : right_Line.lossPoint;
                break;
            }
            if(bin_image[y][x] <= image_thereshold
            && bin_image[y][x+1]<= image_thereshold)  //黑-黑
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

//---------------------------------------------------------------------------------------
//
//                                 迷宫法
//
//---------------------------------------------------------------------------------------
//上交迷宫法左寻线
void findline_lefthand_adaptive(uint8_t Bin_Image[HEIGHT][WIDTH])
{
    left_edgeline.state = 0;  //状态
    left_edgeline.lossPoint = 0;  //丢线点记数
    int x = leftbase[1];  //左边界起始基点坐标
    int y = leftbase[0];
    int step = 0, dir = 0, turn = 0;  //已走过步数，方向，转弯次数
    int half = 2;  //边界余量
    while (step < MAX_POINT && turn < 4)   //步数未超过最大限制(250),且转弯次数小于四次
    {
        // if(half >= x) {
        //     left_edgeline.state = 'A';
        //     break;
        if (x >= WIDTH - half - 1) {   //基点太靠右退出
            left_edgeline.state = 'D';
            break;
        } else if (half >= y) {   //基点太靠上
            left_edgeline.state = 'W';
            break;
        } else if (y >= HEIGHT - half - 1) {  //基点太靠近图像底部
            left_edgeline.state = 'S';
            break;
        }

        // int local_thres = Otsu_Threshold;

        // int current_value = bin_image[y][x];  //当前位置灰度值
        int front_value = bin_image[y + dir_front[dir][1]] [x + dir_front[dir][0]];  //正前方像素
        int frontleft_value = bin_image[y + dir_frontleft[dir][1]][ x + dir_frontleft[dir][0]];  //左前放像素
        if (front_value <= image_thereshold) {  //正前方为黑，遇墙右转
            dir = (dir + 1) % 4;
            turn++;
        } else if (frontleft_value <= image_thereshold) {  //正前方为白，左前方为黑，前方有路
            x += dir_front[dir][0];
            y += dir_front[dir][1];
            left_edgeline.edgelinepoint[step][2] = dir;
            left_edgeline.edgelinepoint[step][1] = x;
            left_edgeline.edgelinepoint[step][0] = y;
            step++;
            turn = 0;
        } else {  //正前方为白，左前方为白，墙拐弯了，左转，并跳到左前方
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
    left_edgeline.pointsNum = step;  //步数
}

//上交迷宫法右寻线
void findline_righthand_adaptive(uint8_t Bin_Image[HEIGHT][WIDTH])
{
    right_edgeline.state = 0;
    right_edgeline.lossPoint = 0;
    int x = rightbase[1];
    int y = rightbase[0];
    int step = 0, dir = 0, turn = 0;
    int half = 2;
    while (step < MAX_POINT && turn < 4) 
    {
        // if (x >= WIDTH - half - 1) {
        //     right_edgeline.state = 'D';
        //     break;
        if(half >= x) {
            right_edgeline.state = 'A';
            break;
        } else if (half >= y) {
            right_edgeline.state = 'W';
            break;
        } else if (y >= HEIGHT - half - 1) {
            right_edgeline.state = 'S';
            break;
        }

        // int local_thres = Otsu_Threshold;

        // int current_value = bin_image[y][x];
        int front_value = bin_image[ y + dir_front[dir][1] ][ x + dir_front[dir][0] ];
        int frontright_value = bin_image[(y + dir_frontright[dir][1])][(x + dir_frontright[dir][0])];
        if (front_value <= image_thereshold) {
            dir = (dir + 3) % 4;
            turn++;
        } else if (frontright_value <= image_thereshold) {
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

                left_points_turnDown[0] = left_edgeline.edgelinepoint[i][0];//y
                left_points_turnDown[1] = left_edgeline.edgelinepoint[i][1];//x
                left_points_turnDown[2] = 1;//找到
                left_points_turnDown[3] = i;//索引
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
    for(int i = left_Line.pointsNum - 5; i >= 10; i --)
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
    for(int i = right_Line.pointsNum - 5; i >= 10; i --)
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
//直道检测
bool checking_Normal_straightaway(void)
{
    if( left_edgeline.lossPoint == 0 && left_edgeline.pointsNum > 80
    &&  right_edgeline.lossPoint == 0 && right_edgeline.pointsNum > 80
    &&  abs(left_edgeline.edgelinepoint[left_edgeline.pointsNum - 1][1] - maxWhite.starlineX) < 5
    &&  abs(right_edgeline.edgelinepoint[right_edgeline.pointsNum - 1][1] - maxWhite.endlineX) < 5
    &&  maxWhite.maxNum == HEIGHT-imgcutup )
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
    if(maxWhite.maxNum < HEIGHT-imgcutup) {
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
    // struct Edgeline other = left_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] - halfRoad[edge.edgelinepoint[i][0]];
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//右边线+赛道how半宽
void rightLine_plus_HOWhalfWide()
{
    struct Edgeline edge = right_Line;
    // struct Edgeline other = left_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] - halfRoad[edge.edgelinepoint[i][0]]*3/5;
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//左边线+赛道半半宽
void leftLine_plus_HOWhalfWide()
{
    struct Edgeline edge = left_Line;
    // struct Edgeline other = right_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] + halfRoad[edge.edgelinepoint[i][0]]*3/5;
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

//左边线+赛道半宽
void leftLine_plus_halfWide(void)
{
    struct Edgeline edge = left_Line;
    // struct Edgeline other = right_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1] + halfRoad[edge.edgelinepoint[i][0]];
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

void leftLine(void)
{
    struct Edgeline edge = left_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1]+5;
        if(midline.edgelinepoint[i][1]<0)
        {
            midline.edgelinepoint[i][1]=0;
        }

        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
    }
}

void rightLine(void)
{
    struct Edgeline edge = right_Line;
    midline.pointsNum = 0;
    for(int i = 0; i < edge.pointsNum; i++)
    {
        midline.edgelinepoint[i][1] = edge.edgelinepoint[i][1]-5 ;
        if(midline.edgelinepoint[i][1]>140)
        {
            midline.edgelinepoint[i][1]=140;
        }
        midline.edgelinepoint[i][0] = edge.edgelinepoint[i][0];
        midline.pointsNum++;
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





/* 斑马线检测 */
extern int flag;
int t=0;
bool zebraCrossing_checking(int line)
{
    //发车区屏蔽斑马线检测，时间
    if(shieldZebra == true) 
    {
        t++;   
        if(t==200)
        {
            shieldZebra = false;
        }
        return false;
    }

    //从左到右扫
    bool zebra = false;
    int16_t y = HEIGHT-1 - line;
    int zebraCounts = 0;
    for(int i = 0; i < WIDTH-1; i++) 
    {
        if(bin_image[y][i] < image_thereshold && bin_image[y][i+1] >= image_thereshold)
        {
            zebraCounts++;
        }
    }
    if(zebraCounts > 6)
    {
        zebra = true;
    }

    //从右到左扫
    if(!zebra)
    {
        zebraCounts = 0;
        for(int i = WIDTH-1; i > 0; i--) 
        {
            if(bin_image[y][i] < image_thereshold && bin_image[y][i-1] >= image_thereshold)
            {
                zebraCounts++;
            }
        }
        
        if(zebraCounts > 6)
        {
            zebra = true;
        }

    }
    return zebra;
}


int roadMid = 0;

#define WEIGHTARR           10              /* 跟踪权重点集数 */
const   int16_t     weight[WEIGHTARR] = {3,3,5,5,5,6,6,7,9,11};     /* 跟踪位置权重 */
//const   int16_t     weight[WEIGHTARR] = {11,9,7,6,6,5,5,5,3,3};     /* 跟踪位置权重 */
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
    return err;
}


uint16_t color_image[HEIGHT][WIDTH] = {0};


// 主图像处理函数
void image_process(uint8_t* gray_ptr) 
{
        memcpy(original_image[0], gray_ptr, UVC_WIDTH * UVC_HEIGHT);
        crop_and_resize_bilinear(&original_image[0][0], &bin_image[0][0]);
        
        image_thereshold = otsuThreshold(bin_image[0], WIDTH, HEIGHT);
        
        image_draw_rectan(bin_image);// 绘制边框   
        
         /* 获取最长白列信息 */
        maxWhite = max_White_Line(bin_image);
        
        /* 找左基点 */
        l_basepoint(bin_image, maxWhite.starlineX);

        /* 找右基点 */
        r_basepoint(bin_image, maxWhite.endlineX);

        /* 局部二值化迷宫法寻左边线（上交源码） */
        findline_lefthand_adaptive(bin_image);

        /* 局部二值化迷宫法寻右边线（上交源码） */
        findline_righthand_adaptive(bin_image);
        
        /* 最长白列串道 */
        reMaxWhite(bin_image);

        /* 最长白列获取左边线 */
        use_Whiteline_find_leftline(bin_image);

        /* 最长白列获取右边线 */
        use_Whiteline_find_rightline(bin_image);

        //处理生长算法
        dispose_Edgeline();
        //处理扫线算法
        dispose_line();
}

void generate_color_image_for_upload() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t bin_val = bin_image[y][x];
            // 直接映射为全白或全黑的 RGB565 颜色
            if (bin_val > image_thereshold) {
                color_image[y][x] = 0xFFFF;
            } else {
                color_image[y][x] = 0x0000;
            }
        }
    }
}

void drawPixelOnColor(uint16_t image[HEIGHT][WIDTH], int x, int y, uint16_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        image[y][x] = color;
    }
}

void drawFilledCircleOnColor(uint16_t image[HEIGHT][WIDTH], int x0, int y0, int radius, uint16_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y == radius*radius) {
                drawPixelOnColor(image, x0 + x, y0 + y, color);
            }
        }
    }
}

void drawLineOnColor(uint16_t image[HEIGHT][WIDTH], int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        drawPixelOnColor(image, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}



// ---------- 字模（与原代码完全相同）----------
static const uint8_t Image_Flags[10][9][8] = {
    { // 0
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 1
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 2
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 3
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0xFF,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 4
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00},
    },
    { // 5
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 6
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 7
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 8
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    },
    { // 9
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0xFF,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
        {0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00},
        {0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    }
};

/**
 * @brief 在彩色图像指定位置绘制单个数字（0-9）
 * @param img         彩色图像数组（RGB565）
 * @param x0, y0      左上角起始坐标（像素）
 * @param digit       要显示的数字（0~9，超过则显示空白）
 * @param fg_color    前景色（数字笔画颜色），例如 0xFFFF（白）
 * @param bg_color    背景色，若为 0x0000 则保持原图像素不变（透明背景）
 */
void draw_digit_on_color(uint16_t img[HEIGHT][WIDTH], int x0, int y0,
                         uint8_t digit, uint16_t fg_color, uint16_t bg_color) {
    if (digit > 9) return;  

    for (int row = 0; row < 9; ++row) {
        int y = y0 + row;
        if (y < 0 || y >= HEIGHT) continue;
        for (int col = 0; col < 8; ++col) {
            int x = x0 + col;
            if (x < 0 || x >= WIDTH) continue;
            if (Image_Flags[digit][row][col] == 0xFF) {
                img[y][x] = fg_color;         // 画前景
            } else {
                if (bg_color != 0x0000) {     // 0x0000 表示透明，不覆盖
                    img[y][x] = bg_color;
                }
            }
        }
    }
}

/**
 * @brief 在 color_image 左上角显示两位十进制状态值
 * @param state  要显示的数字（0~99，超出则显示 --）
 * @param fg     前景色，例如 0x07E0（绿色）
 * @param bg     背景色，例如 0x0000（黑色/透明）
 *
 * 调用示例：
 *   display_status_top_left(roadstate, 0xFFFF, 0x0000);  // 白字透明背景
 */
void display_status_top_left(uint8_t state, uint16_t fg, uint16_t bg) 
{
    int x = 1, y = 41;   // 左上角留 1 像素边距
    uint8_t tens = state / 10;
    uint8_t ones = state % 10;

    draw_digit_on_color(color_image, x, y, tens, fg, bg);
    draw_digit_on_color(color_image, x + 9, y, ones, fg, bg);  // 每个数字宽 8 像素，间隔 1 像素
}