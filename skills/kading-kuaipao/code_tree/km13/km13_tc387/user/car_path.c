#include "car_path.h"

#pragma section all "cpu0_dsram"

#define CAR_PATH_FLASH_MAGIC          (0x4B503133UL) /* "KP13" */
#define CAR_PATH_FLASH_SCHEMA         (2UL)
#define CAR_PATH_FLASH_HEADER_WORDS   (8U)
#define CAR_PATH_FLASH_POINT_WORDS    (4U)
#define CAR_PATH_FLASH_TOTAL_WORDS    (CAR_PATH_FLASH_PAGE_COUNT * EEPROM_PAGE_LENGTH)

volatile uint32 car_path_count = 0;
volatile float car_record_s_m = 0.0f;
volatile uint32 car_path_duration_ms = 0;
volatile uint8 car_path_full = 0;
volatile uint8 car_path_saved_valid = 0;

static car_path_point_struct path_points[CAR_PATH_MAX_POINTS];
static uint32 path_flash_page_words[EEPROM_PAGE_LENGTH];
static uint32 path_flash_cached_page_offset = 0xFFFFFFFFUL;
static uint32 record_start_ms = 0;
static float distance_since_point_m = 0.0f;
static float record_yaw_filtered_deg = 0.0f;
static float record_steer_filtered_deg = 0.0f;

static float path_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float path_filter_record_yaw(float yaw_deg)
{
    float error_deg = yaw_deg - record_yaw_filtered_deg;

    if(path_absf(error_deg) > CAR_PATH_YAW_DEADBAND_DEG)
    {
        record_yaw_filtered_deg += error_deg * CAR_PATH_YAW_LOWPASS_ALPHA;
    }
    return record_yaw_filtered_deg;
}

static float path_filter_record_steer(float steer_deg)
{
    float error_deg = steer_deg - record_steer_filtered_deg;

    if(path_absf(error_deg) > CAR_PATH_STEER_DEADBAND_DEG)
    {
        record_steer_filtered_deg += error_deg * CAR_PATH_STEER_LOWPASS_ALPHA;
    }
    return record_steer_filtered_deg;
}

static uint32 path_float_to_word(float value)
{
    flash_data_union converter;
    converter.float_type = value;
    return converter.uint32_type;
}

static float path_word_to_float(uint32 word)
{
    flash_data_union converter;
    converter.uint32_type = word;
    return converter.float_type;
}

static uint32 path_crc32_update_word(uint32 crc, uint32 value)
{
    uint8 byte_index;

    for(byte_index = 0U; byte_index < 4U; byte_index++)
    {
        uint8 bit_index;
        crc ^= value & 0xFFU;
        for(bit_index = 0U; bit_index < 8U; bit_index++)
        {
            crc = (crc >> 1U) ^ ((0U - (crc & 1U)) & 0xEDB88320UL);
        }
        value >>= 8U;
    }
    return crc;
}

static void path_flash_buffer_clear(void)
{
    uint16 i;

    for(i = 0U; i < EEPROM_PAGE_LENGTH; i++)
    {
        path_flash_page_words[i] = 0xFFFFFFFFUL;
    }
}

static uint8 path_flash_write_word(uint32 *word_index, uint32 word)
{
    uint32 page_offset;
    uint16 page_word;

    if(*word_index >= CAR_PATH_FLASH_TOTAL_WORDS) return 0U;

    page_offset = *word_index / EEPROM_PAGE_LENGTH;
    page_word = (uint16)(*word_index % EEPROM_PAGE_LENGTH);
    if(page_word == 0U)
    {
        path_flash_buffer_clear();
    }

    path_flash_page_words[page_word] = word;
    (*word_index)++;

    if((page_word + 1U) >= EEPROM_PAGE_LENGTH)
    {
        flash_write_page(0U, CAR_PATH_FLASH_PAGE_START + page_offset,
                         path_flash_page_words, EEPROM_PAGE_LENGTH);
    }
    return 1U;
}

