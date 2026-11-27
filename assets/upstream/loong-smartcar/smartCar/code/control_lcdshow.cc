#include "control_lcdshow.h"

Mat imageDraw;          //拷贝用于显示的图像1
Mat imageDraw1;         //拷贝用于显示的图像2

void showRoadState(void)
{
    tft180.showInt(120, 8*0, crose_test, 2);

    switch (roadstate)
    {
        case Normal_Road :
        {
            switch (normalstate)
            {
            case Normal_longway:
                {
                    tft180.showString(80, 8*0, "long  ");
                    break;
                }

            case Normal_curve:
                {
                    tft180.showString(80, 8*0, "curve ");
                    break;
                }
            case Normal_longway_weak:
                {
                    tft180.showString(80, 8*0, "long_w");
                    break;
                }
                
            }
        }
        break;

        case Cross_Road :
        {
            switch (cross_state)
            {
            case front_crossRoad_1:
                {
                    tft180.showString(80, 8*0, "crlong");
                }
                break;

            case front_crossRoad_2:
                {
                    tft180.showString(80, 8*0, "crlong");
                }
                break;

            case front_crossRoad_3:
                {
                    tft180.showString(80, 8*0, "crlong");
                }
                break;
            
            case in_crossRoad_1:
                {
                    tft180.showString(80, 8*0, "in_cr ");
                }
                break;
            }
        }
        break;

        case Ring_Road :
            switch (ringstate)
            {
            case Right_intoRing:
                {
                    tft180.showString(80, 8*0, "ring1R");
                }
                break;

            case Right_inRing:
                {
                    tft180.showString(80, 8*0, "ring2R");
                }
                break;
            
            case Right_ReadyoutRing:
                {
                    tft180.showString(80, 8*0, "ring3R");
                }
                break;
            
            case Right_outRing:
                {
                    tft180.showString(80, 8*0, "ring4R");
                }
                break;

            case Left_intoRing:
                {
                    tft180.showString(80, 8*0, "ring1L");
                }
                break;

            case Left_inRing:
                {
                    tft180.showString(80, 8*0, "ring2L");
                }
                break;
            
            case Left_ReadyoutRing:
                {
                    tft180.showString(80, 8*0, "ring3L");
                }
                break;
            
            case Left_outRing:
                {
                    tft180.showString(80, 8*0, "ring4L");
                }
                break;
            
            break;
        }

        case Ring_into_weak:
            switch (ringstate)
            {
            case Right_intoWeak:
                tft180.showString(80, 8*0, "RingWR");
                break;
            case Left_intoWeak:
                tft180.showString(80, 8*0, "RingWL");
                break;
            }
        break;

        case Barrier_Road:
            tft180.showString(80, 8*0, "Barrie");
        break;

        case Bridge_Road:
            tft180.showString(80, 8*0, "Bridge");
        break;
    }
    
}

void dynamic_show(Mat imgG, Mat img)
{
    imageDraw = img.clone();
    imageDraw1 = imgG.clone();
    
    cvtColor(imgG, imageDraw1, cv::COLOR_GRAY2BGR);

    if(!check_red_enable)
        drawEdgeLines(imageDraw);
    drawRoadLines(imageDraw1);

    tft180.showCVImage(0, 0, imageDraw, 80, 50);                            //显示原始图像+部分赛道特征
    tft180.showCVImage(0, 50, imageDraw1, 80, 50);

    //显示当前状态
    showRoadState();
    
    char temchar[20];
    sprintf(temchar, "er:%.2lf   ", ImgErr);
    tft180.showString(80, 8*1, temchar);
    //编码器积分
    tft180.showInt(80, 8*2, motor_distance, 7);
    //陀螺仪当前值
    tft180.showFloat(80, 8*3, imc20602.gyro_z, 3, 1);
    tft180.showFloat(80, 8*4, imc20602.gyro_y, 3, 1);
    //陀螺仪积分
    tft180.showFloat(80, 8*6, gyro_z_integration, 3, 1);
    tft180.showFloat(80, 8*7, gyro_y_integration, 2, 1);

    //圆环个数
    tft180.showInt(80, 8*9, RingCounts, 1);
    //圆环当前索引
    tft180.showInt(100, 8*9, nowRingIndex, 1);
    
    //负压当前速度
    tft180.showInt(80, 8*11, fans_time_duty, 3);

    menu_show_All();
}