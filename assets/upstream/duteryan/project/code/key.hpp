#ifndef __KEY_hpp__
#define __KEY_hpp__
#include  "zf_common_headfile.hpp"
#define KEY_1_PATH        ZF_GPIO_KEY_1
#define KEY_2_PATH        ZF_GPIO_KEY_2
#define KEY_3_PATH        ZF_GPIO_KEY_3
#define KEY_4_PATH        ZF_GPIO_KEY_4
#define KEY_5_PATH        ZF_GPIO_KEY_5
#define LONG_PRESS_THRESHOLD_MS (1000) // 长按判定阈值（ms）
// uint8_t key_check_status(zf_driver_gpio& key_obj, uint16_t long_press_ms);//0=释放，1=短按，2=长按
uint8_t key_short_press_detect(zf_driver_gpio& key_obj);
uint8_t key1_short_press_detect(void);
uint8_t key2_short_press_detect(void);
uint8_t key4_short_press_detect(void);  
uint8_t key5_short_press_detect(void);
uint8_t key3_press_detect(void);
#endif
