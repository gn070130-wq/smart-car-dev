#ifndef _car_config_h_
#define _car_config_h_

#include "zf_common_headfile.h"

//==================================================== app ====================================================
#define CAR_APP_VERSION_TEXT           "KM13_TC387"

#define CAR_ERROR_NONE                 (0)
#define CAR_ERROR_SELF_TEST            (1)
#define CAR_ERROR_PATH_SHORT           (2)
#define CAR_ERROR_IMU_LOST             (3)
#define CAR_ERROR_RECORD_FULL          (4)
#define CAR_ERROR_ENCODER              (5)
#define CAR_ERROR_REPLAY_HEADING       (6)
#define CAR_ERROR_FLASH                (7)

//==================================================== keys ====================================================
#define CAR_USE_BOARD_KEYS             (1)
#define CAR_GUIDE_KEY_PIN              (P20_6)
#define CAR_RESET_KEY_PIN              (P20_7)
#define CAR_KEY_RELEASE_LEVEL          (GPIO_HIGH)
#define CAR_KEY_ACTIVE_LEVEL           (GPIO_LOW)
#define CAR_KEY_COOLDOWN_COUNT         (30)          // 30 * 10ms.

//==================================================== motors ====================================================
#define CAR_PWM_FREQ_HZ                (17000)

#define CAR_LEFT_MOTOR_DIR_PIN         (P21_5)
#define CAR_LEFT_MOTOR_PWM_CH          (ATOM0_CH2_P21_4)
#define CAR_RIGHT_MOTOR_DIR_PIN        (P21_2)
#define CAR_RIGHT_MOTOR_PWM_CH         (ATOM0_CH1_P21_3)
#define CAR_MOTOR_FORWARD_LEVEL        (GPIO_LOW)
#define CAR_MOTOR_REVERSE_LEVEL        (GPIO_HIGH)
#define CAR_LEFT_MOTOR_DUTY_SIGN       (-1)
#define CAR_RIGHT_MOTOR_DUTY_SIGN      (-1)
#define CAR_REAR_DUTY_LIMIT            (5800)
#define CAR_REAR_PWM_SLEW_STEP         (140)
#define CAR_REAR_DIFF_TRIGGER_DEG      (59.0f)
#define CAR_REAR_DIFF_INNER_SCALE      (0.60f)
#define CAR_REAR_DIFF_OUTER_SCALE      (1.40f)

#define CAR_STEER_MOTOR_DIR_PIN        (P02_5)
#define CAR_STEER_MOTOR_PWM_CH         (ATOM0_CH4_P02_4)
#define CAR_STEER_FORWARD_LEVEL        (GPIO_HIGH)
#define CAR_STEER_REVERSE_LEVEL        (GPIO_LOW)
#define CAR_STEER_MOTOR_DUTY_SIGN      (1.0f)
#define CAR_STEER_PWM_LIMIT            (5000)
#define CAR_STEER_MOVE_MIN_DUTY        (0)

//==================================================== encoders ====================================================
#define CAR_ENCODER_MODE_QUAD          (0)
#define CAR_ENCODER_MODE_DIR           (1)
#define CAR_STEER_SENSOR_INCREMENTAL   (0)
#define CAR_STEER_SENSOR_ABSOLUTE_SPI  (1)
#define CAR_REAR_ENCODER_SOURCE_BOTH   (0)
#define CAR_REAR_ENCODER_SOURCE_LEFT   (1)
#define CAR_REAR_ENCODER_SOURCE_RIGHT  (2)

// Board schematic: BMQ2 uses P02.6/P02.7 and BMQ1 uses P20.3/P20.0.
#define CAR_LEFT_REAR_ENCODER          (TIM3_ENCODER)
#define CAR_LEFT_REAR_ENCODER_CH1_PIN  (TIM3_ENCODER_CH1_P02_6)
#define CAR_LEFT_REAR_ENCODER_CH2_PIN  (TIM3_ENCODER_CH2_P02_7)
// DIR mode uses CH1 as pulse/count input and CH2 as direction input.
// Wire left rear pulse to P02.6 and direction to P02.7.
#define CAR_LEFT_REAR_ENCODER_MODE     (CAR_ENCODER_MODE_DIR)
#define CAR_LEFT_REAR_ENCODER_SIGN     (1.0f)

