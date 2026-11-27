/*********************************************************************************************************************
* TC387 Opensourec Library ����TC387 ��Դ�⣩��һ�����ڹٷ� SDK �ӿڵĵ�������Դ��
* Copyright (c) 2022 SEEKFREE ��ɿƼ�
*
* ���ļ��� TC387 ��Դ���һ����
*
* TC387 ��Դ�� ���������
* �����Ը���������������ᷢ���� GPL��GNU General Public License���� GNUͨ�ù�������֤��������
* �� GPL �ĵ�3�棨�� GPL3.0������ѡ��ģ��κκ����İ汾�����·�����/���޸���
*
* ����Դ��ķ�����ϣ�����ܷ������ã�����δ�������κεı�֤
* ����û�������������Ի��ʺ��ض���;�ı�֤
* ����ϸ����μ� GPL
*
* ��Ӧ�����յ�����Դ���ͬʱ�յ�һ�� GPL �ĸ���
* ���û�У������<https://www.gnu.org/licenses/>
*
* ����ע����
* ����Դ��ʹ�� GPL3.0 ��Դ����֤Э�� ������������Ϊ���İ汾
* ��������Ӣ�İ��� libraries/doc �ļ����µ� GPL3_permission_statement.txt �ļ���
* ����֤������ libraries �ļ����� �����ļ����µ� LICENSE �ļ�
* ��ӭ��λʹ�ò����������� ���޸�����ʱ���뱣����ɿƼ��İ�Ȩ����������������
*
* �ļ�����          zf_device_imu963ra
* ��˾����          �ɶ���ɿƼ����޹�˾
* �汾��Ϣ          �鿴 libraries/doc �ļ����� version �ļ� �汾˵��
* ��������          ADS v1.10.2
* ����ƽ̨          TC387QP
* ��������          https://seekfree.taobao.com/
*
* �޸ļ�¼
* ����              ����                ��ע
* 2022-11-04       pudding            first version
* 2023-04-28       pudding            ��������ע��˵��
* 2024-01-30       pudding            ������ת������ ������������
********************************************************************************************************************/
/********************************************************************************************************************
* ���߶��壺
*                  ------------------------------------
*                  ģ��ܽ�             ��Ƭ���ܽ�
*                  // Ӳ�� SPI ����
*                  SCL/SPC            �鿴 zf_device_imu963ra.h �� IMU963RA_SPC_PIN �궨��
*                  SDA/DSI            �鿴 zf_device_imu963ra.h �� IMU963RA_SDI_PIN �궨��
*                  SA0/SDO            �鿴 zf_device_imu963ra.h �� IMU963RA_SDO_PIN �궨��
*                  CS                 �鿴 zf_device_imu963ra.h �� IMU963RA_CS_PIN  �궨��
*                  VCC                3.3V��Դ
*                  GND                ��Դ��
*                  ������������
*
*                  // ���� IIC ����
*                  SCL/SPC            �鿴 zf_device_imu963ra.h �� IMU963RA_SCL_PIN �궨��
*                  SDA/DSI            �鿴 zf_device_imu963ra.h �� IMU963RA_SDA_PIN �궨��
*                  VCC                3.3V��Դ
*                  GND                ��Դ��
*                  ������������
*                  ------------------------------------
********************************************************************************************************************/
#ifndef _zf_device_imu963ra_h_
#define _zf_device_imu963ra_h_

#include "zf_common_typedef.h"

//================================================���� IMU963RA ��������================================================
#define IMU963RA_USE_SOFT_IIC                       (1)                         // Ĭ��ʹ��Ӳ�� SPI ��ʽ����
#if IMU963RA_USE_SOFT_IIC                                                       // ������ ��ɫ�����Ĳ�����ȷ�� ��ɫ�ҵľ���û���õ�
//====================================================���� IIC ����====================================================
#define IMU963RA_SOFT_IIC_DELAY                     (59)                       // ���� IIC ��ʱ����ʱ���� ��ֵԽС IIC ͨ������Խ��
#define IMU963RA_SCL_PIN                            (P20_11)                    // ���� IIC SCL ���� ���� IMU963RA �� SCL ����
#define IMU963RA_SDA_PIN                            (P20_12)                    // ���� IIC SDA ���� ���� IMU963RA �� SDA ����
//====================================================���� IIC ����====================================================
#else

