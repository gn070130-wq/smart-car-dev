#include  "zf_common_headfile.hpp"
#include "ncnn_model.hpp"
#include <chrono>   // 文件顶部加上
// ============================================================
// 🔥 宏定义区（方便后续统一修改配置，无需动底层逻辑）
// ============================================================
#define NUM_CLASSES 6    // 类别数量
#define INPUT_SIZE 64    // 推理图像尺寸（你实测 60x60 能达到 99% 的准确率）

int DETECT_PROCESS_DIS=0 ;//按场景锁定的MERGE衰减距离；直行动作作为通过距离
int Detect_dis_isp=0;//本次卡片动作已启动标志
int Detect_real_dis=0;//绕行完全退出后的识别冷却实际距离
int evade_cooldown_active=0;//绕行退出后的冷却锁:1=冷却中,期间禁止检测/推理/绕行
bool g_ncnn_inited = false;
ncnn::Net g_ncnn_net;

// 严格对齐你的 6 个类别名（必须和 labels.txt 顺序一致！）
const char* class_labels[NUM_CLASSES] = {
    "APC",
    "Aidbag",
    "ambulance",
    "grenade",
    "gun",
    "telescope"
};

int single_out=0;//单个识别输出
float single_out_prob=0;//单个识别输出置信度

// 注意：这两个分配器必须定义为全局变量（或者静态变量），它们的生命周期必须和 g_ncnn_net 一样长！
static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::UnlockedPoolAllocator g_workspace_pool_allocator;
bool ncnn_init(const char* param_path, const char* bin_path)
{
    g_ncnn_inited = false;
    g_ncnn_net.clear();

    g_blob_pool_allocator.clear();
    g_workspace_pool_allocator.clear();
    g_blob_pool_allocator.set_size_compare_ratio(0.0f);
    g_workspace_pool_allocator.set_size_compare_ratio(0.0f);
    g_ncnn_net.opt.blob_allocator      = &g_blob_pool_allocator;
    g_ncnn_net.opt.workspace_allocator = &g_workspace_pool_allocator;

    g_ncnn_net.opt.num_threads         = 1;       // 单核，保持 1
    g_ncnn_net.opt.use_vulkan_compute  = false;
    g_ncnn_net.opt.use_int8_inference  = false;
    g_ncnn_net.opt.use_fp16_arithmetic = false;   // LA264 无 fp16 硬件，必须 false
    g_ncnn_net.opt.use_fp16_packed     = false;
    g_ncnn_net.opt.use_fp16_storage    = false;   // 同上，无 fp16 硬件
    g_ncnn_net.opt.lightmode           = true;

    // ★ 本板无 LSX/任何 SIMD（cpuinfo 已确认），必须关 packing。
    //   packing 是给向量内核用的内存布局；标量路径上打开它只会多做
    //   pack/unpack 转置，纯属浪费。关掉是对的。
    g_ncnn_net.opt.use_packing_layout  = false;

    int ret1 = g_ncnn_net.load_param(param_path);
    if (ret1 != 0) { printf("❌ Param 加载失败: %d\n", ret1); return false; }

    int ret2 = g_ncnn_net.load_model(bin_path);
    if (ret2 != 0) { printf("❌ Bin 加载失败: %d\n", ret2); return false; }

    g_ncnn_inited = true;
    printf("✅ NCNN 初始化成功 (纯标量)\n");
    return true;
}
// ------------------------------------------------------------------
// fast_exp：Schraudolph 近似，误差 < 2%，龙芯上比 expf 快 3-5x
// softmax 只需要相对比例正确，这个精度完全够用
// ------------------------------------------------------------------
static inline float fast_exp(float x) {
    if (x < -88.0f) return 0.0f;
    if (x >  88.0f) return 3.4e38f;
    union { float f; int i; } u;
    u.i = (int)(x * 1.4426950408f * (float)(1 << 23)) + 0x3f800000;
    return u.f;
}