static void path_flash_write_finish(uint32 word_index)
{
    uint32 page_offset;
    uint16 page_word;

    if(word_index == 0U) return;
    page_word = (uint16)(word_index % EEPROM_PAGE_LENGTH);
    if(page_word == 0U) return;

    page_offset = word_index / EEPROM_PAGE_LENGTH;
    flash_write_page(0U, CAR_PATH_FLASH_PAGE_START + page_offset,
                     path_flash_page_words, EEPROM_PAGE_LENGTH);
}

static uint32 path_flash_read_word(uint32 word_index)
{
    uint32 page_offset = word_index / EEPROM_PAGE_LENGTH;
    uint16 page_word = (uint16)(word_index % EEPROM_PAGE_LENGTH);

    if(page_offset != path_flash_cached_page_offset)
    {
        flash_read_page(0U, CAR_PATH_FLASH_PAGE_START + page_offset,
                        path_flash_page_words, EEPROM_PAGE_LENGTH);
        path_flash_cached_page_offset = page_offset;
    }
    return path_flash_page_words[page_word];
}

static void path_store(float yaw_deg, float steer_deg, uint32 now_ms)
{
    car_path_point_struct *point;
    uint32 elapsed_ms;

    if(car_path_count >= CAR_PATH_MAX_POINTS)
    {
        car_path_full = 1;
        return;
    }

    elapsed_ms = now_ms - record_start_ms;
    point = &path_points[car_path_count];
    point->s_m = car_record_s_m;
    point->yaw_deg = yaw_deg;
    point->steer_deg = steer_deg;
    point->time_10ms = (elapsed_ms >= 655350U) ? 65535U : (uint16)(elapsed_ms / 10U);
    car_path_count++;
    car_path_duration_ms = elapsed_ms;
    distance_since_point_m = 0.0f;
}

void car_path_clear(void)
{
    car_path_count = 0;
    car_record_s_m = 0.0f;
    car_path_duration_ms = 0;
    car_path_full = 0;
    car_path_saved_valid = 0;
    record_start_ms = 0;
    distance_since_point_m = 0.0f;
    record_yaw_filtered_deg = 0.0f;
    record_steer_filtered_deg = 0.0f;
}

void car_path_record_begin(float yaw_deg, float steer_deg, uint32 now_ms)
{
    car_path_clear();
    record_start_ms = now_ms;
    record_yaw_filtered_deg = yaw_deg;
    record_steer_filtered_deg = steer_deg;
    path_store(record_yaw_filtered_deg, record_steer_filtered_deg, now_ms);
}

void car_path_record_update(float delta_s_m, float yaw_deg, float steer_deg, uint32 now_ms)
{
    float move_m;
    float yaw_change_deg;
    float filtered_yaw_deg;
    float filtered_steer_deg;

    if(car_path_full || (0U == car_path_count)) return;

    // The taught leg is forward-only. Reverse wheel motion is never folded into the route.
    move_m = (delta_s_m >= CAR_PATH_RECORD_MOVE_EPS_M) ? delta_s_m : 0.0f;
    if(move_m <= 0.0f)
    {
        // An Ackermann car cannot change body heading while stopped. Do not
        // turn stationary gyro drift into a recorded bend.
        return;
    }
    car_record_s_m += move_m;
    distance_since_point_m += move_m;
    filtered_yaw_deg = path_filter_record_yaw(yaw_deg);
    filtered_steer_deg = path_filter_record_steer(steer_deg);
    yaw_change_deg = path_absf(filtered_yaw_deg - path_points[car_path_count - 1U].yaw_deg);

    if((distance_since_point_m >= CAR_PATH_STRAIGHT_SPACING_M)
            || ((distance_since_point_m >= CAR_PATH_TURN_MIN_MOVE_M)
                && (yaw_change_deg >= CAR_PATH_TURN_YAW_STEP_DEG)))
    {
        path_store(filtered_yaw_deg, filtered_steer_deg, now_ms);
    }
}

