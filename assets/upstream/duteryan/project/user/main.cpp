/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-12-27        大W            first version
********************************************************************************************************************/
#include "zf_common_headfile.hpp"
/* @brief 程序主函数，程序入口
 * @return int 程序退出状态码，正常退出返回0，异常返回-1
 */
// 全局变量（给线程用）


uint8_t* gray_image = NULL;
uint16_t* rgb565_image = NULL;
extern int min_efect_row_end;
int a=0;
int main() 
{
    system_init(); 
    ncnn_init("model32.param", "model32.bin");
    // ncnn_benchmark_single_jpg("image.jpg", 1000);
    pit_init(); 
    CameraStreamServer server;
    // carama_send_init();
    // fps_init();  
    // if(server.start_server(8080) < 0)
    // {
    //     printf("摄像头图传开启失败\n");
    //     return -1;
    // }
    if(start_data_server(9090) < 0)
    {
        server.stop_server();
        printf("调试数据服务器启动失败\n");
        return -1;
    }
    speed_left_error_integral=0;
    speed_right_error_integral=0;
    Gyro_error_integral=0;
    while(1)
    {
        // fps_update();
        if(launch_request != 0)
        {
            // 长按发车只在PIT中断里置请求；实际清理在主线程完成。
            prepare_new_run();
        }

        if(uvc_dev.wait_image_refresh() == 0)
        {       
            // rgb565_image = uvc_dev.get_rgb565_image_ptr();
            // ips200.show_rgb565_image(0, 0, rgb565_image, UVC_WIDTH, UVC_HEIGHT, UVC_WIDTH, UVC_HEIGHT, 0);
                gray_image = uvc_dev.get_gray_image_ptr();
                // 判空：确保图像指针有效
                if(NULL != gray_image)
                {   
                    find_line();
                    gray_image = compress_gray_image(gray_image);
                    Convert_1D_to_2D(gray_image);//转为2维图像
                    image_threshold = GetOSTUThreshold(Image, 0, IMAGE_HEIGHT, 0, IMAGE_WIDTH);
                    set_image_twovalues(image_threshold);//将图像转为01二维数组arr
                    LvBoSaiDao(arr,IMAGE_HEIGHT-2,SouXianValidHeight);//赛道滤波
                    Find_End_Line();
                    image_draw_rectan(arr);//为arr画黑框
                    search_neighborhood();//八邻域爬线
                    find_longest_col();//寻找最长白列
                    current_wide();
                    effect_line();
                    search_corner();
                    if (recog_staute != 1) //偏转中不识别元素
                    {
                        shizi_judge(arr);
                        shizi_one();
                        shizi_two();
                        circle_judge();
                        circle_process();
                    }
                    reNiHe();
                    DIUXIAN_XIUZLZ();
                    banmaxian_start(arr,current_left_col,current_right_col,30);
                     //仅在有效行以下寻找色块
                    min_efect_row_end = (left_effect_row_end <= right_effect_row_end) ? left_effect_row_end : right_effect_row_end;
                    if(min_efect_row_end<=10)min_efect_row_end=10;//限制最小在10行
                    // evaluate_track_curvature_from_edges(min_efect_row_end);
                    qulv_characteriristic();
                    wd_flag_debounce();//wd_flag 去抖,维护 wd_flag_stable(绕行起始/距离判定用,防单帧噪声)
                    // printf("w=%d r=%d z=%d\n",wd_flag,ruwan_flag,zhi_flag);
                    if(banmaxian_flag!=1 && recog_staute!=1 &&
                       evade_cooldown_active!=1 &&
                       card_reverse_phase==CARD_REVERSE_IDLE)
                    {
                        // 红砖避障期间(brick_dis_isp==1)也继续调用：find_black_rect 内部走"显示刷新模式"，
                        // 只更新绿框 target_* 供实时观察，不改动已锁定的 Brick_pos_flag/触发状态(见函数内 7.5 段)。
                        // 冷却期整段不检测色块，等退出绕行后的累计路程超过
                        // EVADE_COOLDOWN_DIS 才放行；新绕行执行期(recog_staute==1)同样冻结色块检测，
                        // TURN_IN/CLEAR/MERGE/SETTLE只使用分类时锁定的动作、场景和参数，
                        // 防止原目标或下一目标改写已经开始的绕行。
                        find_black_rect_in_border(arr, current_left_col, current_right_col, min_efect_row_end);
                        // 红砖判定方式2：边界突变/赛道宽度收缩法(方式1本帧未判到砖/目标时补充)
                        find_brick_by_border();
                        // [排查] 黑块首检边沿:对比"黑块几何检出距离"和起刹打印的"红验证确认距离"——
                        // 黑块首检 y2 远小于起刹 y2 → 晚在红色验证;两者接近 → 晚在黑块检测本身
                        {
                            static int br_found_prev = 0;
                            if (br_found == 1 && br_found_prev == 0)
                            {
                                // printf("黑块首检 y2=%d\n", target_y2);
                            }
                            br_found_prev = br_found;
                        }
                    }
                    // 按实际出现顺序给卡片编号，并根据 Para5 的 Card1~Card5 决定
                    // 本张卡片是进入现有识别流程，还是清掉预刹/软限速后直接通过。
                    update_card_sequence_gate();
                    if(Target_flag==1)
                    {
                        // printf("ok\n");
                        // ncnn_recog_run_roi_float();
                        // capture_save_roi_mjpg("image",a++);
                    }
                    // find_black_rect_in_border(arr, current_left_col, current_right_col, min_efect_row_end);
                    EvaSD_decision();
                    // update_pre_brake 必须在 Detect_process 之前调用：先建立预减速状态。
                    // Detect_process满足正式输入条件后只发停车请求，编码器连续确认停稳后才运行NCNN。
                    update_pre_brake();//识别前期刹车latch:按检测帧维护(红块起刹/黑块保持/极短兜底松)
                    Detect_process();
                    bottom_width_judge();//绕行补线后再判定BNF,能看到中线/边界是否已经贴边
                    //printf("pre_brake=%d\n",pre_brake);
                    // 新绕行执行期或退出后的识别冷却期不检测斑马线/出界停车，
                    // 避免本张卡片尚未完全离开视野时误停。
                    if(recog_staute != 1 && evade_cooldown_active != 1)
                    {
                        // 斑马线按事件边沿计数，同一条线连续多帧只计一次；ZbStop=n对应第n+1次停车，负数关闭。
                        update_zebra_stop_counter();
                        if(out_of_bounds_judge()==1&&car_running==1)//出界停车
                        {
                            car_running=0;
                            printf("停车原因: 出界\n");
                        }
                    }
                    // for(int i=0;i<IMAGE_HEIGHT;i++)
                    // {
                    //     printf("第%d行，宽度%d\n",i,current_right_col[i]-current_left_col[i]);
                    // }
                    turn_propect_decision();//前瞻行选择
                    getdiffer(turn_propect);
                    screen_auto_display();
                    // oscilloscope_send_data();//示波器传数据
                    // memcpy(image_copy[0], arr, IMAGE_WIDTH * IMAGE_HEIGHT);
                    // seekfree_assistant_camera_send();
                    // for(int i=0;i<IMAGE_HEIGHT;i++)
                    // {
                    //     printf("第%d行，宽度%d\n",i,(current_right_col[i]-current_left_col[i]));
                    // }
                    // printf("nihao \n");
                    //red_mask_tuning_process(server);

                }
        }
        system_delay_ms(2);//延时2ms
    }
    // ========== 循环结束后，安全地在主线程上下文中执行清理 ==========
    // printf("\n收到退出信号，跳出主循环，开始安全清理...\n");
    // cleanup();
    
     return 0; // 理论上不会执行到这里，主循环为死循环
}

// **************************** 代码区域 ****************************
