#ifndef FIND_ROAD_H
#define FIND_ROAD_H

#define PI 3.14159265

// 链表插值
// @param dir   插值方向，0为左线，1为右线
// @param num   插值数量
// @details     对指定方向的线条进行插值处理，填补缺失点，提高线条连续性。
void inter_num(int dir, int num);

// 滑动平均滤波
// @param type      0为左线，1为右线，2为中线
// @param strength  滤波强度（窗口大小）
// @details         对指定类型的线条进行滑动平均滤波，平滑线条，去除噪声。
void filter_lines(int type, int strength);

// 寻找边线
// @return   返回检测到的边线数量
// @details  扫描图像，检测并提取道路左右边线的位置。
int find_edge_line();

// 选择合适的边线
// @param circle_flag   环岛标志
// @param barrier_flag  障碍物标志
// @return              返回选中的边线类型
// @details             根据场景（如环岛、障碍物）选择最优的边线用于后续处理。  
int chose_line_without_circle(int barrier_flag);

int chose_line(int circle_flag, int barrier_flag);

// 选择合适的道路宽度
// @param barrier_flag  障碍物标志
// @return              返回选中的道路宽度
// @details             根据障碍物情况动态调整道路宽度。
int chose_line_width(int line);

// 寻路：扫中线
// @param dir    扫描方向
// @param width  扫描宽度
// @details      在指定方向和宽度范围内，检测并提取道路中线。
void find_mid_line_from_right(int dir, int width);

void find_mid_line_from_left(int dir, int width);

// 寻路：坡道区域扫中线
// @details      针对坡道区域，检测并提取道路中线。
void find_mid_line_ramp();

// 计算四点夹角（浮点型）
// @param x1,y1,x2,y2,x3,y3,x4,y4  四个点的坐标
// @return                         返回夹角值（浮点型）
// @details                        计算由四个点组成的两条线段之间的夹角，结果为浮点数。
float calculate_angle_float(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

// 计算四点夹角（整型）
// @param x1,y1,x2,y2,x3,y3,x4,y4  四个点的坐标
// @return                         返回夹角值（整型）
// @details                        计算由四个点组成的两条线段之间的夹角，结果为整数。
int calculate_angle(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

// 寻路：偏差值计算
// @return    返回当前车辆与道路中心的偏差值
// @details   计算车辆当前位置与道路中心线的横向偏差，用于控制调整。
float find_curve_from_left();

float find_curve_all();

float find_curve_from_right();

float find_curve_from_mid();

// 获取期望速度（根据偏差）
// @param Error   当前偏差值
// @return        返回期望速度
// @details       根据车辆偏差动态调整期望速度，实现平稳行驶。
int get_expect_speed(int Error);

// 寻路：扫左拐点
// @param circle_flag   环岛标志
// @return              返回检测到的左拐点数量
// @details             检测道路上的左拐点，用于路径规划。
int find_left_corner(int circle_flag);

// 寻路：扫右拐点
// @param circle_flag   环岛标志
// @return              返回检测到的右拐点数量
// @details             检测道路上的右拐点，用于路径规划。
int find_right_corner(int circle_flag);

// 寻路：扫斑马线
// @return    返回检测到的斑马线数量
// @details   检测图像中的斑马线区域，用于特殊场景处理。
int find_zebra();

// 寻路：补线前处理
// @param circle_flag   环岛标志
// @details             检测补线区域
void patch_line(int circle_flag);

// 寻路：扫环岛
// @param circle_flag   环岛标志
// @return              返回检测到的环岛数量
// @details             检测道路中的环岛区域，用于路径决策。
int find_circle(int circle_flag);

// 左侧线条生长
// @details   对左侧边线进行生长处理
void line_grow_left();

// 右侧线条生长
// @details   对右侧边线进行生长处理
void line_grow_right();

// 计算两点距离
// @param y1,x1,y2,x2   两个点的坐标
// @return              返回两点间的欧氏距离
// @details             计算图像中两点之间的直线距离。
float point_dist(float y1, float x1, float y2, float x2);

// 线性插值
// @param a,b   两个端点
// @param t     插值参数（0~1）
// @return      返回插值结果
// @details     计算a和b之间的线性插值值。
float lerp(float a, float b, float t);

// 寻找障碍物
// @return    检测到左障碍物返回0，右障碍物返回1，否则返回0
// @details   检测图像中的障碍物区域，用于避障处理。
int find_barrier(int type);
int find_barrier_accurate(int type);
int find_brick();

void resample_track_line(int type, float spacing);

// 计算两点间的距离
// @param x1,y1,x2,y2   两个点的坐标
// @return              返回两点间的距离
// @details             计算图像中两点之间的直线距离。
int point_distance(int x1, int y1, int x2, int y2);

void line_init();

int find_ramp();

#endif // FIND_ROAD_H
