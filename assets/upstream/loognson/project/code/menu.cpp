#include "menu.h"

/****************************** Menu Config ******************************
Up Button: + (移动/增加)
Down Button: - (移动/减少)
Left Button: 进入子菜单 (若是文件夹) / 锁定编辑 (若是参数)
Right Button: 退出子菜单 (若是文件夹) / 退出编辑 (若是参数)
Mid Button: 确认选择
 ****************************** Menu Config ******************************/

// Menu initial
UI_menu first_totalnode; // 根菜单
static uint8 menu_array_index = 0; // 当前分配菜单的节点数量
UI_menu *last_totalnode = NULL; // 上一次菜单层级
UI_menu first_subnode, second_subnode, third_subnode, fourth_subnode, fifth_subnode; // 子菜单
UI_menu *arrow; // 当前选中的菜单指针(光标)
UI_menu *last_arrow; // 上一次选中的菜单项
static bool arrow_lock = false; // 箭头是否处于“编辑”状态 = arrow->select
static uint8 step_index = STEP_DEFAULT_NUMBER; // 当前步进值索引
static float step_unit[STEP_LENGTH] = {0.02, 0.05, 0.1, 1.0, 2.0, 5.0, 10.0}; // 可调节的步进值
const char* menu_action_name = NULL; // 菜单动作名称
static bool departure_dialog_show = false; // 发车确认对话框显示标志
// Flash initial
static UI_menu menu_array[MENU_MAX_SIZE]; // 存储菜单节点
uint8_t Flash_Page; // 当前使用FLASH的页号
// Button initial
uint8_t key_num = 0;
zf_driver_gpio  key_1(KEY_1_PATH, O_RDWR);
zf_driver_gpio  key_2(KEY_2_PATH, O_RDWR);
zf_driver_gpio  key_3(KEY_3_PATH, O_RDWR);
zf_driver_gpio  key_4(KEY_4_PATH, O_RDWR);
zf_driver_gpio  key_5(KEY_5_PATH, O_RDWR);
zf_driver_gpio *keys[5] = {&key_1, &key_2, &key_3, &key_4, &key_5}; // 按键指针数组

static int16_t calculate_string_width(const char *string);
static void fill_rect(uint16 x, uint16 y, uint16 w, uint16 h, uint16 color);

/**
 * @brief 初始化菜单系统
 * @param void
 * @return void
 * @note 初始化菜单系统，包括根菜单、子菜单、参数菜单、文件夹菜单、箭头光标、参数菜单指针数组等
 * @author YuYu
 */
