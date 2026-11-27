#include "zf_common_headfile.h"
#include "cst816.h"

int *keyboard_scan(int fd_keyboard)
{

    struct input_event ev;
    static int key[2] = {0, 0};
    static int no_key[2] = {-1, -1};
    // 读取输入事件
    ssize_t n = read(fd_keyboard, &ev, sizeof(struct input_event));

    if (n == (ssize_t)sizeof(struct input_event))
    {

        key[0] = ev.value;
        key[1] = ev.code;

        if (ev.type == EV_KEY)
        {

            if (ev.value == 0)
            {

                return no_key;
            }
            return key;
        }
    }

    if (key[0] == 2 || key[0] == 1)
    {

        return key;
    }
    return no_key;
}

int keyboard_scan_gpio()
{

    if (gpio_get(ENTER_GPIO) == 0)
    {

        return ENTER;
    }

    /* if(gpio_get(ESC_GPIO) == 0){

        return ESC;
    } */

    if (gpio_get(UP_GPIO) == 0)
    {

        return UP;
    }

    if (gpio_get(DOWN_GPIO) == 0)
    {

        return DOWN;
    }

    if (gpio_get(LEFT_GPIO) == 0)
    {

        return LEFT;
    }

    if (gpio_get(RIGHT_GPIO) == 0)
    {

        return RIGHT;
    }
    return -1;
}
#include "my_timeer.h"
void cst816_test(){
    uint64_t start_time = get_wall_time_ns();
    CST816_Scan();
    uint64_t end_time = get_wall_time_ns();
    uint64_t elapsed_time = end_time - start_time;
    
    if (TouchInfo.flag){
        cout << "used time: " << elapsed_time / 1e6 << " ms" << endl;

    }
}
int keyboard_scan_cst816()
{

    CST816_Scan(); /* 轮询触摸状态，更新TouchInfo结构体 */
    /* 检查是否有触摸事件（由PIT定时器回调函数设置） */
    if (TouchInfo.flag)
    {   
        TouchInfo.flag = 0;
        //printf("Touch detected: x=%d, y=%d\n", TouchInfo.x[0], TouchInfo.y[0]);
        if (TouchInfo.x[0] < 120)
        {

            if (TouchInfo.y[0] < 106)
            {

                return ENTER;
            }
            else if (TouchInfo.y[0] < 214)
            {

                return UP;
            }
            else
            {

                return ESC;
            }
        }

        else if (TouchInfo.x[0] >= 120)
        {

            if (TouchInfo.y[0] < 106)
            {

                return LEFT;
            }
            else if (TouchInfo.y[0] < 214)
            {

                return DOWN;
            }
            else
            {

                return RIGHT;
            }
        }
        else
        {

            return -1;
        }
    }
    return -1;
}