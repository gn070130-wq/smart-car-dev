/*********************************************************************************************************************
* TC377 Opensourec Library ����TC377 ��Դ�⣩��һ�����ڹٷ� SDK �ӿڵĵ�������Դ��
* Copyright (c) 2022 SEEKFREE ��ɿƼ�
*
* ���ļ��� TC377 ��Դ���һ����
*
* TC377 ��Դ�� ���������
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
* �ļ�����          zf_device_wifi_spi
* ��˾����          �ɶ���ɿƼ����޹�˾
* �汾��Ϣ          �鿴 libraries/doc �ļ����� version �ļ� �汾˵��
* ��������          ADS v1.10.2
* ����ƽ̨          TC377TP
* ��������          https://seekfree.taobao.com/
* 
* �޸ļ�¼
* ����              ����                ��ע
* 2024-01-18        SeekFree            first version
********************************************************************************************************************/
/*********************************************************************************************************************
* ���߶��壺
*                   ------------------------------------
*                   ģ��ܽ�            ��Ƭ���ܽ�
*                   RST                 �鿴 zf_device_wifi_spi.h �� WIFI_SPI_RST_PIN �궨��
*                   INT                 �鿴 zf_device_wifi_spi.h �� WIFI_SPI_INT_PIN �궨��
*                   CS                  �鿴 zf_device_wifi_spi.h �� WIFI_SPI_CS_PIN �궨��
*                   MISO                �鿴 zf_device_wifi_spi.h �� WIFI_SPI_MISO_PIN �궨��
*                   SCK                 �鿴 zf_device_wifi_spi.h �� WIFI_SPI_SCK_PIN �궨��
*                   MOSI                �鿴 zf_device_wifi_spi.h �� WIFI_SPI_MOSI_PIN �궨��
*                   5V                  5V ��Դ
*                   GND                 ��Դ��
*                   ������������
*                   ------------------------------------
*********************************************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "zf_common_clock.h"
#include "zf_common_debug.h"
#include "zf_common_fifo.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_spi.h"
#include "zf_device_type.h"

#include "zf_device_wifi_spi.h"

#define WIFI_CONNECT_TIME_OUT       12000        // ��λ����
#define SOCKET_CONNECT_TIME_OUT     5000        // ��λ����
#define OTHER_TIME_OUT              80          // ��λ����
#define WIFI_SPI_RESET_LOW_MS       50
#define WIFI_SPI_BOOT_WAIT_MS       500
#define WIFI_SPI_VERSION_RETRY      10

#if ((WIFI_SPI_RECVIVE_SIZE < 32) || (WIFI_SPI_RECVIVE_SIZE > 4088))
    #error "WIFI_SPI_RECVIVE_SIZE must be >= 32 or <= 4088"
#endif

#if (WIFI_SPI_RECVIVE_SIZE >= WIFI_SPI_RECVIVE_FIFO_SIZE)
    #error "WIFI_SPI_RECVIVE_FIFO_SIZE must be > WIFI_SPI_RECVIVE_SIZE"
#endif


#if (WIFI_SPI_TRANSFER_SIZE != 4088)
    #error "WIFI_SPI_TRANSFER_SIZE must be == 4088"
#endif



char wifi_spi_version[12];                      // ����ģ��̼��汾��Ϣ
char wifi_spi_mac_addr[20];                     // ����ģ��MAC��ַ��Ϣ
char wifi_spi_ip_addr_port[25];                 // ����ģ��IP��ַ��˿���Ϣ
uint8 wifi_spi_debug_step = 0;
uint8 wifi_spi_debug_last_cmd = 0;
uint8 wifi_spi_debug_last_reply = 0;
uint16 wifi_spi_debug_last_len = 0;

static fifo_struct  wifi_spi_fifo;
static uint8        wifi_spi_buffer[WIFI_SPI_RECVIVE_FIFO_SIZE];
static volatile     wifi_spi_state_enum wifi_spi_mutex;

#ifndef WIFI_SPI_SOFT_DELAY_US
#define WIFI_SPI_SOFT_DELAY_US          (5)
#endif

static void wifi_spi_bus_write_8bit_array (const uint8 *data, uint16 length)
{
#if (WIFI_SPI_SOFT_ENABLE)
    uint16 i;
    uint8 mask;
    uint8 value;

    for(i = 0; i < length; i++)
    {
        value = data[i];
        for(mask = 0x80; 0 != mask; mask >>= 1)
        {
            gpio_set_level(WIFI_SPI_SOFT_MOSI_PIN, (0 != (value & mask)));
            gpio_low(WIFI_SPI_SOFT_SCK_PIN);
            system_delay_us(WIFI_SPI_SOFT_DELAY_US);
            gpio_high(WIFI_SPI_SOFT_SCK_PIN);
            system_delay_us(WIFI_SPI_SOFT_DELAY_US);
        }
    }
#else
    spi_write_8bit_array(WIFI_SPI_INDEX, data, length);
#endif
}

static void wifi_spi_bus_transfer_8bit (const uint8 *write_buffer, uint8 *read_buffer, uint16 length)
{
#if (WIFI_SPI_SOFT_ENABLE)
    uint16 i;
    uint8 mask;
    uint8 tx;
    uint8 rx;

    for(i = 0; i < length; i++)
    {
        tx = (NULL == write_buffer) ? 0xFF : write_buffer[i];
        rx = 0;
        for(mask = 0x80; 0 != mask; mask >>= 1)
        {
            gpio_set_level(WIFI_SPI_SOFT_MOSI_PIN, (0 != (tx & mask)));
            gpio_low(WIFI_SPI_SOFT_SCK_PIN);
            system_delay_us(WIFI_SPI_SOFT_DELAY_US);
            gpio_high(WIFI_SPI_SOFT_SCK_PIN);
            if(gpio_get_level(WIFI_SPI_SOFT_MISO_PIN))
            {
                rx |= mask;
            }
            system_delay_us(WIFI_SPI_SOFT_DELAY_US);
        }
        if(NULL != read_buffer)
        {
            read_buffer[i] = rx;
        }
    }
#else
    spi_transfer_8bit(WIFI_SPI_INDEX, write_buffer, read_buffer, length);
#endif
}
//-------------------------------------------------------------------------------------------------------------------
// �������     �ȴ�WIFI SPI����
// ����˵��     wait_time       ���ȴ�ʱ�� ��λ����
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_wait_idle (uint32 wait_time)
{
    uint32 time = 0;
    
    wait_time = wait_time*100;
    while(0 == gpio_get_level(WIFI_SPI_INT_PIN))
    {
        system_delay_us(10);
        time++;
        if(wait_time <= time)
        {
            break;
        }
    }
    return (wait_time <= time);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     д�����ݵ�WIFI SPI
// ����˵��     *buffer1        ��һ����Ҫ���͵����ݻ�������ַ
// ����˵��     length1         ��һ�����ݳ���
// ����˵��     *buffer2        �ڶ�����Ҫ���͵����ݻ�������ַ
// ����˵��     length2         �ڶ������ݳ���
// ���ز���     void
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static void wifi_spi_write (const uint8 *buffer1, uint16 length1, const uint8 *buffer2, uint16 length2)
{
    gpio_low(WIFI_SPI_CS_PIN);
    if(NULL != buffer1)
    {
        wifi_spi_bus_write_8bit_array(buffer1, length1);
    }
    if(NULL != buffer2)
    {
        wifi_spi_bus_write_8bit_array(buffer2, length2);
    }
    gpio_high(WIFI_SPI_CS_PIN);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ���������ͬʱ���У������շ���
// ����˵��     *packets        ��������յĵ�ַ
// ����˵��     length          ��Ҫ���յĳ���
// ���ز���     void
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static void wifi_spi_transfer_command (wifi_spi_packets_struct *packets, uint16 length)
{
    gpio_low(WIFI_SPI_CS_PIN);
    
    wifi_spi_bus_transfer_8bit((uint8 *)&(packets->head), (uint8 *)&(packets->head), sizeof(wifi_spi_head_struct));
    
    if(length)
    {
        wifi_spi_bus_transfer_8bit((const uint8 *)(packets->buffer), packets->buffer, length);
    }
    
    gpio_high(WIFI_SPI_CS_PIN);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ���������ͬʱ����(�����շ�)
// ����˵��     *write_data     ���͵����ݻ�������ַ
// ����˵��     *read_data      ���յ������ݵĴ洢��ַ
// ����˵��     length          ��Ҫ���յĳ���
// ���ز���     void
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static void wifi_spi_transfer_data (const uint8 *write_data, wifi_spi_packets_struct *read_data, uint16 length)
{
    gpio_low(WIFI_SPI_CS_PIN);
    
    read_data->head.command = WIFI_SPI_DATA;
    read_data->head.length  = length;
    
    wifi_spi_bus_transfer_8bit((uint8 *)&(read_data->head), (uint8 *)&(read_data->head), sizeof(wifi_spi_head_struct));
    
    if(WIFI_SPI_RECVIVE_SIZE < length)
    {
        wifi_spi_bus_transfer_8bit(write_data, read_data->buffer, WIFI_SPI_RECVIVE_SIZE);
        wifi_spi_bus_write_8bit_array(&write_data[WIFI_SPI_RECVIVE_SIZE], length - WIFI_SPI_RECVIVE_SIZE);
    }
    else
    {
        // ����Ҫ���͵����ݿ�������ȡ���������������write_dataԽ�����
        memcpy(read_data->buffer, write_data, length);
        wifi_spi_bus_transfer_8bit(read_data->buffer, read_data->buffer, WIFI_SPI_RECVIVE_SIZE);
    }
    gpio_high(WIFI_SPI_CS_PIN);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ��������
// ����˵��     command         ��������
// ����˵��     *buffer         ������ַ
// ����˵��     length          ��������
// ����˵��     wait_time       ���ȴ�ʱ�� ��λ100΢��
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_set_parameter (wifi_spi_packets_command_enum command, uint8 *buffer, uint16 length, uint32 wait_time)
{
    uint8 return_state;
    wifi_spi_head_struct head;
    return_state = 1;
    wifi_spi_debug_last_cmd = command;
    wifi_spi_debug_last_reply = 0;
    wifi_spi_debug_last_len = 0;
    do
    {
        head.command = command;
        head.length  = length;
        
        // �ȴ��ӻ�׼������
        if(wifi_spi_wait_idle(wait_time))
        {
            break;
        }

        wifi_spi_write(&head.command, sizeof(wifi_spi_head_struct), buffer, length);
        if(wifi_spi_wait_idle(wait_time))
        {
            break;
        }
        // ����Ӧ���ź�

        head.command = WIFI_SPI_DATA;
        head.length = 0;
        wifi_spi_transfer_command((wifi_spi_packets_struct *)&head, head.length);
        wifi_spi_debug_last_reply = head.command;
        wifi_spi_debug_last_len = head.length;
        system_delay_us(20);
        if(WIFI_SPI_REPLY_OK == head.command)
        {
            return_state = 0;
        }
    }while(0);
    
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ģ����Ϣ��ȡ
// ����˵��     command         ��������
// ����˵��     *buffer         ������յ��Ĳ�����ַ
// ����˵��     wait_time       ���ȴ�ʱ�� ��λ100΢��
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     �ڲ�ʹ�ã��û��������
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_get_parameter (wifi_spi_packets_command_enum command, wifi_spi_packets_struct *read_data, uint32 wait_time)
{
    uint8 return_state;

    return_state = 1;
    wifi_spi_debug_last_cmd = command;
    wifi_spi_debug_last_reply = 0;
    wifi_spi_debug_last_len = 0;
    do
    {
        // �ȴ��ӻ�׼������
        if(wifi_spi_wait_idle(wait_time))
        {
            break;
        }
        read_data->head.command = command;
        wifi_spi_write(&(read_data->head.command), WIFI_SPI_RECVIVE_SIZE, NULL, 0);

        if(wifi_spi_wait_idle(wait_time))
        {
            break;
        }
        read_data->head.command = WIFI_SPI_DATA;
        read_data->head.length = 0;
        wifi_spi_transfer_command(read_data, WIFI_SPI_RECVIVE_SIZE);
        wifi_spi_debug_last_reply = read_data->head.command;
        wifi_spi_debug_last_len = read_data->head.length;
        return_state = 0;
    }while(0);
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI �̼��汾��ȡ
// ����˵��     void            �˿ں�
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ���ú���֮�󣬹̼��汾��Ϣ���ַ�����ʽ������wifi_spi_version������
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_get_version (void)
{
    uint8 return_state;
    wifi_spi_packets_struct temp_packets;

    memset(&temp_packets, 0, sizeof(temp_packets));
    return_state = wifi_spi_get_parameter(WIFI_SPI_GET_VERSION, &temp_packets, OTHER_TIME_OUT);
    if((0 == return_state) && (WIFI_SPI_REPLY_VERSION == temp_packets.head.command))
    {
        if(temp_packets.head.length >= sizeof(wifi_spi_version))
        {
            temp_packets.head.length = sizeof(wifi_spi_version) - 1;
        }
        memcpy(wifi_spi_version, temp_packets.buffer, temp_packets.head.length);
        wifi_spi_version[temp_packets.head.length] = 0;
    }
    else
    {
        return_state = 1;
    }
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI MAC��ַ��ȡ
// ����˵��     void            �˿ں�
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ���ú���֮��MAC��ַ��Ϣ���ַ�����ʽ������wifi_spi_mac_addr������
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_get_mac_addr (void)
{
    uint8 return_state;
    wifi_spi_packets_struct temp_packets;

    memset(&temp_packets, 0, sizeof(temp_packets));
    return_state = wifi_spi_get_parameter(WIFI_SPI_GET_MAC_ADDR, &temp_packets, OTHER_TIME_OUT);
    if((0 == return_state) && (WIFI_SPI_REPLY_MAC_ADDR == temp_packets.head.command))
    {
        if(temp_packets.head.length >= sizeof(wifi_spi_mac_addr))
        {
            temp_packets.head.length = sizeof(wifi_spi_mac_addr) - 1;
        }
        memcpy(wifi_spi_mac_addr, temp_packets.buffer, temp_packets.head.length);
        wifi_spi_mac_addr[temp_packets.head.length] = 0;
    }
    else
    {
        return_state = 1;
    }
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI IP��ַ��˿ںŻ�ȡ
// ����˵��     void            �˿ں�
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ���ú���֮��IP��ַ��˿ں���Ϣ���ַ�����ʽ������wifi_spi_ip_addr_port������
//              ��Ҫ������Socket֮����ô˺�������������ȡ��Ϣ
//-------------------------------------------------------------------------------------------------------------------
static uint8 wifi_spi_get_ip_addr_port (void)
{
    uint8 return_state;
    wifi_spi_packets_struct temp_packets;

    memset(&temp_packets, 0, sizeof(temp_packets));
    return_state = wifi_spi_get_parameter(WIFI_SPI_GET_IP_ADDR, &temp_packets, OTHER_TIME_OUT);
    if((0 == return_state) && (WIFI_SPI_REPLY_IP_ADDR == temp_packets.head.command))
    {
        if(temp_packets.head.length >= sizeof(wifi_spi_ip_addr_port))
        {
            temp_packets.head.length = sizeof(wifi_spi_ip_addr_port) - 1;
        }
        memcpy(wifi_spi_ip_addr_port, temp_packets.buffer, temp_packets.head.length);
        wifi_spi_ip_addr_port[temp_packets.head.length] = 0;
    }
    else
    {
        return_state = 1;
    }
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ��ȡϵͳʱ��
// ����˵��     time_format     ʱ���ʽ
// ���ز���     *buffer         ����ʱ���ַ�����ַ ��������С������Ҫ30���ֽ�
// ���ز���     buffer_size     ��������С
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ��Ҫ������������ȵ�֮����ܻ�ȡ����ȷ��ʱ�䣬�����ڵ����������֮ǰ��Ҫȷ��WIFIģ������Ҫ���յ������Ѿ�ȫ���������
// ��ע��Ϣ     ��WIFI SPI ������߰汾֧��
// ��ע��Ϣ     �������ϢΪ�ַ�����Ϣ����ֱ��printf���������ֲ鿴
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_get_time (wifi_spi_time_enum time_format, char *buffer, uint8 buffer_size)
{
    uint8 return_state = 1;
    wifi_spi_packets_struct temp_packets;
    uint8 send_cmd, receive_cmd;

    // ����ʱ�������������30���ֽ�
    // ����ģ��̼�������V2�汾
    if((30 <= buffer_size) && (!strncmp(wifi_spi_version, "V2", 2)))
    {
        send_cmd = WIFI_SPI_GET_TIME1 + time_format - WIFI_SPI_UTC_0;
        receive_cmd = WIFI_SPI_REPLY_TIME1 + time_format - WIFI_SPI_UTC_0;

        return_state = wifi_spi_get_parameter(send_cmd, &temp_packets, OTHER_TIME_OUT);
        if((0 == return_state) && (receive_cmd == temp_packets.head.command))
        {
            return_state = (uint8)strncmp((const char *)temp_packets.buffer, "OK", 2);
            if(0 == return_state)
            {
                memcpy(buffer, &temp_packets.buffer[3], temp_packets.head.length - 3);
                buffer[temp_packets.head.length - 3] = 0;
            }
        }
    }
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ɨ���ȵ�
// ����˵��     *buffer         ����ɨ�赽���ȵ���Ϣ �����������ź�ǿ�ȣ�ÿ���ź�ǿ�Ⱥ�����һ�����з�
// ����˵��     buffer_size     �������ĳ���
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ��WIFI SPI ������߰汾֧��
// ��ע��Ϣ     �������ϢΪ�ַ�����Ϣ����ֱ��printf���������ֲ鿴
// ��ע��Ϣ     ÿһ�а���һ��wifi����������
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_wifi_scan (char *buffer, uint16 buffer_size)
{
    uint8 return_state;

    // ģ�������V2�汾�Ĺ̼���֧�ִ˹���
    if(!strncmp(wifi_spi_version, "V2", 2))
    {
        return_state = wifi_spi_set_parameter(WIFI_SPI_SET_WIFI_SCAN, NULL, 0, WIFI_CONNECT_TIME_OUT);

        if(0 == return_state)
        {
        #if (0 == WIFI_SPI_READ_TRANSFER)
            wifi_spi_send_buffer(NULL, 0);
        #endif
            while(0 == wifi_spi_read_buffer((uint8 *)buffer, buffer_size))
            {
                system_delay_ms(10);
            }
        }
    }

    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI �������ӵ�WiFi��Ϣ����������WiFi
// ����˵��     *wifi_ssid      WIFI����
// ����˵��     *pass_word      WIFI����
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     wifi_spi_wifi_connect("SEEKFREE", "SEEKFREE123");
// ��ע��Ϣ     wifi_spi_wifi_connect("SEEKFREE", NULL); // ����û�������WIFI�ȵ�
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_wifi_connect (char *wifi_ssid, char *pass_word)
{
    uint8 return_state;
    uint8 temp_buffer[64];
    uint16 length;
    
    if(NULL != pass_word)
    {
        // WIFI�ȵ������뷢���ȵ�����������
        length = (uint16)sprintf((char *)temp_buffer, "%s\r\n%s\r\n", wifi_ssid, pass_word);
    }
    else
    {
        // WIFI�ȵ�û������ֻ��Ҫ�����ȵ�����
        length = (uint16)sprintf((char *)temp_buffer, "%s\r\n", wifi_ssid);
    }

    return_state = wifi_spi_set_parameter(WIFI_SPI_SET_WIFI_INFORMATION, temp_buffer, length, WIFI_CONNECT_TIME_OUT);

    if(0 == return_state)
    {
        wifi_spi_get_ip_addr_port();
    }

    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI �������ӵ�Socket��Ϣ����������Socket
// ����˵��     *transport_type ��������
// ����˵��     *ip_addr        IP��ַ
// ����˵��     *port           Ŀ��˿ں�
// ����˵��     *local_port     �����˿ں�
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     wifi_spi_socket_connect("TCP", "192.168.2.5", "8080", "6060");
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_socket_connect (char *transport_type, char *ip_addr, char *port, char *local_port)
{
    uint8 return_state;
    uint8 temp_buffer[41];
    uint16 length;
    
    length = (uint16)sprintf((char *)temp_buffer, "%s\r\n%s\r\n%s\r\n%s\r\n", transport_type, ip_addr, port, local_port);

    return_state = wifi_spi_set_parameter(WIFI_SPI_SET_SOCKET_INFORMATION, temp_buffer, length, SOCKET_CONNECT_TIME_OUT);

    if(0 == return_state)
    {
        wifi_spi_get_ip_addr_port();
    }

    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI �Ͽ�Socket����
// ����˵��     void
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     wifi_spi_socket_disconnect();
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_socket_disconnect (void)
{
    wifi_spi_packets_struct temp_packets;

    return wifi_spi_get_parameter(WIFI_SPI_CLOSE_SOCKET, &temp_packets, OTHER_TIME_OUT);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ����λ
// ����˵��     void
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_reset (void)
{
    uint8 return_state;
    wifi_spi_head_struct head;
    return_state = 1;
    do
    {
        head.command = WIFI_SPI_RESET;
        head.length  = 0xA5A5;
        return_state = wifi_spi_wait_idle(OTHER_TIME_OUT);
        if(return_state)
        {
            break;
        }
        wifi_spi_write(&head.command, sizeof(wifi_spi_head_struct), NULL, 0);
    }while(0);
    
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI UDPģʽʱ�������ͺ���
// ����˵��     void
// ���ز���     uint8           ״̬ 0-�ɹ� 1-����
// ʹ��ʾ��
// ��ע��Ϣ     ��UDPģʽ��ģ���յ����ݺ��ȴ�2���룬2�����δ�յ�����������ͨ��socket���͵����磬���ϣ�����������������ݴ�����Ϻ���ô˺���
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_udp_send_now (void)
{
    uint8 return_state = 1;
    wifi_spi_packets_struct temp_packets;
    
    if(WIFI_SPI_IDLE == wifi_spi_mutex)
    {
        // ��ͨѶ״̬����Ϊæ
        wifi_spi_mutex = WIFI_SPI_BUSY;
        do
        {
            if(wifi_spi_wait_idle(OTHER_TIME_OUT))
            {
                break;
            }

            // ������ʼsocket����
            temp_packets.head.command = WIFI_SPI_UDP_SEND;
            temp_packets.head.length = 0;
            wifi_spi_transfer_command(&temp_packets, WIFI_SPI_RECVIVE_SIZE);
            
            // ����յ��İ����Ƿ�������
            if((WIFI_SPI_REPLY_DATA_START == temp_packets.head.command) || (WIFI_SPI_REPLY_DATA_END == temp_packets.head.command))
            {
                // ������յ�������
                if(temp_packets.head.length)
                {
                    fifo_write_buffer(&wifi_spi_fifo, temp_packets.buffer, temp_packets.head.length);
                }
            }
            
            // �ȴ�Ӧ���ź�
            if(wifi_spi_wait_idle(OTHER_TIME_OUT))
            {
                break;
            }
            
            // ����Ӧ���ź�
            temp_packets.head.command = WIFI_SPI_DATA;
            temp_packets.head.length = 0;
            wifi_spi_transfer_command(&temp_packets, temp_packets.head.length);
            
            if(WIFI_SPI_REPLY_OK == temp_packets.head.command)
            {
                return_state = 0;
            }
            
        }while(0);
        
        // ��ͨѶ״̬����Ϊ����
        wifi_spi_mutex = WIFI_SPI_IDLE;
    } 
    
    return return_state;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ���ݿ鷢�ͺ�����ͬ����������
// ����˵��     *buff           ��Ҫ���͵����ݵ�ַ
// ����˵��     length          ���ͳ���
// ���ز���     uint32          ʣ��δ���͵ĳ���
// ʹ��ʾ��     wifi_spi_send_buffer(buffer, 100);
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
uint32 wifi_spi_send_buffer (const uint8 *buffer, uint32 length)
{
    uint16 send_length;
    wifi_spi_packets_struct temp_packets;
    
    // ���WIFI SPI״̬������������жϻ����߳����Ѿ�������ͨѶ���򱾴β��ܷ�������
    if(WIFI_SPI_IDLE == wifi_spi_mutex)
    {
        // ��ͨѶ״̬����Ϊæ
        wifi_spi_mutex = WIFI_SPI_BUSY;
        
        while(length)
        {
            send_length = (uint16) (length > WIFI_SPI_TRANSFER_SIZE ? WIFI_SPI_TRANSFER_SIZE : length);
            
            if(wifi_spi_wait_idle(OTHER_TIME_OUT))
            {
                break;
            }
            
            wifi_spi_transfer_data(buffer, &temp_packets, send_length);
            
            // ����յ��İ����Ƿ�������
            if((WIFI_SPI_REPLY_DATA_START == temp_packets.head.command) || (WIFI_SPI_REPLY_DATA_END == temp_packets.head.command))
            {
                // ������յ�������
                if(temp_packets.head.length)
                {
                    fifo_write_buffer(&wifi_spi_fifo, temp_packets.buffer, temp_packets.head.length);
                }
            }
            
            length -= send_length;
            buffer += send_length;
        }
        
        // ������һ�εĽ����Ƿ����е����ݶ��������
        while(WIFI_SPI_REPLY_DATA_START == temp_packets.head.command)
        {
            if(wifi_spi_wait_idle(OTHER_TIME_OUT))
            {
                break;
            }
            
            // ������ȡģ��ʣ������
            temp_packets.head.command = WIFI_SPI_DATA;
            temp_packets.head.length  = 0;
            wifi_spi_transfer_command(&temp_packets, WIFI_SPI_RECVIVE_SIZE);
            // ����յ��İ����Ƿ�������
            if((WIFI_SPI_REPLY_DATA_START == temp_packets.head.command) || (WIFI_SPI_REPLY_DATA_END == temp_packets.head.command))
            {
                // ������յ�������
                if(temp_packets.head.length)
                {
                    fifo_write_buffer(&wifi_spi_fifo, temp_packets.buffer, temp_packets.head.length);
                }
            }
        }
        wifi_spi_mutex = WIFI_SPI_IDLE;
    }
    return length;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI �ַ������ͺ�����ͬ����������
// ����˵��     *string           ��Ҫ���͵��ַ���
// ���ز���     void
// ʹ��ʾ��     wifi_spi_send_string("123");
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
void wifi_spi_send_string(const char *string)
{
    wifi_spi_send_buffer((uint8*)string, strlen(string));
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WIFI SPI ��ȡ������
// ����˵��     *buff           ���ջ�����
// ����˵��     length          ��ȡ���ݳ���
// ���ز���     uint32          ʵ�ʶ�ȡ���ݳ���
// ʹ��ʾ��     wifi_spi_read_buffer(buffer, 100);
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
uint32 wifi_spi_read_buffer (uint8 *buffer, uint32 length)
{
    zf_assert(NULL != buffer);
    uint32 fifo_read_length;
    uint32 write_length = 0;
    wifi_spi_packets_struct temp_packets;

    // �����ж�FIFO���Ƿ������ݣ���������ȴ�FIFO��ȡ
    if(fifo_used(&wifi_spi_fifo))
    {
        fifo_read_length = fifo_used(&wifi_spi_fifo);
        fifo_read_length = length < fifo_read_length ? length : fifo_read_length;
        fifo_read_buffer(&wifi_spi_fifo, buffer, &fifo_read_length, FIFO_READ_AND_CLEAN);

        buffer += fifo_read_length;
        length -= fifo_read_length;
        write_length += fifo_read_length;
    }
    
#if(1 == WIFI_SPI_READ_TRANSFER)
    if(WIFI_SPI_IDLE == wifi_spi_mutex)
    {
        wifi_spi_mutex = WIFI_SPI_BUSY;                         // ��ͨѶ״̬����Ϊæ
        
        do
        {
            if( (WIFI_SPI_RECVIVE_SIZE > wifi_spi_fifo.size) || // ����������ռ䲻�����ٶ�ȡ
                (wifi_spi_wait_idle(OTHER_TIME_OUT))            // ��ʱ�˳�
              )
            {
                break;
            }

            temp_packets.head.command = WIFI_SPI_DATA;
            temp_packets.head.length  = 0;
            wifi_spi_transfer_command(&temp_packets, WIFI_SPI_RECVIVE_SIZE);
            // ����յ��İ����Ƿ�������
            if( ((WIFI_SPI_REPLY_DATA_START == temp_packets.head.command) || (WIFI_SPI_REPLY_DATA_END == temp_packets.head.command)) &&
                (temp_packets.head.length)
              )
            {
                if(length)
                {
                    fifo_read_length = length >= temp_packets.head.length ? temp_packets.head.length : length;
                    memcpy(buffer, temp_packets.buffer, fifo_read_length);
                    buffer += fifo_read_length;
                    length -= fifo_read_length;
                    write_length += fifo_read_length;
                    if(fifo_read_length < temp_packets.head.length) // �ⲿ���岻����ಿ��д��FIFO
                    {
                        fifo_write_buffer(&wifi_spi_fifo, temp_packets.buffer + fifo_read_length, temp_packets.head.length - fifo_read_length);
                    }
                }
                else                                                // ȫ��д��FIFO
                {
                    fifo_write_buffer(&wifi_spi_fifo, temp_packets.buffer, temp_packets.head.length);
                }
            }
        }while(WIFI_SPI_REPLY_DATA_START == temp_packets.head.command);
        wifi_spi_mutex = WIFI_SPI_IDLE;
    }
#endif
    
    return write_length;
}

//-------------------------------------------------------------------------------------------------------------------
// �������     WiFi ģ���ʼ��
// ����˵��     *wifi_ssid      Ŀ�����ӵ� WiFi ������ �ַ�����ʽ
// ����˵��     *pass_word      Ŀ�����ӵ� WiFi ������ �ַ�����ʽ
// ���ز���     uint8           ģ���ʼ��״̬ 0-�ɹ� 1-����
// ʹ��ʾ��     wifi_spi_init("SEEKFREE", "SEEKFREE123");
// ��ע��Ϣ     wifi_spi_init("SEEKFREE", NULL); // ����û�������WIFI�ȵ�
//-------------------------------------------------------------------------------------------------------------------
uint8 wifi_spi_init (char *wifi_ssid, char *pass_word)
{
    uint8 return_state = 0;
    
    fifo_init(&wifi_spi_fifo, FIFO_DATA_8BIT, wifi_spi_buffer, WIFI_SPI_RECVIVE_FIFO_SIZE);
#if (WIFI_SPI_SOFT_ENABLE)
    gpio_init(WIFI_SPI_SOFT_SCK_PIN,  GPO, 1, GPO_PUSH_PULL);
    gpio_init(WIFI_SPI_SOFT_MOSI_PIN, GPO, 1, GPO_PUSH_PULL);
    gpio_init(WIFI_SPI_SOFT_MISO_PIN, GPI, 0, GPI_PULL_UP);
#else
    spi_init(WIFI_SPI_INDEX, SPI_MODE3, WIFI_SPI_SPEED, WIFI_SPI_SCK_PIN, WIFI_SPI_MOSI_PIN, WIFI_SPI_MISO_PIN, SPI_CS_NULL);//Ӳ��SPI��ʼ��
#endif
    gpio_init(WIFI_SPI_CS_PIN,  GPO, 1, GPO_PUSH_PULL);
    gpio_init(WIFI_SPI_RST_PIN, GPO, 1, GPO_PUSH_PULL);
    gpio_init(WIFI_SPI_INT_PIN, GPI, 0, GPI_PULL_DOWN);
    
    // ��λ
    gpio_set_level(WIFI_SPI_RST_PIN, 0);
    system_delay_ms(WIFI_SPI_RESET_LOW_MS);
    gpio_set_level(WIFI_SPI_RST_PIN, 1);
    
    // Wait for the module firmware to finish booting.
    system_delay_ms(WIFI_SPI_BOOT_WAIT_MS);
    wifi_spi_mutex = WIFI_SPI_IDLE;
    wifi_spi_debug_step = 1;

    do
    {
        // �̼��汾��Ϣ���ַ�����ʽ������wifi_spi_version������
        for(uint8 retry = 0; retry < WIFI_SPI_VERSION_RETRY; retry++)
        {
            return_state = wifi_spi_get_version();
            if(0 == return_state)
            {
                break;
            }
            system_delay_ms(100);
        }
        if(return_state)
        {
            break;
        }
        wifi_spi_debug_step = 2;

        // MAC��ַ��Ϣ���ַ�����ʽ������wifi_spi_mac_addr������
        wifi_spi_get_mac_addr();
        wifi_spi_debug_step = 3;

        if(NULL == wifi_ssid)
        {
            // ��ʼ����ʱ����Ҫ����WIFI
            break;
        }
        wifi_spi_debug_step = 4;
        return_state = wifi_spi_wifi_connect(wifi_ssid, pass_word);
        if(return_state)
        {
            break;
        }
        wifi_spi_debug_step = 5;
        
    #if(1 == WIFI_SPI_AUTO_CONNECT)
        return_state = wifi_spi_socket_connect("TCP", WIFI_SPI_TARGET_IP, WIFI_SPI_TARGET_PORT, WIFI_SPI_LOCAL_PORT);
        if(return_state)
        {
            break;
        }
    #endif
        
    #if(2 == WIFI_SPI_AUTO_CONNECT)
        return_state = wifi_spi_socket_connect("UDP", WIFI_SPI_TARGET_IP, WIFI_SPI_TARGET_PORT, WIFI_SPI_LOCAL_PORT);
        if(return_state)
        {
            break;
        }
    #endif
    }while(0);

    return return_state;
}