void UI_menu_init(void)
{
    ips200.show_rgb565_image(0, 40, (const uint16 *)gImage_1, 240, 194, 240, 194, 0); // 少爷
    system_delay_ms(1000);
    ips200.clear();
    ips200.show_rgb565_image(0, 20, (const uint16 *)gImage_2, 239, 240, 239, 240, 0); // 管家
    system_delay_ms(1000);
    ips200.clear();
    // ips200.show_rgb565_image(0, 140, (const uint16 *)gImage_3, 197, 240, 197, 240, 0); // 原神启动
    // system_delay_ms(1000);
    ips200.clear();
    // 初始化根菜单
    first_totalnode.data = NULL;
    first_totalnode.totalnode = NULL;
    first_totalnode.subnode = NULL;
    first_totalnode.nextnode = NULL;
    first_totalnode.lastnode = NULL;
    first_totalnode.type = menu_folder;
    first_totalnode.name = "UI menu";
    first_totalnode.sequence_number = 0;
    first_totalnode.select = false;
    first_totalnode.member = 0;
    last_arrow = NULL;
    // 初始化子菜单
    UI_menu *repository_first = creat_UI_menu_folder(&first_totalnode, "Constant Parameter"); // 常量参数
    UI_menu *repository_second = creat_UI_menu_folder(&first_totalnode, "Image Parameter"); // 图像参数
    UI_menu *repository_third = creat_UI_menu_folder(&first_totalnode, "PID Parameter"); // PID参数
    UI_menu *repository_fourth = creat_UI_menu_folder(&first_totalnode, "Element Parameter"); // 元素参数
    UI_menu *repository_fifth = creat_UI_menu_folder(&first_totalnode, "Model Parameter"); // 模型参数
    UI_menu *repository_sixth = creat_UI_menu_folder(&first_totalnode, "Restore Original"); // 恢复默认参数
    // 初始化子菜单
    if(repository_first != NULL) // Constant Parameter 目录
    {
        creat_UI_menu_parameter(repository_first, "Set Speed", &set_speed, int16_data);
        creat_UI_menu_parameter(repository_first, "BLDC Line", &angle_line.BLDC, uint8_data);
        creat_UI_menu_parameter(repository_first, "Far Line", &angle_line.far, uint8_data);
    }
    if(repository_second != NULL) // Image Parameter 目录
    {
        creat_UI_menu_folder(repository_second, "Color Image");
        creat_UI_menu_folder(repository_second, "Gray Image");
        creat_UI_menu_folder(repository_second, "Binary Image");
        creat_UI_menu_folder(repository_second, "Customized Image");
    }
    if(repository_third != NULL) // PID Parameter 目录
    {
        // 初始化子菜单 PID Parameter
        UI_menu *image_loop = creat_UI_menu_folder(repository_third, "Image Loop");
        UI_menu *angle_loop = creat_UI_menu_folder(repository_third, "Angle Loop");
        UI_menu *acc_loop = creat_UI_menu_folder(repository_third, "ACC Loop");
        UI_menu *speed_loop = creat_UI_menu_folder(repository_third, "Speed Loop");
        if(image_loop != NULL) // Image Loop 目录
        {
            creat_UI_menu_parameter(image_loop, "Kp1", &differ_Kp1[SET_DIFFER_TYPE], float_data);
            creat_UI_menu_parameter(image_loop, "Kp2", &differ_Kp2[SET_DIFFER_TYPE], float_data);
            creat_UI_menu_parameter(image_loop, "Kp3", &differ_Kp3[SET_DIFFER_TYPE], float_data);
            creat_UI_menu_parameter(image_loop, "Kd", &differ_Kd[SET_DIFFER_TYPE], float_data);
        }
        if(angle_loop != NULL) // Angle Loop 目录
        {
            creat_UI_menu_parameter(angle_loop, "Kp", &differ_dir_outer.Kp, float_data);
            creat_UI_menu_parameter(angle_loop, "Ki", &differ_dir_outer.Ki, float_data);
            creat_UI_menu_parameter(angle_loop, "Kd", &differ_dir_outer.Kd, float_data);
        }
        if(acc_loop != NULL) // ACC Loop 目录
        {
            creat_UI_menu_parameter(acc_loop, "Kp", &gyro_dir_inner.Kp, float_data);
            creat_UI_menu_parameter(acc_loop, "Ki", &gyro_dir_inner.Ki, float_data);
            creat_UI_menu_parameter(acc_loop, "Kd", &gyro_dir_inner.Kd, float_data);
        }
        if(speed_loop != NULL) // Speed Loop 目录
        {
            creat_UI_menu_parameter(speed_loop, "Kp", &left_speed.Kp, float_data);
            creat_UI_menu_parameter(speed_loop, "Ki", &left_speed.Ki, float_data);
            creat_UI_menu_parameter(speed_loop, "Kd", &left_speed.Kd, float_data);
            creat_UI_menu_parameter(speed_loop, "Kp", &right_speed.Kp, float_data);
            creat_UI_menu_parameter(speed_loop, "Ki", &right_speed.Ki, float_data);
            creat_UI_menu_parameter(speed_loop, "Kd", &right_speed.Kd, float_data);
        }
    }
    if(repository_fourth != NULL) // Element Parameter 目录
    {
        // 初始化子菜单 Element Parameter
        UI_menu *zebra_line = creat_UI_menu_folder(repository_fourth, "Zebra Line");
        UI_menu *cross_road = creat_UI_menu_folder(repository_fourth, "Cross Road");
        UI_menu *obstacle = creat_UI_menu_folder(repository_fourth, "Obstacle");
        UI_menu *ramp = creat_UI_menu_folder(repository_fourth, "Ramp");
        UI_menu *island = creat_UI_menu_folder(repository_fourth, "Island");
        if(zebra_line != NULL) // Zebra Line 目录
        {
            creat_UI_menu_parameter(zebra_line, "Flag", &judge_zebra_line.flag, uint8_data);
        }
        if(cross_road != NULL) // Cross Road 目录
        {
            creat_UI_menu_parameter(cross_road, "Flag", &judge_crossroad.cross_flag, bool_data);
            creat_UI_menu_parameter(cross_road, "Cross Left Flag", &judge_crossroad.cross_left_flag, bool_data);
            creat_UI_menu_parameter(cross_road, "Cross Right Flag", &judge_crossroad.cross_right_flag, bool_data);
        }
        if(obstacle != NULL) // Obstacle 目录
        {
            UI_menu *left_obstacle = creat_UI_menu_folder(obstacle, "Left Obstacle");
            UI_menu *right_obstacle = creat_UI_menu_folder(obstacle, "Right Obstacle");
            if(left_obstacle != NULL)
            {
                creat_UI_menu_parameter(left_obstacle, "Flag", &judge_roadblock.left_roadblock_flag, bool_data);
            }
            if(right_obstacle != NULL)
            {
                creat_UI_menu_parameter(right_obstacle, "Flag", &judge_roadblock.right_roadblock_flag, bool_data);
            }
        }
        if(ramp != NULL) // Ramp 目录
        {
            creat_UI_menu_parameter(ramp, "Flag", &judge_ramp.flag, bool_data);
            creat_UI_menu_parameter(ramp, "Left Width", &judge_ramp.left_width, uint8_data);
            creat_UI_menu_parameter(ramp, "Right Width", &judge_ramp.right_width, uint8_data);
        }
        if(island != NULL) // Island 目录
        {
            UI_menu *left_island = creat_UI_menu_folder(island, "Left Island");
            UI_menu *right_island = creat_UI_menu_folder(island, "Right Island");
            if(left_island != NULL)
            {
                creat_UI_menu_parameter(left_island, "Left Type", &judge_island.left_island_type, uint8_data);
            }
            if(right_island != NULL)
            {
                creat_UI_menu_parameter(right_island, "Right Type", &judge_island.right_island_type, uint8_data);
            }
        }
    }
    if(repository_fifth != NULL) // Model Parameter 目录
    {
        creat_UI_menu_folder(repository_fifth, "Model Detect");
    }
    if(repository_sixth != NULL) // Restore Original 目录
    {
        // UI_menu *restore = creat_UI_menu_folder(repository_sixth, "Restore");
    }
    // 初始化箭头光标
    if(first_totalnode.subnode)
    {
        arrow = first_totalnode.subnode;
    }
    else
    {
        arrow = &first_totalnode;
    }
    link_data_init(&first_totalnode); // 双向循环链表连接
}

