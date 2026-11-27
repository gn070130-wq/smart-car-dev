#include "menu.h"

#define KEY_1_PATH        ZF_GPIO_KEY_1
#define KEY_2_PATH        ZF_GPIO_KEY_2
#define KEY_3_PATH        ZF_GPIO_KEY_3
#define KEY_4_PATH        ZF_GPIO_KEY_4
zf_driver_gpio  key_1(KEY_1_PATH, O_RDWR);
zf_driver_gpio  key_2(KEY_2_PATH, O_RDWR);
zf_driver_gpio  key_3(KEY_3_PATH, O_RDWR);
zf_driver_gpio  key_4(KEY_4_PATH, O_RDWR);

uint8_t Key_GetNum()
{
    uint8_t KeyNum=0;
    if(key_1.get_level()==0)
    {
        system_delay_ms(10);			//延时消抖
		while (key_1.get_level()==0);	//等待按键松手
        KeyNum=1;
    }
    if(key_2.get_level()==0)
    {
        system_delay_ms(10);			//延时消抖
		while (key_2.get_level()==0);	//等待按键松手
        KeyNum=2;
    }
    if(key_3.get_level()==0)
    {
        system_delay_ms(10);			//延时消抖
		while (key_3.get_level()==0);	//等待按键松手
        KeyNum=3;
    }
    if(key_4.get_level()==0)
    {
        system_delay_ms(10);			//延时消抖
		while (key_4.get_level()==0);	//等待按键松手
        KeyNum=4;
    }
    return KeyNum;
}