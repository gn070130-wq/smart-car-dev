#include  "zf_common_headfile.hpp"
#include <stdint.h>  // 必须包含，定义uint8_t类型
#include <string.h>  // 用于memset清空缓冲区
int jpg_cnt=0;
zf_driver_tcp_client tcp_client_dev;
uint16_t* rgb_image;
uint16 image_copy[SEND_HEIGHT][SEND_WIDTH];
uint16 image_detect[DETECT_HEIGHT][DETECT_WIDTH];
//-------------------------------------------------------------------------------------------------------------------
// 函数简介 TCP发送数据 全局包装函数
// 参数说明 buf 要发送的数据缓冲区指针, len 要发送的数据字节长度
// 返回参数 uint32 实际成功发送的字节数
// 使用示例 供seekfree_assistant_interface_init调用，无需手动调用
// 备注信息 封装tcp_client_dev.send_data成员函数，适配普通函数指针格式要求
//-------------------------------------------------------------------------------------------------------------------
uint32 tcp_send_wrap(const uint8 *buf, uint32 len)
{
    // 调用TCP客户端的发送数据成员函数，完成数据发送
    return tcp_client_dev.send_data(buf, len);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介 TCP接收数据 全局包装函数
// 参数说明 buf 接收数据的缓冲区指针, len 最大可接收的字节长度
// 返回参数 uint32 实际成功接收的字节数
// 使用示例 供seekfree_assistant_interface_init调用，无需手动调用
// 备注信息 封装tcp_client_dev.read_data成员函数，适配普通函数指针格式要求
//-------------------------------------------------------------------------------------------------------------------
uint32 tcp_read_wrap( uint8 *buf, uint32 len)
{
    // 调用TCP客户端的读取数据成员函数，完成数据接收
    return tcp_client_dev.read_data(buf, len);
}
void carama_send_init()
{
     // 初始化UDP客户端
    if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
    {
        printf("tcp_client ok\r\n");
    }
    else
    {
        printf("tcp_client error\r\n");
    }
    seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
    // seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_SCC8660, image_copy[0], SEND_WIDTH, SEND_HEIGHT);
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], IMAGE_WIDTH, IMAGE_HEIGHT);
    // 配置X轴巡线边界信息，Y坐标由图像高度自动匹配
    seekfree_assistant_camera_boundary_config(X_BOUNDARY, IMAGE_HEIGHT, current_left_col, current_right_col, NULL,NULL, NULL ,NULL);
}
void rgb_image_send(const uint16_t* src_rgb)
{
    // for(uint32_t i = 0; i < SEND_WIDTH * SEND_HEIGHT; i++)
    //     {
    //         ((uint16_t *)image_copy[0])[i] = (((uint16_t *)src_rgb)[i] & 0xFF) <<8 | (((uint16_t *)src_rgb)[i] & 0xFF00) >>8;
    //     }
    //     // memcpy((uint8 *)&image_copy[0], (uint8 *)rgb_image, UVC_WIDTH * UVC_HEIGHT * 2); // 原始未转换的拷贝代码，已注释

        // 核心发送函数：将配置好的图像/边界信息，通过TCP发送到上位机(逐飞助手)
        seekfree_assistant_camera_send();
}

/**
 * @brief 单通道 uint16 图像裁剪（带越界保护）
 * @param src      原图数据指针 uint16_t*
 * @param x0       裁剪区域左上角 X
 * @param y0       裁剪区域左上角 Y
 * @param x1       裁剪区域右下角 X
 * @param y1       裁剪区域右下角 Y
 * @param dst      输出固定大小数组：uint16_t dst[SEND_HEIGHT][SEND_WIDTH]
 */
void crop_image(const uint16_t* src,int x0, int y0, int x1, int y1,uint16 dst[DETECT_HEIGHT][DETECT_WIDTH])
{
    // 清空目标数组
    memset(dst, 0, sizeof(uint16_t) * DETECT_HEIGHT * DETECT_WIDTH);

    // 越界保护
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= IMAGE_WIDTH)  x1 = IMAGE_WIDTH - 1;
    if (y1 >= IMAGE_HEIGHT) y1 = IMAGE_HEIGHT - 1;

    int crop_w = x1 - x0 + 1;
    int crop_h = y1 - y0 + 1;

    if (crop_w <= 0 || crop_h <= 0) {
        return;
    }

    int copy_w = (crop_w < DETECT_WIDTH)  ? crop_w : DETECT_WIDTH;
    int copy_h = (crop_h < DETECT_HEIGHT) ? crop_h :DETECT_HEIGHT;

    // 逐像素复制 + 自动高低字节交换
    for (int y = 0; y < copy_h; y++) {
        for (int x = 0; x < copy_w; x++) {
            int src_x = x0 + x;
            int src_y = y0 + y;

            // 读取原图像素
            uint16_t pixel = src[src_y * IMAGE_WIDTH + src_x];

            // ================ 在这里做 高低字节交换 ================
            uint16_t pixel_swap = ((pixel & 0xFF) << 8) | ((pixel & 0xFF00) >> 8);

            // 写入目标数组（已经转换完成）
            dst[y][x] = pixel_swap;
        }
    }
}
void Save_RGB565_To_JPG_ID(uint16 image_detect[DETECT_HEIGHT][DETECT_WIDTH],const char* base_name,int id)
{
    char filename[256];
    sprintf(filename, "%s_%d.jpg", base_name, id);

    cv::Mat img(DETECT_HEIGHT, DETECT_WIDTH, CV_8UC3);

    for (int y = 0; y < DETECT_HEIGHT; y++)
    {
        cv::Vec3b* row_ptr = img.ptr<cv::Vec3b>(y);

        for (int x = 0; x <DETECT_WIDTH; x++)
        {
            uint16_t pixel = image_detect[y][x];

            // ===== 🚨 关键：字节交换 =====
            pixel = (pixel >> 8) | (pixel << 8);

            // ===== RGB565 拆分 =====
            uint8_t r5 = (pixel >> 11) & 0x1F;
            uint8_t g6 = (pixel >> 5)  & 0x3F;
            uint8_t b5 =  pixel        & 0x1F;

            // ===== 正确扩展 =====
            uint8_t r = (r5 << 3) | (r5 >> 2);
            uint8_t g = (g6 << 2) | (g6 >> 4);
            uint8_t b = (b5 << 3) | (b5 >> 2);

            row_ptr[x] = cv::Vec3b(b, g, r);
        }
    }

    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(95);

    if (!cv::imwrite(filename, img, params))
        printf("[ERR] 保存失败!\n");
    else
        printf("[OK] 保存: %s\n", filename);
}