/**
 * @brief 绘制发车确认对话框
 * @param void
 * @return void
 * @note 在屏幕中央绘制"是否发车"对话框，"是"对应左键，"否"对应右键
 * @author YuYu
 */
static void draw_departure_dialog(void)
{
    uint16 box_x = 20, box_y = 20, box_w = 200, box_h = 100;
    fill_rect(box_x, box_y, box_w, box_h, RGB565_BLACK);
    ips200.draw_line(box_x, box_y, box_x + box_w - 1, box_y, RGB565_YELLOW);
    ips200.draw_line(box_x, box_y + box_h - 1, box_x + box_w - 1, box_y + box_h - 1, RGB565_YELLOW);
    ips200.draw_line(box_x, box_y, box_x, box_y + box_h - 1, RGB565_YELLOW);
    ips200.draw_line(box_x + box_w - 1, box_y, box_x + box_w - 1, box_y + box_h - 1, RGB565_YELLOW);
    ips200.show_string(box_x + 15, box_y + 15, "do you want to start?");
    ips200.show_string(box_x + 25, box_y + 70, "yes");
    ips200.show_string(box_x + 160, box_y + 70, "no");
}

/**
 * @brief 显示菜单系统
 * @param void
 * @return void
 * @note 显示菜单系统，包括根菜单、子菜单、参数菜单、文件夹菜单、箭头光标、参数菜单指针数组等
 * @author YuYu、mx
 */