// ------------------------------------------------------------------
// 静态输入 Mat，避免每帧 heap alloc
// ------------------------------------------------------------------
static ncnn::Mat g_input_mat;
// 卡片开始停车时锁存一份有效ROI；停车过程中若检测到更近、更清晰的ROI则持续更新。
// 即使制动期间某一图像帧检测闪断，停稳后仍有可靠输入可供NCNN识别。
static cv::Mat g_card_stop_roi;
static bool g_stopped_announced = false;
int ffa=0;//识别图片计数
void ncnn_recog_run_roi_float()
{
    if (card_recognition_disable == 1)
    {
        single_out = -1;
        single_out_prob = 0.0f;
        return;
    }
    if (!g_ncnn_inited || (Target_flag != 1 && card_stop_request == 0)) return;

    const bool using_stopped_roi = (card_stop_request == 1 && !g_card_stop_roi.empty());
    const cv::Mat& roi = using_stopped_roi ? g_card_stop_roi : final_crop_img;
    if (roi.empty() || roi.cols <= 0 || roi.rows <= 0) {
        single_out = -1; single_out_prob = 0.0f; return;
    }

    // ROI 分辨率门槛:正常裁剪不会小于 30×30。小于说明目标过远/裁剪异常,放大送识别只会糊,
    // 直接释放并按失败返回(single_out=-1 → 该帧不计入识别帧数,见 Detect_process),不送入 NCNN。
    const int MIN_ROI_SIZE = 20;
    if (roi.cols < MIN_ROI_SIZE || roi.rows < MIN_ROI_SIZE) {
        if (!using_stopped_roi) final_crop_img.release();
        single_out = -1; single_out_prob = 0.0f; return;
    }
    // capture_save_roi_mjpg("image",ffa++);
    // 1. 预处理：静态 Mat 复用，第二帧起零分配
    g_input_mat = ncnn::Mat::from_pixels_resize(
        roi.data,
        ncnn::Mat::PIXEL_BGR2RGB,
        roi.cols, roi.rows,
        INPUT_SIZE, INPUT_SIZE
    );

    // 2. 归一化（参数与原版完全一致）
    static const float mean_vals[3] = {123.675f, 116.28f,   103.53f  };
    static const float norm_vals[3] = {0.017124f, 0.017507f, 0.017429f};
    g_input_mat.substract_mean_normalize(mean_vals, norm_vals);

    // 3. 推理
    ncnn::Extractor ex = g_ncnn_net.create_extractor();
    if (ex.input("in0", g_input_mat) != 0) {
        single_out = -1; single_out_prob = 0.0f; return;
    }
    ncnn::Mat out;
    if (ex.extract("out0", out) != 0) {
        single_out = -1; single_out_prob = 0.0f; return;
    }

    // 4. 后处理：先找 max_logit 防溢出，再用 fast_exp 做 softmax
    //    合并两次遍历为一次（找 max 和找 max_idx 同时完成）
    float max_logit = out[0];
    int   max_idx   = 0;
    for (int i = 1; i < NUM_CLASSES; ++i) {
        if (out[i] > max_logit) { max_logit = out[i]; max_idx = i; }
    }

    // fast_exp softmax：只需算最大类的概率，不用算所有类再除
    // sum 全部算，但 max 类的分子直接是 fast_exp(0) = 1.0f
    float sum = 0.0f;
    for (int i = 0; i < NUM_CLASSES; ++i)
        sum += fast_exp(out[i] - max_logit);

    single_out      = max_idx;
    single_out_prob = (sum > 1e-6f) ? (1.0f / sum) : 0.0f;

     // 7. 打印最终识别结果
    // printf("结果 %s, 置信度 %f\n",class_labels[single_out],single_out_prob);
    // fast_exp(max_logit - max_logit) == fast_exp(0) == 1.0f，分子直接省掉
}


