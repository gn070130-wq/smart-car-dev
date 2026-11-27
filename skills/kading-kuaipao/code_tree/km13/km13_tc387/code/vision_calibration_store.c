#include "vision_calibration_store.h"

#include "zf_common_headfile.h"

#include <string.h>

#define VISION_CALIBRATION_MAGIC          (0x5643414CUL) /* "VCAL" */
#define VISION_CALIBRATION_SCHEMA         (1UL)
#define VISION_CALIBRATION_RECORD_WORDS   (8U)
#define VISION_CALIBRATION_SCALE_MIN      (1000UL)
#define VISION_CALIBRATION_SCALE_MAX      (100000000UL)
#define VISION_CALIBRATION_DISTANCE_MAX   (20000U)

typedef struct
{
    uint32_t magic;
    uint32_t schema;
    uint32_t sequence;
    uint32_t distance_scale_mm_px;
    uint32_t follow_near_mm;
    uint32_t follow_far_mm;
    uint32_t reserved;
    uint32_t crc32;
} vision_calibration_record_t;

typedef char vision_calibration_record_size_must_be_32[
    (sizeof(vision_calibration_record_t) == 32U) ? 1 : -1];

static vision_calibration_t s_calibration;

static uint32_t vision_crc32_words(const uint32_t *words, uint16_t word_count)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint16_t word_index;

    for (word_index = 0U; word_index < word_count; ++word_index)
    {
        uint32_t value = words[word_index];
        uint8_t byte_index;
        for (byte_index = 0U; byte_index < 4U; ++byte_index)
        {
            uint8_t bit_index;
            crc ^= value & 0xFFU;
            for (bit_index = 0U; bit_index < 8U; ++bit_index)
            {
                crc = (crc >> 1U) ^ ((0U - (crc & 1U)) & 0xEDB88320UL);
            }
            value >>= 8U;
        }
    }
    return ~crc;
}

static uint8_t vision_calibration_values_valid(uint32_t scale,
                                                uint32_t near_mm,
                                                uint32_t far_mm)
{
    if ((scale < VISION_CALIBRATION_SCALE_MIN) || (scale > VISION_CALIBRATION_SCALE_MAX))
    {
        return 0U;
    }
    if ((near_mm > VISION_CALIBRATION_DISTANCE_MAX) || (far_mm > VISION_CALIBRATION_DISTANCE_MAX))
    {
        return 0U;
    }
    if (((near_mm == 0U) != (far_mm == 0U))
        || ((near_mm != 0U) && (near_mm >= far_mm)))
    {
        return 0U;
    }
    return 1U;
}

static uint8_t vision_calibration_record_valid(const vision_calibration_record_t *record)
{
    if ((record->magic != VISION_CALIBRATION_MAGIC)
        || (record->schema != VISION_CALIBRATION_SCHEMA)
        || (vision_calibration_values_valid(record->distance_scale_mm_px,
                                            record->follow_near_mm,
                                            record->follow_far_mm) == 0U))
    {
        return 0U;
    }
    return (uint8_t)(record->crc32
                     == vision_crc32_words((const uint32_t *)record,
                                           VISION_CALIBRATION_RECORD_WORDS - 1U));
}

static void vision_calibration_read_slot(uint8_t slot, vision_calibration_record_t *record)
{
    uint32_t page = (slot == 0U) ? VISION_CALIBRATION_PAGE_A : VISION_CALIBRATION_PAGE_B;
    flash_read_page(0U, page, (uint32 *)record, VISION_CALIBRATION_RECORD_WORDS);
}

/* Handles uint32 sequence wrap as long as slots differ by less than 2^31 writes. */
static uint8_t vision_sequence_is_newer(uint32_t candidate, uint32_t reference)
{
    return (uint8_t)(((int32_t)(candidate - reference)) > 0);
}