//====================================================Ӳ�� SPI ����====================================================
#define IMU963RA_SPI_SPEED                          (10 * 1000 * 1000)          // Ӳ�� SPI ����
#define IMU963RA_SPI                                (SPI_0           )          // Ӳ�� SPI ��
#define IMU963RA_SPC_PIN                            (SPI0_SCLK_P20_11)          // Ӳ�� SPI SCK ����
#define IMU963RA_SDI_PIN                            (SPI0_MOSI_P20_14)          // Ӳ�� SPI MOSI ����
#define IMU963RA_SDO_PIN                            (SPI0_MISO_P20_12)          // Ӳ�� SPI MISO ����
//====================================================Ӳ�� SPI ����====================================================
#endif

#define IMU963RA_CS_PIN                             (P20_13)                    // CS Ƭѡ����
#define IMU963RA_CS(x)                              (x? (gpio_high(IMU963RA_CS_PIN)): (gpio_low(IMU963RA_CS_PIN)))

typedef enum
{
    IMU963RA_ACC_SAMPLE_SGN_2G ,                                                // ���ٶȼ����� ��2G  (ACC = Accelerometer ���ٶȼ�) (SGN = signum �������� ��ʾ������Χ) (G = g �������ٶ� g��9.80 m/s^2)
    IMU963RA_ACC_SAMPLE_SGN_4G ,                                                // ���ٶȼ����� ��4G  (ACC = Accelerometer ���ٶȼ�) (SGN = signum �������� ��ʾ������Χ) (G = g �������ٶ� g��9.80 m/s^2)
    IMU963RA_ACC_SAMPLE_SGN_8G ,                                                // ���ٶȼ����� ��8G  (ACC = Accelerometer ���ٶȼ�) (SGN = signum �������� ��ʾ������Χ) (G = g �������ٶ� g��9.80 m/s^2)
    IMU963RA_ACC_SAMPLE_SGN_16G,                                                // ���ٶȼ����� ��16G (ACC = Accelerometer ���ٶȼ�) (SGN = signum �������� ��ʾ������Χ) (G = g �������ٶ� g��9.80 m/s^2)
}imu963ra_acc_sample_config;

typedef enum
{
    IMU963RA_GYRO_SAMPLE_SGN_125DPS ,                                           // ���������� ��125DPS  (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
    IMU963RA_GYRO_SAMPLE_SGN_250DPS ,                                           // ���������� ��250DPS  (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
    IMU963RA_GYRO_SAMPLE_SGN_500DPS ,                                           // ���������� ��500DPS  (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
    IMU963RA_GYRO_SAMPLE_SGN_1000DPS,                                           // ���������� ��1000DPS (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
    IMU963RA_GYRO_SAMPLE_SGN_2000DPS,                                           // ���������� ��2000DPS (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
    IMU963RA_GYRO_SAMPLE_SGN_4000DPS,                                           // ���������� ��4000DPS (GYRO = Gyroscope ������) (SGN = signum �������� ��ʾ������Χ) (DPS = Degree Per Second ���ٶȵ�λ ��/S)
}imu963ra_gyro_sample_config;

typedef enum
{
    IMU963RA_MAG_SAMPLE_2G,                                                     // ���������� 2G (MAG = Magnetometer ������) (G = Gs ��˹)
    IMU963RA_MAG_SAMPLE_8G,                                                     // ���������� 8G (MAG = Magnetometer ������) (G = Gs ��˹)
}imu963ra_mag_sample_config;