void car_path_record_finish(float yaw_deg, float steer_deg, uint32 now_ms)
{
    float filtered_yaw_deg = record_yaw_filtered_deg;
    float filtered_steer_deg = record_steer_filtered_deg;

    if(0U == car_path_count)
    {
        car_path_record_begin(yaw_deg, steer_deg, now_ms);
        return;
    }

    // record_yaw_filtered_deg was last updated while the rear wheels moved.
    // Do not inject gyro drift accumulated while waiting for KEY1 here.
    (void)yaw_deg;
    (void)steer_deg;

    if(distance_since_point_m >= CAR_PATH_FINAL_MIN_MOVE_M)
    {
        path_store(filtered_yaw_deg, filtered_steer_deg, now_ms);
    }
    else
    {
        car_path_point_struct *last = &path_points[car_path_count - 1U];
        uint32 elapsed_ms = now_ms - record_start_ms;
        last->s_m = car_record_s_m;
        last->yaw_deg = filtered_yaw_deg;
        last->steer_deg = filtered_steer_deg;
        last->time_10ms = (elapsed_ms >= 655350U) ? 65535U : (uint16)(elapsed_ms / 10U);
        car_path_duration_ms = elapsed_ms;
    }
}

uint8 car_path_is_ready(void)
{
    return ((car_path_count >= CAR_PATH_MIN_POINTS)
            && (car_record_s_m >= CAR_PATH_MIN_TOTAL_M)
            && !car_path_full) ? 1U : 0U;
}

uint8 car_path_flash_save(void)
{
    uint32 header[CAR_PATH_FLASH_HEADER_WORDS];
    uint32 word_index = 0U;
    uint32 crc = 0xFFFFFFFFUL;
    uint32 point_index;
    uint32 erase_index;

    if(!car_path_is_ready()) return 0U;
    if((CAR_PATH_FLASH_HEADER_WORDS + car_path_count * CAR_PATH_FLASH_POINT_WORDS)
            > CAR_PATH_FLASH_TOTAL_WORDS)
    {
        return 0U;
    }

    header[0] = CAR_PATH_FLASH_MAGIC;
    header[1] = CAR_PATH_FLASH_SCHEMA;
    header[2] = car_path_count;
    header[3] = path_float_to_word(car_record_s_m);
    header[4] = car_path_duration_ms;
    header[5] = 0U;
    header[6] = 0U;
    header[7] = 0U;

    for(word_index = 0U; word_index < (CAR_PATH_FLASH_HEADER_WORDS - 1U); word_index++)
    {
        crc = path_crc32_update_word(crc, header[word_index]);
    }
    for(point_index = 0U; point_index < car_path_count; point_index++)
    {
        crc = path_crc32_update_word(crc, path_float_to_word(path_points[point_index].s_m));
        crc = path_crc32_update_word(crc, path_float_to_word(path_points[point_index].yaw_deg));
        crc = path_crc32_update_word(crc, path_float_to_word(path_points[point_index].steer_deg));
        crc = path_crc32_update_word(crc, (uint32)path_points[point_index].time_10ms);
    }
    header[7] = ~crc;

    for(erase_index = 0U; erase_index < CAR_PATH_FLASH_PAGE_COUNT; erase_index++)
    {
        flash_erase_page(0U, CAR_PATH_FLASH_PAGE_START + erase_index);
    }

    word_index = 0U;
    for(erase_index = 0U; erase_index < CAR_PATH_FLASH_HEADER_WORDS; erase_index++)
    {
        if(path_flash_write_word(&word_index, header[erase_index]) == 0U) return 0U;
    }
    for(point_index = 0U; point_index < car_path_count; point_index++)
    {
        if(path_flash_write_word(&word_index, path_float_to_word(path_points[point_index].s_m)) == 0U) return 0U;
        if(path_flash_write_word(&word_index, path_float_to_word(path_points[point_index].yaw_deg)) == 0U) return 0U;
        if(path_flash_write_word(&word_index, path_float_to_word(path_points[point_index].steer_deg)) == 0U) return 0U;
        if(path_flash_write_word(&word_index, (uint32)path_points[point_index].time_10ms) == 0U) return 0U;
    }
    path_flash_write_finish(word_index);

    path_flash_cached_page_offset = 0xFFFFFFFFUL;
    car_path_saved_valid = car_path_flash_load();
    return car_path_saved_valid;
}

