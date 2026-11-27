#include "zf_common_headfile.h"
#include "car_voice.h"

#define CAR_VOICE_HORN_SEG_MAX         (16)
#define CAR_VOICE_LED2_MASK            (0x01U)
#define CAR_VOICE_LED3_MASK            (0x02U)
#define CAR_VOICE_LED4_MASK            (0x04U)
#define CAR_VOICE_LED5_MASK            (0x08U)
#define CAR_VOICE_LED6_MASK            (0x10U)
#define CAR_VOICE_LED_MODE_NONE        (0)
#define CAR_VOICE_LED_MODE_STATIC      (1)
#define CAR_VOICE_LED_MODE_BLINK       (2)
#define CAR_VOICE_LED_MODE_HALF        (3)

typedef struct
{
    uint16 duration_ms;
    uint8 buzzer_on;
} car_voice_horn_seg_struct;

volatile uint32 car_voice_rx_count = 0;
volatile uint32 car_voice_ok_count = 0;
volatile uint32 car_voice_bad_count = 0;
volatile uint32 car_voice_handshake_count = 0;
volatile uint32 car_voice_task_count = 0;
volatile uint8  car_voice_last_command = 0;

static volatile uint8 pending_command = 0;
static volatile uint32 pending_sequence = 0;

static car_voice_horn_seg_struct horn_segments[CAR_VOICE_HORN_SEG_MAX];
static uint8 horn_segment_count = 0;
static uint8 horn_segment_index = 0;
static uint32 horn_segment_end_ms = 0;
static uint8 horn_active = 0;

static uint8 led_action_mode = CAR_VOICE_LED_MODE_NONE;
static uint8 led_action_mask = 0U;
static uint8 led_action_on = 0U;
static uint32 led_action_end_ms = 0U;
static uint32 led_action_next_ms = 0U;

static uint32 last_seen_sequence = 0;

static uint8 voice_same_pin(gpio_pin_enum a, gpio_pin_enum b)
{
    return (a == b) ? 1 : 0;
}

static void voice_gpio_write(gpio_pin_enum pin, uint8 on)
{
    gpio_set_level(pin, on ? CAR_VOICE_LED_ON_LEVEL : CAR_VOICE_LED_OFF_LEVEL);
}

static void voice_buzzer_write(uint8 on)
{
#if CAR_VOICE_BUZZER_USE_PWM
    pwm_set_duty(CAR_VOICE_BUZZER_PWM_CH, on ? CAR_VOICE_BUZZER_PWM_DUTY : 0U);
#else
    gpio_set_level(CAR_VOICE_BUZZER_PIN,
            on ? CAR_VOICE_BUZZER_ON_LEVEL : CAR_VOICE_BUZZER_OFF_LEVEL);
#endif
}

static void voice_set_all_leds(uint8 on)
{
    voice_gpio_write(CAR_VOICE_LEFT_LED_PIN, on);
    voice_gpio_write(CAR_VOICE_RIGHT_LED_PIN, on);
    voice_gpio_write(CAR_VOICE_HIGH_LED_PIN, on);
    voice_gpio_write(CAR_VOICE_LOW_LED_PIN, on);
    voice_gpio_write(CAR_VOICE_AUX_LED_PIN, on);
}

static void voice_write_led_mask(uint8 mask)
{
    voice_gpio_write(CAR_VOICE_LEFT_LED_PIN,  (mask & CAR_VOICE_LED2_MASK) ? 1U : 0U);
    voice_gpio_write(CAR_VOICE_RIGHT_LED_PIN, (mask & CAR_VOICE_LED3_MASK) ? 1U : 0U);
    voice_gpio_write(CAR_VOICE_HIGH_LED_PIN,  (mask & CAR_VOICE_LED4_MASK) ? 1U : 0U);
    voice_gpio_write(CAR_VOICE_LOW_LED_PIN,   (mask & CAR_VOICE_LED5_MASK) ? 1U : 0U);
    voice_gpio_write(CAR_VOICE_AUX_LED_PIN,   (mask & CAR_VOICE_LED6_MASK) ? 1U : 0U);
}