void UI_menu_display(void)
{
    if(system_self_lock == true) // 已经按下退出菜单则不再进入菜单系统
    {
        return;
    }
    // 初始显示
    if(!arrow || !arrow->totalnode) // 回退到根菜单系统
    {
        arrow = first_totalnode.subnode;
        if(!arrow) 
        {
            arrow = &first_totalnode;
        }
        last_arrow = NULL;
        last_totalnode = NULL;
        arrow_lock = false;
        ips200.full(RGB565_BLACK); // 黑色背景
    }
    display_title(); // 显示当前菜单标题
    display_menu(); // 显示当前菜单
    while(1)
    {
        if(system_self_lock == true) // 退出菜单系统
        {
            ips200.full(RGB565_BLACK); // 黑色背景
            break;
        }
        if(departure_dialog_show == true) // 发车确认对话框
        {
            key_num = key_scan();
            if(key_num == 4) // 左键 - 是
            {
                departure_dialog_show = false;
                system_self_lock = true; // 退出菜单系统
                ips200.full(RGB565_BLACK);
                break;
            }
            else if(key_num == 5) // 右键 - 否
            {
                departure_dialog_show = false;
                ips200.full(RGB565_BLACK);
                display_title();
                display_menu();
            }
            system_delay_ms(20);
            continue;
        }
        if(menu_action_name != NULL) // 图像显示模式
        {
            if(uvc_dev.wait_image_refresh() == 0)
            {
                uint16_t* rgb_img = uvc_dev.get_rgb_image_ptr();
                uint8_t* gray_img = uvc_dev.get_gray_image_ptr(); // 获取灰度图像指针
                uint8_t binary_img[ROW * COL];
                if(strcmp(menu_action_name, "Color Image") == 0 && rgb_img != NULL)
                {
                    ips200.displayimage_rgb565(rgb_img, UVC_WIDTH, UVC_HEIGHT);
                }
                else if(strcmp(menu_action_name, "Gray Image") == 0 && gray_img != NULL)
                {
                    ips200.displayimage_gray(gray_img, COL, ROW);
                }
                else if(strcmp(menu_action_name, "Binary Image") == 0 && gray_img != NULL)
                {
                    memcpy(binary_img, gray_img, COL * ROW * sizeof(uint8_t));
                    cv::Mat menu_binary_original(ROW, COL, CV_8UC1, binary_img);
                    float OTSU_threshold = cv::threshold(menu_binary_original, menu_binary_original, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                    float adjusted_threshold = OTSU_threshold - adaption_threshold;
                    cv::threshold(menu_binary_original, menu_binary_original, adjusted_threshold, 255, cv::THRESH_BINARY);
                    ips200.displayimage_gray(binary_img, COL, ROW);
                }
                else if(strcmp(menu_action_name, "Customized Image") == 0 && gray_img != NULL)
                {
                    memcpy(image_buffer[0], gray_img, COL * ROW * sizeof(uint8_t));
                    cv::Mat menu_customized_original(ROW, COL, CV_8UC1, image_buffer[0]);
                    cv::threshold(menu_customized_original, menu_customized_original,
                    cv::threshold(menu_customized_original, menu_customized_original, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU) - adaption_threshold, 255, cv::THRESH_BINARY);
                    gray_image = gray_img; // 更新灰度图像指针
                    search_image_edge();
                    obtain_middle_line();
                    display_image();
                    ips200.displayimage_gray(image_buffer[0], COL, ROW);
                }
                else if(strcmp(menu_action_name, "Model Detect") == 0 && rgb_img != NULL && gray_img != NULL)
                {
                    bool roi_valid = false;
                    // 保存关键状态，避免影响主循环的模型识别逻辑
                    bool saved_detect = stop.detect_RGB_red;
                    int16_t saved_blx = bottom_left_x, saved_bly = bottom_left_y;
                    int16_t saved_brw = red_block_w, saved_brh = red_block_h;
                    float saved_area = model_area;
                    uint8_t saved_coord_flag = model_coord_flag;
                    uint8_t saved_block_x = model_control.block_x;
                    uint8_t saved_block_y = model_control.block_y;
                    bool saved_return_flag = model_control.return_flag;
                    // 处理灰度图像：二值化 + 边界搜索 + 中线提取
                    memcpy(image_buffer[0], gray_img, COL * ROW * sizeof(uint8_t));
                    cv::Mat menu_binary(ROW, COL, CV_8UC1, image_buffer[0]);
                    float OTSU_threshold = cv::threshold(menu_binary, menu_binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                    cv::threshold(menu_binary, menu_binary, OTSU_threshold - adaption_threshold, 255, cv::THRESH_BINARY);
                    gray_image = gray_img; // 更新灰度图像指针
                    rgb_image = rgb_img; // 更新彩色图像指针
                    search_image_edge();
                    obtain_middle_line();
                    // 检测红色色块并计算模型ROI区域
                    model_control.return_flag = false;
                    if(detect_red_block(DETECT_MIN_ROW, DETECT_MAX_ROW, DETECT_MIN_COLUMN, DETECT_MAX_COLUMN))
                    {
                        if(model_area > DEAL_MODEL_AREA && model_area < MAX_MODEL_AREA && Active_classifier.captureFrame())
                        {
                            Active_classifier.runInference();
                            roi_valid = true;
                        }
                    }
                    // 恢复关键状态
                    stop.detect_RGB_red = saved_detect;
                    bottom_left_x = saved_blx; bottom_left_y = saved_bly;
                    red_block_w = saved_brw; red_block_h = saved_brh;
                    model_area = saved_area;
                    model_coord_flag = saved_coord_flag;
                    model_control.block_x = saved_block_x;
                    model_control.block_y = saved_block_y;
                    model_control.return_flag = saved_return_flag;
                    // 显示彩色图像并叠加ROI矩形框
                    ips200.displayimage_rgb565(rgb_img, UVC_WIDTH, UVC_HEIGHT);
                    if(roi_valid == true && Active_classifier.hasRoiRect())
                    {
                        cv::Rect r = Active_classifier.getRoiRect();
                        int x0 = r.x, y0 = r.y, x1 = r.x + r.width - 1, y1 = r.y + r.height - 1;
                        ips200.draw_line(x0, y0, x1, y0, RGB565_RED);
                        ips200.draw_line(x0, y1, x1, y1, RGB565_RED);
                        ips200.draw_line(x0, y0, x0, y1, RGB565_RED);
                        ips200.draw_line(x1, y0, x1, y1, RGB565_RED);
                        // 在ROI框上方绘制标签
                        char label_text[32];
                        snprintf(label_text, sizeof(label_text), "%s %.2f", Active_classifier.getPredictionLabel(), Active_classifier.getPredictionConfidence());
                        int16_t label_w = calculate_string_width(label_text);
                        int16_t label_x = x0;
                        int16_t label_y = y0 - SCREEN_FONT_SIZE;
                        if(label_y < 0) label_y = y0;
                        // 若标签超出屏幕右边界，则反方向以ROI框右边为基准向左扩展
                        if((label_x + label_w + 4) > SCREEN_WIDTH) label_x = (x1 - label_w - 4);
                        if(label_x < 0) label_x = 0;
                        fill_rect(label_x, label_y, label_w + 4, SCREEN_FONT_SIZE, RGB565_RED);
                        ips200.set_pen_color(RGB565_WHITE);
                        ips200.set_background_color(RGB565_RED);
                        ips200.show_string(label_x + 2, label_y, label_text);
                        ips200.set_pen_color(DEFAULT_PENCOLOR);
                        ips200.set_background_color(DEFAULT_BGCOLOR);
                        // 清除菜单模式下产生的多数投票历史，避免污染主循环推理
                        Active_classifier.resetMajorityFilter();
                    }
                }
            }
            uint8_t key = key_scan();
            if(key == 5)
            {
                menu_action_name = NULL;
                ips200.full(RGB565_BLACK);
                display_title();
                display_menu();
            }
            system_delay_ms(20);
            continue;
        }
        if(!arrow || !arrow->totalnode) // 回退到根菜单系统
        {
            arrow = first_totalnode.subnode;
            if(!arrow) 
            {
                arrow = &first_totalnode;
            }
            last_arrow = NULL;
            last_totalnode = NULL;
            arrow_lock = false;
            ips200.full(RGB565_BLACK); // 黑色背景
            display_title(); // 显示当前菜单标题
            display_menu(); // 显示当前菜单
        }
        key_num = key_scan(); // 扫描按键
        if(key_num != 0) // 有按键按下
        {
            arrow_scan(); // 扫描方向键
            if(menu_action_name != NULL)
            {
                continue;
            }
            if(departure_dialog_show == true)
            {
                continue; // 对话框已显示，跳过菜单重绘
            }
            ips200.full(RGB565_BLACK); // 黑色背景
            display_title(); // 显示当前菜单标题
            display_menu(); // 显示当前菜单
        }
        system_delay_ms(20); // 延时，防止CPU占用过高
    }
}

/**
 * @brief 创建菜单项
 * @param totalnode 父菜单指针
 * @param nownode 子菜单指针
 * @param name 菜单项名称
 * @param data 菜单项数据
 * @param type 菜单项类型
 * @return void
 * @note 创建菜单项，包括子菜单项、参数菜单项、文件夹菜单项等
 * @author YuYu
 */
void creat_UI_menu(UI_menu *totalnode, UI_menu *nownode, const char name[], void *data, menu_type type)
{
    if(totalnode->type != menu_folder) // 文件夹才能添加子项
    {
        return;
    }
    // 初始化子菜单项
    nownode->name = name;
    nownode->member = 0;
    nownode->totalnode = totalnode;
    nownode->subnode = NULL;
    nownode->nextnode = NULL;
    nownode->lastnode = NULL;
    nownode->data = data;
    nownode->type = type;
    // 添加子菜单项
    if(totalnode->member == 0)
    {
        totalnode->subnode = nownode; // 第一个子节点
    }
    else
    {
        UI_menu *p = totalnode->subnode;
        while(p->nextnode != NULL)
        {
            p = p->nextnode;
        }
        p->nextnode = nownode;
        nownode->lastnode = p;
    }
    nownode->sequence_number = totalnode->member;
    totalnode->member++; // 父节点成员数加一
}

/**
 * @brief 创建文件夹菜单项
 * @param totalnode 父菜单指针
 * @param name 菜单项名称
 * @return UI_menu* 文件夹菜单项指针
 * @note 创建文件夹菜单项，包括子菜单项、参数菜单项、文件夹菜单项等
 * @author YuYu
 */
UI_menu* creat_UI_menu_folder(UI_menu *totalnode, const char name[])
{
    if(menu_array_index >= MENU_MAX_SIZE)
    {
        return NULL;
    }
    UI_menu *nownode = &menu_array[menu_array_index++];
    creat_UI_menu(totalnode, nownode, name, NULL, menu_folder);
    return nownode;
}

/**
 * @brief 创建参数菜单项
 * @param totalnode 父菜单指针
 * @param name 菜单项名称
 * @param data 菜单项数据
 * @param type 菜单项类型
 * @return void
 * @note 创建参数菜单项，包括子菜单项、参数菜单项、文件夹菜单项等
 * @author YuYu
 */
void creat_UI_menu_parameter(UI_menu *totalnode, const char name[], void *data, menu_type type)
{
    if(menu_array_index >= MENU_MAX_SIZE)
    {
        return;
    }
    UI_menu *nownode = &menu_array[menu_array_index++];
    creat_UI_menu(totalnode, nownode, name, data, type);
}

/**
 * @brief 初始化双向循环链表
 * @param arr 菜单指针
 * @return void
 * @note 初始化双向循环链表，包括子菜单项、参数菜单项、文件夹菜单项等
 * @author YuYu
 */
void link_data_init(UI_menu *arr)
{
    if(!arr || !arr->subnode)
    {
        return;
    }
    UI_menu *first = arr->subnode;
    UI_menu *current = first;
    while(current->nextnode) // 先连接 next/last (形成双向链表)
    {
        current->nextnode->lastnode = current;
        current = current->nextnode;
    }
    first->lastnode = current;
    current->nextnode = first; // 循环闭环
    current = first;
    do{
        if(current->type == menu_folder)
        {
            link_data_init(current); // 递归处理子菜单
        }
        current = current->nextnode;
    }while(current != first);
}

/**
 * @brief 计算字符串宽度
 * @param string 字符串指针
 * @return int16_t 字符串宽度，单位为像素
 * @note 计算字符串宽度，包括 ASCII 字符、2 字节 UTF-8（如带音标的拉丁文，不常见）和3 字节 UTF-8（中文）
 * @author YuYu
 */
static int16_t calculate_string_width(const char *string)
{
    int16_t width = 0;
    for(int16_t i = 0; string[i] != '\0'; i++)
    {
        unsigned char length = string[i];
        if(length < 0x80) // ASCII 字符：8 像素宽
        {
            width += 8;
        }
        else if((length & 0xE0) == 0xC0 && string[i + 1] != '\0') // 2 字节 UTF-8（如带音标的拉丁文，不常见）：16 像素
        {
            width += 16;
            i++;
        }
        else if((length & 0xF0) == 0xE0 && string[i+1] != '\0' && string[i+2] != '\0') // 3 字节 UTF-8（中文）：16 像素
        {
            width += 16;
            i += 2;
        }
    }
    return width;
}

/**
 * @brief 显示当前菜单标题
 * @param void
 * @return void
 * @note 显示当前菜单标题，包括根菜单、子菜单、参数菜单、文件夹菜单等
 * @author YuYu
 */
void display_title(void)
{
    if(!arrow || !arrow->totalnode)
    {
        return;
    }
    if(arrow_lock)
    {
        ips200.show_float(0, 0, step_unit[step_index], 1, 2);
    }
    const char *title = "UI MENU";
    if(arrow != NULL && arrow->totalnode != NULL)
    {
        title = arrow->totalnode->name;
    }
    int16_t length = calculate_string_width(title);
    int8_t x = (SCREEN_WIDTH - length) / 2;
    ips200.show_string(x, (0 * SCREEN_FONT_SIZE), title);
    // 标题分隔线
    ips200.draw_line(0, (SCREEN_FONT_SIZE + 1), SCREEN_WIDTH, (SCREEN_FONT_SIZE + 1), DEFAULT_PENCOLOR);
    ips200.draw_line(0, (SCREEN_FONT_SIZE + 2), SCREEN_WIDTH, (SCREEN_FONT_SIZE + 2), DEFAULT_PENCOLOR);
}

/**
 * @brief 填充矩形
 * @param x 矩形左上角 x 坐标
 * @param y 矩形左上角 y 坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 埫充颜色
 * @return void
 * @note 填充矩形，包括边框和内部
 * @author YuYu
 */
static void fill_rect(uint16 x, uint16 y, uint16 w, uint16 h, uint16 color)
{
    for(uint16 row = y; row < y + h; row++)
    {
        ips200.draw_line(x, row, x + w - 1, row, color);
    }
}

/**
 * @brief 显示当前菜单
 * @param void
 * @return void
 * @note 显示当前菜单，包括子菜单项、参数菜单项、文件夹菜单项等
 * @author YuYu
 */
void display_menu(void)
{
    if(!arrow || !arrow->totalnode)
    {
        return;
    }
    UI_menu *h = arrow->totalnode;  // 当前要显示的菜单层级
    UI_menu *s = h->subnode;
    for(int16_t i = 0; i < h->member; i++)
    {
        if(s == NULL) 
        {
            break;
        }
        int16_t y_pos = (i + 1) * SCREEN_FONT_SIZE + 4;
        if(s == arrow)
        {
            int16_t name_width = calculate_string_width(s->name);
            int16_t highlight_left = 25;
            if(highlight_left < 0) 
            {
                highlight_left = 0;
            }
            int16_t highlight_right = 35 + name_width;
            fill_rect(highlight_left, y_pos + 1, highlight_right - highlight_left, 18, RGB565_WHITE);
            ips200.set_pen_color(RGB565_BLACK);
            ips200.set_background_color(RGB565_WHITE);
        }
        // 显示菜单项名称
        ips200.show_string(30, y_pos + 2, s->name);
        if(s == arrow)
        {
            ips200.set_pen_color(DEFAULT_PENCOLOR);
            ips200.set_background_color(DEFAULT_BGCOLOR);
        }
        // 显示选中标记
        if(s->select)
        {
            ips200.show_string((SCREEN_WIDTH - 20), y_pos, "<<");
        }
        else
        {
            ips200.show_string((SCREEN_WIDTH - 20), y_pos, "  ");
        }
        // 显示菜单项数值
        switch(s->type)
        {
            case int8_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(int8_t *)s->data, 3);
                break;
            case int16_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(int16_t *)s->data, 3);
                break;
            case int32_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(int32_t *)s->data, 3);   
                break;
            case uint8_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(uint8_t *)s->data, 3);
                break;
            case uint16_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(uint16_t *)s->data, 3);  
                break;
            case uint32_data:
                ips200.show_int((SCREEN_WIDTH - 70), y_pos, *(uint32_t *)s->data, 3);
                break;
            case float_data:
                ips200.show_float((SCREEN_WIDTH - 70), y_pos, *(float *)s->data, 3, 1);
                break;
            case bool_data:
                ips200.show_string((SCREEN_WIDTH - 70), y_pos, (*(bool *)s->data) ? "True" : "False");
                break;
            default:
                break;
        }
        // 显示箭头
        if(s == arrow)
        {
            ips200.show_string(0, y_pos + 2, ">>");
        }
        else
        {
            ips200.show_string(0, y_pos + 2, "  ");
        }
        s = s->nextnode; // 移动到下一个节点
    }
    int16_t bottom_line_y = (h->member + 1) * SCREEN_FONT_SIZE + 6;
    ips200.draw_line(0, bottom_line_y + 1, SCREEN_WIDTH, bottom_line_y + 1, DEFAULT_PENCOLOR);
    ips200.draw_line(0, bottom_line_y + 2, SCREEN_WIDTH, bottom_line_y + 2, DEFAULT_PENCOLOR);
    int16_t help_y = bottom_line_y + 6;
    if(arrow_lock)
    {
        ips200.show_string(90, help_y, "+Value");
        ips200.show_string(40, help_y + 18, "Step   Start   Exit");
        ips200.show_string(90, help_y + 36, "-Value");
    }
    else
    {
        ips200.show_string(95, help_y, "+Move");
        ips200.show_string(40, help_y + 18, "Into   Start   Back");
        ips200.show_string(95, help_y + 36, "-Move");
    }
    last_arrow = arrow;
    last_totalnode = h;  // 记录当前菜单层级
}