#define CAR_RIGHT_REAR_ENCODER         (TIM6_ENCODER)
#define CAR_RIGHT_REAR_ENCODER_CH1_PIN (TIM6_ENCODER_CH1_P20_3)
#define CAR_RIGHT_REAR_ENCODER_CH2_PIN (TIM6_ENCODER_CH2_P20_0)
#define CAR_RIGHT_REAR_ENCODER_MODE    (CAR_ENCODER_MODE_DIR)
#define CAR_RIGHT_REAR_ENCODER_SIGN    (-1.0f)

// Absolute steering angle sensor on the old magnetic encoder connector.
// Software SPI mapping: SCLK=P10.3, MOSI=P10.1, MISO=P10.2, CS=P10.5.
#define CAR_STEER_SENSOR_TYPE          (CAR_STEER_SENSOR_ABSOLUTE_SPI)
#define CAR_STEER_ENCODER              (TIM5_ENCODER)
#define CAR_STEER_ENCODER_CH1_PIN      (TIM5_ENCODER_CH1_P10_3)
#define CAR_STEER_ENCODER_CH2_PIN      (TIM5_ENCODER_CH2_P10_1)
#define CAR_STEER_ENCODER_MODE         (CAR_ENCODER_MODE_DIR)
#define CAR_STEER_ENCODER_SIGN         (1.0f)

//==================================================== vehicle model ====================================================
#define CAR_CONTROL_PERIOD_MS          (10)
#define CAR_CONTROL_PERIOD_S           (0.01f)
#define CAR_IPS200_TYPE                (IPS200_TYPE_SPI)

// 0: race/no display. 1: vision debug display when CAR_VISION_ENABLE=1.
#define CAR_DISPLAY_MODE               (1)

#define CAR_DEG_TO_RAD                 (0.01745329252f)
#define CAR_RAD_TO_DEG                 (57.295779513f)
#define CAR_GRAVITY_MPS2               (9.80665f)

#define CAR_WHEELBASE_M                (0.260f)
#define CAR_REAR_TRACK_M               (0.210f)
#define CAR_REAR_WHEEL_DIAMETER_M      (0.250f)
// Both rear encoders are available now; average them for odometry to reduce single-wheel bias in turns.
#define CAR_REAR_ENCODER_SOURCE        (CAR_REAR_ENCODER_SOURCE_BOTH)
// Rear encoder calibration: one rear wheel revolution is about 1024 counts.
#define CAR_LEFT_REAR_COUNT_PER_REV    (1024.0f)
#define CAR_RIGHT_REAR_COUNT_PER_REV   (1024.0f)

#define CAR_STEER_ABS_RAW_PERIOD       (4096)
#define CAR_STEER_ABS_ZERO_SAMPLE_COUNT (8)
#define CAR_STEER_ABS_CENTER_RAW       (2006)
#define CAR_STEER_ABS_LEFT_LIMIT_RAW   (2960)
#define CAR_STEER_ABS_RIGHT_LIMIT_RAW  (925)
#define CAR_STEER_ABS_LEFT_SOFT_RAW    (2960)
#define CAR_STEER_ABS_RIGHT_SOFT_RAW   (925)
#define CAR_STEER_ENC_MAX_ABS_COUNT    ((float)CAR_STEER_ABS_RAW_PERIOD / 360.0f * CAR_STEER_MAX_ANGLE_DEG)
#define CAR_STEER_ABS_LEFT_DELTA_RAW   (954.0f)
#define CAR_STEER_ABS_RIGHT_DELTA_RAW  (1081.0f)
#define CAR_STEER_MAX_ANGLE_DEG        (60.0f)
#define CAR_STEER_ANGLE_SIGN           (1.0f)
#define CAR_STEER_TARGET_LIMIT_DEG     (60.0f)
#define CAR_STEER_TARGET_SLEW_DEG      (1.5f)
#define CAR_STEER_SOFT_LIMIT_DEG       (57.0f)
#define CAR_STEER_POSITION_TOL_CNT     (6.0f)
#define CAR_STEER_POSITION_TOL_DEG     (2.0f)
#define CAR_STEER_POS_KP               (90.0f)
#define CAR_STEER_POS_KI               (0.0f)
#define CAR_STEER_POS_KD               (5.0f)
#define CAR_STEER_RATE_FILTER_ALPHA    (0.25f)
#define CAR_STEER_RAW_POSITION_TOL     (6.0f)
#define CAR_STEER_READY_CENTER_TOL_RAW (35)
#define CAR_STEER_RAW_KP               (7.0f)
#define CAR_STEER_RAW_KD               (0.30f)
#define CAR_STEER_RAW_ERROR_NEAR       (110.0f)
#define CAR_STEER_RAW_ERROR_FAR        (340.0f)
#define CAR_STEER_INTEGRAL_LIMIT       (800.0f)
#define CAR_STEER_DEBUG_COUNT_PER_PIXEL (6.0f)
#define CAR_STEER_PWM_START_DUTY       (1800)
#define CAR_STEER_PWM_NEAR_DUTY        (2500)
#define CAR_STEER_PWM_MID_DUTY         (3600)
#define CAR_STEER_PWM_FAR_DUTY         (4500)
#define CAR_STEER_ERROR_NEAR_DEG       (8.0f)
#define CAR_STEER_ERROR_FAR_DEG        (25.0f)
#define CAR_STEER_STALL_MIN_DUTY       (1000)
#define CAR_STEER_STALL_MOVE_DEG       (0.8f)
#define CAR_STEER_STALL_MOVE_RAW       (10.0f)
#define CAR_STEER_STALL_TICKS          (40U)
#define CAR_STEER_BOOT_SELF_TEST_ENABLE (0)
#define CAR_STEER_SELF_TEST_TARGET_DEG (30.0f)
#define CAR_STEER_SELF_TEST_HOLD_MS    (1000U)
#define CAR_STEER_SELF_TEST_STABLE_MS  (200U)
#define CAR_STEER_SELF_TEST_TIMEOUT_MS (3000U)