#define IMU963RA_ACC_SAMPLE_DEFAULT     ( IMU963RA_ACC_SAMPLE_SGN_8G )          // ��������Ĭ�ϵ� ���ٶȼ� ��ʼ������
#define IMU963RA_GYRO_SAMPLE_DEFAULT    ( IMU963RA_GYRO_SAMPLE_SGN_2000DPS )    // ��������Ĭ�ϵ� ������   ��ʼ������
#define IMU963RA_MAG_SAMPLE_DEFAULT     ( IMU963RA_MAG_SAMPLE_8G )              // ��������Ĭ�ϵ� ������   ��ʼ������
#define IMU963RA_TIMEOUT_COUNT                      (0x00FF)                    // IMU963RA ��ʱ����
//================================================���� IMU963RA ��������================================================


//================================================���� IMU963RA �ڲ���ַ================================================
#define IMU963RA_DEV_ADDR                           (0x6B)                      // SA0�ӵأ�0x6A SA0������0x6B ģ��Ĭ������
#define IMU963RA_SPI_W                              (0x00)
#define IMU963RA_SPI_R                              (0x80)

#define IMU963RA_FUNC_CFG_ACCESS                    (0x01)
#define IMU963RA_INT1_CTRL                          (0x0D)
#define IMU963RA_WHO_AM_I                           (0x0F)
#define IMU963RA_CTRL1_XL                           (0x10)
#define IMU963RA_CTRL2_G                            (0x11)
#define IMU963RA_CTRL3_C                            (0x12)
#define IMU963RA_CTRL4_C                            (0x13)
#define IMU963RA_CTRL5_C                            (0x14)
#define IMU963RA_CTRL6_C                            (0x15)
#define IMU963RA_CTRL7_G                            (0x16)
#define IMU963RA_CTRL9_XL                           (0x18)
#define IMU963RA_OUTX_L_G                           (0x22)
#define IMU963RA_OUTX_L_A                           (0x28)

//������������ؼĴ��� ��Ҫ��FUNC_CFG_ACCESS��SHUB_REG_ACCESSλ����Ϊ1������ȷ����
#define IMU963RA_SENSOR_HUB_1                       (0x02)
#define IMU963RA_MASTER_CONFIG                      (0x14)
#define IMU963RA_SLV0_ADD                           (0x15)
#define IMU963RA_SLV0_SUBADD                        (0x16)
#define IMU963RA_SLV0_CONFIG                        (0x17)
#define IMU963RA_DATAWRITE_SLV0                     (0x21)
#define IMU963RA_STATUS_MASTER                      (0x22)

#define IMU963RA_MAG_ADDR                           (0x0D)                      // 7λIIC��ַ
#define IMU963RA_MAG_OUTX_L                         (0x00)
#define IMU963RA_MAG_CONTROL1                       (0x09)
#define IMU963RA_MAG_CONTROL2                       (0x0A)
#define IMU963RA_MAG_FBR                            (0x0B)
#define IMU963RA_MAG_CHIP_ID                        (0x0D)

#define IMU963RA_ACC_SAMPLE                         (0x3C)                      // ���ٶȼ�����
// ����Ϊ:0x30 ���ٶ�����Ϊ:��2G      ��ȡ���ļ��ٶȼ����� ����16393������ת��Ϊ��������λ�����ݣ���λ��g(m/s^2)
// ����Ϊ:0x38 ���ٶ�����Ϊ:��4G      ��ȡ���ļ��ٶȼ����� ����8197�� ����ת��Ϊ��������λ�����ݣ���λ��g(m/s^2)
// ����Ϊ:0x3C ���ٶ�����Ϊ:��8G      ��ȡ���ļ��ٶȼ����� ����4098�� ����ת��Ϊ��������λ�����ݣ���λ��g(m/s^2)
// ����Ϊ:0x34 ���ٶ�����Ϊ:��16G     ��ȡ���ļ��ٶȼ����� ����2049�� ����ת��Ϊ��������λ�����ݣ���λ��g(m/s^2)

