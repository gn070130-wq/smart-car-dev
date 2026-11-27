#ifndef _car_voice_h_
#define _car_voice_h_

#include "car_config.h"

typedef enum
{
    CAR_VOICE_CMD_LEFT_TURN_ALT = 0x00,
    CAR_VOICE_CMD_HORN_1S       = 0x02,
    CAR_VOICE_CMD_HORN_2S       = 0x03,
    CAR_VOICE_CMD_HORN_3S       = 0x05,
    CAR_VOICE_CMD_HORN_2_BEEP   = 0x06,
    CAR_VOICE_CMD_HORN_3_BEEP   = 0x08,
    CAR_VOICE_CMD_HORN_4_BEEP   = 0x09,
    CAR_VOICE_CMD_HORN_LONGSHORT= 0x10,
    CAR_VOICE_CMD_HORN_RAPID    = 0x11,
    CAR_VOICE_CMD_HORN_ALARM    = 0x12,
    CAR_VOICE_CMD_LEFT_TURN     = 0x21,
    CAR_VOICE_CMD_RIGHT_TURN    = 0x22,
    CAR_VOICE_CMD_HIGH_BEAM     = 0x23,
    CAR_VOICE_CMD_LOW_BEAM      = 0x24,
    CAR_VOICE_CMD_FOG_LIGHT     = 0x25,
    CAR_VOICE_CMD_HAZARD        = 0x26,
    CAR_VOICE_CMD_WIPER         = 0x27,
    CAR_VOICE_CMD_INTERIOR      = 0x28,
} car_voice_command_enum;

extern volatile uint32 car_voice_rx_count;
extern volatile uint32 car_voice_ok_count;
extern volatile uint32 car_voice_bad_count;
extern volatile uint32 car_voice_handshake_count;
extern volatile uint32 car_voice_task_count;
extern volatile uint8  car_voice_last_command;

void car_voice_init(void);
void car_voice_uart_callback(void);
void car_voice_task(uint32 now_ms);

#endif
