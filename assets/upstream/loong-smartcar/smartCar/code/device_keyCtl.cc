#include "device_keyCtl.h"

int fd_key;
thread key_thread;

int Key_init(void)
{
    fd_key = open(KEY_FILE_ADD, O_RDONLY);
    if(fd_key < 0) {
        cerr << "Error: 按键初始化失败!!" << endl;
        return -1;
    } 
    return 0;
}

void keythread_Star(void)
{
    key_thread = thread(key_listener);
    key_thread.detach();
}


inline void Key1_Ctrl(void)
{
    Menu_Setup_Index();
}

inline void Key2_Ctrl(void)
{
    Menu_enterFuntion();
}

inline void Key3_Ctrl(void)
{
    Menu_upFuntion();
}

inline void Key4_Ctrl(void)
{
    Menu_quitFuntion();
}

inline void Key5_Ctrl(void)
{
    go_iCar();
}

inline void Key6_Ctrl(void)
{
    Menu_downFuntion();
}

inline void Key7_Ctrl(void)
{
    fan_Enable();
}

inline void Key8_Ctrl(void)
{
    reset_ALL_state();
}

void key_listener(void)
{
    struct input_event ev;
    while(1) {
        if(read(fd_key, &ev, sizeof(ev)) == sizeof(ev)) {
            if(ev.type == EV_KEY && ev.value == 1) {
                switch(ev.code) {
                    case 2:             //中间
                        Key1_Ctrl();
                        break;
                    case 3:             //右边
                        Key2_Ctrl();
                        break;
                    case 4:             //上边
                        Key3_Ctrl();
                        break;
                    case 5:             //左边
                        Key4_Ctrl();
                        break;
                    case 6:             //3
                        Key5_Ctrl();
                        break;
                    case 7:             //下边
                        Key6_Ctrl();
                        break;
                    case 8:             //2
                        Key7_Ctrl();
                        break;
                    case 9:             //1
                        Key8_Ctrl();
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

