#include "zf_common_headfile.h"
#include "pwm.h"
#include "classifier.h"
#include "my_timeer.h"
#include "led.h"

int line = 0;
int num_list = 6;
int now_list = 1;
int i;
int j;
float road_1 = 0.35;
float road_2 = 0.3;
float road_3 = 0.25;
float road_4 = 0.1;
int times = 0;
int out_road = 0;
int mid_line_from_left[1000][2] = {0};
int mid_line_from_right[1000][2] = {0};
int left_line[1000][2] = {0};
int right_line[1000][2] = {0};
int last_error = 0;
int zebra_count = 0;
int round_num = 5;
float kp = 5.0;
float kd = 3.0;
float kp_dif = 1.0;
float kd_dif = 0.0;
int curve_far_num = 80;

int fast_speed = 400;
int mid_speed = 350;
int barrier_speed = 300;
int slow_speed = 300;
int far_num = 50;
int close_num = 40;
int mid_num = 45;
int zebra_cold_time = 0;
int circle_hold_time = 0;
int circle_cold_time = 0;
int direction = 0;
int line_flag = 50;
int zebra_sum = 0;
int white_sum = 0;
int up_corner_sum = 0;
int down_corner_sum = 0;
int patch_flag = 0;
int previous_output = 0;
// 滤波系数 alpha
double alpha = 0.1;
volatile bool exit_flag = false;

int16 encoder_left;
int16 encoder_right;

int circle_flag = 0;
int circle_save = 0;
int circle_save_old = 0;
int barrier_save_old = -1;
int max_time = 0;
int dirc = 2;
float error_last = 0.0;
float error_val = 0.0;
int left_patch = 0;
int right_patch = 0;
int expect_speed = 0;
int line_chose = 0;
int width_chose_left = 0;
int width_chose_right = 0;
int left_expect_speed = 0;
int right_expect_speed = 0;
int barrier_flag = -1;
int brick_save = 0;
int brick_save_old = 0;
int ramp_flag = 0;
int ramp_flag_hold = 0;
int barrier_cold_way = 0;
int barrier_hold_way = 0;
int barrier_hold_true = 0;
int ramp_save = 0;
int ramp_save_old = 0;
int ramp_hold_time = 0;
int ramp_cold_time = 0;
int brick_cold_time = 0;
int brick_hold_time = 0;
int barrier_save = 0;
int circle_flag_count = 0;
int barrier_flag_count = 0;
int circle_save_line[10000] = {0};
int barrier_save_line[10000] = {0};
int ramp_flag_old = 0;
int ramp_flag_cold = 0;
uint8 encoder_left_old = 0;
uint8 encoder_right_old = 0;
void *model = nullptr;
int barrier_hold = 50;
int zebra_count_sum = 0;
int zebra_hold_count = 0;
int dianya[20000] = {0};
int dianya_count = 0;
int start_flag = 0;
int change_flag = 0;
int circle_sum_count = 0;

extern int ttt;
extern int *key_scan;
extern int key_scan_gpio;
extern int fd_keyboard;

extern int accel_num[];
extern int gyro_num[];
extern int left_corner[100][2];
extern int right_corner[100][2];
extern uint8_t *PerImg_ip[240][320];
extern int test_bianliang;
extern int left_straight;
extern int right_straight;
extern Mat grayImage;
extern Mat square;
extern Mat binaryImage;
extern int mid_x;
extern int mid_y;
extern int left_lenth;
extern int mid_lenth_left;
extern int mid_lenth_right;
extern int mid_lenth_raw_left;
extern int mid_lenth_raw_right;
extern int right_lenth;
extern int left_start;
extern int right_start;
extern int left_line_raw[1000][2];
extern int mid_line_raw_from_left[1000][2];
extern int mid_line_raw_from_right[1000][2];
extern int right_line_raw[1000][2];
extern int pwm_fan;
int pwm_fan_true = 0;
extern int left_raw_lose;
extern int right_raw_lose;
extern int mid_lose;
extern int pwm_servo;
extern int pwm_servo_true;
extern float dif_speed;
extern int mid_straight;
extern int barrier_point[2];
extern int zebra_hold;
extern ClassifyResult result;
extern float angle_save;
extern float angle_barrier_save;
extern float error_raw;
extern int left_width[240];
extern int right_width[240];
extern int true_expect_speed;
extern int speed_true;
extern int start_num;

int expect_speed_save = 0;
int start_hold_time = 0;

