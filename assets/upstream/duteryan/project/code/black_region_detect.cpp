

#include "zf_common_headfile.hpp"
#include "black_region_detect.hpp"
#include "red_region_verify.hpp" // 引入刚才剥离的红标二次校验库
#include <algorithm>
#include <cmath>

int DETECT_START_DIS=0; 
 
// ==================== 全局变量定义 ====================
int br_found = 0;       
int br_center_x = 0;    
int br_center_y = 0;    
int pre_brake_hint = 0;   // 直道早检出刹车提示:仅直道,finder 粗筛+位置约束过即置1,驱动 pre_brake 早刹,不触发偏转

int Red_flag = 0;       
cv::Mat final_crop_img; 

int target_x1 = 0;
int target_y1 = 0;
int target_x2 = 0;
int target_y2 = 0;

int orig_crop_x1 = 0;
int orig_crop_y1 = 0;
int orig_crop_x2 = 0;
int orig_crop_y2 = 0;

int recog_cnt = 0;
int recog_staute = 0;
int final_detect = 0;
int detect_out[DETECT_CNT] = {0};             
float detect_out_prob[DETECT_CNT] = {0.0f};   
int Brick_pos_flag=0;                         

int Brick_flag;
int Target_flag;
int brick_enable = 0;   // 红砖识别总开关:0=关闭(位置法/边界法/避障触发全禁),1=开启。菜单 BrkEn 可调
int faa=0;

void reset_black_region_runtime_state(void)
{
    br_found = 0;
    br_center_x = 0;
    br_center_y = 0;
    pre_brake_hint = 0;
    Red_flag = 0;

    target_x1 = 0;
    target_y1 = 0;
    target_x2 = 0;
    target_y2 = 0;
    orig_crop_x1 = 0;
    orig_crop_y1 = 0;
    orig_crop_x2 = 0;
    orig_crop_y2 = 0;

    recog_cnt = 0;
    recog_staute = 0;
    final_detect = 0;
    memset(detect_out, 0, sizeof(detect_out));
    memset(detect_out_prob, 0, sizeof(detect_out_prob));

    Brick_pos_flag = 0;
    Brick_flag = 0;
    Target_flag = 0;
    faa = 0;
    final_crop_img.release();
}