/**
 * @brief 上移箭头光标
 * @param void
 * @return void
 * @note 上移箭头光标，将箭头光标移动到上一个菜单项
 * @author YuYu
 */
static void arrow_up(void)
{
    if(arrow->totalnode && arrow->totalnode->subnode)
    {
        UI_menu *parent = arrow->totalnode;
        UI_menu *current = arrow;
        arrow = arrow->lastnode;
        if (arrow->totalnode != parent)
        {
            arrow = current;
        }
    }
}

/**
 * @brief 下移箭头光标
 * @param void
 * @return void
 * @note 下移箭头光标，将箭头光标移动到下一个菜单项
 * @author YuYu
 */
static void arrow_down(void)
{
    if(arrow->totalnode && arrow->totalnode->subnode)
    {
        UI_menu *parent = arrow->totalnode;
        UI_menu *current = arrow;
        arrow = arrow->nextnode;
        if (arrow->totalnode != parent)
        {
            arrow = current;
        }
    }
}

/**
 * @brief 进入子菜单项
 * @param void
 * @return void
 * @note 进入子菜单项，将箭头光标移动到子菜单项
 * @author YuYu
 */
static void arrow_enter(void) // 进入子菜单
{
    if(arrow->type == menu_folder && arrow->member > 0)
    {
        arrow = arrow->subnode;
        ips200.clear();
        last_arrow = NULL;
        last_totalnode = NULL;
        // system_self_lock = false;
    }
}

