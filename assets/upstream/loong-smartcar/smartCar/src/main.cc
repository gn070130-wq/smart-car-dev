#include "headfile.h"

TFT180 tft180;

int main()
{
    cv::VideoCapture capture;
    capture = cv::VideoCapture("/dev/video0");
    if (!capture.isOpened()) {
        std::cerr << "Error: 无法打开摄像头" << std::endl;
        return -1;
    }
    capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    capture.set(cv::CAP_PROP_FRAME_WIDTH, COLSIMAGE);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, ROWSIMAGE);
    capture.set(cv::CAP_PROP_FPS, 120);
    std::cout << "摄像头尺寸: " << capture.get(cv::CAP_PROP_FRAME_WIDTH) 
        << "x" << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    std::cout << "摄像头帧数: " << capture.get(cv::CAP_PROP_FPS) << " FPS" 
        << std::endl;

    /* 参数读取初始化 */
    if(preprocess.Preprocess_init() < 0) {
        return -1;
    }

    /* 按键功能初始化 */
    if(Key_init() < 0) {
        return -1;
    }
    /* 电机初始化 */
    if(motor.Motor_init() < 0) {
        return -1;
    }
    /* 舵机初始化 */
    if(servo.Servo_init(SERVO_HZ, SERVO_MID) < 0) {
        return -1;
    }
    /* 陀螺仪初始化 */
    if(imc20602.ICM20602_init() < 0) {
        return -1;
    }
    /* 屏幕初始化 */
    if(tft180.TFT180_init() < 0) {
        return -1;
    }
    /* 负压初始化 */
    if(fans.Motor_fans_init() < 0) {
        return -1;
    }
    /* 菜单初始化 */
    menu_init();

    /* 圆环初始化 */
    Ring_init();
    /* 跟踪初始化 */
    tracking_base_init(preprocess.rowCutUp, preprocess.rowCutBottom);

    Positional_PID_Init(&servo_pid, 0, 0, 0, 0, SERVO_MAXERROR);
    motor.set_Lmotor_pid(preprocess.L_motor_kp, preprocess.L_motor_ki, 0);
    motor.set_Rmotor_pid(preprocess.R_motor_kp, preprocess.R_motor_ki, 0);
    motor.setSpeed(0,0);
    
    Mat img;
    Mat imageGray;
    Mat hsv_red;
    Mat red_mask;
    Mat hsv_yellow;
    Mat yellow_mask;

    //以上为所有外设初始化

    //以下为图像处理
    controlThread_Star();   //控制线程启动
    keythread_Star();       //按键读取线程

    while(1)
    {
        capture >> img;
        if(img.empty()) {
            cerr << "Error: 捕获到空帧" << std::endl;
            return 0;
        }
        //图像预处理
        #define IMGSTART    27
        #define IMGLEN      48
        imageGray = preprocess.Cut_and_imageGrayING(img, IMGSTART, IMGLEN);

        //出界检测
        if(outCrossing_checking(imageGray))
        {
            motor_enable_off();
            fans_enable = false;
        }

        //斑马线检测
        if(zebraCrossing_checking(imageGray, 0))
        {
            motor_enable_off();
            fans_enable = false;
        }

        //基础边线提取
        get_TrackingBase(imageGray);
        
        //十字判断，判断上一帧是否为常规路段或十字路段
        if(roadstate == Normal_Road || roadstate == Cross_Road)
        {
            do
            {
                //判断十字状态
                cross_state = isCrossRoad(imageGray);
                if(cross_state == 0) {
                    roadstate = Normal_Road;
                    cross_state = no_crossRoad;
                    //非十字退出不进行后续操作
                    break;
                } else {
                    //进入十字状态并进行处理
                    roadstate = Cross_Road;
                }
                
                //十字处理
                crossRoad_processing();

            } while (0);
            
        }

        //坡道检测（决赛用）
        if(preprocess.Bridge_Enable)
        {
            cv::cvtColor(img, hsv_red, cv::COLOR_BGR2HSV);
            // 定义红色的 HSV 范围（低红色和高红色）
            cv::Scalar lower_red2(preprocess.h_l, preprocess.s_l, preprocess.v_l);   // 红色(0, 235, 120)
            cv::Scalar upper_red2(preprocess.h_h, preprocess.s_h, preprocess.v_h);   // 红色(20, 255, 255)
            cv::inRange(hsv_red, lower_red2, upper_red2, red_mask);

            //坡道路段
            if( (roadstate == Normal_Road && (normalstate == Normal_longway || normalstate == Normal_longway_weak))
            || roadstate == Bridge_Road)
            {
                roadstate = check_Bridge(red_mask) ? Bridge_Road : Normal_Road;

                if(roadstate == Bridge_Road)
                {
                    Bridge_Road_preprocess();
                }
            }
        }

        //障碍路段
        if( roadstate == Normal_Road || roadstate == Barrier_Road )
        {
            cv::cvtColor(img, hsv_yellow, cv::COLOR_BGR2HSV);
            // 定义红色的 HSV 范围（低红色和高红色）
            cv::Scalar lower_yellow2(25, 100, 0);   
            cv::Scalar upper_yellow2(40, 255, 255); 
            cv::inRange(hsv_yellow, lower_yellow2, upper_yellow2, yellow_mask);

            roadstate = check_Barrier_Road(yellow_mask) ? Barrier_Road : Normal_Road;

            if(roadstate == Barrier_Road)
            {
                Barrier_Road_preprocess();
            }
        }

        //圆环判断，判断上一帧是否为常规路段或圆环路段
        if( (roadstate == Normal_Road || roadstate == Ring_Road || roadstate == Ring_into_weak)
        &&  RingCounts > 0) //圆环数量大于0
        {
            //弱圆环检测
            if(preprocess.weakRing)
            {
                if( (roadstate == Normal_Road && normalstate == Normal_curve)
                ||  (roadstate == Ring_into_weak && ringstate == Right_intoWeak)) {
                    roadstate = (check_Right_intoRing_weak()) ? Ring_into_weak : Normal_Road;
                }

                if( (roadstate == Normal_Road && normalstate == Normal_curve)
                ||  (roadstate == Ring_into_weak && ringstate == Left_intoWeak)) {
                    roadstate = (check_Left_intoRing_weak()) ? Ring_into_weak : Normal_Road;
                }
            }

            //右圆环状态检测
            RightRing_State_checking(imageGray);

            //左圆环状态检测
            LeftRing_State_checking(imageGray);
            

            //圆环处理
            switch (roadstate)
            {
            case Ring_into_weak:
                switch (ringstate)
                {
                case Right_intoWeak:
                    leftLine_plus_halfWide();
                    break;
                case Left_intoWeak:
                    rightLine_plus_halfWide();
                    break;
                }
                break;

            case Ring_Road:
                ring_processing(imageGray, preprocess.Ring_Y, *inRing_X_Arr[nowRingIndex], *outRing_X_Arr[nowRingIndex]);
                break;
            
            default:
                break;
            }
                
        }

        //常规路段
        if(roadstate == Normal_Road)
        {
            if(checking_Normal_straightaway()) {
                normalstate = Normal_longway;
            } else {
                normalstate = Normal_curve;
            }
            normalRoad_decusion();
        }
        
        //跟踪距离决策
        switch (roadstate)
        {
        case Bridge_Road:
            trackAim = 20;
            break;

        case Ring_Road:
            trackAim = *ringTrackAim[nowRingIndex];
            break;
        
        default:
            trackAim = preprocess.trackAim;
            break;
        }
        
        //提取误差
        float Imgerrtmp;
        Imgerrtmp = get_roadError(trackAim);
        
        //赋值
        control_mutex.lock();
        ImgErr = Imgerrtmp;
        control_mutex.unlock();

        Mat image;
        if(check_red_enable)
            image = red_mask.clone();
        else
            image = img.clone();

        //图像显示
        if(motor_enable && !preprocess.showImg) ;
        else
            dynamic_show(imageGray, image);

        all_key_Ctl();

    }
}