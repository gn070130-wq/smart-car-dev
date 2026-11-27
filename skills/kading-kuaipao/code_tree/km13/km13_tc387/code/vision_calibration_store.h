#ifndef VISION_CALIBRATION_STORE_H
#define VISION_CALIBRATION_STORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DFlash ownership for visual distance calibration:
 *   logical page 62: 0xAF03E000..0xAF03EFFF
 *   logical page 63: 0xAF03F000..0xAF03FFFF
 * Do not reuse these two 4 KiB pages in another module.
 */
#define VISION_CALIBRATION_PAGE_A (62U)
#define VISION_CALIBRATION_PAGE_B (63U)

typedef struct
{
    uint32_t distance_scale_mm_px;
    uint16_t follow_near_mm;
    uint16_t follow_far_mm;
    uint32_t sequence;
    uint8_t valid;
    uint8_t slot;
} vision_calibration_t;

/* Load the newest CRC-valid slot. Returns 1 when persistent data is valid. */
uint8_t vision_calibration_store_init(void);

/* Copy the boot-time or most recently saved calibration. */
uint8_t vision_calibration_store_get(vision_calibration_t *calibration);

/* CRC-32/ISO-HDLC over scale, near and far as three little-endian uint32 words. */
uint32_t vision_calibration_command_crc(uint32_t distance_scale_mm_px,
                                        uint16_t follow_near_mm,
                                        uint16_t follow_far_mm);

/*
 * Save to the inactive slot, verify by readback, then publish it as current.
 * Returns 1 on success and 0 on validation, CRC, programming or readback failure.
 */
uint8_t vision_calibration_store_save(uint32_t distance_scale_mm_px,
                                      uint16_t follow_near_mm,
                                      uint16_t follow_far_mm,
                                      uint32_t command_crc);

#ifdef __cplusplus
}
#endif

#endif /* VISION_CALIBRATION_STORE_H */
