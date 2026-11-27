#include "zf_common_headfile.hpp"

int main(int, char**) 
{
    // 自动检测设备
    if(device_self_detect() < 0)
    {
        return -1;
    }
    while(1)
    {
        if(system_self_lock == false || green_detect == false) // 菜单界面
        {
            UI_menu_display(); // 显示菜单系统
            #if 1 == USE_GREEN_DETECT
            if(uvc_dev.wait_image_refresh() == 0) // 阻塞等待摄像头完成新一帧图像的采集和刷新
            {
                rgb_image = uvc_dev.get_rgb_image_ptr(); // 获取RGB图像指针
                float green_area = 0.0f; // 绿色色块面积
                green_area = detect_green_block(DETECT_MIN_ROW, DETECT_MAX_ROW, DETECT_MIN_COLUMN, DETECT_MAX_COLUMN); // 检测绿色块
                // printf("green_area: %.2f\n", green_area);
                if(green_area < MIN_START_AERA)
                {
                    green_detect = true;
                    printf("[SUCCESS]发车成功\n");
                }
            }
            #endif
            #if 1 == USE_WECHAT_APP // 微信小程序
            
            #endif
        }
        else
        {
            if(BLDC_control.start_lock == false)
            {
                system_delay_ms(BLDC_START_WAIT_TIME); // 等待BLDC起动
                #if 1 == USE_BLDC
                bldc_control_start(); // BLDC控制起动
                #endif
                BLDC_control.start_lock = true;
            }
            else
            {
                #if 1 == USE_BLDC
                if(stop.stop_flag == 2)
                {
                    esc_set_duty(BLDC_MIN_PWM);
                }
                else
                {
                    esc_set_duty(BLDC_control.bldc_constant_pwm);
                }
                #endif
            }
            if(uvc_dev.wait_image_refresh() == 0) // 阻塞等待摄像头完成新一帧图像的采集和刷新
            {
                rgb_image = uvc_dev.get_rgb_image_ptr(); // 获取RGB图像指针
                gray_image = uvc_dev.get_gray_image_ptr(); // 获取灰度图像指针
                if(NULL != rgb_image && NULL != gray_image) // 检查图像指针是否有效
                {
                    // 压缩灰度图像存储到image_buffer中
                    cv::Mat gray_original(ROW, COL, CV_8UC1, gray_image); // 包装灰度图像
                    float OTSU_threshold = cv::threshold(gray_original, gray_original, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU); // 使用OTSU算法计算阈值
                    float adjusted_threshold = OTSU_threshold - adaption_threshold; // 调整阈值
                    cv::threshold(gray_original, gray_original, adjusted_threshold, 255, cv::THRESH_BINARY); // 使用调整后的阈值进行二值化
                    memcpy(image_buffer[0], gray_original.data, COL * ROW * sizeof(uint8_t)); // 复制到image_buffer
                    if(stop.misjudge_flag == false)
                    {
                        motor_control_stop();
                        // printf("[SUCCESS]STOP.\r\n");
                    }
                    search_image_edge(); // 搜索图像边界
                    obtain_middle_line(); // 获取中线
                    // 检测红色色块
                    if(stop.misjudge_flag == false && judge_ramp.flag == false
                        && judge_roadblock.left_roadblock_flag == false && judge_roadblock.right_roadblock_flag == false && element_cooldown.roadblock_cooldown == 0
                        ) // 部分元素内不判断红色色块 && judge_island.left_island_type == 0 && judge_island.right_island_type == 0
                    {
                        static uint8_t detect_red_block_count = 0;
                        if(stop.detect_RGB_red == false)
                        {
                            if(++detect_red_block_count >= 2)
                            {
                                detect_red_block(DETECT_MIN_ROW, DETECT_MAX_ROW, DETECT_MIN_COLUMN, DETECT_MAX_COLUMN);
                                detect_red_block_count = 0;
                            }
                        }
                        else
                        {
                            if(detect_red_block(DETECT_MIN_ROW, DETECT_MAX_ROW, DETECT_MIN_COLUMN, DETECT_MAX_COLUMN))
                            {
                                detect_red_block_count = 0;
                            }
                            else
                            {
                                // 连续未检测到红色色块时清除状态，避免卡在低速状态
                                static uint8_t red_miss_count = 0;
                                if(++red_miss_count >= 10)
                                {
                                    stop.detect_RGB_red = false;
                                    model_coord_flag = 0;
                                    red_miss_count = 0;
                                    printf("[ERROR] Red block lost, reset state.\r\n");
                                }
                            }
                        }
                    }
                    // 模型识别
                    if(stop.detect_RGB_red == true)
                    {
                        if(model_area > DEAL_MODEL_AREA && model_area < MAX_MODEL_AREA && Active_classifier.captureFrame())
                        {
                            Active_classifier.runInference();
                            detect_model();
                        }
                    }
                    // display_image();
                    // ips200.displayimage_gray(image_buffer[0], COL, ROW);
                    // ips200.show_float(0, 120, angle_line.differ_far, 4,2);
                    // ips200.displayimage_rgb565(rgb_image, UVC_WIDTH, UVC_HEIGHT); // 显示RGB图像
                    // ips200.displayimage_gray(gray_image, COL, ROW); // 显示二值化图像
                }
                // ips200.show_int(0, 120, model_control.middle_end_row, 3);
                // ips200.show_int(80, 120, model_control.middle_end_column, 3);
                #if USE_TCP_SERVER != 0
                static uint8_t TCP_count = 0;
                TCP_count++;
                if(TCP_count > 0)
                {
                    TCP_send_data();
                    TCP_count = 0;
                }
                #endif
                // crossroad & island
                // ips200.show_string(0, 120, "lose line:");
                // ips200.show_string(110, 120, "flag:");ips200.show_int(160, 120, verge.left_lose_line.flag, 1);ips200.show_int(180, 120, verge.right_lose_line.flag, 1);
                // ips200.show_string(0, 140, "row:");ips200.show_int(60, 140, verge.left_lose_line.row, 3);ips200.show_int(100, 140, verge.right_lose_line.row, 3);
                // ips200.show_string(0, 160, "first point:");
                // ips200.show_string(110, 160, "flag:");ips200.show_int(160, 160, verge.left_first_point.flag, 1);ips200.show_int(180, 160, verge.right_first_point.flag, 1);
                // ips200.show_int(0, 180, verge.left_first_point.row, 3);ips200.show_int(40, 180, verge.left[verge.left_first_point.row].column, 3);
                // ips200.show_int(90, 180, verge.right_first_point.row, 3);ips200.show_int(130, 180, verge.right[verge.right_first_point.row].column, 3);
                // ips200.show_string(0, 200, "inflection point:");
                // ips200.show_int(0, 220, verge.left_inflection.row, 3);ips200.show_int(40, 220, verge.left[verge.left_inflection.row].column, 3);
                // ips200.show_int(90, 220, verge.right_inflection.row, 3);ips200.show_int(130, 220, verge.right[verge.right_inflection.row].column, 3);
                // ips200.show_string(0, 240, "last point:");
                // ips200.show_int(0, 260, verge.left_last_point.row, 3);ips200.show_int(40, 260, verge.left[verge.left_last_point.row].column, 3);
                // ips200.show_int(90, 260, verge.right_last_point.row, 3);ips200.show_int(130, 260, verge.right[verge.right_last_point.row].column, 3);
                // ips200.show_string(0, 280, "angle:");ips200.show_float(50, 280, gyro_element.island, 5,2);

                // // ramp
                // ips200.show_int(0, 120, verge.left[verge.left_last_point.row + 10].column - verge.left[verge.left_first_point.row - 10].column, 3);
                // ips200.show_int(90, 120, verge.right[verge.right_first_point.row - 10].column - verge.right[verge.right_last_point.row + 10].column, 3);
                // ips200.show_int(0, 160, verge.left_last_point.row, 3);ips200.show_int(40, 260, verge.left[verge.left_last_point.row].column, 3);
                // ips200.show_int(90, 160, verge.right_last_point.row, 3);ips200.show_int(130, 260, verge.right[verge.right_last_point.row].column, 3);
                // ips200.show_int(0, 200, verge.left_lose_line.flag, 3);
                // ips200.show_int(90, 200, verge.right_lose_line.flag, 3);
                // ips200.show_int(0, 240, verge.left_first_point.row, 3);
                // ips200.show_int(90, 240, verge.right_first_point.row, 3);

                // printf("time_interval: %f, island: %f\n", gyro_element.time_interval, gyro_element.island);
                // printf("angle_line.differ_far: %f\n", angle_line.differ_far);
                // printf("target_speed: %d\n", decision.target_speed);

                // if(judge_zebra_line.flag == 2 || stop.stop_flag == 2) // 识别到线或停止标志进入菜单界面
                // {
                //     system_self_lock = false;
                //     drv8701e_pwm_1.set_duty(0);
                //     drv8701e_pwm_2.set_duty(0);
                //     left_speed.Ki = 0;
                //     right_speed.Ki = 0;
                // }
            }
            #if 1 == USE_WECHAT_APP
            // wechat_app_receive_data(); // 处理微信小程序数据
            #endif
        }
        system_delay_ms(2); // 控制循环频率，避免占用过多CPU资源
    }
    return 0;
}