// ============================================================
// 🔥 单张 JPG 识别 (龙芯极致优化版)
// ============================================================
// 🔴 改造1：返回类型改为 int
int ncnn_test_single_jpg_for_eval(const char* image_path)
{
    if (!g_ncnn_inited) {
        printf("请先初始化模型！\n");
        return -1; // -1 表示错误
    }

    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
    if (img.empty()) {
        // printf("❌ 图片读取失败：%s\n", image_path);
        return -1; 
    }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(
        img.data, ncnn::Mat::PIXEL_BGR2RGB, 
        img.cols, img.rows, INPUT_SIZE, INPUT_SIZE
    );

    const float mean_vals[3] = {123.675f, 116.28f, 103.53f};
    const float norm_vals[3] = {0.017124f, 0.017507f, 0.017429f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = g_ncnn_net.create_extractor();
    ex.input("in0", in);
    ncnn::Mat out;
    ex.extract("out0", out);

    float max_logit = -1e9;
    for (int i = 0; i < NUM_CLASSES; i++) {
        if (out[i] > max_logit) max_logit = out[i];
    }

    float sum = 0;
    float scores[NUM_CLASSES];
    for (int i = 0; i < NUM_CLASSES; i++) {
        scores[i] = exp(out[i] - max_logit);
        sum += scores[i];
    }

    int max_idx = 0;
    float max_prob = scores[0] / sum;
    for (int i = 1; i < NUM_CLASSES; i++) {
        float prob = scores[i] / sum;
        if (prob > max_prob) {
            max_prob = prob;
            max_idx = i;
        }
    }

    // 🔴 改造2：返回最大概率对应的索引
    return max_idx; 
}




// ============================================================
// 推理性能基准：对同一张图反复推理 N 次，打印平均延迟和帧率
// 用法：ncnn_benchmark_single_jpg("/home/root/photo60/xxx.jpg", 10000);
// ============================================================
void ncnn_benchmark_single_jpg(const char* image_path, int loops )
{
    if (!g_ncnn_inited) { printf("请先初始化模型！\n"); return; }

    // ---- 1. 预处理只做一次（与模型格式无关，不计入循环）----
    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
    if (img.empty()) { printf("❌ 图片读取失败：%s\n", image_path); return; }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(
        img.data, ncnn::Mat::PIXEL_BGR2RGB,
        img.cols, img.rows, INPUT_SIZE, INPUT_SIZE);

    const float mean_vals[3] = {123.675f, 116.28f, 103.53f};
    const float norm_vals[3] = {0.017124f, 0.017507f, 0.017429f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    // ---- 2. 预热（不计时），稳定缓存/分配器 ----
    for (int i = 0; i < 50; i++) {
        ncnn::Extractor ex = g_ncnn_net.create_extractor();
        ex.input("in0", in);
        ncnn::Mat out;
        ex.extract("out0", out);
    }

    // ---- 3. 正式计时 ----
    volatile long long checksum = 0;   // 防止编译器优化掉整个循环
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < loops; i++) {
        ncnn::Extractor ex = g_ncnn_net.create_extractor();
        ex.input("in0", in);
        ncnn::Mat out;
        ex.extract("out0", out);

        int   max_idx = 0;
        float max_v   = out[0];
        for (int c = 1; c < NUM_CLASSES; c++)
            if (out[c] > max_v) { max_v = out[c]; max_idx = c; }
        checksum += max_idx;
    }
    auto t1 = std::chrono::steady_clock::now();

    // ---- 4. 统计 ----
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avg_ms   = total_ms / loops;

    printf("====== 推理基准 ======\n");
    printf("输入尺寸 : %dx%d\n", INPUT_SIZE, INPUT_SIZE);
    printf("循环次数 : %d\n", loops);
    printf("总耗时   : %.1f ms\n", total_ms);
    printf("单帧平均 : %.3f ms\n", avg_ms);
    printf("帧率     : %.2f FPS\n", 1000.0 / avg_ms);
    printf("(checksum=%lld 忽略)\n", (long long)checksum);
}


extern cv::Mat final_crop_img;
extern int Target_flag;
// 【终极稳定版】 - 加入全状态积分冷却
int turn_action=0;//偏转动作
int fuzzy_default_action=1;//置信度<0.6(模糊)时的默认动作:0左拐/1直行/2右拐(菜单 FzAct 可调)
int fmfcnt=0;
int aa=0;

// 🚀 新增：正在识别中的标志位 (0: 未在识别, 1: 正在跑模型识别中)
// 该标志位仅在目标进入视野且正在累积识别帧数时为 1，出结果进入避障或未识别时均为 0
int is_recognizing_now = 0;


int EvaD_N = 0; //逆弯道偏转距离
int EvaD_S = 0; //顺弯道偏转距离
int EvaD_O = 0; //普通偏转距离

int EvaSD_Z = 0; //直道起始识别距离
int EvaSD_W = 0; //弯道起始识别距离
int EvaSD_H = 0; //环岛内部单独的识别距离
int EVADE_OFFSET = 30; //并线阶段的初始虚拟中线偏移像素(菜单 EvOft)
int evade_turn_angle = 25; //停车/倒车完成后的固定偏转角绝对值(菜单 EvAng)
int evade_clear_distance = 40; //保持固定角度向前清障的基础距离(菜单 EvClr)

volatile int card_evade_phase = CARD_EVADE_IDLE;
volatile int card_evade_phase_dis = 0;
volatile float card_evade_relative_yaw = 0.0f;
volatile float card_evade_target_yaw = 0.0f;

static int card_evade_settle_frames = 0;
static const int CARD_EVADE_SETTLE_REQUIRED_FRAMES = 5;
static const int CARD_EVADE_SETTLE_MAX_DIS = 60;

static void reset_card_evade_control_history()
{
    speed_left_error_integral = 0;
    speed_right_error_integral = 0;
    speed_left_error_last = 0;
    speed_right_error_last = 0;
    Gyro_error_last = 0;
    Gyro_output = 0;
    Gyro_Feedforward = 0.0f;
    Last_col_Differ = col_Differ;
}

static void begin_card_evade_motion()
{
    card_evade_phase_dis = 0;
    card_evade_relative_yaw = 0.0f;
    card_evade_target_yaw = 0.0f;
    card_evade_settle_frames = 0;
    Detect_real_dis = 0;
    Detect_dis_isp = 1;
    reset_card_evade_control_history();

    if (turn_action == 0 || turn_action == 2)
    {
        int angle_abs = (evade_turn_angle >= 0) ? evade_turn_angle : -evade_turn_angle;
        card_evade_target_yaw = (turn_action == 0) ?
                                -static_cast<float>(angle_abs) :
                                 static_cast<float>(angle_abs);
        card_evade_phase = CARD_EVADE_TURN_IN;
        printf("新绕行启动: TURN_IN act=%d target_yaw=%.1f, clear=%d+reverse%d, merge=%d, offset=%d\n",
               turn_action, static_cast<double>(card_evade_target_yaw),
               evade_clear_distance, static_cast<int>(card_reverse_real_dis),
               DETECT_PROCESS_DIS, EVADE_OFFSET);
    }
    else
    {
        // APC/ambulance及模糊默认直行继续保持原来的“按场景距离锁住本次卡片”行为。
        card_evade_phase = CARD_EVADE_STRAIGHT;
        printf("卡片直行通过: distance=%d\n", DETECT_PROCESS_DIS);
    }
}

static void finish_card_evade_motion(const char* reason)
{
    printf("新绕行退出: %s, phase=%d, phase_dis=%d, yaw=%.2f\n",
           reason, static_cast<int>(card_evade_phase),
           static_cast<int>(card_evade_phase_dis),
           static_cast<double>(card_evade_relative_yaw));

    card_evade_phase = CARD_EVADE_IDLE;
    card_evade_phase_dis = 0;
    card_evade_relative_yaw = 0.0f;
    card_evade_target_yaw = 0.0f;
    card_evade_settle_frames = 0;
    recog_staute = 0;
    recog_cnt = 0;
    Detect_dis_isp = 0;
    Detect_real_dis = 0;
    Evade_angle_z = 0.0f;
    evade_cooldown_active = 1;
    Target_flag = 0;
    Brick_flag = 0;
    card_reverse_real_dis = 0;
    reset_card_evade_control_history();
}

static bool card_evade_real_track_is_stable()
{
    int row = cd_turn_propect;
    if (evade_curve == 2) row = halo_turn_propect;
    if (row < End_Line) row = End_Line;
    if (row < 0) row = 0;
    if (row > IMAGE_HEIGHT - 3) row = IMAGE_HEIGHT - 3;

    int reliable_rows = 0;
    for (int i = row; i < row + 3; i++)
    {
        int left = current_left_col[i];
        int right = current_right_col[i];
        int width = right - left;
        if (left > 1 && right < IMAGE_WIDTH - 2 && width > 20)
        {
            reliable_rows++;
        }
    }

    if (reliable_rows < 2) return false;
    int abs_gyro = (groyz_now >= 0) ? groyz_now : -groyz_now;
    // MERGE结束时虚拟偏移已经严格为0，因此虚拟中线与本帧真实中线天然相同。
    // SETTLE只确认真实边界可用且车身没有剧烈甩动；弯道真实中线本来就可能不在75列，
    // 不能再用“中线必须接近图像中心”作为退出条件。
    return (abs_gyro < 250);
}

void reset_ncnn_runtime_state(void)
{
    DETECT_PROCESS_DIS = 0;
    Detect_dis_isp = 0;
    Detect_real_dis = 0;
    evade_cooldown_active = 0;
    single_out = -1;
    single_out_prob = 0.0f;
    turn_action = 0;
    fmfcnt = 0;
    aa = 0;
    ffa = 0;
    is_recognizing_now = 0;
    card_evade_phase = CARD_EVADE_IDLE;
    card_evade_phase_dis = 0;
    card_evade_relative_yaw = 0.0f;
    card_evade_target_yaw = 0.0f;
    card_evade_settle_frames = 0;
    g_stopped_announced = false;
    g_card_stop_roi.release();
    g_input_mat.release();
}

void Detect_process()
{
    // 高置信快速确认：第一次有效识别结果 >95% 时，立即按该具体类别进入绕行；
    // 第一次未超过阈值时，不再走快速通道，继续收满 DETECT_CNT 次做原综合判断。
    const float FAST_CONFIRM_PROB = 0.95f;

    // CdOff=1 时彻底退出卡片状态机：不预刹、不停车、不推理、不倒车、不触发卡片绕行。
    // 红砖使用独立的 Brick_flag/brick_dis_isp 状态链，因此不在这里清除。
    if (card_recognition_disable == 1)
    {
        recog_cnt = 0;
        is_recognizing_now = 0;
        Target_flag = 0;
        pre_brake = 0;
        pre_brake_hint = 0;
        pre_brake_frames = 0;
        pre_brake_giveup = 0;
        pre_brake_dis = 0;
        card_stop_request = 0;
        card_vehicle_stopped = 0;
        card_reverse_phase = CARD_REVERSE_IDLE;
        card_reverse_stopped = 0;
        card_reverse_real_dis = 0;
        recog_staute = 0;
        Detect_dis_isp = 0;
        Detect_real_dis = 0;
        evade_cooldown_active = 0;
        card_evade_phase = CARD_EVADE_IDLE;
        card_evade_phase_dis = 0;
        card_evade_relative_yaw = 0.0f;
        card_evade_target_yaw = 0.0f;
        card_evade_settle_frames = 0;
        card_recognition_cooldown_ticks = 0;
        g_card_stop_roi.release();
        final_crop_img.release();
        return;
    }

    // 五卡片顺序门禁的第二道保护。正常主循环会先在
    // update_card_sequence_gate() 中清掉禁用卡片的Target/ROI；这里再次检查，
    // 保证CardN!=1或Card5之后的候选即使因某帧状态重新置位，也进不了停车和NCNN。
    if (card_sequence_recognition_allowed() != 1 &&
        recog_staute == 0 &&
        card_reverse_phase == CARD_REVERSE_IDLE)
    {
        recog_cnt = 0;
        is_recognizing_now = 0;
        Target_flag = 0;
        pre_brake = 0;
        pre_brake_hint = 0;
        pre_brake_frames = 0;
        pre_brake_giveup = 0;
        pre_brake_dis = 0;
        card_stop_request = 0;
        card_vehicle_stopped = 0;
        evade_slow_lock = 0;
        g_card_stop_roi.release();
        final_crop_img.release();
        return;
    }

    // 分类完成后的倒车阶段完全锁存本次结果，不再读取新的 Target/ROI，也不再运行NCNN。
    // 倒到设定距离后还要等编码器确认停稳；随后进入新绕行的固定角度阶段。
    // 实际倒车距离只补到 CLEAR 清障距离，不再污染 MERGE 并线衰减距离。
    if (card_reverse_phase != CARD_REVERSE_IDLE)
    {
        is_recognizing_now = 0;

        if (card_reverse_phase == CARD_REVERSE_BRAKING &&
            card_reverse_stopped == 1 && recog_staute == 1)
        {
            speed_left_error_integral = 0;
            speed_right_error_integral = 0;
            speed_left_error_last = 0;
            speed_right_error_last = 0;
            Gyro_error_last = 0;
            Gyro_output = 0;
            Gyro_Feedforward = 0.0f;
            Last_col_Differ = col_Differ;
            card_reverse_stopped = 0;
            card_reverse_phase = CARD_REVERSE_IDLE;

            printf("倒车完成并停稳: 实际倒车=%d, CLEAR总距离=%d+%d=%d\n",
                   static_cast<int>(card_reverse_real_dis), evade_clear_distance,
                   static_cast<int>(card_reverse_real_dis),
                   evade_clear_distance + static_cast<int>(card_reverse_real_dis));
            begin_card_evade_motion();
        }
        else
        {
            return;
        }
    }

    // 只禁止冷却期内启动“下一次”卡片识别；上一张卡片已锁存的倒车/绕行继续执行。
    if (card_recognition_cooldown_ticks > 0 &&
        recog_staute == 0 && card_stop_request == 0)
    {
        recog_cnt = 0;
        is_recognizing_now = 0;
        return;
    }

    // “满足NCNN输入条件”只负责触发停车，不在这一帧直接推理。
    // 停车请求一旦置位就锁存，后续即使目标短暂丢帧也继续刹停，并使用锁存ROI。
    bool live_roi_valid =
        (Target_flag == 1) &&
        g_ncnn_inited &&
        !final_crop_img.empty() &&
        final_crop_img.cols >= 20 &&
        final_crop_img.rows >= 20;

    if (card_stop_request == 1 && live_roi_valid)
    {
        g_card_stop_roi = final_crop_img.clone();
    }

    bool ncnn_input_ready =
        (car_running == 1) &&
        (card_sequence_recognition_allowed() == 1) &&
        live_roi_valid &&
        (target_y2 > DETECT_START_DIS) &&
        (recog_staute == 0) &&
        (pre_brake_frames >= 1);

    if (card_stop_request == 0 && ncnn_input_ready)
    {
        g_card_stop_roi = final_crop_img.clone();
        card_vehicle_stopped = 0;
        card_stop_request = 1;
        recog_cnt = 0;
        is_recognizing_now = 0;
        printf("卡片满足NCNN输入条件，开始强制停车: y2=%d, speed=%d, roi=%dx%d\n",
               target_y2, speed_car, g_card_stop_roi.cols, g_card_stop_roi.rows);
        return;
    }

    uint8 need_run = (Target_flag == 1) || (recog_staute == 1) || (card_stop_request == 1);

    // 🔴 状态机兜底清理：如果不需要运行，或者已经完成了识别进入了偏转锁死状态，则必然不在"识别中"
    if (!need_run || recog_staute == 1) 
    {
        is_recognizing_now = 0;
    }

    if (need_run)
    {

        // ==============================================
        // 策略1：距离过远重置计数器
        // ==============================================
        if (card_stop_request == 0 && target_y2 < DETECT_START_DIS && recog_staute == 0)
        {
            recog_cnt = 0;
            // 目标距离过远，尚未开始累积帧数
            is_recognizing_now = 0;
        }

        // ==============================================
        // 策略2：识别流程
        // ==============================================
        // 停车请求由上面的正式输入条件触发；这里只在编码器连续确认左右轮停稳后才允许推理。
        // 停稳前直接跳过整个识别分支，确保高置信快速确认也不会抢在刹车完成前进入绕行。
        if (card_stop_request == 1 && card_vehicle_stopped == 0 && recog_staute == 0)
        {
            is_recognizing_now = 0;
            return;
        }

        if (card_stop_request == 1 && card_vehicle_stopped == 1 && recog_staute == 0)
        {
            if (!g_stopped_announced)
            {
                g_stopped_announced = true;
                printf("车辆已停稳，开始NCNN识别: left=%d, right=%d, 最多%d次\n",
                       speed_left_now, speed_right_now, DETECT_CNT);
            }

            int fast_confirm_class = -1;
            float fast_confirm_prob = 0.0f;

            if (recog_cnt < DETECT_CNT)
            {
                // 🟢 目标已进入有效距离，且正在累积 NCNN 识别帧数！
                // 此刻正是"识别中"的物理时间窗口，将标志位拉高为 1
                is_recognizing_now = 1;

                ncnn_recog_run_roi_float();
                // capture_save_roi_mjpg("image",aa++);
                if(single_out != -1)
                {
                    detect_out[recog_cnt] = single_out;
                    detect_out_prob[recog_cnt] = single_out_prob;
                    recog_cnt++;

                    // 只允许“第一次有效结果”触发快速确认；后续结果继续收满3次综合判断。
                    if (recog_cnt == 1 &&
                        single_out >= 0 && single_out < NUM_CLASSES &&
                        single_out_prob > FAST_CONFIRM_PROB)
                    {
                        fast_confirm_class = single_out;
                        fast_confirm_prob = single_out_prob;
                    }
                }
            }

            // 收齐3次走综合判断；第一次有效结果 >95% 则立即快速确认。
            if (recog_cnt >= DETECT_CNT || fast_confirm_class >= 0)
            {
                // 🔴 规定的帧数已经收齐，识别计算阶段正式结束，标志位置回 0
                is_recognizing_now = 0;

                const int collected_count = recog_cnt;
                const bool fast_confirmed = (fast_confirm_class >= 0);
                recog_cnt = 0; // 无论什么结果，先清空计数器备用

                float sum_prob[NUM_CLASSES] = {0};
                
                // 1. 80% 置信度过滤墙 (代码注释写的是 80%，实际逻辑是 >= 0.60f 即 60%)
                for (int i = 0; i < collected_count; i++)
                {
                    int class_id = detect_out[i];
                    if (class_id >= 0 && class_id < NUM_CLASSES && detect_out_prob[i] >= 0.60f) 
                    {
                        sum_prob[class_id] += detect_out_prob[i];
                    }
                }
                
                // 2. 寻找最高分
                int best_class = -1;  
                float max_sum = 0.0f; 
                
                for (int i = 0; i < NUM_CLASSES; i++)
                {
                    // 快速确认已经锁定具体类别，控制台结果必须与触发类别一致。
                    if (fast_confirmed && i != fast_confirm_class) continue;
                    if (sum_prob[i] > max_sum)
                    {
                        max_sum = sum_prob[i];
                        best_class = i;
                    }
                }

                // 🔴 核心修改：判断是否有效，无效则强制触发直行
                if (best_class != -1) 
                {
                    // 正常情况：有达标目标
                    final_detect = best_class; 
                    fmfcnt++;
                    if (fast_confirmed)
                    {
                        printf("%d-> 首次高置信提前确认: %s (prob=%.4f > %.2f)\n",
                               fmfcnt, class_labels[final_detect],
                               fast_confirm_prob, FAST_CONFIRM_PROB);
                    }
                    else
                    {
                        printf("%d-> 识别到目标: %s (综合得分:%.4f) \n", fmfcnt, class_labels[final_detect], max_sum);
                    }
                }
                else
                {
                    // 异常情况：这3次数据置信度全低于阈值
                    // 赋值为 -1(未知)，避免 class_labels[-1] 数组越界死机
                    final_detect = -1; 
                    fmfcnt++;
                    printf("%d-> 识别为直行\n", fmfcnt);
                }

                // 🔴 无论是否达标，统统进入锁死状态！
                // 如果达标，按分类偏转/直行；如果不达标(final_detect == -1)，后续会默认执行直行

                // 🔴 修复中断抢占 Bug：先用临时变量计算，算完再统一赋值！
                // 模糊(final_detect==-1,置信度<0.6)时的默认动作由菜单 FzAct 决定：0左拐/1直行/2右拐；
                // 下面识别到有效类别时会覆盖它。
                int temp_action = fuzzy_default_action;
                if (temp_action < 0 || temp_action > 2) temp_action = 1; // 越界保护,退回直行

                // 3: grenade, 4: gun -> 武器(左)
                if (final_detect == 3 || final_detect == 4) {
                    temp_action = 0; 
                }
                // 1: Aidbag, 5: telescope -> 物资(右)
                else if (final_detect == 1 || final_detect == 5) {
                    temp_action = 2; 
                }
                // 0: APC, 2: ambulance -> 车辆
                else if (final_detect == 0 || final_detect == 2) {
                    temp_action = 1; 
                }

                // 🟢 一次性赋值给全局变量！这样定时器中断无论什么时候打断，
                // 读到的都是最终确定的结果，绝不会读到中间的过渡态。
                turn_action = temp_action;

                // 只有获得有效类别才算“成功识别”；三次结果均低于阈值而进入模糊兜底时不启动冷却。
                if (best_class != -1)
                {
                    start_card_recognition_cooldown();
                }

                // 每个目标只在“NCNN最终分类完成”这一处计数一次。到达 CdStop 后，本次识别结果
                // 已经按上方原逻辑输出，但不再进入倒车/绕行，保持当前已停稳状态并结束本次运行。
                card_classified_count++;
                if (card_stop_target_count > 0)
                {
                    printf("卡片计数: %d/%d | class=%d act=%d\n",
                           card_classified_count, card_stop_target_count,
                           final_detect, turn_action);
                    if (card_classified_count >= card_stop_target_count)
                    {
                        car_running = 0;
                        speed_left_target = 0;
                        speed_right_target = 0;
                        pre_brake = 0;
                        pre_brake_hint = 0;
                        pre_brake_frames = 0;
                        pre_brake_giveup = 0;
                        pre_brake_dis = 0;
                        card_stop_request = 0;
                        card_vehicle_stopped = 0;
                        card_reverse_phase = CARD_REVERSE_IDLE;
                        card_reverse_stopped = 0;
                        card_reverse_real_dis = 0;
                        recog_staute = 0;
                        Detect_dis_isp = 0;
                        card_evade_phase = CARD_EVADE_IDLE;
                        card_evade_phase_dis = 0;
                        card_evade_relative_yaw = 0.0f;
                        card_evade_target_yaw = 0.0f;
                        is_recognizing_now = 0;
                        evade_slow_lock = 0;
                        Target_flag = 0;
                        g_card_stop_roi.release();
                        final_crop_img.release();
                        g_stopped_announced = false;
                        speed_left_error_integral = 0;
                        speed_right_error_integral = 0;
                        Gyro_output = 0;
                        Gyro_Feedforward = 0.0f;

                        printf("停车原因: 已完成第%d个卡片分类（配置CdStop=%d），不执行绕行\n",
                               card_classified_count, card_stop_target_count);
                        return;
                    }
                }

                //积分距离判定
                {
                    // 环岛内(halo≠0)优先按【环岛方向×绕行方向】判顺/逆(环内 wd_flag_stable 判定不稳,不看它):
                    //   左环岛右绕行 / 右环岛左绕行 → EvaD_N(逆环)
                    //   左环岛左绕行 / 右环岛右绕行 → EvaD_S(顺环)
                    // 直行(act==1)不进此分支,落到下方 wd 逻辑(通常 EvaD_O)。
                    if ((left_halo_flag != 0 || right_halo_flag != 0) && (turn_action == 0 || turn_action == 2))
                    {
                        if ((left_halo_flag != 0 && turn_action == 2) || (right_halo_flag != 0 && turn_action == 0))
                        {
                            DETECT_PROCESS_DIS = EvaD_N;   // 左环岛右绕行 / 右环岛左绕行(逆环)
                            evade_reverse = 1;
                        }
                        else
                        {
                            DETECT_PROCESS_DIS = EvaD_S;   // 左环岛左绕行 / 右环岛右绕行(顺环)
                            evade_reverse = 0;
                        }
                    }
                    else if((wd_flag_stable==-1&&turn_action==2)||(wd_flag_stable==1&&turn_action==0))//左弯道右偏||右弯道左偏(逆弯)
                    {
                        DETECT_PROCESS_DIS = EvaD_N;
                        evade_reverse = 1;   // 弯道逆绕行:锁定,供绕行前瞻行提前(target_y2-10),防冲出赛道
                    }
                    else if((wd_flag_stable==-1&&turn_action==0)||(wd_flag_stable==1&&turn_action==2))//左弯道左偏||右弯道右偏(顺弯)
                    {
                        DETECT_PROCESS_DIS = EvaD_S;
                        evade_reverse = 0;
                    }
                    else //普通直道偏转
                    {
                        DETECT_PROCESS_DIS = EvaD_O;
                        evade_reverse = 0;
                    }
                }
                // 锁定绕行场景,供绕行方向环/角速度环选独立参数:0=直道(EV_*Z) 1=弯道(EV_*W) 2=环岛(EV_*H)。
                // 环岛优先级最高:环内曲率比普通弯道紧得多,单列一档;且不看 wd_flag_stable(环内它判定不稳,
                // 偶判0会让整段绕行错用直道参数,咬边能力腰斩)。
                if (left_halo_flag != 0 || right_halo_flag != 0) evade_curve = 2;   // 环岛
                else if (wd_flag_stable != 0)                    evade_curve = 1;   // 弯道
                else                                             evade_curve = 0;   // 直道

                // 只有真正需要左右绕行的类别才倒车；直行车辆保持原逻辑直接通过。
                // 倒车期间 recog_staute 保持1，相当于锁存本次分类和场景，防止再次识别。
                card_reverse_real_dis = 0;
                card_reverse_stopped = 0;
                int reverse_speed_abs = (card_reverse_speed >= 0) ?
                                        card_reverse_speed : -card_reverse_speed;
                if ((turn_action == 0 || turn_action == 2) &&
                    card_reverse_target_dis > 0 && reverse_speed_abs > 0)
                {
                    card_reverse_phase = CARD_REVERSE_DRIVING;
                }
                else
                {
                    card_reverse_phase = CARD_REVERSE_IDLE;
                }

                // [排查] 打出本次绕行实际用的距离和弯型判定:若 EvaD=0 或 wd/act 与预期不符,
                // 就是"改菜单没用"的原因(改的不是被选中的那一项)
                // printf("绕行开始: EvaD=%d (wd=%d,act=%d)\n", DETECT_PROCESS_DIS, wd_flag_stable, turn_action);

                recog_staute = 1;
                // 当前帧已经完成分类并进入绕行，立即结束预刹车状态。
                // update_pre_brake 在本函数之前调用；若不在这里清除，pre_brake 会残留到下一图像帧。
                pre_brake = 0;
                pre_brake_frames = 0;
                card_stop_request = 0;
                card_vehicle_stopped = 0;
                g_card_stop_roi.release();
                g_stopped_announced = false;
                Detect_dis_isp = 0;  // 倒车时保持0；直接绕行由 begin_card_evade_motion() 立即置1
                // 进绕行:清掉刹车段的反向积分偏置(起刹时打的-5000)。刹车窗口短时该偏置几乎没消耗,
                // 若带进绕行(软PID同一分支,Ski=30时拖拽达-15000)会把绕行/直行起步狠狠按住。
                // 刹车归刹车(起刹边沿打-5000),绕行归绕行(此处清0),各清一次、互不拖累。
                speed_left_error_integral  = 0;
                speed_right_error_integral = 0;

                if (card_reverse_phase == CARD_REVERSE_DRIVING)
                {
                    printf("分类已锁存，开始倒车: 目标距离=%d, 速度=%d, act=%d, scene=%d\n",
                           card_reverse_target_dis, reverse_speed_abs,
                           turn_action, evade_curve);
                }
                else
                {
                    begin_card_evade_motion();
                }
            }
        }

        // ==============================================
        // 策略3：新绕行状态机的图像侧处理
        // ==============================================
        if (recog_staute == 1 && card_reverse_phase == CARD_REVERSE_IDLE)
        {
            // TURN_IN 与 CLEAR 由陀螺仪角度闭环接管，不读取图像中线。
            // MERGE 才重新使用图像：在真实中线上叠加一个按 smoothstep 衰减的虚拟偏移，
            // 从 EvOft 平滑回到0。偏移为0时虚拟中线与真实中线完全相同。
            if (card_evade_phase == CARD_EVADE_MERGE &&
                (turn_action == 0 || turn_action == 2))
            {
                int merge_distance = (DETECT_PROCESS_DIS > 0) ? DETECT_PROCESS_DIS : 1;
                float progress = static_cast<float>(card_evade_phase_dis) /
                                 static_cast<float>(merge_distance);
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;
                float smooth = progress * progress * (3.0f - 2.0f * progress);
                int offset_abs = (EVADE_OFFSET >= 0) ? EVADE_OFFSET : -EVADE_OFFSET;
                int offset = static_cast<int>(
                    static_cast<float>(offset_abs) * (1.0f - smooth) + 0.5f);
                if (turn_action == 0) offset = -offset;

                for (int i = 0; i < IMAGE_HEIGHT; i++)
                {
                    int virtual_center = centralline[i] + offset;
                    if (virtual_center < 0) virtual_center = 0;
                    if (virtual_center > IMAGE_WIDTH - 1) virtual_center = IMAGE_WIDTH - 1;
                    centralline[i] = virtual_center;
                }
            }
            else if (card_evade_phase == CARD_EVADE_SETTLE)
            {
                // 此阶段不再修改 centralline，连续确认真实边界可用且车身没有剧烈甩动。
                if (card_evade_real_track_is_stable()) card_evade_settle_frames++;
                else                                   card_evade_settle_frames = 0;

                if (card_evade_settle_frames >= CARD_EVADE_SETTLE_REQUIRED_FRAMES)
                {
                    finish_card_evade_motion("真实中线稳定");
                }
                else if (card_evade_phase_dis >= CARD_EVADE_SETTLE_MAX_DIS)
                {
                    // 边线长期不完整时不能永久卡住；超距后退出，但打印原因便于实车排查。
                    finish_card_evade_motion("SETTLE超距兜底");
                }
            }
            else if (card_evade_phase == CARD_EVADE_STRAIGHT &&
                     card_evade_phase_dis >= DETECT_PROCESS_DIS)
            {
                finish_card_evade_motion("直行通过距离完成");
            }
        }
    }
}



void evaluate_apc_dataset(const char* folder_path, int apc_class_id)
{
    int total_images = 772 + 1; // 0 到 772 共 773 张
    int correct_count = 0;
    int valid_count = 0;        // 实际成功读取的图片数

    printf("\n🚀 开始批量测试 APC 数据集...\n");

    for (int i = 0; i <= 772; i++) {
        char img_path[256];
        // 动态拼接路径，例如: "/home/root/dataset/APC/APC_0.jpg"
        snprintf(img_path, sizeof(img_path), "%s/APC_%d.jpg", folder_path, i);

        // 调用刚才改造的有返回值的推理函数
        int pred_idx = ncnn_test_single_jpg_for_eval(img_path);

        if (pred_idx == -1) {
            // 图片不存在或读取失败，跳过
            continue; 
        }

        valid_count++;

        // 🎯 核心逻辑：比对预测结果与真实标签
        if (pred_idx == apc_class_id) {
            correct_count++;
        } else {
            // 🔴 极其重要的 Debug 信息：把认错的图打出来！
            printf("❌ 误判发生！图片: APC_%d.jpg 被错误识别为了 -> %s\n", 
                   i, class_labels[pred_idx]);
        }

        // 简单的进度条提示，每 100 张打印一次
        if (i > 0 && i % 100 == 0) {
            printf("... 已处理 %d 张 ...\n", i);
        }
    }

    // ================== 输出最终战报 ==================
    if (valid_count > 0) {
        float accuracy = (float)correct_count / valid_count * 100.0f;
        printf("\n==================================================\n");
        printf("🎯 APC 数据集测试报告\n");
        printf("--------------------------------------------------\n");
        printf("共寻找到有效图片 : %d 张\n", valid_count);
        printf("模型正确识别数量 : %d 张\n", correct_count);
        printf("误判或无法识别数 : %d 张\n", valid_count - correct_count);
        printf("⚡ 最终准确率 (Accuracy) : %.2f%%\n", accuracy);
        printf("==================================================\n\n");
    } else {
        printf("❌ 没找到任何有效图片，请检查文件夹路径是否正确！\n");
    }
}

void EvaSD_decision()
{
       //直道||弯道起始推理距离判定
        DETECT_START_DIS = EvaSD_Z;
        if(left_halo_flag == 4 || right_halo_flag == 4) //环岛只在状态4检测/识别色块
        {
            DETECT_START_DIS = EvaSD_H;
        }
        else if(wd_flag_stable==-1||wd_flag_stable==1) //弯道(用去抖后的稳定值,防起始距离乱跳)
        {
            DETECT_START_DIS = EvaSD_W;
        }
}