#define IMU963RA_GYR_SAMPLE                         (0x5C)                      // ����������
// ����Ϊ:0x52 ����������Ϊ:��125dps  ��ȡ�������������ݳ���228.6��   ����ת��Ϊ��������λ�����ݣ���λΪ����/s
// ����Ϊ:0x50 ����������Ϊ:��250dps  ��ȡ�������������ݳ���114.3��   ����ת��Ϊ��������λ�����ݣ���λΪ����/s
// ����Ϊ:0x54 ����������Ϊ:��500dps  ��ȡ�������������ݳ���57.1��    ����ת��Ϊ��������λ�����ݣ���λΪ����/s
// ����Ϊ:0x58 ����������Ϊ:��1000dps ��ȡ�������������ݳ���28.6��    ����ת��Ϊ��������λ�����ݣ���λΪ����/s
// ����Ϊ:0x5C ����������Ϊ:��2000dps ��ȡ�������������ݳ���14.3��    ����ת��Ϊ��������λ�����ݣ���λΪ����/s
// ����Ϊ:0x51 ����������Ϊ:��4000dps ��ȡ�������������ݳ���7.1��     ����ת��Ϊ��������λ�����ݣ���λΪ����/s

#define IMU963RA_MAG_SAMPLE                         (0x19)                      // �شż�����
// ����Ϊ:0x19 ����������Ϊ:8G     ��ȡ���ļ��ٶȼ����� ����3000�� ����ת��Ϊ��������λ�����ݣ���λ��G(��˹)
// ����Ϊ:0x09 ����������Ϊ:2G     ��ȡ���ļ��ٶȼ����� ����12000������ת��Ϊ��������λ�����ݣ���λ��G(��˹)
//================================================���� IMU963RA �ڲ���ַ================================================


//================================================���� IMU963RA ȫ�ֱ���================================================
extern int16 imu963ra_acc_x,  imu963ra_acc_y,  imu963ra_acc_z;                  // ��������������      GYRO (������)
extern int16 imu963ra_gyro_x, imu963ra_gyro_y, imu963ra_gyro_z;                 // ������ٶȼ�����     ACC  (accelerometer ���ٶȼ�)
extern int16 imu963ra_mag_x,  imu963ra_mag_y,  imu963ra_mag_z;                  // �������������      MAG  (magnetometer ������)
extern float imu963ra_transition_factor[3];                                     // ת��ʵ��ֵ�ı���
//================================================���� IMU963RA ȫ�ֱ���================================================


//================================================���� IMU963RA ��������================================================
void    imu963ra_get_acc            (void);                                     // ��ȡ IMU963RA ���ٶȼ�����
void    imu963ra_get_gyro           (void);                                     // ��ȡ IMU963RA ����������
void    imu963ra_get_mag            (void);                                     // ��ȡ IMU963RA ����������
uint8   imu963ra_init               (void);                                     // ��ʼ�� IMU963RA
//================================================���� IMU963RA ��������================================================


//================================================���� IMU963RA ��չ����================================================
//-------------------------------------------------------------------------------------------------------------------
// �������     �� IMU963RA ���ٶȼ�����ת��Ϊʵ����������
// ����˵��     acc_value       ������ļ��ٶȼ�����
// ���ز���     void
// ʹ��ʾ��     float data = imu963ra_acc_transition(imu963ra_acc_x);               // ��λΪ g(m/s^2)
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_acc_transition(acc_value)      ((float)(acc_value) / imu963ra_transition_factor[0])

//-------------------------------------------------------------------------------------------------------------------
// �������     �� IMU963RA ����������ת��Ϊʵ����������
// ����˵��     gyro_value      �����������������
// ���ز���     void
// ʹ��ʾ��     float data = imu963ra_gyro_transition(imu963ra_gyro_x);             // ��λΪ ��/s
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_gyro_transition(gyro_value)    ((float)(gyro_value) / imu963ra_transition_factor[1])

//-------------------------------------------------------------------------------------------------------------------
// �������     �� IMU963RA ����������ת��Ϊʵ����������
// ����˵��     mag_value       ������Ĵ���������
// ���ز���     void
// ʹ��ʾ��     float data = imu963ra_mag_transition(imu963ra_mag_x);               // ��λΪ G
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
#define imu963ra_mag_transition(mag_value)    ((float)mag_value / imu963ra_transition_factor[2])
//================================================���� IMU963RA ��չ����================================================

#endif