uint8 car_path_flash_load(void)
{
    uint32 header[CAR_PATH_FLASH_HEADER_WORDS];
    uint32 crc = 0xFFFFFFFFUL;
    uint32 stored_crc;
    uint32 point_count;
    uint32 point_index;
    uint32 word_index;
    float total_m;

    path_flash_cached_page_offset = 0xFFFFFFFFUL;
    for(word_index = 0U; word_index < CAR_PATH_FLASH_HEADER_WORDS; word_index++)
    {
        header[word_index] = path_flash_read_word(word_index);
    }

    point_count = header[2];
    if((header[0] != CAR_PATH_FLASH_MAGIC)
            || (header[1] != CAR_PATH_FLASH_SCHEMA)
            || (point_count > CAR_PATH_MAX_POINTS)
            || ((CAR_PATH_FLASH_HEADER_WORDS + point_count * CAR_PATH_FLASH_POINT_WORDS)
                > CAR_PATH_FLASH_TOTAL_WORDS))
    {
        car_path_clear();
        return 0U;
    }

    for(word_index = 0U; word_index < (CAR_PATH_FLASH_HEADER_WORDS - 1U); word_index++)
    {
        crc = path_crc32_update_word(crc, header[word_index]);
    }

    word_index = CAR_PATH_FLASH_HEADER_WORDS;
    for(point_index = 0U; point_index < point_count; point_index++)
    {
        uint32 s_word = path_flash_read_word(word_index++);
        uint32 yaw_word = path_flash_read_word(word_index++);
        uint32 steer_word = path_flash_read_word(word_index++);
        uint32 time_word = path_flash_read_word(word_index++);

        crc = path_crc32_update_word(crc, s_word);
        crc = path_crc32_update_word(crc, yaw_word);
        crc = path_crc32_update_word(crc, steer_word);
        crc = path_crc32_update_word(crc, time_word);

        path_points[point_index].s_m = path_word_to_float(s_word);
        path_points[point_index].yaw_deg = path_word_to_float(yaw_word);
        path_points[point_index].steer_deg = path_word_to_float(steer_word);
        path_points[point_index].time_10ms = (uint16)time_word;
    }

    stored_crc = header[7];
    if(stored_crc != ~crc)
    {
        car_path_clear();
        return 0U;
    }

    total_m = path_word_to_float(header[3]);
    car_path_count = point_count;
    car_record_s_m = total_m;
    car_path_duration_ms = header[4];
    car_path_full = 0U;
    car_path_saved_valid = car_path_is_ready();
    record_start_ms = 0U;
    distance_since_point_m = 0.0f;

    if(car_path_saved_valid == 0U)
    {
        car_path_clear();
        return 0U;
    }
    return 1U;
}

float car_path_total_length(void)
{
    return car_record_s_m;
}

uint8 car_path_sample_yaw(float s_m, float *yaw_deg, uint32 *lower_index)
{
    uint32 low;
    uint32 high;
    uint32 mid;
    const car_path_point_struct *p0;
    const car_path_point_struct *p1;
    float ds;
    float ratio;

    if((NULL == yaw_deg) || (0U == car_path_count)) return 0U;

    if(s_m <= 0.0f)
    {
        low = 0U;
    }
    else if(s_m >= car_record_s_m)
    {
        low = car_path_count - 1U;
    }
    else
    {
        low = 0U;
        high = car_path_count - 1U;
        while((high - low) > 1U)
        {
            mid = low + (high - low) / 2U;
            if(path_points[mid].s_m <= s_m) low = mid;
            else high = mid;
        }
    }

    if(low + 1U >= car_path_count)
    {
        *yaw_deg = path_points[low].yaw_deg;
        if(lower_index != NULL) *lower_index = low;
        return 1U;
    }

    p0 = &path_points[low];
    p1 = &path_points[low + 1U];
    ds = p1->s_m - p0->s_m;
    ratio = (ds > 0.0001f) ? ((s_m - p0->s_m) / ds) : 0.0f;
    if(ratio < 0.0f) ratio = 0.0f;
    if(ratio > 1.0f) ratio = 1.0f;

    *yaw_deg = p0->yaw_deg + (p1->yaw_deg - p0->yaw_deg) * ratio;
    if(lower_index != NULL) *lower_index = low;
    return 1U;
}

