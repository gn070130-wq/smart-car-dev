#include "headfile.h"

/**************************************************************************
 * @brief       摄像头图传服务器例程
 * 
 * @note        Creation Time :         2025/12/19
 * @note        Author :                Blockingsys
 * @note        E-mail :                lty18455633351@163.com
 * 
 * @note        Attention :             本例程展示了如何使用摄像头图传服务器实现实时视频流传输:
 *                                      
 *                                      1. 服务器采用MJPEG格式通过HTTP协议传输视频流
 *                                      支持多客户端同时访问, 可在浏览器中实时查看
 *                                      
 *                                      2. 图传流使用低质量JPEG压缩以降低延迟
 *                                      同时保存原始帧用于高质量拍照
 *                                      
 *                                      3. 点击浏览器"拍照保存"按钮可下载原始高质量图片
 *                                      图片直接下载到客户端, 不占用开发板存储空间
 *                                      
 *                                      4. 访问方式: 在浏览器输入 http://<开发板IP>:8080
 *                                      即可查看实时画面并进行拍照操作
 *                                      
 *                                      5. 服务器在后台线程运行, 主线程负责采集摄像头
 *                                      数据并通过update_frame_mat()更新到服务器
 *                                          
 ***************************************************************************/

int main()
{
    //局部变量
    TransmissionStreamServer transmission_server;
    // 1. 启动图传服务器（默认端口8080）
    if(transmission_server.start_server(8080) < 0)
    {
        return -1;
    }
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 2. 初始化摄像头
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) 
    {
        std::cerr << "Camera cannot be opened" << std::endl;
        return -1;
    }
    
    // 设置摄像头参数
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));  // 使用MJPG编码
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);     // 设置分辨率320x240
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(cv::CAP_PROP_FPS, 60);              // 设置帧率60FPS

    // 定义帧变量
    cv::Mat frame;

    // 3. 主循环：不断读取摄像头帧并更新到服务器
    while(transmission_server.is_running())
    {
        // 读取一帧
        cap >> frame;
        if(frame.empty()) 
        {
            std::cerr << "Frame cannot be read" << std::endl;
            break;
        }
        
        // 更新到图传服务器
        transmission_server.update_frame_mat(frame);
    }
    
    // 释放摄像头资源
    cap.release();
    std::cout << "Program exited" << std::endl;
    return 0;
}