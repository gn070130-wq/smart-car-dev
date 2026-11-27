#include "display_image.hpp"

static zf_device_ips200 g_display_ips200;

static void display_image_draw_boundaries(const VisionResult *vision)
{
    uint16_t row = 0;
    uint16_t start_row = 0;
    uint16_t end_row = 0;

    if(NULL == vision)
    {
        return;
    }

    start_row = vision->valid_row_start;
    end_row = vision->valid_row_end;

    if(end_row >= SEARCH_UVC_HEIGHT)
    {
        end_row = SEARCH_UVC_HEIGHT - 1;
    }

    if(start_row >= end_row)
    {
        return;
    }

    for(row = start_row + 1; row <= end_row; row++)
    {
        g_display_ips200.draw_line(vision->left_boundary[row - 1], row - 1,
                                   vision->left_boundary[row], row, RGB565_BLUE);
        g_display_ips200.draw_line(vision->right_boundary[row - 1], row - 1,
                                   vision->right_boundary[row], row, RGB565_RED);
        g_display_ips200.draw_line(vision->center_line[row - 1], row - 1,
                                   vision->center_line[row], row, RGB565_GREEN);
    }
}

static void display_image_draw_reference(const VisionResult *vision)
{
    uint16_t row = 0;

    if(NULL == vision)
    {
        return;
    }

    for(row = 0; row < SEARCH_UVC_HEIGHT; row++)
    {
        if(0 == (row % 4))
        {
            g_display_ips200.draw_point(vision->reference_col, row, RGB565_YELLOW);
        }
    }

    g_display_ips200.draw_point(vision->reference_col, vision->reference_row, RGB565_CYAN);
    if(vision->reference_col > 0)
    {
        g_display_ips200.draw_point(vision->reference_col - 1, vision->reference_row, RGB565_CYAN);
    }
    if(vision->reference_col + 1 < SEARCH_UVC_WIDTH)
    {
        g_display_ips200.draw_point(vision->reference_col + 1, vision->reference_row, RGB565_CYAN);
    }
    if(vision->reference_row > 0)
    {
        g_display_ips200.draw_point(vision->reference_col, vision->reference_row - 1, RGB565_CYAN);
    }
    if(vision->reference_row + 1 < SEARCH_UVC_HEIGHT)
    {
        g_display_ips200.draw_point(vision->reference_col, vision->reference_row + 1, RGB565_CYAN);
    }
}

void display_image_init(void)
{
    g_display_ips200.init();
    g_display_ips200.full(RGB565_BLACK);
}

void display_image_show(const uint8_t *gray, const VisionResult *vision)
{
    if(NULL != gray)
    {
        g_display_ips200.show_gray_image(0, 0, gray, UVC_WIDTH, UVC_HEIGHT, UVC_WIDTH, UVC_HEIGHT, 0);
    }

    if(NULL != vision)
    {
        display_image_draw_boundaries(vision);
        display_image_draw_reference(vision);
    }
}

zf_device_ips200 *display_image_get_device(void)
{
    return &g_display_ips200;
}