void find_black_rect_in_border(
    const uint8_t image[IMAGE_HEIGHT][IMAGE_WIDTH],
    const uint8_t left_border[IMAGE_HEIGHT],
    const uint8_t right_border[IMAGE_HEIGHT],
    uint16_t end_row
)
{
    // ★环岛内(仅状态4放行,窗口本来就窄)整体放宽黑块门槛:环内目标常贴环心/边界,
    //   赛道几何和直道差很多,直道那套尺寸/白环/实心度门槛容易把真目标滤掉。
    //   误检风险由"只在状态4检测"这道门控 + 后面的红色完整验证兜住。
    const bool in_halo = (left_halo_flag != 0 || right_halo_flag != 0);

    // ------------------------------------------------------
    // 1. 黑色块检测参数
    // ------------------------------------------------------
    const int MIN_BLACK_AREA   = in_halo ? 4 : 6;    // 环内:更小的块也认
    const int MIN_BLACK_WIDTH  = in_halo ? 2 : 3;
    const int MAX_BLACK_WIDTH  = 90;
    const int MIN_BLACK_HEIGHT = in_halo ? 1 : 2;
    const int BOX_EXPAND       = 1;

    // 新增：二值图中生成的固定画框大小
    const int BIN_ROI_SIZE     = 15;   

    // ------------------------------------------------------
    // 2. 原始彩色图中红色验证参数 (已转移至 red_region_verify)
    // ------------------------------------------------------        
    const int RED_ROI_SIZE = 100;            

    // ------------------------------------------------------
    // 3. 清空所有输出状态
    //   避障期间(brick_dis_isp==1)进入"显示刷新模式"：保留已锁定的控制状态
    //   (Brick_flag/Brick_pos_flag/br_center 等)不清零，只在下方更新显示框 target_*，
    //   以免打断正在进行的绕行(reNiHe 靠 Brick_pos_flag 决定左右偏移)。
    // ------------------------------------------------------
    if (brick_dis_isp == 0)
    {
        br_found = 0;
        pre_brake_hint = 0;
        Red_flag = 0;
        Brick_flag = 0;
        Target_flag = 0;
        Brick_pos_flag = 0;
        br_center_x = 0;
        br_center_y = 0;
        target_x1 = 0;
        target_y1 = 0;
        target_x2 = 0;
        target_y2 = 0;
        orig_crop_x1 = 0;
        orig_crop_y1 = 0;
        orig_crop_x2 = 0;
        orig_crop_y2 = 0;
        final_crop_img.release();
    }

    if (image == nullptr || left_border == nullptr || right_border == nullptr) return;
    if (end_row >= IMAGE_HEIGHT) end_row = IMAGE_HEIGHT - 1;

    // 环岛内目标限制:环岛中只有状态4才检测色块/验证红色块,其余环岛状态(1/2/3/5/6)不检测,
    // 直接返回(上方已清空所有输出/远距提示,起刹/识别/绕行整条链自然不触发),避免环岛出入口误检。
    /*
    if ((left_halo_flag  != 0 && left_halo_flag  != 4) ||
        (right_halo_flag != 0 && right_halo_flag != 4)) return;
    */
    if (left_halo_flag != 0 || right_halo_flag != 0)
    {
        return;
    }
    // ------------------------------------------------------
    // 4. BFS 缓存 (静态内存防止堆栈溢出)
    // ------------------------------------------------------
    static uint8_t visited[IMAGE_HEIGHT][IMAGE_WIDTH];
    static uint16_t qx[IMAGE_HEIGHT * IMAGE_WIDTH];
    static uint16_t qy[IMAGE_HEIGHT * IMAGE_WIDTH];

    for (int yy = 0; yy < IMAGE_HEIGHT; ++yy) {
        for (int xx = 0; xx < IMAGE_WIDTH; ++xx) {
            visited[yy][xx] = 0;
        }
    }

    // ------------------------------------------------------
    // 5. 记录最佳黑色连通块
    // ------------------------------------------------------
    int best_area = 0;
    int best_min_x = IMAGE_WIDTH;
    int best_max_x = -1;
    int best_min_y = IMAGE_HEIGHT;
    int best_max_y = -1;
    long best_sum_x = 0;
    long best_sum_y = 0;

    // ------------------------------------------------------
    // 6. BFS 搜索逻辑
    // ------------------------------------------------------
    // 扫描/生长走廊边距:弯道/环岛里目标常贴内弯边界放,边界本身在图像里是斜的,
    // 固定 BR_MARGIN=3 的走廊容易把块的大部分切在走廊外(整块找不到/被切成碎片,
    // 过不了后面的面积/实心度门槛)。弯道/环岛收窄到 1,直道保持 3 不变。
    int br_margin_cur = (wd_flag_stable != 0 || left_halo_flag != 0 || right_halo_flag != 0) ? 1 : BR_MARGIN;
    for (int y = IMAGE_HEIGHT - 1; y >= static_cast<int>(end_row); --y) {
        int scan_l = static_cast<int>(left_border[y]) + br_margin_cur;
        int scan_r = static_cast<int>(right_border[y]) - br_margin_cur;

        if (scan_l < 0) scan_l = 0;
        if (scan_r >= IMAGE_WIDTH) scan_r = IMAGE_WIDTH - 1;
        if (scan_l > scan_r) continue;

        for (int x = scan_l; x <= scan_r; ++x) {
            if (visited[y][x]) continue;
            if (image[y][x] != 0) continue;

            int head = 0;
            int tail = 0;
            qx[tail] = static_cast<uint16_t>(x);
            qy[tail] = static_cast<uint16_t>(y);
            ++tail;
            visited[y][x] = 1;

            int cur_area = 0;
            int cur_min_x = x;
            int cur_max_x = x;
            int cur_min_y = y;
            int cur_max_y = y;
            long cur_sum_x = 0;
            long cur_sum_y = 0;

            while (head < tail) {
                int cx = qx[head];
                int cy = qy[head];
                ++head;
                ++cur_area;
                cur_sum_x += cx;
                cur_sum_y += cy;

                if (cx < cur_min_x) cur_min_x = cx;
                if (cx > cur_max_x) cur_max_x = cx;
                if (cy < cur_min_y) cur_min_y = cy;
                if (cy > cur_max_y) cur_max_y = cy;

                static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
                static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

                for (int i = 0; i < 8; ++i) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];

                    if (nx < 0 || nx >= IMAGE_WIDTH) continue;
                    if (ny < static_cast<int>(end_row) || ny >= IMAGE_HEIGHT) continue;
                    if (visited[ny][nx]) continue;

                    int nl = static_cast<int>(left_border[ny]) + br_margin_cur;
                    int nr = static_cast<int>(right_border[ny]) - br_margin_cur;
                    if (nl < 0) nl = 0;
                    if (nr >= IMAGE_WIDTH) nr = IMAGE_WIDTH - 1;

                    if (nx < nl || nx > nr) continue;

                    if (image[ny][nx] == 0) {
                        visited[ny][nx] = 1;
                        if (tail < IMAGE_WIDTH * IMAGE_HEIGHT) {
                            qx[tail] = static_cast<uint16_t>(nx);
                            qy[tail] = static_cast<uint16_t>(ny);
                            ++tail;
                        }
                    }
                }
            }

            int cur_width  = cur_max_x - cur_min_x + 1;
            int cur_height = cur_max_y - cur_min_y + 1;

            if (cur_area < MIN_BLACK_AREA) continue;
            if (cur_width < MIN_BLACK_WIDTH || cur_width > MAX_BLACK_WIDTH) continue;
            if (cur_height < MIN_BLACK_HEIGHT) continue;

            // ★ 黑块四周必须是白色赛道:黑块外扩一圈(外框内、黑块本体外)采样,
            //   白(image!=0)占比不够 → 该黑块贴在赛道边沿/边界,丢弃。
            //   超出图像的环像素不计入(near车时底边在图外,不影响判定)。
            {
                const int SUR_M         = 2;    // 外扩像素
                const int SUR_WHITE_PCT = in_halo ? 40 : 60;   // 环上白占比门槛(%),越大越严;环内放宽
                int sx0 = cur_min_x - SUR_M, sx1 = cur_max_x + SUR_M;
                int sy0 = cur_min_y - SUR_M, sy1 = cur_max_y + SUR_M;
                int wcnt = 0, tcnt = 0;
                for (int yy = sy0; yy <= sy1; ++yy) {
                    if (yy < 0 || yy >= IMAGE_HEIGHT) continue;
                    for (int xx = sx0; xx <= sx1; ++xx) {
                        if (xx < 0 || xx >= IMAGE_WIDTH) continue;
                        // 只统计边界走廊内的环像素:弯道/环岛里块贴着内弯边界,外扩环有一半落在
                        // 赛道外黑区——赛道外像素不该拿来否决"四周是白赛道",不计入
                        if (xx < (int)left_border[yy] || xx > (int)right_border[yy]) continue;
                        if (xx >= cur_min_x && xx <= cur_max_x &&
                            yy >= cur_min_y && yy <= cur_max_y) continue;  // 跳过黑块本体
                        ++tcnt;
                        if (image[yy][xx] != 0) ++wcnt;   // 非0=白赛道
                    }
                }
                if (tcnt == 0 || wcnt * 100 < tcnt * SUR_WHITE_PCT) continue;  // 四周非白 → 丢弃
            }

            // ============================================================
            // ★【可删】黑块中心所在行的实际赛道宽度必须≈实车标定宽度 straightwidth[],否则不算
            // ------------------------------------------------------------
            // 原理:straightwidth[row] 是实车标定的"每行标准赛道宽度"。
            //      取黑块中心行的实际赛道宽度(右边界 right_border - 左边界 left_border),
            //      与该行标定值 straightwidth[center_row] 对照;差太多说明该黑块所在行
            //      边界异常/不在期望位置 → 不算有效目标,跳过(continue)。
            // 调整:只有一个容差 BLK_ROW_TRACK_W_TOL 可调,越小越严。
            // 若此限制不合适,直接删掉本整段 { ... } 即可,不影响其它逻辑。
            // ============================================================
            // ★仅直道生效:弯道/环岛里赛道斜穿图像行,行向跨度远大于垂直赛宽(轻松超标定1.3~1.5倍),
            //   ±20容差必然爆掉 → 弯道里所有黑块被此校验误杀。故弯道/环岛跳过,直道保留防误检。
            if (wd_flag_stable == 0 && left_halo_flag == 0 && right_halo_flag == 0)
            {
                const int BLK_ROW_TRACK_W_TOL = 20;   // ★容差:允许偏差(±),越小越严

                int center_row = (cur_min_y + cur_max_y) / 2;            // 黑块中心所在行
                if (center_row < 0) center_row = 0;
                if (center_row > IMAGE_HEIGHT - 1) center_row = IMAGE_HEIGHT - 1;

                // 该行实际赛道宽度 = 右边界列 - 左边界列
                int row_track_w = (int)right_border[center_row] - (int)left_border[center_row];

                // 与实车标定的标准赛道宽度对照
                int dw = row_track_w - straightwidth[center_row];        // 与标定宽度的差
                if (dw < 0) dw = -dw;                                    // 取绝对值(不依赖 std::abs 头文件)
                if (dw > BLK_ROW_TRACK_W_TOL) continue;                  // 与标定宽度差太多 → 不算
            }
            // ★【可删】结束 ============================================

            // 优先选离车最近的黑块:图像越靠下(max_y越大)离车越近 → 以 max_y 为主判据;
            // 最下沿相同(一样近)时再比面积,取更大的。
            bool is_better = false;
            if (cur_max_y > best_max_y) {
                is_better = true;
            } else if (cur_max_y == best_max_y && cur_area > best_area) {
                is_better = true;
            }

            if (is_better) {
                best_area = cur_area;
                best_min_x = cur_min_x;
                best_max_x = cur_max_x;
                best_min_y = cur_min_y;
                best_max_y = cur_max_y;
                best_sum_x = cur_sum_x;
                best_sum_y = cur_sum_y;
            }
        }
    }

    if (best_area <= 0) return;

    // ------------------------------------------------------
    // 7.9 ★ 黑块实心度校验:连通域黑像素数/外接框面积 > 门槛 才算有效目标。
    //   真目标(色块/底座上的物体)在二值图里是实心的;运动拖影、边界碎片、噪声斑
    //   外接框大但内部黑像素稀疏 → 在此滤除(起刹/识别整条链都不触发)。
    //   环岛内放宽到 0.5(环内目标常被环心/边界啃掉一角,0.6 容易误杀真目标)。
    // ------------------------------------------------------
    {
        int bw = best_max_x - best_min_x + 1;
        int bh = best_max_y - best_min_y + 1;
        const int solid_pct10 = in_halo ? 5 : 6;    // ×10: 5=0.5(环内) 6=0.6(其余)
        if (best_area * 10 < bw * bh * solid_pct10) return;   // 实心度不足 → 不算目标(输出已在顶部清零)
    }

    // ------------------------------------------------------
    // 8. ★ 在二值图中以物理极值点为基准，生成绝对 60x60 框
    // ------------------------------------------------------
    int custom_center_x = (best_min_x + best_max_x) / 2;
    int custom_bottom_y = best_max_y;

    // ------------------------------------------------------
    // 7.5 ★ 避障期间(brick_dis_isp==1)：显示刷新模式
    //   只把绿框(黑块实际外接框)更新成当前最近黑块，供实时观察；
    //   已锁定的 Brick_flag/Brick_pos_flag/br_center/触发状态一律不动(上面已跳过清零)，
    //   避免打断正在进行的绕行。找不到黑块时(best_area<=0 已提前 return)保留上一帧的框。
    // ------------------------------------------------------
    if (brick_dis_isp == 1)
    {
        target_x1 = best_min_x;
        target_y1 = best_min_y;
        target_x2 = best_max_x;
        target_y2 = custom_bottom_y + 2;
        if (target_y2 >= IMAGE_HEIGHT) target_y2 = IMAGE_HEIGHT - 1;
        return;
    }

    // ------------------------------------------------------
    // 8.0 ★【红砖：纯二值图按位置判定，不做红色验证】
    //   仅直道(wd_flag_stable==0)生效：按黑块中心横坐标落在赛道左/右 1/3 判左右红砖。
    //   赛道左右界与宽度取黑块中心行的 left_border/right_border（即“当前赛道宽度”）。
    //     中心在左 1/3 → 左红砖 Brick_pos_flag=1（reNiHe 向右避障）
    //     中心在右 1/3 → 右红砖 Brick_pos_flag=2（reNiHe 向左避障）
    //     中间 1/3     → 不在此处理，落到下方红色验证 / NCNN（物体识别）
    //   弯道(wd_flag_stable!=0)不可能有红砖 → 一律走下方正常识别流程。
    //   命中左/右红砖时直接输出并 return，跳过彩图映射与 verify_and_extract_red_target。
    //   ★禁区：整个环岛(halo≠0,进入/内部/出) / 十字内部(cross_cnt∈{1,2}) 不存在红砖 → 不按位置判砖。
    // ------------------------------------------------------
    int brick_forbid_zone =
        (left_halo_flag  != 0) ||
        (right_halo_flag != 0) ||
        (cross_cnt == 1 || cross_cnt == 2);

    // 砖块基本门槛(防误判)：黑块高度>10 且 中心纵坐标>20，否则不按位置判砖(交给下方正常识别流程)
    int brick_h        = best_max_y - best_min_y + 1;    // 黑块高度
    int brick_center_y = (best_min_y + best_max_y) / 2;  // 黑块中心纵坐标

    if (brick_enable == 1 && wd_flag_stable == 0 && !brick_forbid_zone && brick_h > 8 && brick_center_y > 10)
    {
        int brick_row = (best_min_y + best_max_y) / 2;   // 黑块中心行
        if (brick_row < 0) brick_row = 0;
        if (brick_row > IMAGE_HEIGHT - 1) brick_row = IMAGE_HEIGHT - 1;

        int tl = static_cast<int>(left_border[brick_row]);   // 该行赛道左界
        int tr = static_cast<int>(right_border[brick_row]);  // 该行赛道右界
        int tw = tr - tl;                                    // 当前赛道宽度

        if (tw > 0)
        {
            int left_third_x  = tl + tw / 3;    // 左 1/3 分界
            int right_third_x = tr - tw / 3;    // 右 1/3 分界

            int brick_pos = 0;                  // 0=中间(交给下方红色验证)
            if (custom_center_x <= left_third_x)       brick_pos = 1;   // 左红砖
            else if (custom_center_x >= right_third_x) brick_pos = 2;   // 右红砖

            if (brick_pos != 0)
            {
                Brick_flag     = 1;
                Target_flag    = 0;
                Brick_pos_flag = brick_pos;
                br_center_x    = custom_center_x;
                br_center_y    = custom_bottom_y;
                br_found       = 1;
                // target_y2 供 Distance_interial 的 target_y2>20 近距门槛判定
                target_y2 = custom_bottom_y + 2;
                if (target_y2 >= IMAGE_HEIGHT) target_y2 = IMAGE_HEIGHT - 1;
                // 填充 target 框(二值图 150x60 坐标系)供 menu.cpp 画绿框可视化：用黑块实际外接框
                target_x1 = best_min_x;
                target_y1 = best_min_y;
                target_x2 = best_max_x;
                return;   // 红砖已判定，跳过红色验证
            }
        }
    }

    target_x1 = custom_center_x - BIN_ROI_SIZE / 2;
    target_x2 = target_x1 + BIN_ROI_SIZE - 1;
    target_y2 = custom_bottom_y + 2; 
    target_y1 = target_y2 - BIN_ROI_SIZE + 1;

    // 【防爆栈安全卡死逻辑】
    if (target_x1 < 0) { target_x1 = 0; target_x2 = BIN_ROI_SIZE - 1; }
    if (target_x2 >= IMAGE_WIDTH) { target_x2 = IMAGE_WIDTH - 1; target_x1 = IMAGE_WIDTH - BIN_ROI_SIZE; }
    if (target_y2 >= IMAGE_HEIGHT) { target_y2 = IMAGE_HEIGHT - 1; target_y1 = IMAGE_HEIGHT - BIN_ROI_SIZE; }
    if (target_y1 < 0) { target_y1 = 0; target_y2 = BIN_ROI_SIZE - 1; }

    // 边缘禁区:直道±10;弯道/环岛里赛道整体偏向画面一侧,目标块天然靠边 → 放宽到±2,
    // 否则大弯道里 15 宽的目标框一碰禁区整帧作废(弯道检不到黑块的主因之一)
    {
        int edge_guard = (wd_flag_stable != 0 || left_halo_flag != 0 || right_halo_flag != 0) ? 2 : 10;
        if (target_x1 <= edge_guard || target_x2 >= IMAGE_WIDTH - edge_guard) return;
    }

    br_center_x = custom_center_x;
    br_center_y = custom_bottom_y;
    br_found = 1;

    // ------------------------------------------------------
    // 9. 将黑色块中心从 150x60 映射到原始 320x240 图像
    // ------------------------------------------------------

    // //弯道偏移一点
    // if(wd_flag==-1)//左弯道
    // {
    //     br_center_x = (int)(0.8 * br_center_x);
    //     br_center_y = (int)(1.2 * br_center_x);

    // }
    // else if(wd_flag==1)//右弯道
    // {
    //     br_center_x = (int)(1.2 * br_center_x);
    //     br_center_y = (int)(1.2 * br_center_x);
    // }

    const float scale_x = 320.0f / 150.0f;
    const float scale_y = 240.0f / 60.0f;

    int ocx = static_cast<int>(br_center_x * scale_x + 0.5f);
    int ocy = static_cast<int>(br_center_y * scale_y + 0.5f);

    // ------------------------------------------------------
    // 10. 在原始彩色图中截取更大的 100x100 ROI
    // ------------------------------------------------------
    int ox1 = ocx - RED_ROI_SIZE / 2;
    int ox2 = ox1 + RED_ROI_SIZE - 1;
    // ROI 下边距按距离分档:远距(y2<35)加大到 +24——高速运动模糊会把二值黑块底边"啃掉"1~2行,
    // ocy(块底×4)随之偏高4~8px,红底座(在块底下方)被 +10 的窄下边距切出 ROI → finder真没红(R1a),
    // 且啃边量逐帧抖动 → 远距验证时好时坏。近距(y2>=35,含识别区y2>EvaSD)保持 +10,
    // 识别裁剪行为与原来完全一致(caijian.cpp 一致性不受影响)。
    int oy2 = ocy + ((target_y2 < 35) ? 24 : 10);
    int oy1 = oy2 - RED_ROI_SIZE + 1;

    // 终极防爆栈安全边界卡死逻辑
    if (ox1 < 0) { ox1 = 0; ox2 = RED_ROI_SIZE - 1; }
    if (ox2 >= 320) { ox2 = 319; ox1 = 320 - RED_ROI_SIZE; }
    if (oy2 >= 240) { oy2 = 239; oy1 = 240 - RED_ROI_SIZE; }
    if (oy1 < 0) { oy1 = 0; oy2 = RED_ROI_SIZE - 1; }

    orig_crop_x1 = ox1;
    orig_crop_y1 = oy1;
    orig_crop_x2 = ox2;
    orig_crop_y2 = oy2;

    // ------------------------------------------------------
    // 11. 从原始彩色图中取 ROI 并将其交由验证模块
    // ------------------------------------------------------
    cv::Mat roi = uvc_dev.get_frame_mjpg_roi(ox1, oy1, ox2, oy2);
    // final_crop_img = roi.clone(); // 备份原始彩色 ROI
    // capture_save_roi_mjpg("image",faa++);

    if (roi.empty() || roi.cols <= 0 || roi.rows <= 0)
    {
        Red_flag = 0;
        Target_flag = 0;
        Brick_flag = 0;
        return;
    }

    // 调用解耦出去的红标/红砖分类模块
    verify_and_extract_red_target(roi, target_y2, ocx, ocy, ox1, oy1, br_center_x, left_border, right_border);
}