/**
 * @brief 退出子菜单项
 * @param void
 * @return void
 * @note 退出子菜单项，将箭头光标移动到上一级菜单项
 * @author YuYu
 */
static void arrow_quit(void) // 退出子菜单
{
    if(arrow->totalnode && arrow->totalnode->totalnode != NULL)
    {
        arrow = arrow->totalnode;
        ips200.clear();
        last_arrow = NULL;
        last_totalnode = NULL;
        // system_self_lock = false;
    }
}

/**
 * @brief 扫描按键
 * @param void
 * @return void
 * @note 扫描按键，包括上移、下移、进入子菜单项、退出子菜单项、确认选择、锁定编辑、退出编辑等
 * @author YuYu
 */
void arrow_scan(void)
{
    if(key_num == 1) // Up Button
    {
        if(arrow_lock == false)
        {
            arrow_up();
        }
        else
        {
            key_increase();
        }
    }
    if(key_num == 2) // Down Button
    {
        if(arrow_lock == false)
        {
            arrow_down();
        }
        else
        {
            key_decrease();
        }
    }
    if(key_num == 3) // Mid Button
    {
        departure_dialog_show = true;
        ips200.full(RGB565_BLACK);
        draw_departure_dialog();
    }
    if(key_num == 4) // Left Button
    {
        if(arrow->type == menu_folder)
        {
            if(strcmp(arrow->name, "Color Image") == 0
                || strcmp(arrow->name, "Gray Image") == 0
                || strcmp(arrow->name, "Binary Image") == 0
                || strcmp(arrow->name, "Customized Image") == 0
                || strcmp(arrow->name, "Model Detect") == 0)
            {
                menu_action_name = arrow->name;
            }
            else
            {
                arrow_enter();
            }
        }
        else if(arrow->type != bool_data)
        {
            if(arrow_lock == false)
            {
                arrow->select = true;
                arrow_lock = true;
            }
            else
            {
                step_index++;
                if(step_index >= STEP_LENGTH)
                {
                    step_index = 0;
                }
            }
        }
    }
    if(key_num == 5) // Right Button
    {
       if(arrow_lock == false)
       {
           arrow_quit();
       }
       else
       {
           // 退出锁定状态
           arrow->select = false;
           arrow_lock = false;
       }
    }
}

