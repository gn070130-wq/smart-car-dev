#include "deal_model.h"

// model
uint8_t model_type = 0; // Weapon(left):1;Supplies(right):2;Vehicle:3
uint8_t last_model_type = 0;
uint8_t model_coord_flag = 0; // 0:未获取坐标 1:已获取坐标
float red_block_area = 0.0f; // 红色色块面积
float model_area = 0.0f; // 模型面积
int16_t bottom_left_x = 0, bottom_left_y = 0, red_block_w = 0, red_block_h = 0;
int8_t model_output[5] = {-1, -1, -1, -1, -1}; // 模型输出缓冲区（根据实际模型输出调整）
const char* class_labels[] = {"Supplies", "Vehicle", "Weapon"}; // 需要与train.py提示顺序一致

/****************************** model ******************************/
/**
 * @brief 搜索红色色块
 * @param min_row 最小行索引
 * @param max_row 最大行索引
 * @param min_col 最小列索引
 * @param max_col 最大列索引
 * @return void
 * @note 搜索红色色块，返回其左下角坐标和宽度高度
 * @author YuYu
 */
bool detect_red_block(int16_t min_row, int16_t max_row, int16_t min_col, int16_t max_col)
{
    if(model_control.turn_flag == true || model_control.return_flag == true)
    {
        return false;
    }
    bool find_block = false;
    if(rgb_image == NULL)
    {
        stop.detect_RGB_red = false;
        return false;
    }
    // 边界处理
    int16_t row_min = std::min(min_row, max_row);
    int16_t row_max = std::max(min_row, max_row);
    int16_t col_min = std::min(min_col, max_col);
    int16_t col_max = std::max(min_col, max_col);
    row_min = std::max(static_cast<int16_t>(0), row_min);
    row_max = std::min(static_cast<int16_t>(UVC_HEIGHT), row_max);
    col_min = std::max(static_cast<int16_t>(0), col_min);
    col_max = std::min(static_cast<int16_t>(UVC_WIDTH), col_max);
    if(row_max <= row_min || col_max <= col_min)
    {
        stop.detect_RGB_red = false;
        return false;
    }
    // 提取矩形框 ROI
    int16_t roi_w = col_max - col_min;
    int16_t roi_h = row_max - row_min;
    cv::Mat roi_rgb565(roi_h, roi_w, CV_8UC2);
    for(int16_t r = 0; r < roi_h; r++)
    {
        memcpy(roi_rgb565.ptr(r), rgb_image + (row_min + r) * UVC_WIDTH + col_min, roi_w * 2);
    }
    static cv::Mat roi_bgr;
    cv::cvtColor(roi_rgb565, roi_bgr, cv::COLOR_BGR5652BGR);
    static cv::Mat hsv;
    cv::cvtColor(roi_bgr, hsv, cv::COLOR_BGR2HSV);
    // 红色 HSV 检测
    static cv::Mat mask1, mask2, mask3, mask4, red_mask;
    // cv::inRange(hsv, cv::Scalar(0, 125, 143), cv::Scalar(179, 255, 255), mask1);
    // cv::inRange(hsv, cv::Scalar(144, 136, 109), cv::Scalar(179, 255, 255), mask2);
    // cv::inRange(hsv, cv::Scalar(0, 114, 138), cv::Scalar(93, 255, 255), mask3);
    // cv::inRange(hsv, cv::Scalar(163, 46, 94), cv::Scalar(179, 255, 189), mask4);
    // 低红
    cv::inRange(hsv, cv::Scalar(0, 125, 143), cv::Scalar(62, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(144, 136, 109), cv::Scalar(179, 255, 255), mask2);
    // 高红
    cv::inRange(hsv, cv::Scalar(0, 114, 138), cv::Scalar(62, 255, 255), mask3);
    cv::inRange(hsv, cv::Scalar(163, 46, 94), cv::Scalar(179, 255, 189), mask4);
    red_mask = mask1 | mask2 | mask3 | mask4;
    // 无红色像素直接返回
    if(cv::countNonZero(red_mask) == 0)
    {
        model_coord_flag = 0; // 无红色像素，重置坐标标志以便下次重新获取
        return false;
    }
    // 用左右边界生成 mask, 裁剪到赛道内
    static cv::Mat boundary_mask(red_mask.rows, red_mask.cols, CV_8UC1, cv::Scalar(0));
    int16_t valid_row_count = 0;
    boundary_mask.setTo(cv::Scalar(0));  // 全黑
    for(int16_t r = row_min; r < row_max; r++)
    {
        int16_t local_r = r - row_min;  // 相对于 roi 的行索引
        int16_t gray_r = r / 2; // 相对于 gray_img 的行索引
        int16_t left_bound = col_min;
        int16_t right_bound = col_max;
        if(verge.left[gray_r].flag == 1 && verge.right[gray_r].flag == 1)
        {
            left_bound = (verge.left[gray_r].column * 2) + 5;
            right_bound = (verge.right[gray_r].column * 2) - 5;
        }
        else if(verge.left[gray_r].flag == 1 && verge.right[gray_r].flag == 0)
        {
            left_bound = (verge.left[gray_r].column * 2) + 5;
            right_bound = (verge.right[verge.right_last_point.row + 5].column * 2) - 5; // verge.right[verge.right_last_point.row + 5].column RIGHT_BOUNDARY_COLUMN
        }
        else if(verge.left[gray_r].flag == 0 && verge.right[gray_r].flag == 1)
        {
            left_bound = (verge.left[verge.left_last_point.row + 5].column * 2) + 5; // verge.left[verge.left_last_point.row + 5].column LEFT_BOUNDARY_COLUMN
            right_bound = (verge.right[gray_r].column * 2) - 5;
        }
        // 与矩形框取交集
        left_bound = std::max(left_bound, col_min);
        right_bound = std::min(right_bound, col_max);
        if(left_bound < right_bound)
        {
            int16_t local_left = left_bound - col_min;
            int16_t local_right = right_bound - col_min;
            boundary_mask.row(local_r).colRange(local_left, local_right).setTo(cv::Scalar(255));
            valid_row_count++;
        }
    }
    // printf("\033[1;33m[DEBUG]\033[0m boundary rows: %d / %d\n", valid_row_count, row_max - row_min);
    // 红色 mask AND 边界 mask = 只保留赛道内的红色区域
    static cv::Mat final_mask;
    cv::bitwise_and(red_mask, boundary_mask, final_mask);
    // 形态学操作
    static cv::Mat kernel;
    if(kernel.empty())
    {
        kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    }
    cv::morphologyEx(final_mask, final_mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1);
    cv::morphologyEx(final_mask, final_mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
    // 连通域分析
    static std::vector<std::vector<cv::Point>> contours;
    static std::vector<cv::Vec4i> hierarchy;
    contours.clear();
    hierarchy.clear();
    cv::findContours(final_mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    double max_area = 0;
    cv::Rect best_rect;
    for(size_t i = 0; i < contours.size(); ++i)
    {
        red_block_area = cv::contourArea(contours[i]);
        if(red_block_area < MIN_AREA) continue;
        // printf("area: %.2f\n", red_block_area);
        cv::Moments m = cv::moments(contours[i]);
        if(m.m00 == 0) continue;
        double cx = m.m10 / m.m00 + col_min;  // 绝对 x
        cv::Rect current_rect = cv::boundingRect(contours[i]);
        // // 过滤宽高比不符的候选区域
        // if(current_rect.height >= current_rect.width) continue;
        double absolute_y = current_rect.y + row_min + current_rect.height / 2;
        bool box_center = ((cx - col_min) >= (col_max - col_min) * CENTER_TOLERANCE) && ((cx - col_min) <= (col_max - col_min) * (1.0f - CENTER_TOLERANCE));
        bool box_enough = red_block_area > MIN_AREA && red_block_area < MAX_AREA;
        if(box_center && box_enough)
        {
            find_block = true;
            if(red_block_area > max_area)
            {
                max_area = red_block_area;
                // printf("\033[1;33m[DEBUG]\033[0m New best block found: area = %.2f\n", max_area);
                best_rect = current_rect;
            }
            if(model_coord_flag == 0)
            {
                model_coord_flag = 1;
                model_control.block_x = (uint8_t)(cx/2); // y 轴坐标
                model_control.block_y = (uint8_t)(absolute_y/2); // x 轴坐标
                model_control.middle_end_row = center.middle_end_row + 5;
                model_control.middle_end_column = center.middle[center.middle_end_row + 5].column - START_MIDDLE_COLUMN;
                printf("[SUCCESS] Red Block Found!\r\n");
                printf("\033[1;33m[DEBUG]\033[0m red block: (row = %d, column = %d)   end point = (row = %d, column = %d) \n", model_control.block_y, model_control.block_x, model_control.middle_end_row, model_control.middle_end_column);
            }
        }
    }
    if(find_block)
    {
        bottom_left_x = best_rect.x + col_min;
        bottom_left_y = best_rect.y + row_min + best_rect.height;
        red_block_w = best_rect.width;
        red_block_h = best_rect.height;
        model_area = red_block_w * red_block_h;
        stop.detect_RGB_red = true;
    }
    else
    {
        model_coord_flag = 0; // 未找到有效色块，重置坐标标志
    }
    return find_block;
}

/**
 * @brief 检测绿色块
 * 
 * @param min_row 最小行索引
 * @param max_row 最大行索引
 * @param min_col 最小列索引
 * @param max_col 最大列索引
 * @return float 绿色色块面积
 */
float detect_green_block(int16_t min_row, int16_t max_row, int16_t min_col, int16_t max_col)
{
    // 基础安全检查
    if(rgb_image == NULL)
    {
        return 0.0f;
    }
    // 边界处理
    int16_t row_min = std::min(min_row, max_row);
    int16_t row_max = std::max(min_row, max_row);
    int16_t col_min = std::min(min_col, max_col);
    int16_t col_max = std::max(min_col, max_col);
    row_min = std::max(static_cast<int16_t>(0), row_min);
    row_max = std::min(static_cast<int16_t>(UVC_HEIGHT), row_max);
    col_min = std::max(static_cast<int16_t>(0), col_min);
    col_max = std::min(static_cast<int16_t>(UVC_WIDTH), col_max);
    // 检查是否有效
    if(row_max <= row_min || col_max <= col_min)
    {
        return 0.0f;
    }
    // 提取矩形框ROI
    int16_t roi_w = col_max - col_min;
    int16_t roi_h = row_max - row_min;
    cv::Mat roi_rgb565(roi_h, roi_w, CV_8UC2);
    for(int16_t r = 0; r < roi_h; r++)
    {
        memcpy(roi_rgb565.ptr(r), rgb_image + (row_min + r) * UVC_WIDTH + col_min, roi_w * 2);
    }
    // 颜色空间转换
    static cv::Mat roi_bgr, hsv;
    cv::cvtColor(roi_rgb565, roi_bgr, cv::COLOR_BGR5652BGR);
    cv::cvtColor(roi_bgr, hsv, cv::COLOR_BGR2HSV);
    // 绿色HSV阈值过滤
    // H: 35-77, S: 43-255, V: 46-255
    static cv::Mat green_mask;
    cv::inRange(hsv, cv::Scalar(35, 43, 46), cv::Scalar(77, 255, 255), green_mask);
    // 形态学操作
    static cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(green_mask, green_mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 1);
    cv::morphologyEx(green_mask, green_mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
    // 查找轮廓,找出面积最大的绿色块
    static std::vector<std::vector<cv::Point>> contours;
    contours.clear();
    cv::findContours(green_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    float max_area = 0.0f; // 记录当前帧找到的最大面积
    for(size_t i = 0; i < contours.size(); ++i)
    {
        float area = cv::contourArea(contours[i]);
        if(area < MIN_AREA) 
        {
            continue; // 过滤掉太小的噪点
        }
        if(area > max_area)
        {
            max_area = area;
        }
    }
    return max_area;
}

uint8_t model_index_categorize(void)
{
    uint8_t type = 0;
    #if (MODEL_SELECTION == 2 &&NCNN_OUTPUT_CLASS_NUM == 3) || (MODEL_SELECTION == 1 && TFLITE_OUTPUT_CLASS_NUM == 3)
    switch (Active_classifier.getPredictionIndex())
    {
        case Weapon:        //武器
            type = 1;
            break;
        case Supplies:      //物资
            type = 2;
            break;
        case Vehicle:       //载具
            type = 3;
            break;
        default:
            break;
    }
    #elif (MODEL_SELECTION == 2 && NCNN_OUTPUT_CLASS_NUM == 6) || (MODEL_SELECTION == 1 && TFLITE_OUTPUT_CLASS_NUM == 6)
    switch (Active_classifier.getPredictionIndex())
    {
        case Firearms:      //武器-枪械
            type = 1;
            break;
        case Explosive:     //武器-炸药
            type = 1;
            break;
        case First_Aid_Kit: //物资-急救包
            type = 2;
            break;
        case Telescope:     //物资-望远镜
            type = 2;
            break;
        case Armoured:      //载具-装甲车
            type = 3;
            break;
        case Ambulance:     //载具-救护车   
            type = 3;
            break;
        default:
            break;
    }
    #endif
    return type;
}

/**
 * @brief 搜索模型
 * @param void
 * @return void
 * @note 搜索模型，返回其类型
 * @author YuYu
 */
#if MODEL_TESTING == 0
static bool model_finish_flag = false;
#endif

void detect_model(void)
{
    // 模型识别
    #if MODEL_TESTING == 1
    static int16_t i = 0;
    i++;
    if(i >= 2)
    {
        model_type = 2;
        printf("model type: %d\n", model_type);
    }
    if(i >= 4) // 假设0.03s识别完模型
    {
        // classifier.resetMajorityFilter(); // 清除滤波器状态，避免误判持续影响后续识别(静态测试保持注释状态,车辆动态识别时启用)
        i = 0;
    }
    #else
    static uint8_t i = 0;
    float confidence = Active_classifier.getPredictionConfidence();
    if(confidence >= CONFIDENCE_THRESHOLD)
    {
        model_output[i] = Active_classifier.getPredictionIndex();
    }
    else
    {
        model_output[i] = -1; // 低于阈值，丢弃本次预测
    }
    i = (i + 1) % 5;
    if(model_output[0] == model_output[1] && model_output[0] != -1 && model_finish_flag == false)
    {
        model_type = model_index_categorize();
        model_finish_flag = true;
        printf("model output: %s; model type: %d\n", Active_classifier.getPredictionLabel(), model_type);
    }
    else if (model_output[1] == model_output[2] && model_output[1] != -1 && model_finish_flag == false)
    {
        model_type = model_index_categorize();
        model_finish_flag = true;
        printf("model output: %s; model type: %d\n", Active_classifier.getPredictionLabel(), model_type);
    }
    else if (model_output[2] == model_output[3] && model_output[2] != -1 && model_finish_flag == false)
    {
        model_type = model_index_categorize();
        model_finish_flag = true;
        printf("model output: %s; model type: %d\n", Active_classifier.getPredictionLabel(), model_type);
    }
    else if (model_output[3] == model_output[4] && model_output[3] != -1 && model_finish_flag == false)
    {
        model_type = model_index_categorize();
        model_finish_flag = true;
        printf("model output: %s; model type: %d\n", Active_classifier.getPredictionLabel(), model_type);
    }
    else if (i > 3 && model_finish_flag == false)
    {
        model_type = 3; // 识别失败时默认为载具(直行)，避免卡在低速状态
        model_finish_flag = true;
        printf("model output failed; model type: %d (default Vehicle)\n", model_type);
    }
    if(model_finish_flag == true)
    {
        i = 0;
        Active_classifier.resetMajorityFilter(); // 清除滤波器状态，避免误判持续影响后续识别(静态测试保持注释状态,车辆动态识别时启用)
        memset(model_output, -1, sizeof(model_output));
        model_finish_flag = false;
    }
    #endif
}
/****************************** model ******************************/
  
/****************************** filter ******************************/
/**
 * @brief 初始化多数投票滤波器
 * @param window_size 滑动窗口大小
 * @param max_classes 类别总数（假设类别ID从0到max_classes-1）
 * @return MajorityVoteFilter* 多数投票滤波器指针
 * @note 初始化多数投票滤波器，用于对模型识别结果进行滑动窗口滤波
 * @author 毛线
 */
MajorityVoteFilter* majority_vote_init(int16_t window_size, int16_t max_classes)
{
    if(window_size <= 0 || max_classes <= 0)
    {
        return NULL;
    }
    MajorityVoteFilter *filter = (MajorityVoteFilter*)malloc(sizeof(MajorityVoteFilter));
    if(!filter) 
    {
        return NULL;
    }
    // 初始化窗口
    filter->window_size = window_size;
    filter->max_classes = max_classes;
    filter->history = (int16_t*)calloc(window_size, sizeof(int16_t));
    filter->counts = (int16_t*)calloc(max_classes, sizeof(int16_t));
    filter->head = 0;
    filter->filled = 0;
    // 初始化计数数组
    if(!filter->history || !filter->counts)
    {
        free(filter->history);
        free(filter->counts);
        free(filter);
        return NULL;
    }
    return filter;
}

/**
 * @brief 释放多数投票滤波器资源
 * @param filter 多数投票滤波器指针
 * @return void
 * @note 释放多数投票滤波器占用的内存资源
 * @author 毛线
 */
void majority_vote_free(MajorityVoteFilter *filter)
{
    if(filter)
    {
        free(filter->history);
        free(filter->counts);
        free(filter);
    }
}

/**
 * @brief 更新多数投票滤波器
 * @param filter 多数投票滤波器指针
 * @param class_id 类别ID（0到max_classes-1）
 * @return int 更新后的多数投票结果（0到max_classes-1）
 * @note 更新多数投票滤波器，根据当前类别ID更新窗口内的统计信息
 * @author 毛线
 */
int16_t majority_vote_update(MajorityVoteFilter *filter, int16_t class_id)
{
    if(!filter || class_id < 0 || class_id >= filter->max_classes)
    {
        return -1;
    }
    // 如果窗口已满，先移除将要被覆盖的旧类别
    if(filter->filled == filter->window_size)
    {
        int16_t old_class = filter->history[filter->head];
        filter->counts[old_class]--;
    } 
    else
    {
        filter->filled++;
    }
    // 插入新类别
    filter->history[filter->head] = class_id;
    filter->counts[class_id]++;
    filter->head = (filter->head + 1) % filter->window_size;
    // 统计当前窗口中出现次数最多的类别
    int16_t best_class = 0;
    int16_t best_count = 0;
    for (int16_t i = 0; i < filter->max_classes; i++)
    {
        if (filter->counts[i] > best_count)
        {
            best_count = filter->counts[i];
            best_class = i;
        }
    }
    return best_class;
}

/**
 * @brief 获取当前多数投票结果（不更新窗口）
 * @param filter 多数投票滤波器指针
 * @return int16_t 当前多数投票结果（0到max_classes-1）
 * @note 获取当前多数投票结果，不更新窗口内的统计信息
 * @author 毛线
 */
int16_t majority_vote_get(MajorityVoteFilter *filter)
{
    if(!filter) 
    {
        return -1;
    }
    int16_t best_class = 0;
    int16_t best_count = 0;
    for(int16_t i = 0; i < filter->max_classes; i++)
    {
        if (filter->counts[i] > best_count)
        {
            best_count = filter->counts[i];
            best_class = i;
        }
    }
    return best_class;
}
/****************************** filter ******************************/

/****************************** model_roi ******************************/
/**
 * @brief 从全帧BGR图像中根据色块坐标计算并裁剪ROI
 * @param src_bgr   源图像（BGR格式，全帧）
 * @param roi_img   输出参数，裁剪出的ROI图像
 * @param roi_rect  输出参数，计算出的ROI矩形
 * @return bool 是否成功提取ROI
 * @note 基于detect_red_block()输出的全局色块坐标和陀螺仪数据动态调整ROI区域
 * @author 毛线
 */
bool get_model_roi(const cv::Mat& src_bgr, cv::Mat& roi_img, cv::Rect& roi_rect)
{
    const int block_x = bottom_left_x;
    const int block_y = bottom_left_y - red_block_h;
    const int block_w = red_block_w;
    const int block_h = red_block_h;

    const int cx = block_x + block_w / 2;
    const int cy = block_y + block_h / 2;
    const int base = std::max(block_w, block_h);
    const float turn = std::fabs(model_control.middle_end_column);

    int size_max = 70;
    float gain = 1.5;
    if(turn > DEVIATION_THRESHOLD_LOW)
    {
        size_max = 60;
        gain = 1.5f;
    }
    if(turn > DEVIATION_THRESHOLD_HIGH)
    {
        size_max = 60;
        gain = 1.65;
    }
    const int size = (int)std::lround(std::clamp(base * gain, 40.0f, (float)size_max));
    const int extra_w = (int)std::lround(std::clamp(size * std::max(0.0f, (gain - 1.4f)), 0.0f, (float)(size / 2)));
    const int y0 = cy - (int)std::lround(size * 0.65f);
    int x0 = cx - size / 2;
    int w = size;
    if(extra_w > 0)
    {
        const float signed_turn = model_control.middle_end_column;
        printf("\033[1;33m[DEBUG]\033[0m turn: %.2f, signed_turn: %.2f, extra_w: %d\n", turn, signed_turn, extra_w);
        if(signed_turn < 0.0f)
        {
            if(fabs(signed_turn) > DEVIATION_THRESHOLD_HIGH)
            {
                x0 = x0 - extra_w;
                w = size + extra_w / 2;
            }
            else if(fabs(signed_turn) > DEVIATION_THRESHOLD_LOW)
            {
                x0 = x0 - extra_w / 2.2f;
            }
        }
        else if(signed_turn > 0.0f)
        {
            if(fabs(signed_turn) > DEVIATION_THRESHOLD_HIGH)
            {
                x0 = x0 + extra_w / 2;
                w = size - extra_w / 4;
            }
            else if(fabs(signed_turn) > DEVIATION_THRESHOLD_LOW)
            {
                x0 = x0 + extra_w / 2.2f;
            }
        }
    }
    roi_rect = cv::Rect(x0, y0, w, size);
    roi_rect &= cv::Rect(0, 0, src_bgr.cols, src_bgr.rows);
    if(roi_rect.area() <= 0)
    {
        printf("❌ ROI 区域无效！\n");
        return false;
    }
    else if(roi_rect.area() <= ROI_MIN_AREA)
    {
        printf("❌ ROI 区域面积过小！\n");
        return false;
    }
    roi_img = src_bgr(roi_rect).clone();
    return true;
}
/****************************** model_roi ******************************/

/****************************** debug ******************************/
/**
 * @brief 调试日志回调函数
 * @param s 调试日志字符串
 * @return void
 * @note 打印调试日志，用于调试和查看模型运行状态
 * @author 毛线
 */
void debug_log_printf_callback(const char* s)
{
    if(s == NULL)
    {
        return;
    }
    printf("%s", s);
}
/****************************** debug ******************************/

/****************************** tensorflow ******************************/

/**
 * @brief 初始化TFLite相机分类器
 * @param void
 * @return void
 * @note 初始化TFLite相机分类器，加载模型并配置解释器
 * @author 毛线
 */
TFLiteCameraClassifier::TFLiteCameraClassifier()
    : model_(nullptr),
      resolver_(),
      interpreter_(nullptr),          // 初始化为空指针
      frame_count_(0),
      first_frame_logged_(false),
      last_inference_time_us_(0),
      max_index_(-1),
      max_confidence_(0.0f),
      is_quantized_(false),
      input_scale_(1.0f),
      input_zero_point_(0),
      output_scale_(1.0f),
      output_zero_point_(0),
      majority_filter_(nullptr)
{

 }

/**
 * @brief 释放TFLite相机分类器资源
 * @param void
 * @return void
 * @note 释放TFLite相机分类器占用的内存资源
 * @author 毛线
 */
TFLiteCameraClassifier::~TFLiteCameraClassifier()
{
    delete interpreter_;  // 释放内存
    if(majority_filter_)
    {
        majority_vote_free(majority_filter_);
        majority_filter_ = nullptr;
    }
}

/**
 * @brief 初始化TFLite相机分类器
 * @param uvc 摄像头对象指针
 * @return bool 是否初始化成功
 * @note 初始化TFLite相机分类器，加载模型并配置解释器
 * @author 毛线
 */
bool TFLiteCameraClassifier::init(zf_device_uvc* uvc)
{
    uvc_ptr_ = uvc;   // 保存指针
    if(uvc_ptr_ == nullptr)
    {
        return false;
    }
    RegisterDebugLogCallback(debug_log_printf_callback);
    tflite::InitializeTarget();
    model_ = tflite::GetModel(loong_cnn_model_simple_tflite);
    if(model_->version() != TFLITE_SCHEMA_VERSION)
    {
        printf("TFLITE: 模型版本不匹配！\n");
        return false;
    }
    majority_filter_ = majority_vote_init(MAJORITY_FILTER_SIZE, TFLITE_OUTPUT_CLASS_NUM);
    if(!majority_filter_)
    {
        printf("TFLITE: 多数投票滤波器初始化失败\n");
        return false;
    }
    // 添加算子
    resolver_.AddConv2D();
    resolver_.AddDepthwiseConv2D();
    resolver_.AddMaxPool2D();
    resolver_.AddFullyConnected();
    resolver_.AddRelu6();
    resolver_.AddSoftmax();
    resolver_.AddReshape();
    resolver_.AddShape();
    resolver_.AddSlice();
    resolver_.AddQuantize();
    resolver_.AddDequantize();
    resolver_.AddCast();
    resolver_.AddSqueeze();
    resolver_.AddExpandDims();
    resolver_.AddConcatenation();
    resolver_.AddTranspose();
    resolver_.AddStridedSlice();
    resolver_.AddPack();
    resolver_.AddMean();
    // 创建解释器
    interpreter_ = new tflite::MicroInterpreter(model_, resolver_, tensor_arena_, TENSOR_ARENA_SIZE);
    if(interpreter_ == nullptr)
    {
        printf("TFLITE: 解释器创建失败！\n");
        return false;
    }
    if(interpreter_->AllocateTensors() != kTfLiteOk)
    {
        printf("TFLITE: 分配张量失败, 张量空间太小!\n");
        return false;
    }
    // 检测输入张量类型，保存量化参数
    TfLiteTensor* input_tensor = interpreter_->input(0);
    TfLiteTensor* output_tensor = interpreter_->output(0);
    if (input_tensor->type == kTfLiteInt8) 
    {
        is_quantized_ = true;
        input_scale_ = input_tensor->params.scale;
        input_zero_point_ = input_tensor->params.zero_point;
        output_scale_ = output_tensor->params.scale;
        output_zero_point_ = output_tensor->params.zero_point;
        printf("TFLITE: 模型量化: int8, input scale=%.6f zp=%d, output scale=%.6f zp=%d\n",
               input_scale_, input_zero_point_, output_scale_, output_zero_point_);
    } else {
        is_quantized_ = false;
        printf("TFLITE: 模型量化: float32\n");
    }
    printf("===== TFLITE模型初始化成功 =====\n");
    return true;
}

/**
 * @brief 捕获一帧图像
 * @param void
 * @return bool 是否捕获成功
 * @note 从摄像头捕获一帧图像，返回是否成功
 * @author 毛线
 */
bool TFLiteCameraClassifier::captureFrame()
{
    if(uvc_ptr_ == nullptr)
    {
        return false;
    }

    if(rgb_image == nullptr)
    {
        return false;
    }

    cv::Mat rgb565_img(UVC_HEIGHT, UVC_WIDTH, CV_8UC2, rgb_image);
    cv::cvtColor(rgb565_img, src_img_, cv::COLOR_BGR5652BGR);
    if(src_img_.empty())
    {
        printf("TFLITE第%d帧:图像为空\n", frame_count_ + 1);
        return false;
    }
    // 提取ROI区域
    if (!get_model_roi(src_img_, roi_img_, roi_rect_))
    {
        return false;
    }
    frame_count_++;
    return true;
}

/**
 * @brief 预处理图像
 * @param void
 * @return bool 是否预处理成功
 * @note 对图像进行缩放、归一化和颜色转换，准备输入模型
 * @author 毛线
 */
bool TFLiteCameraClassifier::preprocessImage()
{
    resize(roi_img_, resized_img_, cv::Size(TFLITE_INPUT_WIDTH, TFLITE_INPUT_HEIGHT), cv::INTER_LINEAR);
    resized_img_.convertTo(float_img_, CV_32FC3, 1.0);  // 若需归一化改为 1.0/255.0
    cvtColor(float_img_, float_img_, cv::COLOR_BGR2RGB);
    if(!float_img_.isContinuous())
    {
        float_img_ = float_img_.clone();
    }
    if(ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged_ && frame_count_ == 1)
    {
        printf("\n===== TFLITE第1帧预处理日志 =====\n");
        printf("原始尺寸：%d x %d\n", src_img_.cols, src_img_.rows);
        printf("原始图像数据类型：%s\n", src_img_.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");
        printf("ROI 矩形:x=%d, y=%d, w=%d, h=%d\n", roi_rect_.x, roi_rect_.y, roi_rect_.width, roi_rect_.height);
        printf("ROI 图像尺寸：%d x %d\n", roi_img_.cols, roi_img_.rows);
        printf("ROI 图像数据类型：%s\n", roi_img_.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");
        printf("缩放后尺寸：%d x %d\n", resized_img_.cols, resized_img_.rows);
        printf("缩放后图像数据类型：%s\n", resized_img_.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");
        printf("输入数据大小：%zu 字节\n", float_img_.total() * float_img_.elemSize());
        first_frame_logged_ = true;
    }
    return true;
}

/**
 * @brief 运行推理
 * @param void
 * @return bool 是否推理成功
 * @note 运行模型推理，返回是否成功
 * @author 毛线
 */
bool TFLiteCameraClassifier::runInference()
{
    if(!interpreter_ || src_img_.empty() || roi_img_.empty())
    {
        return false;
    }
    if(!preprocessImage())
    {
        return false;
    }
    if (is_quantized_) {
        int8_t* input_data = interpreter_->input(0)->data.int8;
        float* float_data = float_img_.ptr<float>(0);
        for (int i = 0; i < TFLITE_INPUT_SIZE; ++i) {
            float val = roundf(float_data[i] / input_scale_ + input_zero_point_);
            val = std::max(-128.0f, std::min(127.0f, val));
            input_data[i] = static_cast<int8_t>(val);
        }
    } else {
        float* input_data = interpreter_->input(0)->data.f;
        memcpy(input_data, float_img_.ptr<float>(0), TFLITE_INPUT_SIZE * sizeof(float));
    }
    auto start = std::chrono::high_resolution_clock::now();
    interpreter_->Invoke();
    auto end = std::chrono::high_resolution_clock::now();
    last_inference_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    parseOutput();
    if (frame_count_ % LOG_PRINT_FREQUENCY == 0 )
    {
        printf("[TFLITE第%d帧] %s (%.4f) | 耗时 %lld us\n",frame_count_, class_labels[max_index_], max_confidence_, last_inference_time_us_);
        printf("ROI 矩形:x=%d, y=%d, w=%d, h=%d\n", roi_rect_.x, roi_rect_.y, roi_rect_.width, roi_rect_.height);
        printf("红色块: 左下(%d,%d) 长：%d 宽：%d\n", bottom_left_x, bottom_left_y, red_block_w, red_block_h);
    }
    return true;
}

/**
 * @brief 解析推理输出
 * @param void
 * @return void
 * @note 解析模型推理输出，提取类别ID和置信度
 * @author 毛线
 */
void TFLiteCameraClassifier::parseOutput()
{
    float dequant_output[TFLITE_OUTPUT_CLASS_NUM];
    float* output;
    if (is_quantized_) {
        int8_t* int8_output = interpreter_->output(0)->data.int8;
        for (int i = 0; i < TFLITE_OUTPUT_CLASS_NUM; ++i) {
            dequant_output[i] = (int8_output[i] - output_zero_point_) * output_scale_;
        }
        output = dequant_output;
    } else {
        output = interpreter_->output(0)->data.f;
    }
    int current_class = 0;
    float current_confidence = output[0];
    max_index_ = 0;
    max_confidence_ = output[0];
    for(int16_t i = 1; i < TFLITE_OUTPUT_CLASS_NUM; ++i)
    {
        if(output[i] > current_confidence)
        {
            current_confidence = output[i];
            current_class = i;
        }
    }
    int16_t filtered_class = majority_vote_update(majority_filter_, current_class);
    max_index_ = filtered_class;
    max_confidence_ = output[filtered_class];
}

/**
 * @brief 重置多数投票滤波器
 * @param void
 * @return void
 * @note 重置多数投票滤波器，清除窗口内的统计信息
 * @author 毛线
 */
void TFLiteCameraClassifier::resetMajorityFilter()
{
    if (majority_filter_)
    {
        // 清空计数数组
        memset(majority_filter_->counts, 0, majority_filter_->max_classes * sizeof(int16_t));
        // 清空历史数组
        memset(majority_filter_->history, 0, majority_filter_->window_size * sizeof(int16_t));
        // 重置指针
        majority_filter_->head = 0;
        majority_filter_->filled = 0;
    }
}
/****************************** tensorflow ******************************/

/****************************** NCNN ******************************/
/**
 * @brief NCNN推理类构造函数
 * @param void
 * @return void
 * @note 初始化NCNN推理器，设置默认输入尺寸、blob名称和归一化参数
 * @author 毛线
 */
NCNNCameraClassifier::NCNNCameraClassifier()
    : initialized_(false),
      input_width_(NCNN_INPUT_WIDTH),
      input_height_(NCNN_INPUT_HEIGHT),
      input_name_("in0"),
      output_name_("out0"),
      majority_filter_(nullptr),
      frame_count_(0),
      first_frame_logged_(false),
      last_inference_time_us_(0),
      max_index_(-1),
      max_confidence_(0.0f)
{
    // 默认使用ImageNet标准归一化
    mean_vals_[0] = 123.675f;
    mean_vals_[1] = 116.28f;
    mean_vals_[2] = 103.53f;
    norm_vals_[0] = 0.01712475f;
    norm_vals_[1] = 0.017507f;
    norm_vals_[2] = 0.01742919f;
}

/**
 * @brief NCNN推理类析构函数
 * @param void
 * @return void
 * @note 释放多数投票滤波器资源（NCNN网络自动管理）
 * @author 毛线
 */
NCNNCameraClassifier::~NCNNCameraClassifier()
{
    if(majority_filter_)
    {
        majority_vote_free(majority_filter_);
        majority_filter_ = nullptr;
    }
}

/**
 * @brief 初始化NCNN模型
 * @param void
 * @return bool 是否初始化成功
 * @note 配置NCNN选项，加载模型参数和权重文件，初始化多数投票滤波器
 * @author 毛线
 */
bool NCNNCameraClassifier::init()
{
    // 模型路径：若未通过SetModelPath设置则使用默认值
    if (param_path_.empty())
    {
        param_path_ = NCNN_DEFAULT_PARAM_PATH;
        bin_path_   = NCNN_DEFAULT_BIN_PATH;
    }

    // 设置NCNN选项
    net_.opt.use_vulkan_compute = false;
    net_.opt.num_threads = NCNN_NUM_THREADS;

    // 加载模型参数
    if (net_.load_param(param_path_.c_str()) != 0)
    {
        printf("NCNN: 加载参数文件失败: %s\n", param_path_.c_str());
        return false;
    }

    // 加载模型权重
    if (net_.load_model(bin_path_.c_str()) != 0)
    {
        printf("NCNN: 加载模型文件失败: %s\n", bin_path_.c_str());
        return false;
    }

    initialized_ = true;
    majority_filter_ = majority_vote_init(MAJORITY_FILTER_SIZE, NCNN_OUTPUT_CLASS_NUM);
    if(!majority_filter_)
    {
        printf("NCNN: 多数投票滤波器初始化失败\n");
        return false;
    }
    printf("===== NCNN模型初始化成功 =====\n");
    return true;
}

/**
 * @brief 捕获一帧图像
 * @param void
 * @return bool 是否捕获成功
 * @note 从全局rgb_image获取摄像头帧，转换为BGR后调用get_model_roi提取ROI
 * @author 毛线
 */
bool NCNNCameraClassifier::captureFrame()
{
    if (!initialized_)
    {
        printf("NCNN: 模型未初始化，请先调用 init()\n");
        return false;
    }

    if (rgb_image == nullptr)
    {
        return false;
    }

    cv::Mat rgb565_img(UVC_HEIGHT, UVC_WIDTH, CV_8UC2, rgb_image);
    cv::cvtColor(rgb565_img, src_img_, cv::COLOR_BGR5652BGR);
    if (src_img_.empty())
    {
        printf("NCNN: 第%d帧:图像为空\n", frame_count_ + 1);
        return false;
    }

    // 提取ROI区域
    if (!get_model_roi(src_img_, roi_img_, roi_rect_))
    {
        return false;
    }
    frame_count_++;
    return true;
}

/**
 * @brief 预处理图像
 * @param void
 * @return bool 是否预处理成功
 * @note 缩放、BGR→RGB转换、from_pixels、归一化，准备输入模型
 * @author 毛线
 */
bool NCNNCameraClassifier::preprocessImage()
{
    cv::resize(roi_img_, resized_img_, cv::Size(input_width_, input_height_));
    cv::cvtColor(resized_img_, rgb_img_, cv::COLOR_BGR2RGB);

    // 转换为NCNN格式并归一化
    ncnn_input_ = ncnn::Mat::from_pixels
    (
        rgb_img_.data,
        ncnn::Mat::PIXEL_RGB,
        input_width_,
        input_height_
    );
    ncnn_input_.substract_mean_normalize(mean_vals_, norm_vals_);

    if(ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged_ && frame_count_ == 1)
    {
        printf("\n===== NCNN第1帧预处理日志 =====\n");
        printf("原始尺寸：%d x %d\n", src_img_.cols, src_img_.rows);
        printf("原始图像数据类型：%s\n", src_img_.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");
        printf("ROI 矩形:x=%d, y=%d, w=%d, h=%d\n", roi_rect_.x, roi_rect_.y, roi_rect_.width, roi_rect_.height);
        printf("ROI 图像尺寸：%d x %d\n", roi_img_.cols, roi_img_.rows);
        printf("ROI 图像数据类型：%s\n", roi_img_.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");
        printf("缩放后尺寸：%d x %d\n", resized_img_.cols, resized_img_.rows);
        printf("NCNN输入尺寸：%d x %d x %d\n", ncnn_input_.w, ncnn_input_.h, ncnn_input_.c);
        printf("归一化参数: mean=[%.2f, %.2f, %.2f] norm=[%.6f, %.6f, %.6f]\n",
               mean_vals_[0], mean_vals_[1], mean_vals_[2],
               norm_vals_[0], norm_vals_[1], norm_vals_[2]);
        first_frame_logged_ = true;
    }
    return true;
}

/**
 * @brief 运行推理
 * @param void
 * @return bool 是否推理成功
 * @note 调用预处理、执行NCNN推理、计时、解析输出
 * @author 毛线
 */
bool NCNNCameraClassifier::runInference()
{
    if (!initialized_ || roi_img_.empty())
    {
        return false;
    }

    if (!preprocessImage())
    {
        return false;
    }

    // 推理计时
    auto start = std::chrono::high_resolution_clock::now();
    ncnn::Extractor ex = net_.create_extractor();
    ex.input(input_name_.c_str(), ncnn_input_);

    ncnn::Mat logits;
    ex.extract(output_name_.c_str(), logits);
    auto end = std::chrono::high_resolution_clock::now();
    last_inference_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    parseOutput(logits);

    if (frame_count_ % LOG_PRINT_FREQUENCY == 0)
    {
        printf("[NCNN第%d帧] %s (%.4f) | 耗时 %lld us\n",
               frame_count_, getPredictionLabel(), max_confidence_, last_inference_time_us_);
        printf("ROI 矩形:x=%d, y=%d, w=%d, h=%d\n", roi_rect_.x, roi_rect_.y, roi_rect_.width, roi_rect_.height);
        printf("红色块: 左下(%d,%d) 长：%d 宽：%d\n", bottom_left_x, bottom_left_y, red_block_w, red_block_h);
    }
    return true;
}

/**
 * @brief 解析推理输出
 * @param logits NCNN输出矩阵
 * @return void
 * @note 从输出logits中提取类别索引和置信度，经多数投票滤波后存入成员变量
 * @author 毛线
 */
void NCNNCameraClassifier::parseOutput(const ncnn::Mat& logits)
{
    // Softmax: 将logits转为概率分布
    float softmax[NCNN_OUTPUT_CLASS_NUM];
    float exp_sum = 0.0f;
    for (int i = 0; i < logits.w && i < NCNN_OUTPUT_CLASS_NUM; ++i)
    {
        softmax[i] = exp(logits[i]);
        exp_sum += softmax[i];
    }
    for (int i = 0; i < logits.w && i < NCNN_OUTPUT_CLASS_NUM; ++i)
    {
        softmax[i] /= exp_sum;
    }

    // 获取最大概率类别
    int class_id = 0;
    float best_prob = softmax[0];
    for (int i = 1; i < logits.w && i < NCNN_OUTPUT_CLASS_NUM; ++i)
    {
        if (softmax[i] > best_prob)
        {
            best_prob = softmax[i];
            class_id = i;
        }
    }

    // 多数投票滤波
    int16_t filtered_class = majority_vote_update(majority_filter_, class_id);

    max_index_ = filtered_class;
    max_confidence_ = (filtered_class >= 0 && filtered_class < static_cast<int>(logits.w))
                      ? softmax[filtered_class] : best_prob;
}

/**
 * @brief 重置多数投票滤波器
 * @param void
 * @return void
 * @note 清除窗口内的计数、历史和指针，避免跨周期误判
 * @author 毛线
 */
void NCNNCameraClassifier::resetMajorityFilter()
{
    if (majority_filter_)
    {
        memset(majority_filter_->counts, 0, majority_filter_->max_classes * sizeof(int16_t));
        memset(majority_filter_->history, 0, majority_filter_->window_size * sizeof(int16_t));
        majority_filter_->head = 0;
        majority_filter_->filled = 0;
    }
}

/**
 * @brief 获取预测类别标签
 * @param void
 * @return const char* 预测类别名称，非法时返回"Unknown"
 * @note 根据max_index_从labels_中获取对应的类别名称
 * @author 毛线
 */
const char* NCNNCameraClassifier::getPredictionLabel() const
{
    if (max_index_ >= 0 && max_index_ < static_cast<int>(labels_.size()))
    {
        return labels_[max_index_].c_str();
    }
    return "Unknown";
}

/**
 * @brief 设置模型路径
 * @param param_path 模型参数文件路径(.param)
 * @param bin_path   模型权重文件路径(.bin)
 * @return void
 * @note 应在 init() 之前调用，不设置则使用默认路径
 * @author 毛线
 */
void NCNNCameraClassifier::SetModelPath(const std::string& param_path, const std::string& bin_path)
{
    param_path_ = param_path;
    bin_path_ = bin_path;
}

/**
 * @brief 设置输入尺寸
 * @param width  输入宽度
 * @param height 输入高度
 * @return void
 * @note 应在 init() 之前调用
 * @author 毛线
 */
void NCNNCameraClassifier::SetInputSize(int width, int height)
{
    input_width_ = width;
    input_height_ = height;
}

/**
 * @brief 设置类别标签
 * @param labels 类别名称列表
 * @return void
 * @note 应在 init() 之前调用，需与模型输出顺序一致
 * @author 毛线
 */
void NCNNCameraClassifier::SetLabels(const std::vector<std::string>& labels)
{
    labels_ = labels;
}

/**
 * @brief 设置归一化参数
 * @param mean_vals 均值数组（3个值，对应RGB三通道）
 * @param norm_vals 标准差数组（3个值，对应RGB三通道）
 * @return void
 * @note 应在 init() 之前调用，默认使用ImageNet标准归一化
 * @author 毛线
 */
void NCNNCameraClassifier::SetNormalize(const float mean_vals[3], const float norm_vals[3])
{
    mean_vals_[0] = mean_vals[0];
    mean_vals_[1] = mean_vals[1];
    mean_vals_[2] = mean_vals[2];
    norm_vals_[0] = norm_vals[0];
    norm_vals_[1] = norm_vals[1];
    norm_vals_[2] = norm_vals[2];
}
/****************************** NCNN ******************************/