// ============================================================
// 【红砖判定方式2】边界突变 / 赛道宽度收缩法（与方式1“黑块位置法”并列）
//   直道正常时左右边界连续、宽度≈straightwidth[y]。若一侧放砖，砖处该侧边界会
//   突然向图像中央收缩(边界不连续)、赛道宽度相比标定值骤减。
//     右边界向内收缩 → 右红砖(Brick_pos_flag=2, reNiHe 向左避)
//     左边界向内收缩 → 左红砖(Brick_pos_flag=1, reNiHe 向右避)
//   命中后写 Brick_flag/Brick_pos_flag/br_center_y/target_y2，交由 Distance_interial
//   按与方式1相同的距离门槛(br_center_y>30 等)触发 brick_dis_isp、由 reNiHe 偏移中线。
//   仅在直道、非环岛/十字禁区、非避障期、且方式1本帧未判到砖/目标时启用。
// ============================================================
void find_brick_by_border(void)
{
    // ---- 可调参数 ----
    const int WIDTH_SHRINK = 20;  // 赛道宽比标定小多少算“突然收缩”(越大越严)
    const int SIDE_DEV     =15;  // 某侧边界相对下方参考行向内偏移多少算“这一侧不连续”
    const int NEED_ROWS    =6;   // 连续收缩行数(砖有竖向厚度,抗单行噪声)
    const int SCAN_TOP     = 16;  // 只扫到第16行(更上方边界不可靠, effect_line 以15为界)

    // ---- 门控：只在直道 / 非禁区 / 非避障 / 方式1本帧未判到砖或目标 时启用 ----
    if (brick_enable != 1)  return;                    // 红砖识别总开关关闭 → 不判砖
    if (brick_dis_isp == 1) return;                    // 避障中：判定冻结,不改
    if (recog_staute == 1)  return;                    // NCNN 绕行中不判砖
    if (Brick_flag == 1 || Target_flag == 1) return;   // 方式1本帧已判到砖/目标 → 不覆盖
    if (wd_flag_stable != 0) return;                   // 只在直道
    if ((left_halo_flag  != 0) ||
        (right_halo_flag != 0) ||
        (cross_cnt == 1 || cross_cnt == 2)) return;    // 整个环岛(halo≠0)/十字禁区不判砖

    const int EDGE_L = 1, EDGE_R = IMAGE_WIDTH - 2;    // 贴边=丢线的列值(见 effect_line)

    // ---- 从近(下)到远(上)找连续收缩段 ----
    int run = 0, run_bot = -1, run_top = -1;
    for (int y = IMAGE_HEIGHT - 2; y >= SCAN_TOP; --y)
    {
        int lc = current_left_col[y], rc = current_right_col[y];
        if (lc <= EDGE_L || rc >= EDGE_R) { run = 0; continue; }  // 丢线行跳过并断开连续
        int w = rc - lc;
        if (w > 0 && (straightwidth[y] - w) >= WIDTH_SHRINK)
        {
            if (run == 0) run_bot = y;                 // 收缩段最近(最下)行
            run_top = y;                               // 收缩段最远(最上)行
            if (++run >= NEED_ROWS) break;
        }
        else run = 0;
    }
    if (run < NEED_ROWS) return;                       // 没有够长的收缩段

    // ---- 参考行：收缩段正下方的正常行(边界未收缩) ----
    int ref = run_bot + 1;
    if (ref > IMAGE_HEIGHT - 1) ref = IMAGE_HEIGHT - 1;
    int left_ref  = current_left_col[ref];
    int right_ref = current_right_col[ref];
    if (left_ref <= EDGE_L || right_ref >= EDGE_R) return;   // 参考行本身丢线 → 无法判侧,放弃

    // ---- 收缩段内各侧“最大向内偏移”,判定哪侧边界不连续 ----
    int left_dev = 0, right_dev = 0;
    for (int y = run_top; y <= run_bot; ++y)
    {
        int ld = (int)current_left_col[y] - left_ref;    // 左边界右移(向内)为正
        int rd = right_ref - (int)current_right_col[y];  // 右边界左移(向内)为正
        if (ld > left_dev)  left_dev  = ld;
        if (rd > right_dev) right_dev = rd;
    }

    int pos = 0;
    if (right_dev >= left_dev && right_dev >= SIDE_DEV)     pos = 2;  // 右边界收缩 → 右红砖(向左避)
    else if (left_dev >  right_dev && left_dev >= SIDE_DEV) pos = 1;  // 左边界收缩 → 左红砖(向右避)
    if (pos == 0) return;

    // ---- 命中：写入与方式1相同的输出,交给 Distance_interial 触发红砖避障 ----
    Brick_flag     = 1;
    Target_flag    = 0;
    Brick_pos_flag = pos;
    br_center_x    = (current_left_col[run_bot] + current_right_col[run_bot]) / 2;
    br_center_y    = run_bot;                    // 收缩段最近行 → 距离门槛判定(br_center_y>30)
    br_found       = 1;
    target_y2      = run_bot + 2;
    if (target_y2 >= IMAGE_HEIGHT) target_y2 = IMAGE_HEIGHT - 1;
    // 显示绿框:贴收缩侧,竖向覆盖收缩段
    target_y1 = run_top;
    if (pos == 2) { target_x2 = right_ref; target_x1 = right_ref - 12; }
    else          { target_x1 = left_ref;  target_x2 = left_ref + 12; }
    if (target_x1 < 0) target_x1 = 0;
    if (target_x2 >= IMAGE_WIDTH) target_x2 = IMAGE_WIDTH - 1;
}