/**
 * @brief 选择当前菜单项
 * @param void
 * @return void
 * @note 选择当前菜单项，将选中标记切换到当前菜单项上
 * @author YuYu
 */
void key_select(void)
{
    // start_departure = 0;
    if(arrow->type != menu_folder && arrow->type != bool_data)
    {
        arrow->select = !arrow->select;
        arrow_lock = !arrow_lock;
    }
}

/**
 * @brief 增加当前菜单项的值
 * @param void
 * @return void
 * @note 增加当前菜单项的值，根据当前菜单项的类型进行增加
 * @author YuYu
 */
void key_increase(void)
{
    switch(arrow->type)
    {
        case int8_data:
            *(int8_t *)arrow->data = *(int8_t *)(arrow->data) + (int8_t)step_unit[step_index];
            break;
        case int16_data:
            *(int16_t *)arrow->data = *(int16_t *)(arrow->data) + (int16_t)step_unit[step_index];
            break;
        case int32_data:
            *(int32_t *)arrow->data = *(int32_t *)(arrow->data) + (int32_t)step_unit[step_index];
            break;
        case uint8_data:
            *(uint8_t *)arrow->data = *(uint8_t *)(arrow->data) + (uint8_t)step_unit[step_index];
            break;
        case uint16_data:
            *(uint16_t *)arrow->data = *(uint16_t *)(arrow->data) + (uint16_t)step_unit[step_index];
            break;
        case uint32_data:
            *(uint32_t *)arrow->data = *(uint32_t *)(arrow->data) + (uint32_t)step_unit[step_index];
            break;
        case float_data:
            *(float *)arrow->data = *(float *)(arrow->data) + step_unit[step_index];
            break;
        default:
            break;
    }
}