void pit_callback_speed()
{
    // encoder_left = (int)(-encoder_get_count(ENCODER_LEFT) * 0.93);
    encoder_left = encoder_get_count(ENCODER_LEFT);
    encoder_right = -encoder_get_count(ENCODER_RIGHT);
    speed_true = (encoder_left + encoder_right) / 2;
    // left_expect_speed = 2 * expect_speed - encoder_right;
    // right_expect_speed = 2 * expect_speed - encoder_left;

    speed_control(encoder_left, encoder_right, left_expect_speed, right_expect_speed);
}

/**
 * 将整型数组保存到指定路径的文本文件中
 * * @param arr      指向整型数组的指针
 * @param size     数组中元素的数量
 * @param filepath 目标文件的路径
 * @return         保存成功返回 true，失败返回 false
 */
bool save_array_to_file(const int *arr, int size, const char *filepath)
{
    // 1. 安全检查：确保指针有效且数组大小大于 0
    if (arr == NULL || size <= 0 || filepath == NULL)
    {
        return false;
    }

    // 2. 打开文件（"w" 模式：写模式，文件不存在则创建，存在则清空）
    FILE *file = fopen(filepath, "w");
    if (file == NULL)
    {
        perror("打开文件失败");
        return false;
    }

    // 3. 遍历数组并写入文件
    for (int i = 0; i < size; i++)
    {
        // 写入当前元素
        fprintf(file, "%d", arr[i]);

        // 如果不是最后一个元素，则在后面加一个空格
        if (i < size - 1)
        {
            fprintf(file, " ");
        }
    }

    // 4. 关闭文件
    fclose(file);
    return true;
}

