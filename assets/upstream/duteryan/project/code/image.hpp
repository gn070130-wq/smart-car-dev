#ifndef __IMAGE_hpp__
#define __IMAGE_hpp__
#include  "zf_common_headfile.hpp"
#define IMAGE_WIDTH 150  //裁剪后的图像大小
#define IMAGE_HEIGHT 60
#define END_LINE_GAP 8
#define END_LINE_NUM  ((IMAGE_WIDTH-1)/END_LINE_GAP)
#define SouXianValidHeight 10 //搜线有效行数（图像顶部）
#define Black 0
#define White 255
#define Max_Point 250 //爬线最多点个数
#define _left 0
#define _right 1
#define range_left 1
#define range_right (IMAGE_WIDTH-2)
#define LOST_COL_DIFFER 40   //绕行丢线时给定的固定偏差(左绕行取+,右绕行取-);上车自行调大小
#define _small (0)
#define _big (1)
extern int End_Line;
extern int image_threshold;//大津法阈值
extern uint8_t Image[IMAGE_HEIGHT][IMAGE_WIDTH]; //定义裁剪后的图像数组(1D转2D)
extern uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH];          // 二值化图像数组
extern int Search_Stop_Line;
extern int straightwidth[IMAGE_HEIGHT];
typedef struct
{
  int x;
  int y;
} Point;

struct LEFT_EDGE
{
    int16 row; //行坐标
    int16 col; //列坐标
    uint8 flag; //存在边界的标志
};
struct RIGHT_EDGE
{
    int16 row; //行坐标
    int16 col; //列坐标
    uint8 flag; //存在边界的标志
};
extern int turn_propect;//前瞻行
extern LEFT_EDGE L_edge[Max_Point]; //左边界结构体
extern RIGHT_EDGE R_edge[Max_Point]; //右边界结构体
extern int left_halo_flag ;
extern int right_halo_flag ;
extern int cross_cnt ;   // 十字状态机标志(0=无, 1=补线中, 2=退出段)，供红砖禁区判定
extern int left_effect_row_start;
extern int left_effect_index_start;
extern int right_effect_index_start;
extern int right_effect_row_start;
extern int left_effect_row_end;
extern int right_effect_row_end;
extern int left_effect_index_end;
extern int right_effect_index_end;







/* 
 * 全局输出变量：用于给车控制逻辑使用
 * curve_score: 弯曲程度绝对得分 (0~N，通常直道<3，弯道可能十几到几十)
 * curve_dir:   弯曲方向 (1 代表右弯，-1 代表左弯，0 代表直道)
 */
// extern int curve_score ; 
// extern int curve_dir ;  

extern int min_efect_row_end;


extern uint8 current_left_col[IMAGE_HEIGHT];
extern uint8 current_right_col[IMAGE_HEIGHT];
extern int bottom_narrow_flag;   //底部15行赛道宽度是否全部<40:1=是(窄),0=否
void bottom_width_judge();        //底部窄道判定
void reset_image_runtime_state(void); // 发车前清空图像与赛道元素运行状态
extern int centralline[IMAGE_HEIGHT];
extern uint8 current_left_row[IMAGE_HEIGHT];
extern uint8 current_right_row[IMAGE_HEIGHT];
extern float col_Differ;//偏差
extern float Last_col_Differ;
extern int wd_flag,ruwan_flag,zhi_flag;
extern int wd_flag_stable;//wd_flag 去抖后的稳定值(供绕行起始/距离判定)
void wd_flag_debounce(void);//每帧在 qulv_characteriristic 后调用,维护 wd_flag_stable
void Convert_1D_to_2D(const uint8_t* src); //一维数组转二维数组函数
uint8_t GetOSTUThreshold(uint8_t img[IMAGE_HEIGHT][IMAGE_WIDTH], uint8_t start_row, uint8_t end_row, uint8_t start_col, uint8_t end_col);//大津法求阈值
void set_image_twovalues(uint8 value_O);//将图像转为01二维数组
void LvBoSaiDao(uint8 arr[IMAGE_HEIGHT][IMAGE_WIDTH], uint8 startline, uint8 endline); //赛道滤波
void image_draw_rectan(uint8 image[IMAGE_HEIGHT][IMAGE_WIDTH]);//为图像画黑框
int find_longest_col();//寻找最长白列
int edge_point_ornot(uint8 row, uint8 side);//寻找爬线起始点
void find_line();
void DIUXIAN_XIUZLZ();
void search_neighborhood();//八邻域爬线
void current_wide();
void effect_line();
void search_corner();
void shizi_judge(const uint8 arr[IMAGE_HEIGHT][IMAGE_WIDTH]);//十字判断
void shizi_one();
void shizi_two();
void reNiHe();
void getdiffer(uint8 forward_row);   //得到赛道偏差值
void qulv_characteriristic();
// int banmaxian_start(const uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH]); //斑马线检测
extern int banmaxian_flag;//斑马线标志
void banmaxian_start(
    const uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH],
    const uint8_t left_border[IMAGE_HEIGHT],    // 左边界
    const uint8_t right_border[IMAGE_HEIGHT],   // 右边界
    uint16_t end_row                            // 底部结束行
);
// int stop_judge();
/*--------------------------------------------------------------------------
 * 【函数功能】：直线画线函数 (Bresenham 画线法变种 / 两点式)
 * 【描述】：给定起点和终点坐标，计算直线路径，并将列坐标填入 current_left/right_col
 * 【注意】：如果你的主干代码里已经实现了 ADD_Edge_Line，请删除这段实现！
 *--------------------------------------------------------------------------*/
void ADD_Edge_Line(int edge, int startX, int startY, int endX, int endY, int START_ROW, int END_ROW);

extern int out_flag_judge;       // 出界标志
int out_of_bounds_judge();//出界判断函数
void circle_judge();//圆环判断函数
void circle_process();//圆环处理函数
void Find_End_Line();//寻找底部结束线函数


/*--------------------------------------------------------------------------
 * 【函数功能】：基于“双边线滑窗积分法”判定整体曲率与弯道方向
 * 【参    数】：end_row - 图像中最远处有效行号（索引值）
 * 【返 回 值】：无 (结果存在全局变量 curve_score 和 curve_dir 中)
 * 【备    注】：彻底解决急弯时单侧边线贴死屏幕导致中线畸变的问题
 *--------------------------------------------------------------------------*/
// void evaluate_track_curvature_from_edges(int end_row);
#endif