void capture_save_roi_mjpg(const char* name_prefix, int custom_id)
{
    // 1. 🚀 直接使用已经在 find_black_rect_in_border 中切好的全局变量
    // final_crop_img 是经过 a*a 严格向上裁剪后的纯净特征图
    const cv::Mat& roi_img = final_crop_img;
    
    // 2. 防崩溃安全校验
    if (roi_img.empty())
    {
        // 如果当前帧没检测到目标，final_crop_img 为空，直接退出
        return; 
    }

    // 3. 拼接目标存储路径：/home/root/名称_数字ID.jpg
    char save_path[256];
    snprintf(save_path, sizeof(save_path), 
             "/home/root/%s_%d.jpg", name_prefix, custom_id);

    // 4. 保存图片到主板的 EMMC/SD 卡
    // 注意：imwrite 默认保存的是原始像素，即便你的 final_crop_img 是 25x25，它也会存为 25x25
    bool ret = cv::imwrite(save_path, roi_img);
    
    if (ret)
    {
        // 🔴 重点：打印出当前这一帧动态计算出的 a*a 分辨率
        // 你可以通过串口日志观察：远处目标保存的尺寸小（如 20x20），近处目标尺寸大（如 50x50）
        printf("[成功] 极速采集：%s | 实际裁剪尺寸：%dx%d\n", 
               save_path, roi_img.cols, roi_img.rows);
    }
    else
    {
        printf("[错误] 保存图片失败，请检查路径权限或空间：%s\n", save_path);
    }
}

// ==============================================
// 保存"原始 80x80 彩色 ROI"(未经扶正/去底座)。
//   用 find_black_rect_in_border 存下的 orig_crop_* 坐标，从当前帧彩色图重新截原图ROI。
//   与 capture_save_roi_mjpg 的区别：后者存处理后的 final_crop_img(a×a特征图)，这个存原始ROI。
// ==============================================
void capture_save_raw_roi(const char* name_prefix, int custom_id)
{
    // 无有效目标框(没找到目标时 orig_crop_* 被清零) → 直接退出
    if (orig_crop_x2 <= orig_crop_x1 || orig_crop_y2 <= orig_crop_y1)
        return;

    // 从当前帧彩色图重新截原始 ROI(浅引用,当帧有效;BGR 已在 find_black_rect 解码过)
    cv::Mat roi = uvc_dev.get_frame_mjpg_roi(orig_crop_x1, orig_crop_y1, orig_crop_x2, orig_crop_y2);
    if (roi.empty())
        return;

    char save_path[256];
    snprintf(save_path, sizeof(save_path), "/home/root/%s_%d.jpg", name_prefix, custom_id);

    bool ret = cv::imwrite(save_path, roi);
    if (ret)
        printf("[成功] 原始ROI采集: %s | 尺寸: %dx%d\n", save_path, roi.cols, roi.rows);
    else
        printf("[错误] 原始ROI保存失败(检查路径/权限/空间): %s\n", save_path);
}

void oscilloscope_send_data()//示波器传数据
{
      // 写入需要发送的数据，有几个通道就写多少个数据
        // 这里仅写入4个通道数据
        // speed_left_target=60;
        seekfree_assistant_oscilloscope_data.data[0] = col_Differ;
        seekfree_assistant_oscilloscope_data.data[1] = Dir_output;
        seekfree_assistant_oscilloscope_data.data[2] = Gyro_output;
        // seekfree_assistant_oscilloscope_data.data[3] =0;
        
        // 设置本次需要发送几个通道的数据
        seekfree_assistant_oscilloscope_data.channel_num = 4;
        
        // 这里仅发送了4个通道的数据，最大支持8通道
        seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);
}