/**
 * @brief 减少当前菜单项的值
 * @param void
 * @return void
 * @note 减少当前菜单项的值，根据当前菜单项的类型进行减少
 * @author YuYu
 */
void key_decrease(void)
{
    switch(arrow->type)
    {
        case int8_data:
            *(int8_t *)(arrow->data) = *(int8_t *)(arrow->data) - (int8_t)step_unit[step_index];
            break;
        case int16_data:
            *(int16_t *)(arrow->data) = *(int16_t *)(arrow->data) - (int16_t)step_unit[step_index];
            break;
        case int32_data:
            *(int32_t *)(arrow->data) = *(int32_t *)(arrow->data) - (int32_t)step_unit[step_index];
            break;
        case uint8_data:
            *(uint8_t *)(arrow->data) = *(uint8_t *)(arrow->data) - (uint8_t)step_unit[step_index];
            break;
        case uint16_data:
            *(uint16_t *)(arrow->data) = *(uint16_t *)(arrow->data) - (uint16_t)step_unit[step_index];
            break;
        case uint32_data:
            *(uint32_t *)(arrow->data) = *(uint32_t *)(arrow->data) - (uint32_t)step_unit[step_index];
            break;
        case float_data:
            *(float *)(arrow->data) = *(float *)(arrow->data) - step_unit[step_index];
            break;
        default:
            break;
    }
}

/**
 * @brief 扫描按键
 * @param void
 * @return uint8_t 按键号，0 表示无按键按下，1 ~ KEY_MAX_NUMBER 表示按键号
 * @note 扫描按键，包括上移、下移、进入子菜单项、退出子菜单项、确认选择、锁定编辑、退出编辑等
 * @author YuYu
 */
uint8_t key_scan(void)
{
    for(int i = 0; i < KEY_MAX_NUMBER; i++)
    {
        if(keys[i]->get_level() == 0) // 按键按下
        {
            system_delay_ms(KEY_WAIT_MS);
            if(keys[i]->get_level() == 0)
            {
                while(keys[i]->get_level() == 0);
                system_delay_ms(KEY_WAIT_MS);
                return (i + 1); // 返回按键号 1 ~ KEY_MAX_NUMBER
            }
        }
    }
    return 0;
}
