#include "key.hpp"

/*********************************************************************************************************************/
// 按键状态枚举（便于理解，替代魔法值）
typedef enum {
    KEY_STATE_RELEASED = 1, // 按键释放（高电平）
    KEY_STATE_PRESSED  = 0  // 按键按下（低电平）
} KeyState;

/*
 * @brief 按键消抖+短按/长按判定（单函数封装）
 * @param key_obj 传入zf_driver_gpio类型的按键对象（如key_1/key_2）
 * @param long_press_ms 长按判定阈值（推荐500ms，可自定义）
 * @return uint8_t 0=释放，1=短按，2=长按
 */
uint8_t key_check_status(zf_driver_gpio& key_obj, uint16_t long_press_ms)
{
    // ========== 静态变量：中断内持久化状态（核心） ==========
    static uint8_t is_pressed = 0;        // 是否处于按下状态（0=否，1=是）
    static uint32_t press_tick = 0;       // 按下累计tick数（1tick=10ms，适配PIT中断）
    static uint8_t short_press_printed = 0; // 短按是否已打印（避免重复）
    static uint8_t long_press_reached = 0;  // 是否达到长按阈值（达标后一直返回2）

    const uint8_t KEY_PRESSED_LEVEL = 0;  // 按下为低电平（按需调整）
    uint8_t ret = 0;                      // 默认返回0

    // 第一步：简易消抖（中断内无延时，两次读取电平）
    uint8_t level1 = key_obj.get_level();
    uint8_t level2 = key_obj.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : is_pressed; // 消抖后稳定电平

    // ========== 第二步：核心逻辑（短按单次打印/长按持续打印） ==========
    // 1. 当前按键按下（消抖后）
    if (curr_level == KEY_PRESSED_LEVEL) {
        is_pressed = 1; // 标记按下状态
        
        // 累计按下时长（2ms中断→每tick+2ms）
        press_tick += 200;

        // 长按判定：达标后标记，且只要不松开就一直返回2（持续打印）
        if (press_tick >= long_press_ms) {
            long_press_reached = 1; // 标记达到长按阈值
            ret = 2;                // 一直返回2，直到松开
        }
    }
    // 2. 当前按键释放（消抖后）
    else {
        // 短按判定：按下→释放 + 未达到长按阈值 + 未打印过 → 返回1（仅一次）
        if (is_pressed == 1 && long_press_reached == 0 && short_press_printed == 0) {
            ret = 1;
            short_press_printed = 1; // 标记短按已打印，避免重复
        }

        // 释放后重置所有状态（为下次按键做准备）
        is_pressed = 0;
        press_tick = 0;
        long_press_reached = 0;
        short_press_printed = 0;
    }

    return ret;
}

uint8_t key_short_press_detect(zf_driver_gpio& key_obj)
{
    #define KEY_MAX 5

    typedef struct
    {
        zf_driver_gpio* obj;

        uint8_t is_pressed;          // 是否处于按下状态
        uint8_t short_press_flag;    // 是否已经触发过短按（防重复）
    } key_state_t;

    static key_state_t keys[KEY_MAX] = {0};

    const uint8_t KEY_PRESSED_LEVEL = 0;  // 0=按下

    key_state_t* k = NULL;

    // ===== 查找 =====
    for(int i = 0; i < KEY_MAX; i++)
    {
        if(keys[i].obj == &key_obj)
        {
            k = &keys[i];
            break;
        }
    }

    // ===== 注册 =====
    if(k == NULL)
    {
        for(int i = 0; i < KEY_MAX; i++)
        {
            if(keys[i].obj == NULL)
            {
                keys[i].obj = &key_obj;
                keys[i].is_pressed = 0;
                keys[i].short_press_flag = 0;
                k = &keys[i];
                break;
            }
        }
    }

    if(k == NULL) return 1; // 超出最大按键数

    // ===== 消抖（简单双读）=====
    uint8_t level1 = key_obj.get_level();
    uint8_t level2 = key_obj.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : k->is_pressed;

    uint8_t ret = 1;   // 默认返回1

    // ===== 按下 =====
    if(curr_level == KEY_PRESSED_LEVEL)
    {
        k->is_pressed = 1;
    }
    // ===== 松开 =====
    else
    {
        // 只有经历过按下，才算短按
        if(k->is_pressed == 1 && k->short_press_flag == 0)
        {
            ret = 0;   // ✅ 短按触发
            k->short_press_flag = 1;
        }

        // 释放后复位
        k->is_pressed = 0;
        k->short_press_flag = 0;
    }

    return ret;
}
// #include "key.hpp"