static void voice_start_led_action(uint8 mask, uint8 mode, uint32 now_ms)
{
    led_action_mode = mode;
    led_action_mask = mask;
    led_action_on = 1U;
    led_action_end_ms = now_ms + CAR_VOICE_LED_ACTION_MS;

    if(CAR_VOICE_LED_MODE_BLINK == mode)
    {
        led_action_next_ms = now_ms + CAR_VOICE_LED_BLINK_MS;
    }
    else if(CAR_VOICE_LED_MODE_HALF == mode)
    {
        led_action_next_ms = now_ms + (CAR_VOICE_LED_SOFT_PWM_MS / 2U);
    }
    else
    {
        led_action_next_ms = led_action_end_ms;
    }

    voice_write_led_mask(mask);
}

static void voice_add_horn_segment(uint8 on, uint16 duration_ms)
{
    if(horn_segment_count >= CAR_VOICE_HORN_SEG_MAX) return;
    horn_segments[horn_segment_count].buzzer_on = on;
    horn_segments[horn_segment_count].duration_ms = duration_ms;
    horn_segment_count++;
}

static void voice_start_horn(uint32 now_ms)
{
    horn_segment_index = 0;
    horn_active = (horn_segment_count > 0) ? 1 : 0;
    if(horn_active)
    {
        voice_buzzer_write(horn_segments[0].buzzer_on);
        horn_segment_end_ms = now_ms + horn_segments[0].duration_ms;
    }
    else
    {
        voice_buzzer_write(0);
    }
}

static void voice_program_horn(uint8 command, uint32 now_ms)
{
    uint8 i;

    horn_segment_count = 0;
    horn_segment_index = 0;
    horn_active = 0;
    voice_buzzer_write(0);

    switch(command)
    {
        case CAR_VOICE_CMD_HORN_1S:
            voice_add_horn_segment(1, 1000);
            break;

        case CAR_VOICE_CMD_HORN_2S:
            voice_add_horn_segment(1, 2000);
            break;

        case CAR_VOICE_CMD_HORN_3S:
            voice_add_horn_segment(1, 3000);
            break;

        case CAR_VOICE_CMD_HORN_2_BEEP:
            for(i = 0; i < 2; i++)
            {
                voice_add_horn_segment(1, 180);
                if(i < 1) voice_add_horn_segment(0, 180);
            }
            break;

        case CAR_VOICE_CMD_HORN_3_BEEP:
            for(i = 0; i < 3; i++)
            {
                voice_add_horn_segment(1, 180);
                if(i < 2) voice_add_horn_segment(0, 180);
            }
            break;

        case CAR_VOICE_CMD_HORN_4_BEEP:
            for(i = 0; i < 4; i++)
            {
                voice_add_horn_segment(1, 160);
                if(i < 3) voice_add_horn_segment(0, 150);
            }
            break;

        case CAR_VOICE_CMD_HORN_LONGSHORT:
            voice_add_horn_segment(1, 700);
            voice_add_horn_segment(0, 180);
            voice_add_horn_segment(1, 180);
            break;

        case CAR_VOICE_CMD_HORN_RAPID:
            for(i = 0; i < 6; i++)
            {
                voice_add_horn_segment(1, 90);
                if(i < 5) voice_add_horn_segment(0, 90);
            }
            break;

        case CAR_VOICE_CMD_HORN_ALARM:
            for(i = 0; i < 5; i++)
            {
                voice_add_horn_segment(1, 260);
                if(i < 4) voice_add_horn_segment(0, 180);
            }
            break;

        default:
            return;
    }

    voice_start_horn(now_ms);
}

