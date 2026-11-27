#ifndef __BLACK_REGION_DETECT_HPP__
#define __BLACK_REGION_DETECT_HPP__

#include "zf_common_headfile.hpp"
#include <opencv2/opencv.hpp> 

extern int DETECT_START_DIS; 
extern int Brick_flag; 
extern int Brick_x1;
extern int Brick_y1;
extern int Brick_x2; 
extern int Brick_y2;
#define DETECT_CNT (3)  // 停稳后最多识别3次；第一次置信度>0.95时仍允许立即确认
#define EARLY_BRAKE_Y2_MIN (18)
// ================= 算法参数配置 (垂直投影法专用) =================
#define BR_MARGIN       3   
#define BR_SCAN_ROWS    10  
#define BR_MIN_WIDTH    2   
#define BR_MAX_WIDTH    60  
#define BR_MAX_GAP      6   

// ================= 输出变量声明 =================
extern int br_found;
extern int br_center_x;
extern int br_center_y;

extern int Brick_flag;
extern int Target_flag;
extern int pre_brake_hint;   // 直道早检出刹车提示(Step1 finder+位置约束通过即置1,不等 R2/R3)
extern int brick_enable;   // 红砖识别总开关:0=关,1=开(菜单 BrkEn)
extern int Brick_pos_flag;

extern int Red_flag;             
extern cv::Mat final_crop_img;

extern int target_x1;
extern int target_y1;
extern int target_x2;
extern int target_y2;

extern int orig_crop_x1;
extern int orig_crop_y1;
extern int orig_crop_x2;
extern int orig_crop_y2;

extern int recog_cnt;           
extern int recog_staute;        
extern int final_detect;        
extern int detect_out[DETECT_CNT];  
extern float detect_out_prob[DETECT_CNT];

// ================= 主函数声明 =================
void find_black_rect_in_border(
    const uint8_t image[IMAGE_HEIGHT][IMAGE_WIDTH],
    const uint8_t left_border[IMAGE_HEIGHT],
    const uint8_t right_border[IMAGE_HEIGHT],
    uint16_t end_row
);

// 【红砖判定方式2】边界突变/赛道宽度收缩法(方式1黑块位置法未判到时补充)
void find_brick_by_border(void);
void reset_black_region_runtime_state(void);//发车前清除黑块、红砖、卡片ROI与分类缓存

#endif // __BLACK_REGION_DETECT_HPP__
