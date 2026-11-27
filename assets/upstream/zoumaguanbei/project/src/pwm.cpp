#include "pwm.h"
#include <unistd.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "zf_driver_delay.h"
#include <iomanip>

using namespace std;

#define PWM_CHIP_PATH "/sys/class/pwm/pwmchip"

struct PwmHardwareMap {
    int chip;
    int channel;
    const char* polarity; // 新增极性属性
};

static const PwmHardwareMap pwm_map[] = {   
    {0, 0, "inversed"}, // PWM0 左轮
    {1, 0, "inversed"}, // PWM1 右轮
    {2, 0, "normal"},   // PWM2
    {2, 1, "normal"},   // PWM3 狗叫器
    {2, 2, "normal"},   // PWM4 电调
    {2, 3, "normal"}    // PWM5 舵机
};

static long long current_periods[6] = {0};
static double current_duty_ratios[6] = {0.0};

/**
 * 内部辅助函数：检查文件夹是否存在
 */
static bool directory_exists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return false;
    return (info.st_mode & S_IFDIR);
}

/**
 * 内部辅助函数：死循环校验写入
 */
static void sysfs_write_until_success(const char* path, const char* value) {
    string target = value;
    while (true) {
        FILE *fp = fopen(path, "w");
        if (fp != NULL) {
            fprintf(fp, "%s", value);
            fclose(fp);
        }

        usleep(30000); 

        ifstream fs(path);
        string current_state;
        if (fs >> current_state) {
            if (current_state == target) return; 
        }
    }
}

/**
 * 标准快速写入：专门处理 long long 类型数值
 */
static void sysfs_write_long_long(const char* path, long long value) {
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%lld", value);
        fclose(fp);
    }
}

/**
 * 初始化并设置 PWM
 * 包含：检查导出 -> 取消导出 -> 重新导出 -> 强制状态校验
 */
/**
 * 初始化并设置 PWM
 * 包含：检查导出 -> 取消导出 -> 重新导出 -> 强制状态校验 -> 设置极性 -> 设置周期/占空比
 */
void pwm_init(int pwm_num, int frequency, int duty_cycle) {
    if (pwm_num < 0 || pwm_num > 5 || frequency <= 0) return;

    int chip = pwm_map[pwm_num].chip;
    int ch = pwm_map[pwm_num].channel;
    
    char path_pwm[256], path_export[256], path_unexport[256];
    char path_e[256], path_p[256], path_d[256];

    snprintf(path_pwm, sizeof(path_pwm), "%s%d/pwm%d", PWM_CHIP_PATH, chip, ch);
    snprintf(path_export, sizeof(path_export), "%s%d/export", PWM_CHIP_PATH, chip);
    snprintf(path_unexport, sizeof(path_unexport), "%s%d/unexport", PWM_CHIP_PATH, chip);
    
    snprintf(path_e, sizeof(path_e), "%s%d/pwm%d/enable", PWM_CHIP_PATH, chip, ch);
    snprintf(path_p, sizeof(path_p), "%s%d/pwm%d/period", PWM_CHIP_PATH, chip, ch);
    snprintf(path_d, sizeof(path_d), "%s%d/pwm%d/duty_cycle", PWM_CHIP_PATH, chip, ch);

    // 1. 导出节点逻辑
    if (directory_exists(path_pwm)) {
        FILE *fp_un = fopen(path_unexport, "w");
        if (fp_un) { fprintf(fp_un, "%d", ch); fclose(fp_un); usleep(50000); }
    }
    FILE *fp_ex = fopen(path_export, "w");
    if (fp_ex) { fprintf(fp_ex, "%d", ch); fclose(fp_ex); usleep(50000); }
   
    // 3. 计算并设置时间参数
    current_duty_ratios[pwm_num] = duty_cycle / 10000.0;
    long long new_period = 1000000000LL / frequency; 
    long long new_duty = (long long)(new_period * current_duty_ratios[pwm_num]);

    sysfs_write_long_long(path_d, 0LL); 
    sysfs_write_long_long(path_p, new_period);
    sysfs_write_long_long(path_d, new_duty);

    // 4. 开启使能
    //sysfs_write_until_success(path_e, "1");
    current_periods[pwm_num] = new_period;
}

void pwm_set_polarity(int pwm_num, const char* polarity) {
    if (pwm_num < 0 || pwm_num > 5) return;

    int chip = pwm_map[pwm_num].chip;
    int ch = pwm_map[pwm_num].channel;
    
    char path_e[256], path_pol[256];
    snprintf(path_e, sizeof(path_e), "%s%d/pwm%d/enable", PWM_CHIP_PATH, chip, ch);
    snprintf(path_pol, sizeof(path_pol), "%s%d/pwm%d/polarity", PWM_CHIP_PATH, chip, ch);

    // 1. 必须先关闭使能才能修改极性
    sysfs_write_until_success(path_e, "0");
    msleep(50); // 短暂等待确保硬件状态切换

    // 2. 写入极性并校验
    sysfs_write_until_success(path_pol, polarity);
    sysfs_write_until_success(path_e, "1");
}