static uint8 path_fit_curvature(float center_s_m, float half_window_m,
        float *curvature, float *yaw_change_deg)
{
    float start_s;
    float end_s;
    float span_m;
    float sum_s = 0.0f;
    float sum_yaw = 0.0f;
    float mean_s;
    float mean_yaw;
    float covariance = 0.0f;
    float variance = 0.0f;
    float first_yaw = 0.0f;
    float last_yaw = 0.0f;
    uint32 i;

    if((curvature == NULL) || (yaw_change_deg == NULL)
            || (car_path_count < 2U) || (half_window_m <= 0.0f))
    {
        return 0U;
    }

    start_s = center_s_m - half_window_m;
    end_s = center_s_m + half_window_m;
    if(start_s < 0.0f)
    {
        end_s -= start_s;
        start_s = 0.0f;
    }
    if(end_s > car_record_s_m)
    {
        start_s -= end_s - car_record_s_m;
        end_s = car_record_s_m;
        if(start_s < 0.0f) start_s = 0.0f;
    }
    span_m = end_s - start_s;
    if(span_m < CAR_PATH_CURVATURE_MIN_WINDOW_M) return 0U;

    for(i = 0U; i < CAR_PATH_CURVATURE_FIT_SAMPLES; i++)
    {
        float ratio = (float)i / (float)(CAR_PATH_CURVATURE_FIT_SAMPLES - 1U);
        float sample_s = start_s + span_m * ratio;
        float sample_yaw;

        if(!car_path_sample_yaw(sample_s, &sample_yaw, NULL)) return 0U;
        if(i == 0U) first_yaw = sample_yaw;
        if(i + 1U == CAR_PATH_CURVATURE_FIT_SAMPLES) last_yaw = sample_yaw;
        sum_s += sample_s;
        sum_yaw += sample_yaw;
    }

    mean_s = sum_s / (float)CAR_PATH_CURVATURE_FIT_SAMPLES;
    mean_yaw = sum_yaw / (float)CAR_PATH_CURVATURE_FIT_SAMPLES;
    for(i = 0U; i < CAR_PATH_CURVATURE_FIT_SAMPLES; i++)
    {
        float ratio = (float)i / (float)(CAR_PATH_CURVATURE_FIT_SAMPLES - 1U);
        float sample_s = start_s + span_m * ratio;
        float sample_yaw;
        float ds;

        if(!car_path_sample_yaw(sample_s, &sample_yaw, NULL)) return 0U;
        ds = sample_s - mean_s;
        covariance += ds * (sample_yaw - mean_yaw);
        variance += ds * ds;
    }

    if(variance <= 0.000001f) return 0U;
    *curvature = covariance / variance * CAR_DEG_TO_RAD;
    *yaw_change_deg = last_yaw - first_yaw;
    return 1U;
}

uint8 car_path_sample(float s_m, car_path_sample_struct *sample)
{
    const car_path_point_struct *p0;
    const car_path_point_struct *p1;
    float ds;
    float ratio;

    if((sample == NULL) || !car_path_sample_yaw(s_m, &sample->yaw_deg,
            &sample->lower_index))
    {
        return 0U;
    }

    p0 = &path_points[sample->lower_index];
    if(sample->lower_index + 1U < car_path_count)
    {
        p1 = &path_points[sample->lower_index + 1U];
        ds = p1->s_m - p0->s_m;
        ratio = (ds > 0.0001f) ? ((s_m - p0->s_m) / ds) : 0.0f;
        if(ratio < 0.0f) ratio = 0.0f;
        if(ratio > 1.0f) ratio = 1.0f;
        sample->steer_deg = p0->steer_deg
                + (p1->steer_deg - p0->steer_deg) * ratio;
    }
    else
    {
        sample->steer_deg = p0->steer_deg;
    }

    sample->curvature_short = 0.0f;
    sample->curvature_long = 0.0f;
    sample->yaw_change_short_deg = 0.0f;
    sample->yaw_change_long_deg = 0.0f;
    (void)path_fit_curvature(s_m, CAR_PATH_CURVATURE_SHORT_HALF_M,
            &sample->curvature_short, &sample->yaw_change_short_deg);
    (void)path_fit_curvature(s_m, CAR_PATH_CURVATURE_LONG_HALF_M,
            &sample->curvature_long, &sample->yaw_change_long_deg);
    sample->curvature = sample->curvature_short;
    return 1U;
}

#pragma section all restore