//==================================================== km13 workflow ====================================================
#define CAR_AUTO_START_FOLLOW          (0)
#define CAR_PARK_STOP_MS               (800)
#define CAR_PATH_SAVE_TO_FLASH         (0)

// Enable camera/tag tracking for visual follow testing.
#define CAR_VISION_ENABLE              (1)

//==================================================== imu ====================================================
#define CAR_IMU_TYPE_MPU6050           (0)
#define CAR_IMU_TYPE_ICM45686          (1)
#define CAR_IMU_TYPE_IMU963RA          (2)
#define CAR_IMU_TYPE                   (CAR_IMU_TYPE_IMU963RA)

#define CAR_IMU_AXIS_X                 (0)
#define CAR_IMU_AXIS_Y                 (1)
#define CAR_IMU_AXIS_Z                 (2)
#define CAR_IMU_FORWARD_AXIS           (CAR_IMU_AXIS_X)
#define CAR_IMU_YAW_AXIS               (CAR_IMU_AXIS_Z)
#define CAR_IMU_FORWARD_SIGN           (1.0f)
#define CAR_IMU_YAW_SIGN               (1.0f)

#define CAR_GYRO_STARTUP_SETTLE_MS     (300)
#define CAR_GYRO_BIAS_SAMPLE_COUNT     (1000)
#define CAR_GYRO_BIAS_SAMPLE_DELAY_MS  (2)
#define CAR_IMU_BIAS_MIN_VALID_COUNT   (850)
#define CAR_IMU_RUNTIME_FAIL_CONFIRM   (5)
#define CAR_GYRO_DEADBAND_DPS          (0.60f)
#define CAR_GYRO_LOWPASS_ALPHA         (0.18f)
#define CAR_GYRO_SCALE                 (1.00f)
#define CAR_GYRO_STATIONARY_COUNT      (100)         // 1 s at 10 ms.
#define CAR_GYRO_STATIONARY_ENC_MAX    (2)
#define CAR_GYRO_BIAS_TRACK_ALPHA      (0.0030f)
#define CAR_GYRO_BIAS_TRACK_MAX_ERROR_DPS (0.30f)
#define CAR_SPEED_LOWPASS_ALPHA        (0.28f)

// Keep old ICM45686 compile path available, even though km13 uses IMU963RA.
#define CAR_ICM45686_IIC_ADDR          (0x68)
#define CAR_ICM45686_IIC_DELAY         (59)
#define CAR_ICM45686_SCL_PIN           (P20_11)
#define CAR_ICM45686_SDA_PIN           (P20_14)
#define CAR_ICM45686_WHO_AM_I_VALUE    (0xE9)
#define CAR_ICM45686_ACCEL_FS_G        (8.0f)
#define CAR_ICM45686_GYRO_FS_DPS       (1000.0f)
#define CAR_ICM45686_ACCEL_CONFIG      (0x29)
#define CAR_ICM45686_GYRO_CONFIG       (0x29)
#define CAR_ICM45686_PWR_CONFIG        (0x0F)
#define CAR_ICM45686_STARTUP_MS        (80)