static void voice_apply_command(uint8 command, uint32 now_ms)
{
    car_voice_last_command = command;

    switch(command)
    {
        case CAR_VOICE_CMD_LEFT_TURN_ALT:
        case CAR_VOICE_CMD_LEFT_TURN:
            voice_start_led_action(CAR_VOICE_LED2_MASK,
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        case CAR_VOICE_CMD_RIGHT_TURN:
            voice_start_led_action(CAR_VOICE_LED3_MASK,
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        case CAR_VOICE_CMD_HAZARD:
            voice_start_led_action((uint8)(CAR_VOICE_LED2_MASK | CAR_VOICE_LED3_MASK),
                    CAR_VOICE_LED_MODE_BLINK, now_ms);
            break;

        case CAR_VOICE_CMD_HIGH_BEAM:
            voice_start_led_action((uint8)(CAR_VOICE_LED2_MASK | CAR_VOICE_LED3_MASK),
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        case CAR_VOICE_CMD_LOW_BEAM:
            voice_start_led_action((uint8)(CAR_VOICE_LED2_MASK | CAR_VOICE_LED3_MASK),
                    CAR_VOICE_LED_MODE_HALF, now_ms);
            break;

        case CAR_VOICE_CMD_FOG_LIGHT:
            voice_start_led_action(CAR_VOICE_LED4_MASK,
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        case CAR_VOICE_CMD_INTERIOR:
            voice_start_led_action(CAR_VOICE_LED6_MASK,
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        case CAR_VOICE_CMD_WIPER:
            voice_start_led_action(CAR_VOICE_LED5_MASK,
                    CAR_VOICE_LED_MODE_STATIC, now_ms);
            break;

        default:
            voice_program_horn(command, now_ms);
            break;
    }
}

static void voice_update_horn(uint32 now_ms)
{
    if(!horn_active) return;
    if((int32)(now_ms - horn_segment_end_ms) < 0) return;

    horn_segment_index++;
    if(horn_segment_index >= horn_segment_count)
    {
        horn_active = 0;
        horn_segment_count = 0;
        voice_buzzer_write(0);
        return;
    }

    voice_buzzer_write(horn_segments[horn_segment_index].buzzer_on);
    horn_segment_end_ms = now_ms + horn_segments[horn_segment_index].duration_ms;
}

static void voice_update_led_action(uint32 now_ms)
{
    uint32 toggle_ms;

    if(CAR_VOICE_LED_MODE_NONE == led_action_mode) return;

    if((int32)(now_ms - led_action_end_ms) >= 0)
    {
        led_action_mode = CAR_VOICE_LED_MODE_NONE;
        led_action_mask = 0U;
        led_action_on = 0U;
        voice_set_all_leds(0);
        return;
    }

    if((CAR_VOICE_LED_MODE_BLINK != led_action_mode)
            && (CAR_VOICE_LED_MODE_HALF != led_action_mode))
    {
        return;
    }

    if((int32)(now_ms - led_action_next_ms) < 0) return;

    toggle_ms = (CAR_VOICE_LED_MODE_HALF == led_action_mode)
            ? (CAR_VOICE_LED_SOFT_PWM_MS / 2U)
            : CAR_VOICE_LED_BLINK_MS;
    if(toggle_ms == 0U) toggle_ms = CAR_VOICE_TASK_PERIOD_MS;

    led_action_next_ms = now_ms + toggle_ms;
    led_action_on = led_action_on ? 0U : 1U;
    voice_write_led_mask(led_action_on ? led_action_mask : 0U);
}

static uint8 voice_command_is_valid(uint8 command)
{
    switch(command)
    {
        case CAR_VOICE_CMD_LEFT_TURN_ALT:
        case CAR_VOICE_CMD_HORN_1S:
        case CAR_VOICE_CMD_HORN_2S:
        case CAR_VOICE_CMD_HORN_3S:
        case CAR_VOICE_CMD_HORN_2_BEEP:
        case CAR_VOICE_CMD_HORN_3_BEEP:
        case CAR_VOICE_CMD_HORN_4_BEEP:
        case CAR_VOICE_CMD_HORN_LONGSHORT:
        case CAR_VOICE_CMD_HORN_RAPID:
        case CAR_VOICE_CMD_HORN_ALARM:
        case CAR_VOICE_CMD_LEFT_TURN:
        case CAR_VOICE_CMD_RIGHT_TURN:
        case CAR_VOICE_CMD_HIGH_BEAM:
        case CAR_VOICE_CMD_LOW_BEAM:
        case CAR_VOICE_CMD_FOG_LIGHT:
        case CAR_VOICE_CMD_HAZARD:
        case CAR_VOICE_CMD_WIPER:
        case CAR_VOICE_CMD_INTERIOR:
            return 1U;

        default:
            return 0U;
    }
}

static void voice_reply_handshake(void)
{
#if CAR_VOICE_HANDSHAKE_REPLY_ENABLE
    static const uint8 handshake[8] =
    {
        0xA5U, 0xFAU, 0x00U, 0x82U, 0x0DU, 0x00U, 0x4AU, 0xFBU
    };

    uart_write_buffer(CAR_VOICE_UART_INDEX, handshake, sizeof(handshake));
#endif
}

void car_voice_init(void)
{
#if CAR_VOICE_ENABLE
#if CAR_VOICE_BUZZER_USE_PWM
    pwm_init(CAR_VOICE_BUZZER_PWM_CH, CAR_VOICE_BUZZER_PWM_FREQ_HZ, 0U);
#else
    gpio_init(CAR_VOICE_BUZZER_PIN, GPO, CAR_VOICE_BUZZER_OFF_LEVEL, GPO_PUSH_PULL);
#endif
    gpio_init(CAR_VOICE_LEFT_LED_PIN, GPO, CAR_VOICE_LED_OFF_LEVEL, GPO_PUSH_PULL);
    gpio_init(CAR_VOICE_RIGHT_LED_PIN, GPO, CAR_VOICE_LED_OFF_LEVEL, GPO_PUSH_PULL);
    gpio_init(CAR_VOICE_HIGH_LED_PIN, GPO, CAR_VOICE_LED_OFF_LEVEL, GPO_PUSH_PULL);
    gpio_init(CAR_VOICE_LOW_LED_PIN, GPO, CAR_VOICE_LED_OFF_LEVEL, GPO_PUSH_PULL);
    if(!voice_same_pin(CAR_VOICE_AUX_LED_PIN, CAR_VOICE_LEFT_LED_PIN)
            && !voice_same_pin(CAR_VOICE_AUX_LED_PIN, CAR_VOICE_RIGHT_LED_PIN)
            && !voice_same_pin(CAR_VOICE_AUX_LED_PIN, CAR_VOICE_HIGH_LED_PIN)
            && !voice_same_pin(CAR_VOICE_AUX_LED_PIN, CAR_VOICE_LOW_LED_PIN))
    {
        gpio_init(CAR_VOICE_AUX_LED_PIN, GPO, CAR_VOICE_LED_OFF_LEVEL, GPO_PUSH_PULL);
    }

    voice_buzzer_write(0);
    voice_set_all_leds(0);

#if CAR_VOICE_BOOT_TEST_ENABLE
    voice_buzzer_write(1);
    system_delay_ms(CAR_VOICE_BOOT_TEST_MS);
    voice_buzzer_write(0);
    voice_set_all_leds(0);
    system_delay_ms(80);
#endif

    uart_init(CAR_VOICE_UART_INDEX, CAR_VOICE_UART_BAUD,
            CAR_VOICE_UART_TX_PIN, CAR_VOICE_UART_RX_PIN);
#if CAR_VOICE_USE_RX_INTERRUPT
    uart_rx_interrupt(CAR_VOICE_UART_INDEX, 1);
#else
    uart_rx_interrupt(CAR_VOICE_UART_INDEX, 0);
#endif
#endif
}

void car_voice_uart_callback(void)
{
#if CAR_VOICE_ENABLE
    static uint8 window[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    uint8 data;
    uint8 command;

    while(uart_query_byte(CAR_VOICE_UART_INDEX, &data))
    {
        car_voice_rx_count++;

        window[0] = window[1];
        window[1] = window[2];
        window[2] = window[3];
        window[3] = window[4];
        window[4] = window[5];
        window[5] = window[6];
        window[6] = window[7];
        window[7] = data;

        if((0xA5U == window[0])
                && (0xFAU == window[1])
                && (0x00U == window[2])
                && (0x82U == window[3])
                && (0x0DU == window[4])
                && (0x00U == window[5])
                && (0x4AU == window[6])
                && (0xFBU == window[7]))
        {
            car_voice_handshake_count++;
            voice_reply_handshake();
        }

        if((CAR_VOICE_FRAME_HEAD1 == window[4])
                && (CAR_VOICE_FRAME_HEAD2 == window[5])
                && (CAR_VOICE_FRAME_TAIL == window[7]))
        {
            command = window[6];
            if(voice_command_is_valid(command))
            {
                pending_command = command;
                car_voice_last_command = command;
                pending_sequence++;
                car_voice_ok_count++;
            }
            else
            {
                car_voice_bad_count++;
            }
        }
    }
#endif
}

void car_voice_task(uint32 now_ms)
{
#if CAR_VOICE_ENABLE
    uint32 sequence;

#if !CAR_VOICE_USE_RX_INTERRUPT
    car_voice_uart_callback();
#endif
    sequence = pending_sequence;
    car_voice_task_count++;

    if(sequence != last_seen_sequence)
    {
        last_seen_sequence = sequence;
        voice_apply_command(pending_command, now_ms);
    }

    voice_update_horn(now_ms);
    voice_update_led_action(now_ms);
#else
    (void)now_ms;
#endif
}
