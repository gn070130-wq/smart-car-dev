#ifndef NCNN_MODEL_H
#define NCNN_MODEL_H
#include  "zf_common_headfile.hpp"
extern int DETECT_PROCESS_DIS ;//按场景锁定的MERGE衰减距离；直行动作作为通过距离
#define OFFSET_TOP     20   // 图像上方偏移量（小）
#define OFFSET_BOTTOM  80  // 图像下方偏移量（大）
#define NUM_CLASSES 6  
extern bool g_ncnn_inited;
extern const char* class_labels[];
extern int single_out;//单个识别输出
extern  float single_out_prob;//单个识别输出置信度
extern int Detect_dis_isp;//本次卡片动作已启动标志
extern int Detect_real_dis;//绕行完全退出后的识别冷却实际距离
extern int evade_cooldown_active;//绕行退出后的冷却锁:1=冷却中,期间禁止检测/推理/绕行
// 绕行完全退出后才开始累计的识别冷却路程。冷却只禁止再次检测/识别，
// 不再覆盖正常速度、前瞻行或绕行PID。
#define EVADE_COOLDOWN_DIS 120
extern const char* class_labels[NUM_CLASSES];
extern int turn_action;//偏转动作
extern int fuzzy_default_action;//置信度<0.6(模糊)时的默认动作:0左拐/1直行/2右拐(菜单 FzAct)
extern int EVADE_OFFSET;//偏转补线像素：偏转时中线直接平移量(菜单 EvOft)
extern int evade_turn_angle;//停车/倒车完成后的固定偏转角绝对值(菜单 EvAng)
extern int evade_clear_distance;//固定角度建立后保持航向向前的清障距离(菜单 EvClr)

// 新卡片绕行状态机。倒车仍由 CardReversePhase 独立管理；倒车停稳后从 TURN_IN 开始。
enum CardEvadePhase
{
    CARD_EVADE_IDLE = 0,
    CARD_EVADE_TURN_IN = 1,
    CARD_EVADE_CLEAR = 2,
    CARD_EVADE_MERGE = 3,
    CARD_EVADE_SETTLE = 4,
    CARD_EVADE_STRAIGHT = 5
};
extern volatile int card_evade_phase;
extern volatile int card_evade_phase_dis;
extern volatile float card_evade_relative_yaw;
extern volatile float card_evade_target_yaw;

extern int EvaD_N; //逆弯道偏转距离
extern int EvaD_S; //顺弯道偏转距离
extern int EvaD_O; //普通偏转距离
extern int EvaSD_H ; //环岛内部单独的识别距离
extern int EvaSD_Z; //直道起始识别距离
extern int EvaSD_W; //弯道起始识别距离
// 🚀 新增：正在识别中的标志位 (0: 未在识别, 1: 正在跑模型识别中)
extern int is_recognizing_now ;
// 初始化模型
bool ncnn_init(const char* param_path, const char* bin_path);

// 识别单张 JPG 图片
int ncnn_test_single_jpg(const char* image_path);
void ncnn_benchmark_single_jpg(const char* image_path, int loops );

// 摄像头实时识别（ROI 区域）
void ncnn_recog_run_roi_float();
void Detect_process();//识别处理策略
void reset_ncnn_runtime_state(void);//发车前清除分类/绕行运行态，不重载模型

void evaluate_apc_dataset(const char* folder_path, int apc_class_id);
void EvaSD_decision();
#endif