//==================================================== remote ====================================================
#define CAR_REMOTE_ENABLE              (0)
#define CAR_REMOTE_LINK_UART           (0)
#define CAR_REMOTE_LINK_WIFI_SPI       (0)
#define CAR_REMOTE_UART_INDEX          (UART_2)
#define CAR_REMOTE_UART_BAUD           (115200)
#define CAR_REMOTE_UART_TX_PIN         (UART2_TX_P10_5)
#define CAR_REMOTE_UART_RX_PIN         (UART2_RX_P10_6)
#define CAR_REMOTE_WIFI_SSID           "why137a"
#define CAR_REMOTE_WIFI_PASSWORD       "1234567890"
#define CAR_REMOTE_WIFI_PC_IP          "192.168.137.1"
#define CAR_REMOTE_WIFI_PC_PORT        "8766"
#define CAR_REMOTE_WIFI_LOCAL_PORT     "6666"
#define CAR_REMOTE_WIFI_HELLO_MS       (500)
#define CAR_REMOTE_TELEM_MS            (200)
#define CAR_REMOTE_FRAME_HEAD1         (0xA5)
#define CAR_REMOTE_FRAME_HEAD2         (0x6A)
#define CAR_REMOTE_FRAME_TAIL          (0xFB)
#define CAR_REMOTE_FLAG_ACTIVE         (0x01)
#define CAR_REMOTE_FLAG_READY          (0x02)
#define CAR_REMOTE_FLAG_GUIDE          (0x04)
#define CAR_REMOTE_FLAG_FINISH         (0x08)
#define CAR_REMOTE_FLAG_RESET          (0x10)
#define CAR_REMOTE_TIMEOUT_MS          (300)
#define CAR_REMOTE_REAR_PWM_LIMIT      (3000)
#define CAR_REMOTE_STEER_LIMIT_CNT     (220.0f)

//==================================================== voice ======================================================
#define CAR_VOICE_ENABLE               (1)
#define CAR_VOICE_UART_INDEX           (UART_3)
#define CAR_VOICE_UART_BAUD            (115200)
#define CAR_VOICE_UART_TX_PIN          (UART3_TX_P15_6)
#define CAR_VOICE_UART_RX_PIN          (UART3_RX_P15_7)
#define CAR_VOICE_USE_RX_INTERRUPT     (1)
#define CAR_VOICE_HANDSHAKE_REPLY_ENABLE (1)
#define CAR_VOICE_FRAME_HEAD1          (0xA5)
#define CAR_VOICE_FRAME_HEAD2          (0x5A)
#define CAR_VOICE_FRAME_TAIL           (0xFB)

#define CAR_VOICE_BUZZER_PIN           (P13_1)
#define CAR_VOICE_BUZZER_ON_LEVEL      (GPIO_HIGH)
#define CAR_VOICE_BUZZER_OFF_LEVEL     (GPIO_LOW)
#define CAR_VOICE_BUZZER_USE_PWM       (1)
#define CAR_VOICE_BUZZER_PWM_CH        (ATOM2_CH6_P13_1)
#define CAR_VOICE_BUZZER_PWM_FREQ_HZ   (3000)
#define CAR_VOICE_BUZZER_PWM_DUTY      (5000)

#define CAR_VOICE_LEFT_LED_PIN         (P11_11)
#define CAR_VOICE_RIGHT_LED_PIN        (P11_12)
#define CAR_VOICE_HIGH_LED_PIN         (P11_9)
#define CAR_VOICE_LOW_LED_PIN          (P11_10)
#define CAR_VOICE_AUX_LED_PIN          (P13_0)
#define CAR_VOICE_LED_ON_LEVEL         (GPIO_HIGH)
#define CAR_VOICE_LED_OFF_LEVEL        (GPIO_LOW)

#define CAR_VOICE_BOOT_TEST_ENABLE     (1)
#define CAR_VOICE_BOOT_TEST_MS         (160)
#define CAR_VOICE_TASK_PERIOD_MS       (10)
#define CAR_VOICE_LED_ACTION_MS        (1000)
#define CAR_VOICE_LED_BLINK_MS         (160)
#define CAR_VOICE_LED_SOFT_PWM_MS      (100)

#endif