extern int left_start_save;
extern int right_start_save;
void main_run()
{
    clean_ptoto();
    dianya_count = 0;
    error_val = 0.0;
    out_road = 0;
    white_sum = 0;
    patch_flag = 0;
    zebra_cold_time = 0;
    circle_hold_time = 0;
    circle_cold_time = 10;
    zebra_count = 0;
    circle_save = 0;
    circle_save_old = 0;
    barrier_flag = -1;
    barrier_save = -1;
    barrier_save_old = -1;
    barrier_cold_way = 0;
    brick_cold_time = 50;
    brick_hold_time = 0;
    ramp_save = 0;
    ramp_save_old = 0;
    ramp_hold_time = 0;
    ramp_cold_time = 25;
    brick_save = 0;
    brick_save_old = 0;
    barrier_point[0] = 0;
    barrier_point[1] = 0;
    circle_flag_count = 0;
    barrier_flag_count = 0;
    zebra_count_sum = 0;
    pwm_fan_true = pwm_fan;
    expect_speed_save = 0;
    expect_speed = 0;
    start_hold_time = 100;
    start_flag = 0;
    change_flag = 0;
    circle_sum_count = 0;
    left_start_save = 0;
    right_start_save = 319;
    system("clear");
    cout << "START" << endl;
    pit_ms_init(100, pit_callback_led);
    for (int i = 0; i <= 10; i++)
    {
        key_scan_gpio = keyboard_scan_gpio();
        if (key_scan_gpio == RIGHT)
        {
            pwm_set_duty(POWER_TRANSFER, 0);
            pwm_set_duty(LEFT_WHELL_PWM, 0);
            pwm_set_duty(RIGHT_WHELL_PWM, 0);
            stop_car();
            return;
        }
        system("clear");
        get_mmap_image(1);
        cout << 10 - i << endl;
        msleep(100);
    }
    start_flag = -2;
    get_mmap_image(3);
    ImagePerspective1_Color_Init();
    ImagePerspective2_Color_Init();
    ImagePerspective3_Color_Init();
    for (int i = 600; i <= pwm_fan; i++)
    {

        pwm_set_duty(POWER_TRANSFER, i);
        msleep(2);
    }

    msleep(1000);
    start_flag = 1;
    gpio_set(LEFT_WHELL_GPIO, LEFT_FORWARD);
    gpio_set(RIGHT_WHELL_GPIO, RIGHT_FORWARD);
    speed_control_reset();
    pit_ms_init(5, pit_callback_speed);
    

    // struct timeval t_start, t_end;
    while (1)
    {
        // gettimeofday(&t_start, NULL);
        int get_image_error = get_mmap_image(1);

        if (get_image_error == -1)
        {
            save_all_photo();
            exit(0);
        }
        dirc = 2;
        left_straight = 0;
        right_straight = 0;
        error_last = error_val;
        left_raw_lose = 0;
        right_raw_lose = 0;
        mid_lose = 0;

        line_init();
        left_straight = right_straight = 0;
        /* print_image_toushi(0);
        print_point_and_neighbors(left_line_raw[0][0], left_line_raw[0][1]);
        print_point_and_neighbors(right_line_raw[0][0], right_line_raw[0][1]); */
        out_road = find_edge_line();

        if (out_road == -1 && barrier_save < 0 && barrier_save != -4)
        {
            int curr_len = 0;
            for (int x = 0; x < 320; ++x)
            {
                if (get_image_sum(239, x) == 255)
                { // 白色像素
                    curr_len++;
                }
            }
            if (curr_len <= 25)
            {
                system("clear");
                cout << "Out of Road!" << endl;
                stop_car();
                break;
            }
            else
            {

                out_road = 0;
            }
        }

        filter_lines(0, 13);
        filter_lines(1, 13);

        resample_track_line(0, 5);
        resample_track_line(1, 5);

        find_left_corner(circle_save);
        find_right_corner(circle_save);
        if (circle_cold_time <= 0)
        {

            circle_save = find_circle(circle_save);
        }

        if (brick_cold_time <= 0)
        {
            int brick_flag = find_brick();
            
            if (brick_flag == 1 || brick_flag == 2)
            {   
                brick_save = brick_flag;
                brick_hold_time = 10;
            }
            if (brick_save_old != brick_save)
            {
                brick_save_old = brick_save;
                if (circle_save < 30)
                {
                    circle_save_old = circle_save = 0;
                    circle_cold_time = barrier_hold;
                    circle_save_line[circle_flag_count] = -1;
                    circle_flag_count++;
                }
            }
        }
        
        if (circle_save_old != circle_save)
        {
            if(circle_save < 30 || circle_save > 30){
                circle_hold_time = 10000;
            }
            else{
                circle_hold_time = 20000;
            }
            circle_save_old = circle_save;
            circle_save_line[circle_flag_count] = circle_save;
            circle_flag_count++;
            
        }
        patch_line(circle_save);
        line_chose = chose_line(circle_save, barrier_save);
        int line_chose_without_circle = chose_line_without_circle(barrier_save);
        width_chose_left = chose_line_width(0);
        width_chose_right = chose_line_width(1);

        find_mid_line_from_left(line_chose, width_chose_left);
        find_mid_line_from_right(line_chose, width_chose_right);

        resample_track_line(2, 5);
        resample_track_line(3, 5);
        /* system("clear");
        for(int i = 0; i < 240; i++){
            print_pixel(i, left_width[i], "blue");
            print_pixel(i, right_width[i], "red");
        }
        msleep(200); */
        if (barrier_cold_way <= 0)
        {   
            barrier_flag = find_barrier_accurate(line_chose_without_circle);
            
            /* if (barrier_flag != -1)
            {
                system("clear");ssss
                cout << "angle_save: " << angle_save << endl;
                cout << "mid_lenth: " << mid_lenth_left << " " << mid_lenth_right << endl;
                cout << "circle_flag: " << circle_save << endl;
                cout << "straight: " << left_straight << " " << right_straight << " " << mid_straight << endl;
                cout << "left_corner: " << left_corner[0][0] << " " << left_corner[1][1] << " " << left_corner[2][1] << endl;
                cout << "right_corner: " << right_corner[0][0] << " " << right_corner[1][1] << " " << right_corner[2][1] << endl;
                cout << "barrier_save: " << barrier_save << endl;
                cout << "left lose: " << left_lose << endl;
                cout << "right lose: " << right_lose << endl;
                print_line_pixels();
                print_image(square, 0, 244);
                stop_car();
            } */

            if (barrier_flag == -3)
            {

                barrier_hold_way = 4000;
            }
            if (barrier_flag != barrier_save)
            {
                barrier_save = barrier_flag;
                if (circle_save < 20 && barrier_save >= 0)
                {
                    circle_save_old = circle_save = 0;
                    circle_cold_time = barrier_hold;
                    circle_save_line[circle_flag_count] = -2;
                    circle_flag_count++;
                }
                if (barrier_flag == -2 || barrier_flag == -3)
                {

                    barrier_hold_way = 4000;
                }
                else
                {
                    if (barrier_flag >= 0 && barrier_flag <= 5)
                    {
                        add_photo(square);
                    }

                    if (barrier_flag == 0 || barrier_flag == 1 || barrier_flag == 2 || barrier_flag == 3)
                    {
                        // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
                        // mycount++;
                        // imwrite(path, square);
                        barrier_hold_true = barrier_hold;
                        if ((barrier_save == 0 || barrier_save == 1))
                        {
                            if (angle_barrier_save < -45)
                            {

                                //barrier_hold_time = barrier_hold_time / 1.3;
                                barrier_hold_true = barrier_hold_true / 1.25;
                            }
                            else if (angle_barrier_save > 45)
                            {

                                //barrier_hold_time = barrier_hold_time * 1.3;
                                barrier_hold_true = barrier_hold_true * 1.4;
                            }
                        }
                        else if ((barrier_save == 2 || barrier_save == 3))
                        {
                            if (angle_barrier_save > 45)
                            {

                                //barrier_hold_time = barrier_hold_time / 1.3;
                                barrier_hold_true = barrier_hold_true / 1.25;
                            }
                            else if (angle_barrier_save < -45)
                            {

                                //barrier_hold_time = barrier_hold_time * 1.3;
                                barrier_hold_true = barrier_hold_true * 1.4;
                            }
                        }
                    }
                    else if (barrier_flag == 4 || barrier_flag == 5)
                    {
                        // snprintf(path, sizeof(path), "/home/root/image/image%d.png", mycount);
                        // mycount++;
                        // imwrite(path, square);
                       // barrier_hold_time = 1;
                        barrier_hold_true = 1;
                        barrier_cold_way = barrier_hold;
                    }
                    barrier_hold_way = barrier_hold_true;
                }
            }
        }
        /* if ((circle_sum_count == 2 || circle_sum_count == 5) && change_flag == 0)
        {

            start_load(4);
            start_num = 4;
            change_flag = 1;
            pwm_set_duty(POWER_TRANSFER, pwm_fan);
        }
        if (barrier_save == -4 && (barrier_flag_count == 8 || barrier_flag_count == 17) && change_flag == 1)
        {

            start_load(3);
            start_num = 3;
            change_flag = 0;
            pwm_set_duty(POWER_TRANSFER, pwm_fan);
        } */
        if(ramp_cold_time <= 0)
        {
            ramp_save = find_ramp();
            if(ramp_save_old != ramp_save){
                ramp_save_old = ramp_save;
                if(ramp_save == 1){
                    ramp_hold_time = 100;
                }
                else if(ramp_save == 2){
                    ramp_hold_time = 50;
                }
                else if(ramp_save == 3){
                    ramp_hold_time = 50;
                }
            }
            
        }
        /* if(ramp_save == 3){

            stop_car();
            break;
        } */
        if (zebra_cold_time >= 400)
        {

            zebra_sum = find_zebra();
            if (zebra_sum >= 4)
            {
                zebra_count++;
                if (zebra_count < round_num)
                {
                    pwm_fan_true += 30;
                    if (pwm_fan_true > 980)
                    {
                        pwm_fan_true = 980;
                    }
                    pwm_set_duty(POWER_TRANSFER, pwm_fan_true);
                }

                zebra_cold_time = 0;
                circle_cold_time = 20;
                circle_save_old = circle_save = 0;
                circle_save_line[circle_flag_count] = -3;
                circle_flag_count++;
            }
            else
            {

                zebra_hold_count = 0;
            }
        }

        if (zebra_count >= round_num)
        {
            zebra_hold_count++;
            if (zebra_hold_count > zebra_hold)
            {
                cout << "Find Zebra!" << endl;
                stop_car();
                break;
            }
        }
        //update_quaternion_from_imu();
        error_val = find_curve_all();
        if(start_hold_time > 0){
            expect_speed_save = get_expect_speed((int)error_val);
            if(expect_speed <= expect_speed_save - 10){

                expect_speed += 10;
            }
            else{

                expect_speed = expect_speed_save;
            }
        }
        else{

            expect_speed = get_expect_speed((int)error_val);
        }
        
        int dif = error_compute();
        
        left_expect_speed = expect_speed - dif;
        right_expect_speed = expect_speed + dif;
        //system("clear");
        // /cout << "left: " << left_expect_speed << " right: " << right_expect_speed << endl;

        if (barrier_hold_way > 0)
        {

            barrier_hold_way -= (speed_true / 10);
            if (barrier_hold_way <= 0)
            {
                if(barrier_save >= 0 && barrier_save < 6){

                    barrier_flag = -4;
                    barrier_save_old = barrier_save;
                    barrier_save = -4;
                    barrier_hold_way = barrier_hold;
                    angle_barrier_save = 0;
                }
                else
                {
                    barrier_flag = -1;
                    barrier_save = -1;
                    barrier_save_old = -1;
                    barrier_hold_way = 0;
                    angle_barrier_save = 0;
                }
            }
        }
        else
        {

            barrier_flag = -1;
            barrier_save = -1;
        }
        if (barrier_cold_way > 0)
        {
            barrier_flag = barrier_save = -1;
            barrier_cold_way -= (speed_true / 10);
        }
        if (brick_hold_time > 0)
        {

            brick_hold_time--;
            circle_save = circle_save_old = 0;
            circle_cold_time = 1000;
            if (brick_hold_time == 0)
            {

                brick_cold_time = 20;
                brick_save = 0;
                brick_save_old = 0;
            }
        }
        else
        {

            brick_save = 0;
        }
        if (brick_cold_time > 0)
        {
            brick_save = 0;
            brick_save_old = 0;
            brick_cold_time--;
        }
        if (ramp_hold_time > 0)
        {

            ramp_hold_time--;
            //circle_save = circle_save_old = 0;
            //circle_cold_time = 20;
            brick_hold_time = 0;
            brick_cold_time = 20;
            
            if (ramp_hold_time == 0)
            {

                ramp_cold_time = 20;
                ramp_save = 0;
            }
        }
        else
        {

            ramp_save = 0;
        }
        if (ramp_cold_time > 0)
        {
            ramp_save = 0;
            ramp_hold_time = 0;
            ramp_cold_time--;
        }

        if (zebra_cold_time <= 5000)
        {

            zebra_cold_time++;
        }
        /* if (barrier_save_old != barrier_save)
        {

            // circle_hold_time = 0;
            barrier_save_old = barrier_save;
            if (barrier_save >= 0)
            {

                barrier_save_line[barrier_flag_count] = barrier_save;
                barrier_flag_count++;
            }
        } */
        if (circle_hold_time > 0)
        {

            circle_hold_time -= (speed_true / 10);
        }
        else{
            circle_save_old = circle_save = 0;
            circle_hold_time = 0;

        }
        if (circle_cold_time > 0)
        {

            circle_cold_time -= (speed_true / 10);
            circle_hold_time = 0;
            circle_save_old = circle_save = 0;
        }

        if(start_hold_time > 0){

            start_hold_time--;
        }

        key_scan_gpio = keyboard_scan_gpio();

        if (key_scan_gpio == RIGHT)
        {

            system("clear");
            break;
        }

        else if (key_scan_gpio == LEFT)
        {

            system("clear");
            cout << "angle_save: " << angle_save << endl;
            cout << "mid_lenth: " << mid_lenth_left << " " << mid_lenth_right << endl;
            cout << "circle_flag: " << circle_save << endl;
            cout << "left_patch: " << left_patch << " right_patch: " << right_patch << endl;
            cout << "line_chose: " << line_chose << endl;
            //cout << "straight: " << left_straight << " " << right_straight << " " << mid_straight << endl;
            cout << "left_corner: " << left_corner[0][0] << " " << left_corner[1][1] << " " << left_corner[2][1] << endl;
            cout << "right_corner: " << right_corner[0][0] << " " << right_corner[1][1] << " " << right_corner[2][1] << endl;
            cout << "barrier_save: " << barrier_save << endl;
            cout << "left lose: " << left_raw_lose << endl;
            cout << "right lose: " << right_raw_lose << endl;
            print_line_pixels();
            print_point_and_neighbors(239, left_start_save);
            print_point_and_neighbors(239, right_start_save);
            // print_image_toushi(0);
            /* // 竖线：y方向遍历，x固定
            for (int i = 0; i < 240; i++)
            {
                print_pixel(i, barrier_point[1], "red");
            }
            // 横线：y固定，x方向遍历
            for (int i = 0; i < 320; i++)
            {
                print_pixel(barrier_point[0], i, "red");
            } */
            // cut_image(barrier_point[0] * 2, barrier_point[1] * 2, 100);
            // print_image(square,0,0);
        }

        /*  gettimeofday(&t_end, NULL);
         double elapsed_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                             (t_end.tv_usec - t_start.tv_usec) / 1000.0;
         system("clear");
         printf("%.2f ms\n", elapsed_ms); */
    }
    left_expect_speed = right_expect_speed = 0;
    pit_ms_clean();
    gpio_set(LEFT_WHELL_GPIO, 0);
    gpio_set(RIGHT_WHELL_GPIO, 0);
    pwm_set_duty(LEFT_WHELL_PWM, 0);
    pwm_set_duty(RIGHT_WHELL_PWM, 0);
    pwm_set_duty(POWER_TRANSFER, 0);
    pwm_set_duty(POWER_TRANSFER, 0);
}

void display_run()
{
    
}