uint8_t vision_calibration_store_init(void)
{
    vision_calibration_record_t a;
    vision_calibration_record_t b;
    const vision_calibration_record_t *selected = NULL;
    uint8_t selected_slot = 0U;
    uint8_t a_valid;
    uint8_t b_valid;

    vision_calibration_read_slot(0U, &a);
    vision_calibration_read_slot(1U, &b);
    a_valid = vision_calibration_record_valid(&a);
    b_valid = vision_calibration_record_valid(&b);

    if ((a_valid != 0U) && (b_valid != 0U))
    {
        if (vision_sequence_is_newer(b.sequence, a.sequence) != 0U)
        {
            selected = &b;
            selected_slot = 1U;
        }
        else
        {
            selected = &a;
        }
    }
    else if (a_valid != 0U)
    {
        selected = &a;
    }
    else if (b_valid != 0U)
    {
        selected = &b;
        selected_slot = 1U;
    }

    memset(&s_calibration, 0, sizeof(s_calibration));
    if (selected == NULL)
    {
        return 0U;
    }

    s_calibration.distance_scale_mm_px = selected->distance_scale_mm_px;
    s_calibration.follow_near_mm = (uint16_t)selected->follow_near_mm;
    s_calibration.follow_far_mm = (uint16_t)selected->follow_far_mm;
    s_calibration.sequence = selected->sequence;
    s_calibration.valid = 1U;
    s_calibration.slot = selected_slot;
    return 1U;
}

uint8_t vision_calibration_store_get(vision_calibration_t *calibration)
{
    if (calibration == NULL)
    {
        return 0U;
    }
    *calibration = s_calibration;
    return s_calibration.valid;
}

uint32_t vision_calibration_command_crc(uint32_t distance_scale_mm_px,
                                        uint16_t follow_near_mm,
                                        uint16_t follow_far_mm)
{
    uint32_t words[3];
    words[0] = distance_scale_mm_px;
    words[1] = follow_near_mm;
    words[2] = follow_far_mm;
    return vision_crc32_words(words, 3U);
}

uint8_t vision_calibration_store_save(uint32_t distance_scale_mm_px,
                                      uint16_t follow_near_mm,
                                      uint16_t follow_far_mm,
                                      uint32_t command_crc)
{
    vision_calibration_record_t record;
    vision_calibration_record_t verify;
    uint8_t target_slot;
    uint32_t target_page;

    if ((vision_calibration_values_valid(distance_scale_mm_px,
                                         follow_near_mm,
                                         follow_far_mm) == 0U)
        || (command_crc != vision_calibration_command_crc(distance_scale_mm_px,
                                                          follow_near_mm,
                                                          follow_far_mm)))
    {
        return 0U;
    }

    memset(&record, 0, sizeof(record));
    record.magic = VISION_CALIBRATION_MAGIC;
    record.schema = VISION_CALIBRATION_SCHEMA;
    record.sequence = (s_calibration.valid != 0U) ? s_calibration.sequence + 1U : 1U;
    record.distance_scale_mm_px = distance_scale_mm_px;
    record.follow_near_mm = follow_near_mm;
    record.follow_far_mm = follow_far_mm;
    record.crc32 = vision_crc32_words((const uint32_t *)&record,
                                      VISION_CALIBRATION_RECORD_WORDS - 1U);

    target_slot = (s_calibration.valid != 0U) ? (uint8_t)(s_calibration.slot ^ 1U) : 0U;
    target_page = (target_slot == 0U) ? VISION_CALIBRATION_PAGE_A : VISION_CALIBRATION_PAGE_B;

    flash_erase_page(0U, target_page);
    flash_write_page(0U, target_page, (const uint32 *)&record,
                     VISION_CALIBRATION_RECORD_WORDS);
    vision_calibration_read_slot(target_slot, &verify);

    if ((vision_calibration_record_valid(&verify) == 0U)
        || (memcmp(&record, &verify, sizeof(record)) != 0))
    {
        return 0U;
    }

    s_calibration.distance_scale_mm_px = distance_scale_mm_px;
    s_calibration.follow_near_mm = follow_near_mm;
    s_calibration.follow_far_mm = follow_far_mm;
    s_calibration.sequence = record.sequence;
    s_calibration.valid = 1U;
    s_calibration.slot = target_slot;
    return 1U;
}
