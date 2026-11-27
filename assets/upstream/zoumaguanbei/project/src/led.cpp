#include "led.h"
#include "my_gpio.h"

bool left_led_value = 0;
bool right_led_value = 0;
bool up_led_value = 0;
bool down_led_value = 0;

extern int circle_save;
extern int expect_speed;
extern int fast_speed;
extern int barrier_save;
extern int brick_save;
extern int start_flag;

void pit_callback_led(){

    led_control();
}

int led_init(){

    gpio_init(LEFT_LED_GPIO,"out", 0);//左大灯
    gpio_init(DOWN_LED_GPIO,"out", 0);//右灯
    gpio_init(UP_LED_GPIO,"out", 0);//下蓝灯
    gpio_init(RIGHT_LED_GPIO,"out", 0);//上大灯
    left_led_value = 0;
    right_led_value = 0;
    up_led_value = 0;
    down_led_value = 0;
    return 0;
}
int turn = 0;
void led_control(){

    if(start_flag == 0){

        left_led_value = !left_led_value;
        right_led_value = !left_led_value;
        up_led_value = 0;
        down_led_value = 0;
    }
    else if(start_flag == -1){
        switch(turn){
        case 0:
            left_led_value = 1;
            right_led_value = 0;
            up_led_value = 0;
            down_led_value = 0;
            break;
        case 1:
            left_led_value = 0;
            right_led_value = 0;
            up_led_value = 1;
            down_led_value = 0;
            break;
        case 2:
            left_led_value = 0;
            right_led_value = 1;
            up_led_value = 0;
            down_led_value = 0;
            break;
        case 3:
            left_led_value = 0;
            right_led_value = 0;
            up_led_value = 0;
            down_led_value = 1;
        }
        turn = (turn + 1) % 4;
    }
    else if(start_flag == -2){

        left_led_value = 0;
        right_led_value = 0;
        up_led_value = !up_led_value;
        down_led_value = !up_led_value;
    }
    else if(circle_save % 10 == 1){
        left_led_value = !left_led_value;
        right_led_value = 0;
        up_led_value = 0;
        down_led_value = 0;
    }
    else if(circle_save % 10 == 2){
        left_led_value = 0;
        right_led_value = !right_led_value;
        up_led_value = 0;
        down_led_value = 0;
    }

    else if(expect_speed == fast_speed){

        left_led_value = !left_led_value;
        right_led_value = left_led_value;
        up_led_value =  !left_led_value;
        down_led_value = up_led_value;
    }
    else {

        left_led_value = 0;
        right_led_value = 0;
        up_led_value = 0;
        down_led_value = 0;
    }
    if(barrier_save == -2 || barrier_save == -3 || barrier_save == -4){

        up_led_value = 1;
        down_led_value = 1;
    }
    else if(barrier_save == 0 || barrier_save == 1){

        up_led_value = 0;
        down_led_value = 0;
        left_led_value = 1;
        right_led_value = 0;
    }
    else if(barrier_save == 2 || barrier_save == 3){

        up_led_value = 0;
        down_led_value = 0;
        left_led_value = 0;
        right_led_value = 1;
    }
    if(brick_save != 0){

        left_led_value = 1;
        right_led_value = 1;
    }
    gpio_set(LEFT_LED_GPIO, left_led_value);
    gpio_set(RIGHT_LED_GPIO, right_led_value);
    gpio_set(UP_LED_GPIO, up_led_value);
    gpio_set(DOWN_LED_GPIO, down_led_value);
}

void led_clean(){

    gpio_set(LEFT_LED_GPIO, 0);
    gpio_set(RIGHT_LED_GPIO, 0);
    gpio_set(UP_LED_GPIO, 0);
    gpio_set(DOWN_LED_GPIO, 0);
}