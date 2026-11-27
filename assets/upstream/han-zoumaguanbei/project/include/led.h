#ifndef LED_H
#define LED_H

void pit_callback_led();
int led_init();
void led_control();
void led_clean();

#define DOWN_LED_GPIO 18
#define LEFT_LED_GPIO 19
#define UP_LED_GPIO 24
#define RIGHT_LED_GPIO 23



#endif // LED_H