/**
 * @brief 处理5ms中断
 * @param void
 * @return void
 * @note 处理图像偏差环、角度环、角速度环、速度环
 * @author YuYu
 */
void pit_callback(void)
{
    #if 0 == USE_GREEN_DETECT
    if(system_self_lock && BLDC_control.start_lock)
    {
        // 转向环
        static uint8_t PIT_5ms_count = 0;
        PIT_5ms_count++;
        if(PIT_5ms_count >= 4)
        {
            PIT_5ms_count = 0;
        }
        if(PIT_5ms_count == 0)
        {
            differ_outer_pid(); // 偏差环
        }
        if(PIT_5ms_count % 2 == 0)
        {
            gyroscope_get_data(); // 更新陀螺仪数据
            dir_inner_pid(); // 转向环
        }
        speed_control(); // 电机控制
        // // 测量距离
        // if(NO_FIND_DEVICE != dl1x_dev_type) // 仅当设备初始化成功时读取
        // {
        //     static uint8_t PIT_T2_count = 0;
        //     PIT_T2_count++;
        //     if(PIT_T2_count == 20)
        //     {
        //         dl1x_distance = dl1x_dev.get_distance();
        //         PIT_T2_count = 0;
        //     }
        // }
        if(stop.misjudge_flag == true)
        {
            static uint16_t misjudge_count = 0;
            misjudge_count++;
            if(misjudge_count > 300)
            {
                stop.misjudge_flag = false;
                misjudge_count = 0;
            }
        }
    }
    #elif 1 == USE_GREEN_DETECT
    if(system_self_lock && BLDC_control.start_lock && green_detect)
    {
        // 转向环
        static uint8_t PIT_5ms_count = 0;
        PIT_5ms_count++;
        if(PIT_5ms_count >= 4)
        {
            PIT_5ms_count = 0;
        }
        if(PIT_5ms_count == 0)
        {
            differ_outer_pid(); // 偏差环
        }
        if(PIT_5ms_count % 2 == 0)
        {
            gyroscope_get_data(); // 更新陀螺仪数据
            dir_inner_pid(); // 转向环
        }
        speed_control(); // 电机控制
        // // 测量距离
        // if(NO_FIND_DEVICE != dl1x_dev_type) // 仅当设备初始化成功时读取
        // {
        //     static uint8_t PIT_T2_count = 0;
        //     PIT_T2_count++;
        //     if(PIT_T2_count == 20)
        //     {
        //         dl1x_distance = dl1x_dev.get_distance();
        //         PIT_T2_count = 0;
        //     }
        // }
        if(stop.misjudge_flag == true)
        {
            static uint16_t misjudge_count = 0;
            misjudge_count++;
            if(misjudge_count > 300)
            {
                stop.misjudge_flag = false;
                misjudge_count = 0;
            }
        }
    }
    #endif
}