void pwm_set_duty(int pwm_num, int duty_cycle) {
    if (pwm_num < 0 || pwm_num > 5) return;
    if (duty_cycle < 0) duty_cycle = 0;
    if (duty_cycle > 10000) duty_cycle = 10000;

    int chip = pwm_map[pwm_num].chip;
    int ch = pwm_map[pwm_num].channel;
    char path_d[256];

    current_duty_ratios[pwm_num] = duty_cycle / 10000.0;
    long long duty_ns = (long long)(current_periods[pwm_num] * current_duty_ratios[pwm_num]);

    snprintf(path_d, sizeof(path_d), "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle", chip, ch);
    sysfs_write_long_long(path_d, duty_ns);
}

void pwm_test() {

    pwm_set_polarity(LEFT_WHELL_PWM, pwm_map[LEFT_WHELL_PWM].polarity);
    pwm_set_polarity(RIGHT_WHELL_PWM, pwm_map[RIGHT_WHELL_PWM].polarity);
    pwm_set_polarity(POWER_TRANSFER, pwm_map[POWER_TRANSFER].polarity);
    // 英文描述映射，对应脚本中的中文描述
    const char* descriptions[] = {
        "LeftWheel",  // PWM0
        "RightWheel", // PWM1
        "PWM2",       // PWM2
        "Buzzer",     // PWM3
        "PWM4",       // PWM4
        "PWM5"        // PWM5
    };

    for (int i = 0; i < 6; ++i) {
        int chip = pwm_map[i].chip;
        int ch = pwm_map[i].channel;
        
        char base_path[256];
        snprintf(base_path, sizeof(base_path), "%s%d/pwm%d", PWM_CHIP_PATH, chip, ch);

        // 检查路径是否存在
        if (!directory_exists(base_path)) {
            std::cout << std::left 
                      << std::setw(14) << descriptions[i] 
                      << std::setw(14) << "Unexported" 
                      << "-" << std::endl;
            continue;
        }

        // 读取极性
        std::string polarity = "unknown";
        std::ifstream pol_file(std::string(base_path) + "/polarity");
        if (pol_file >> polarity) {}
        pol_file.close();

        // 判定 Duty 含义
        std::string duty_meaning;
        if (polarity == "normal") {
            duty_meaning = "High";
        } else if (polarity == "inversed") {
            duty_meaning = "Low";
        } else {
            duty_meaning = "Unknown";
        }

        // 读取使能状态
        std::string enable_val;
        std::ifstream en_file(std::string(base_path) + "/enable");
        if (en_file >> enable_val) {}
        en_file.close();

        std::string run_status = (enable_val == "1") ? "ON" : "OFF";

        // 格式化输出
        std::cout << std::left 
                  << std::setw(14) << descriptions[i] 
                  << std::setw(14) << polarity 
                  << run_status << std::endl;
    }

}

void pwm_clean() {
    for (int i = 0; i < 6; ++i) {
        int chip = pwm_map[i].chip;
        int ch = pwm_map[i].channel;
        
        char path_pwm[256], path_e[256], path_d[256];
        snprintf(path_pwm, sizeof(path_pwm), "%s%d/pwm%d", PWM_CHIP_PATH, chip, ch);
        snprintf(path_e, sizeof(path_e), "%s%d/pwm%d/enable", PWM_CHIP_PATH, chip, ch);
        snprintf(path_d, sizeof(path_d), "%s%d/pwm%d/duty_cycle", PWM_CHIP_PATH, chip, ch);

        if (directory_exists(path_pwm)) {
            sysfs_write_long_long(path_d, 0LL);
            sysfs_write_until_success(path_e, "0");
            usleep(10000); 
        }
        
        current_periods[i] = 0;
        current_duty_ratios[i] = 0.0;
    }
}

 /* int power_sum = 500;
    const int STEP = 100;
    const int UPPER_LIMIT = 900;
    const int LOWER_LIMIT = 500;

    while(1) {
        int key = getch();

        // 处理方向键。在 Linux 终端中，方向键通常发送三字节序列：27, 91, (65-68)
        if (key == 27) { // 识别起始转义字符
            if (getch() == 91) {
                switch (getch()) {
                    case 65: // 上方向键 (Up)
                        if (power_sum + STEP <= UPPER_LIMIT) {
                            power_sum += STEP;
                        }
                        break;
                    case 66: // 下方向键 (Down)
                        if (power_sum - STEP >= LOWER_LIMIT) {
                            power_sum -= STEP;
                        }
                        break;
                }
            }
        } else if (key == 'q') { // 额外增加一个退出按键
            break;
        }

        // 执行 PWM 设置
        pwm_set_duty(POWER_TRANSFER, power_sum);
        
        // 刷新显示
        system("clear");
        cout << "Current PWM: " << power_sum << endl;
        cout << "Use UP/DOWN arrows to adjust (q to quit)" << endl;
        
        usleep(100000); // 100ms
    } */