#include "vision_calibration_protocol.h"

#include "vision_calibration_store.h"
#include "zf_common_headfile.h"

#include <string.h>

#define VISION_CAL_LINE_LENGTH (96U)

static char s_line[VISION_CAL_LINE_LENGTH];
static uint8_t s_line_length;

static uint8_t vision_parse_uint32(const char **cursor, uint32_t *value, uint8_t hexadecimal)
{
    const char *p = *cursor;
    uint32_t result = 0U;
    uint8_t digits = 0U;

    while (*p == ' ')
    {
        ++p;
    }
    if ((hexadecimal != 0U) && (p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X')))
    {
        p += 2;
    }

    while (*p != '\0')
    {
        uint8_t digit;
        char c = *p;
        if ((c >= '0') && (c <= '9'))
        {
            digit = (uint8_t)(c - '0');
        }
        else if ((hexadecimal != 0U) && (c >= 'a') && (c <= 'f'))
        {
            digit = (uint8_t)(c - 'a' + 10);
        }
        else if ((hexadecimal != 0U) && (c >= 'A') && (c <= 'F'))
        {
            digit = (uint8_t)(c - 'A' + 10);
        }
        else
        {
            break;
        }
        if ((hexadecimal == 0U) && (digit >= 10U))
        {
            break;
        }
        result = result * ((hexadecimal != 0U) ? 16U : 10U) + digit;
        ++digits;
        ++p;
    }
    if (digits == 0U)
    {
        return 0U;
    }
    *cursor = p;
    *value = result;
    return 1U;
}

static uint8_t vision_only_spaces_remain(const char *cursor)
{
    while (*cursor == ' ')
    {
        ++cursor;
    }
    return (uint8_t)(*cursor == '\0');
}

static void vision_calibration_print(void)
{
    vision_calibration_t calibration;
    if (vision_calibration_store_get(&calibration) == 0U)
    {
        printf("CAL DATA valid=0\r\n");
        return;
    }
    printf("CAL DATA valid=1 scale=%lu near=%lu far=%lu seq=%lu slot=%lu crc=%08lX\r\n",
           (unsigned long)calibration.distance_scale_mm_px,
           (unsigned long)calibration.follow_near_mm,
           (unsigned long)calibration.follow_far_mm,
           (unsigned long)calibration.sequence,
           (unsigned long)calibration.slot,
           (unsigned long)vision_calibration_command_crc(calibration.distance_scale_mm_px,
                                                         calibration.follow_near_mm,
                                                         calibration.follow_far_mm));
}

static void vision_calibration_process_line(char *line)
{
    if (strcmp(line, "CAL GET") == 0)
    {
        vision_calibration_print();
    }
    else if (strncmp(line, "CAL SAVE ", 9U) == 0)
    {
        const char *cursor = line + 9U;
        uint32_t scale;
        uint32_t near_mm;
        uint32_t far_mm;
        uint32_t crc;

        if ((vision_parse_uint32(&cursor, &scale, 0U) == 0U)
            || (vision_parse_uint32(&cursor, &near_mm, 0U) == 0U)
            || (vision_parse_uint32(&cursor, &far_mm, 0U) == 0U)
            || (vision_parse_uint32(&cursor, &crc, 1U) == 0U)
            || (vision_only_spaces_remain(cursor) == 0U))
        {
            printf("CAL ERR reason=format\r\n");
        }
        else if ((near_mm > 0xFFFFU) || (far_mm > 0xFFFFU))
        {
            printf("CAL ERR reason=range\r\n");
        }
        else if (crc != vision_calibration_command_crc(scale,
                                                       (uint16_t)near_mm,
                                                       (uint16_t)far_mm))
        {
            printf("CAL ERR reason=crc\r\n");
        }
        else if (vision_calibration_store_save(scale,
                                               (uint16_t)near_mm,
                                               (uint16_t)far_mm,
                                               crc) == 0U)
        {
            printf("CAL ERR reason=flash_or_range\r\n");
        }
        else
        {
            vision_calibration_t calibration;
            (void)vision_calibration_store_get(&calibration);
            printf("CAL OK scale=%lu near=%lu far=%lu seq=%lu slot=%lu reboot=1\r\n",
                   (unsigned long)calibration.distance_scale_mm_px,
                   (unsigned long)calibration.follow_near_mm,
                   (unsigned long)calibration.follow_far_mm,
                   (unsigned long)calibration.sequence,
                   (unsigned long)calibration.slot);
        }
    }
    else if (line[0] != '\0')
    {
        printf("CAL ERR reason=unknown_command\r\n");
    }
}

void vision_calibration_protocol_poll(void)
{
#if DEBUG_UART_USE_INTERRUPT
    uint8_t input[16];
    uint32_t count = debug_read_ring_buffer(input, sizeof(input));
    uint32_t index;

    for (index = 0U; index < count; ++index)
    {
        char c = (char)input[index];
        if ((c == '\r') || (c == '\n'))
        {
            if (s_line_length != 0U)
            {
                s_line[s_line_length] = '\0';
                vision_calibration_process_line(s_line);
                s_line_length = 0U;
            }
        }
        else if (s_line_length < VISION_CAL_LINE_LENGTH - 1U)
        {
            s_line[s_line_length++] = c;
        }
        else
        {
            s_line_length = 0U;
            printf("CAL ERR reason=line_too_long\r\n");
        }
    }
#endif
}