// /*********************************************************************************************************************/
// // 按键状态枚举
// typedef enum {
//     KEY_STATE_RELEASED = 1, // 松开
//     KEY_STATE_PRESSED  = 0  // 按下
// } KeyState;

// /*********************************************************************************************************************/
// // ========================== KEY1 ==========================
uint8_t key1_short_press_detect(void)
{
    static uint8_t is_pressed = 0;

    uint8_t level1 = key_1.get_level();
    uint8_t level2 = key_1.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : KEY_STATE_RELEASED;

    if(curr_level == KEY_STATE_PRESSED)
    {
        is_pressed = 1;
    }
    else
    {
        if(is_pressed == 1)
        {
            is_pressed = 0;
            return 0;   // 短按触发
        }
    }

    return 1;
}

// /*********************************************************************************************************************/
// ========================== KEY2 ==========================
uint8_t key2_short_press_detect(void)
{
    static uint8_t is_pressed = 0;

    uint8_t level1 = key_2.get_level();
    uint8_t level2 = key_2.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : KEY_STATE_RELEASED;

    if(curr_level == KEY_STATE_PRESSED)
    {
        is_pressed = 1;
    }
    else
    {
        if(is_pressed == 1)
        {
            is_pressed = 0;
            return 0;
        }
    }

    return 1;
}

/*********************************************************************************************************************/
// ========================== KEY3（短按+长按） ==========================
/*
 * @brief  按键3 短按/长按判定（替代原key5硬件）
 * @return 0=无事件，1=短按（松开瞬间触发），2=长按（达到阈值时触发一次）
 * @note   在定时中断中周期性调用；长按阈值通过 KEY3_LONG_PRESS_COUNT 调整
 *         若中断周期为10ms，50次≈500ms；若为2ms，250次≈500ms
 */
#define KEY3_LONG_PRESS_COUNT  50  // 长按判定计数阈值（按中断周期调整）

uint8_t key3_press_detect(void)
{
    static uint8_t  is_pressed     = 0;  // 是否处于按下状态
    static uint16_t press_count    = 0;  // 按下持续计数
    static uint8_t  long_triggered = 0;  // 长按是否已触发（防重复）

    uint8_t level1 = key_3.get_level();
    uint8_t level2 = key_3.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : KEY_STATE_RELEASED;

    // ---- 按下中 ----
    if(curr_level == KEY_STATE_PRESSED)
    {
        is_pressed = 1;
        press_count++;

        // 长按达标 → 仅触发一次
        if(press_count >= KEY3_LONG_PRESS_COUNT && long_triggered == 0)
        {
            long_triggered = 1;
            return 2;  // 长按事件
        }
    }
    // ---- 松开 ----
    else
    {
        if(is_pressed == 1 && long_triggered == 0)
        {
            // 松开前未达到长按阈值 → 短按
            is_pressed  = 0;
            press_count = 0;
            return 1;  // 短按事件
        }

        // 复位所有状态
        is_pressed     = 0;
        press_count    = 0;
        long_triggered = 0;
    }

    return 0;  // 无事件
}

/*********************************************************************************************************************/
// ========================== KEY4 ==========================
uint8_t key4_short_press_detect(void)
{
    static uint8_t is_pressed = 0;

    uint8_t level1 = key_4.get_level();
    uint8_t level2 = key_4.get_level();
    uint8_t curr_level = (level1 == level2) ? level1 : KEY_STATE_RELEASED;

    if(curr_level == KEY_STATE_PRESSED)
    {
        is_pressed = 1;
    }
    else
    {
        if(is_pressed == 1)
        {
            is_pressed = 0;
            return 0;
        }
    }

    return 1;
}

// KEY5 已移除（硬件取消），其功能由 KEY3 长按替代