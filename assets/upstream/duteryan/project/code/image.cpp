#include  "zf_common_headfile.hpp"
// static const int straightwidth[IMAGE_HEIGHT] = { 19,19,19,19,19,19,19,19,19,19,19,19,19,19,21,23,25,27,29,31,33,35,36,38,40,42,44,46,48,49,51,53,55,57,59,61,63,64,66,68,70,72,74,76,77,79,81,83,85,87,89,91,93,95,97,99,101,103,105,107 };
// int straightwidth[IMAGE_HEIGHT] = {0,0,0,0,0,11,13,15,17,19,21,23,27,29,31,33,35,37,39,41,43,46,48,50,52,54,57,59,61,63,66,68,70,72,75,77,79,82,84,86,88,90,93,95,97,99,101,103,106,108,110,112,114,116,118,119,121,123,125,127};
// int straightwidth[IMAGE_HEIGHT] = {
//     0, 0, 0, 0, 0,
//     29, 21, 25, 27, 27,
//     30, 32, 35, 37, 39,
//     42, 44, 46, 48, 51,
//     54, 57, 58, 60, 63,
//     65, 67, 70, 72, 74,
//     77, 79, 81, 83, 85,
//     89, 91, 93, 96, 98,
//     100, 102, 105, 108, 110,
//     112, 114, 117, 119, 121,
//     124, 127, 129, 131, 134,
//     136, 138, 140, 141, 142
// };
int straightwidth[IMAGE_HEIGHT] = {
    0,0,0,0,17,19,21,23,24,26,29,30,32,34,36,39,41,43,44,46,
    49,51,52,54,57,59,61,63,64,66,69,71,73,74,76,78,81,82,84,86,
    88,90,93,95,96,98,100,101,104,106,108,110,111,113,115,117,118,121,122,123
};
// 数组长度：60

int image_threshold=0;//大津法阈值
int ostu_filter_flag = 0;//防反光大津法，1-开启
int Search_Stop_Line;//搜线停止行
int min_efect_row_end=0;
int White_Column[IMAGE_WIDTH]={0};
int Longest_White_Column_Left[IMAGE_WIDTH]={0};
int Longest_White_Column_Right[IMAGE_WIDTH]={0};
uint8_t Image[IMAGE_HEIGHT][IMAGE_WIDTH]={0}; //定义裁剪后的图像数组(1D转2D)
uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH];          // 二值化图像数组
/**
 * @brief 将一维数组数据填充到全局二维数组Image中
 * @param src 输入一维数组指针（尺寸必须为 IMG_WIDTH * IMG_HEIGHT）
 * @throw 输入非法时抛出异常
 */
// void Convert_1D_to_2D(const uint8_t* src)//一维数组转二维数组函数
// {
//     // ========== 核心：将一维数组拷贝到全局二维数组 ==========
//     // 逐行拷贝，保证 [y][x] 对应正确
//     for (int y = 0; y < IMAGE_HEIGHT; ++y)
//     {
//         // 一维数组中第y行的起始地址
//         const uint8_t* src_row = src + y * IMAGE_WIDTH;
//         // 直接拷贝到全局数组的第y行
//         memcpy(Image[y], src_row, IMAGE_WIDTH);
//     }
// }
void Convert_1D_to_2D(const uint8_t* src)
{
    // =====================================================================
    // 【极限优化版】一维转二维
    // 原理：C/C++ 中的全局二维数组 Image 在物理内存中是绝对连续的一整块。
    // 动作：彻底抛弃 for 循环，只用一次底层的块传输指令搞定整张图像！
    // 耗时：从原来的 ~1.2ms 降至微秒级 (接近 0ms)。
    // =====================================================================
    
    if (src == nullptr) return; // 增加一步安全校验，防止空指针引发段错误

    // 仅调用一次 memcpy，利用 CPU 的硬件加速/总线突发传输特性瞬间完成
    memcpy(Image, src, IMAGE_WIDTH * IMAGE_HEIGHT);
}
/*--------------------------------------------------------------------------
 * 【函数功能】：大津法最终版（适配智能车竞赛，含防反光优化）
 * 【参    数】：
 *            img[IMAGE_HEIGHT][IMAGE_WIDTH] - 输入的灰度图像数组
 *            start_row       - 感兴趣区域(ROI)起始行（从上往下）
 *            end_row         - 感兴趣区域(ROI)结束行
 *            start_col       - 感兴趣区域(ROI)起始列（从左往右）
 *            end_col         - 感兴趣区域(ROI)结束列
 * 【返 回 值】：uint8_t - 计算得到的最优二值化阈值（0-255）
 * 【备    注】：1. 采样步长为行+2、列+5，降低算力消耗，适配智能车实时性要求
 *              2. 增加防反光逻辑，翻越反光小峰，提升强光/反光场景下阈值稳定性
 *              3. 依赖全局变量ostu_filter_flag：1=启用防反光，0=禁用
 *--------------------------------------------------------------------------*/
uint8_t GetOSTUThreshold(uint8_t img[IMAGE_HEIGHT][IMAGE_WIDTH], uint8_t start_row, uint8_t end_row, uint8_t start_col, uint8_t end_col)
{
    // 局部变量定义
    int threshold = 0;          // 最终输出的最优阈值
    int sum_gray = 0;           // 所有有效像素的灰度值总和（灰度值×像素数）
    int sum_pix_num = 0;        // 所有有效像素的总数量
    int pl_pix_num = 0;         // 阈值分割后，前景（类别1）的像素数量
    int p2_pix_mum = 0;         // 阈值分割后，背景（类别2）的像素数量
    int p1_sum_gray = 0;        // 前景（类别1）的灰度值总和
    float m1 = 0;               // 前景（类别1）的灰度均值
    float m2 = 0;               // 背景（类别2）的灰度均值
    float V = 0;                // 当前阈值下的类间方差
    float variance = 0;         // 最大类间方差（用于筛选最优阈值）
    int i, j, k = 0;            // 循环变量
    int MinValue = 0, MaxValue = 255; // 图像中实际的最小/最大灰度值（排除无像素的灰度级）
    int HistoGram[256] = {0};   // 灰度直方图数组，HistoGram[g]表示灰度值g的像素数量

    /* 第一步：统计ROI区域内的灰度直方图（带采样步长优化） */
    // 遍历ROI区域的像素，i+=2、j+=5为智能车算力优化：隔行隔列采样，减少计算量
    for (i = start_row; i < end_row; i += 2)
    {
        for (j = start_col; j < end_col; j += 5)
        {
            // 统计每个灰度值的像素数量，构建直方图
            ++HistoGram[(int)img[i][j]];
        }
    }

    /* 第二步：确定有效灰度范围（排除无像素的灰度级，减少后续计算量） */
    // 从0开始找第一个有像素的灰度值（最小有效灰度）
    for (MinValue = 0; MinValue < 256 && HistoGram[MinValue] == 0; ++MinValue);
    // 从255开始找最后一个有像素的灰度值（最大有效灰度）
    for (MaxValue = 255; MaxValue > MinValue && HistoGram[MaxValue] == 0; --MaxValue);

    /* 第三步：计算所有有效像素的总灰度和、总像素数 */
    for (k = MinValue; k <= MaxValue; ++k)
    {
        sum_gray += k * HistoGram[k];    // 总灰度和 = 灰度值 × 对应像素数 累加
        sum_pix_num += HistoGram[k];     // 总像素数 = 各灰度值像素数累加
    }

    /* 第四步：遍历所有可能的阈值，计算类间方差，找到最优阈值 */
    // 核心逻辑：大津法通过最大化类间方差来确定最优阈值（类间方差越大，前景/背景区分越明显）
    for (k = MinValue; k <= MaxValue; ++k)
    {
        pl_pix_num += HistoGram[k];              // 累计当前阈值k下的前景像素数
        p2_pix_mum = sum_pix_num - pl_pix_num;   // 背景像素数 = 总像素数 - 前景像素数
        p1_sum_gray += k * HistoGram[k];         // 累计前景总灰度值

        // 计算前景、背景的灰度均值（避免除0，实际pl_pix_num/p2_pix_mum不会为0）
        m1 = (float)p1_sum_gray / pl_pix_num;
        m2 = (float)(sum_gray - p1_sum_gray) / p2_pix_mum;

        // 计算类间方差（简化公式：V = w1*w2*(m1-m2)²，w1=pl_pix_num/sum_pix_num，w2同理）
        // 此处省略除以sum_pix_num²，不影响最大值的判断，减少浮点运算
        V = pl_pix_num * p2_pix_mum * (m1 - m2) * (m1 - m2);

        // 更新最大类间方差和最优阈值
        if (V > variance)
        {
            variance = V;    // 保存当前最大类间方差
            threshold = k;   // 保存当前最优阈值
        }
    }

    /* 第五步：防反光大津计算（智能车竞赛核心优化） */
    // ostu_filter_flag为全局开关，启用时翻越反光造成的小峰值，避免阈值误判
    if (ostu_filter_flag)
    {
        uint8 temp_threshold = threshold; // 保存第一步得到的初始阈值
        // 重置变量，用于二次计算
        int sum_gr = 0;
        sum_gray = 0;
        sum_pix_num = 0;
        pl_pix_num = 0;
        p2_pix_mum = 0;
        p1_sum_gray = 0;
        variance = 0;
        V = 0;

        // 核心逻辑：将初始阈值左侧的所有像素合并到初始阈值位置，消除反光小峰
        for (int i = MinValue; i < temp_threshold; ++i)
            sum_gr += HistoGram[i];       // 累计初始阈值左侧所有灰度的像素数
        HistoGram[temp_threshold] += sum_gr; // 合并到初始阈值位置，抹平左侧小峰

        // 重新计算合并后的总灰度和、总像素数（仅遍历初始阈值到最大灰度值）
        for (k = temp_threshold; k <= MaxValue; ++k)
        {
            sum_gray += k * HistoGram[k];
            sum_pix_num += HistoGram[k];
        }

        // 二次遍历，重新计算最优阈值（排除反光小峰干扰）
        for (k = temp_threshold; k <= MaxValue; ++k)
        {
            pl_pix_num += HistoGram[k];
            p2_pix_mum = sum_pix_num - pl_pix_num;
            p1_sum_gray += k * HistoGram[k];
            m1 = (float)p1_sum_gray / pl_pix_num;
            m2 = (float)(sum_gray - p1_sum_gray) / p2_pix_mum;

            V = pl_pix_num * p2_pix_mum * (m1 - m2) * (m1 - m2);

            if (V > variance)
            {
                variance = V;
                threshold = k; // 更新为防反光后的最优阈值
            }
        }
    }

    // 返回最终计算得到的最优二值化阈值
    return threshold;
}

/*
@将Image数组转为01数组arr便于后续处理
*/
void set_image_twovalues(uint8 value_O)
{
    uint8 temp_value;
    for (uint8 i = 0; i < IMAGE_HEIGHT; i++)
    {
        for (uint8 j = 0; j < IMAGE_WIDTH; j++)
        {
            temp_value = Image[i][j];
            if (temp_value < value_O)
                arr[i][j] = 0;
            else
                arr[i][j] = 255;
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
//  @brief      赛道滤波（四邻域多数表决滤波）
//  @param      arr         - 输入输出：二值化后的赛道图像数组（IMAGE_HEIGHT行×IMAGE_WIDTH列）
//                            注：0代表背景/噪点，255代表赛道有效区域
//              startline   - 滤波起始行（从上往下的行号，函数内反向遍历，实际是从startline到endline）
//              endline     - 滤波结束行（滤波范围：startline → endline，包含两端）
//  @return     void
//  @message    1. 滤波逻辑：基于四邻域（上下左右）像素的多数值，修正当前像素，消除孤立噪点
//              2. 处理规则：
//                 - 若当前像素为0（黑点），且四邻域中≥3个255（白点），则修正为255（消除孤立黑点）
//                 - 若当前像素为255（白点），且四邻域中≥3个0（黑点），则修正为0（消除孤立白点）
//              3. 遍历方向：从startline行反向遍历到endline行，列范围为1~IMAGE_WIDTH-2（避免越界）
//              4. 适配场景：智能车赛道二值图去噪，修复赛道微小断裂，提升线条检测稳定性
//  Sample usage: LvBoSaiDao(gray_img, 60, 20); // 滤波第60行到第20行的赛道图像
//-------------------------------------------------------------------------------------------------------------------
void LvBoSaiDao(uint8 arr[IMAGE_HEIGHT][IMAGE_WIDTH], uint8 startline, uint8 endline)   
{
    // 1. 遍历指定行范围：从startline行反向遍历到endline行（y-- 实现反向）
    //    反向遍历是智能车常用优化：优先处理靠近车头的下方行，提升实时性
    for(uint8 y = startline; y >= endline; y--)
    {
        // 2. 遍历列范围：1 ~ IMAGE_WIDTH-2（避开第0列和最后1列，防止x±1越界）
        for(uint8 x = 1; x <= IMAGE_WIDTH - 2; x++)
        {
            // 情况1：当前像素为0（黑点，疑似孤立噪点）
            if(arr[y][x] == 0)
            {
                uint8 jj = 0; // 统计四邻域中255（白点）的数量
                // 检查右侧邻域像素
                if(arr[y][x+1] == 255) { jj += 1; }
                // 检查左侧邻域像素
                if(arr[y][x-1] == 255) { jj += 1; }
                // 检查下侧邻域像素
                if(arr[y+1][x] == 255) { jj += 1; }
                // 检查上侧邻域像素
                if(arr[y-1][x] == 255) { jj += 1; }
                
                // 若四邻域中≥3个白点，判定为孤立黑点，修正为白点（修复赛道断裂）
                if(jj >= 3) { arr[y][x] = 255; }
            }
            // 情况2：当前像素为255（白点，疑似孤立噪点）
            else if(arr[y][x] == 255)
            {
                uint8 jj = 0; // 统计四邻域中0（黑点）的数量
                // 检查右侧邻域像素
                if(arr[y][x+1] == 0) { jj += 1; }
                // 检查左侧邻域像素
                if(arr[y][x-1] == 0) { jj += 1; }
                // 检查下侧邻域像素
                if(arr[y+1][x] == 0) { jj += 1; }
                // 检查上侧邻域像素
                if(arr[y-1][x] == 0) { jj += 1; }
                
                // 若四邻域中≥3个黑点，判定为孤立白点，修正为黑点（消除赛道外噪点）
                if(jj >= 3) { arr[y][x] = 0; }
            }
        }
    }
}
//--------------------------------------------------------------------------
// @brief      给图像绘制黑色边框（灰度值0），为后续八邻域操作做越界防护
// @param      image[][IMAGE_WIDTH] - 输入输出：待处理的灰度/二值图像数组（UVC_HEIGHT行×UVC_WIDTH列）
// @return     void
// @since      v1.0
// @message    1. 边框规则：
//               - 左右侧：每一行的第0、1列 和 最后1、2列 设为0（黑色）
//               - 上侧：前2行（第0、1行）的所有列设为0（黑色）
//               - 下侧未处理：因智能车赛道检测通常只关注图像上/中部，下侧无需额外防护
//             2. 设计目的：为八邻域操作预留安全区，避免访问image[i±1][j±1]时数组越界
//             3. 适配场景：智能车图像预处理，配合八邻域滤波/形态学操作使用
// Sample usage: image_draw_rectan(Image_use); // 为Image_use数组绘制黑色边框
//--------------------------------------------------------------------------
void image_draw_rectan(uint8 image[IMAGE_HEIGHT][IMAGE_WIDTH])
{
    uint8 i = 0; // 循环变量，用于遍历行/列

    /* 第一步：处理图像左右两侧的边框（每一行的边缘列设为0） */
    // 遍历图像所有行（0 ~ IMAGE_HEIGHT-1）
    for (i = 0; i < IMAGE_HEIGHT; i++)
    {
        // 左侧边框：第0列、第1列设为0（黑色）
        image[i][0] = 0;
        image[i][1] = 0;
        // 右侧边框：最后1列（UVC_WIDTH-1）、倒数第2列（UVC_WIDTH-2）设为0（黑色）
        image[i][IMAGE_WIDTH - 1] = 0;
        image[i][IMAGE_WIDTH - 2] = 0;
    }

    /* 第二步：处理图像上侧的边框（前2行的所有列设为0） */
    // 遍历图像所有列（0 ~ IMAGE_WIDTH-1）
    for (i = 0; i < IMAGE_WIDTH; i++)
    {
        // 上侧边框：第0行、第1行的所有列设为0（黑色）
        image[0][i] = 0;
        image[1][i] = 0;
    }

    // 说明：下侧未做边框处理，原因是智能车赛道图像中，下侧通常是靠近车头的有效区域，
    // 且八邻域操作一般不会遍历到最后几行，无需额外防护，节省算力
}
/*--------------------------------------------------------------------
 @函数：find_longest_col
 @功能：寻找最长白列（统计每列连续白色像素长度，确定八邻域搜线的停止行）
 @参数：无
 @返回值：Search_Stop_Line - 左右侧最长白列的最大值（从下往上数的白列长度）
 @备注：1. 仅统计 left_start~right_start 列范围（赛道初始边界内），避免无效列干扰
        2. 白列定义：从图像最下方（IMAGE_HEIGHT-1）往上数，连续的白色像素（非Black=0）长度
        3. 输出全局变量：
           - White_Column[q]：第q列的白列长度
           - Longest_White_Column_Left[2]：左侧最长白列[0]=长度，[1]=列坐标
           - Longest_White_Column_Right[2]：右侧最长白列[0]=长度，[1]=列坐标
           - Search_Stop_Line：搜线停止行（最长白列的最大值）
 --------------------------------------------------------------------*/
int find_longest_col()
{
    // 第一步：初始化每列的白列长度为0（避免历史数据干扰）
    for(int q=0;q<IMAGE_WIDTH;q++)
        White_Column[q]=0;
    
    int i;  // 行索引（遍历图像行）
    int j;  // 列索引（遍历图像列）
    // 第二步：获取图像最下方一行（UVC_HEIGHT-1）的左右边界初始列
    // side=0：找左边界初始列；side=1：找右边界初始列
    int left_start=edge_point_ornot(IMAGE_HEIGHT - 1, 0);
    int right_start=edge_point_ornot(IMAGE_HEIGHT - 1, 1);
    
    // 第三步：统计指定列范围（left_start~right_start）内每一列的白列长度
    // 遍历列：仅遍历初始边界内的列（赛道有效列，避免无效列如背景列干扰）
    for (j = left_start; j <= right_start; j++)
    {
        // 遍历行：从图像最下方（UVC_HEIGHT-1）往上数，直到第3行（避开画的黑框）
        for (i = IMAGE_HEIGHT - 1; i > 3; i--)
        {
            // 遇到黑色像素（Black=0），停止统计（白列到此处结束）
            if (arr[i][j] == Black)
                break;
            // 否则是白色像素，该列的白列长度+1
            else
                White_Column[j]++;
        }
    }
    
    // 第四步：从左到右遍历，找左侧最长的白列（记录长度+列坐标）
    Longest_White_Column_Left[0] = 0;  // 初始化最长长度为0
    for (i = left_start; i <= right_start; i++)
    {
        // 找到更长的白列，更新长度和列坐标
        if (Longest_White_Column_Left[0] < White_Column[i])
        {
            Longest_White_Column_Left[0] = White_Column[i]; // 最长白列长度
            Longest_White_Column_Left[1] = i;              // 最长白列的列坐标
        }
    }
    
    // 第五步：从右到左遍历，找右侧最长的白列（记录长度+列坐标）
    Longest_White_Column_Right[0] = 0; // 初始化最长长度为0
    for(i=right_start;i>=left_start;i--)
    {
        // 找到更长的白列，更新长度和列坐标
        if (Longest_White_Column_Right[0] < White_Column[i])
        {
            Longest_White_Column_Right[0] = White_Column[i]; // 最长白列长度
            Longest_White_Column_Right[1] = i;              // 最长白列的列坐标
        }
    }
    
    // 第六步：取左右侧最长白列的最大值，作为搜线停止行（Search_Stop_Line）
    Search_Stop_Line = Longest_White_Column_Left[0]>Longest_White_Column_Right[0]?Longest_White_Column_Left[0]:Longest_White_Column_Right[0];
    
    // 注释掉的代码：原本用于计算封顶行（End_Line），后续被其他函数替代
    /*End_Line = 80 - Search_Stop_Line;
    if(Search_Stop_Line == Longest_White_Column_Left[0])
        End_Line_col=Longest_White_Column_Left[1];
    if(Search_Stop_Line == Longest_White_Column_Right[0])
        End_Line_col=Longest_White_Column_Right[1];*/
    
    // 返回搜线停止行（八邻域搜线时，超过这个行就停止，避免搜出赛道外）
    return Search_Stop_Line;
}
/*--------------------------------------------------------------------------
 * 【函数功能】：判断指定行是否存在左/右赛道边界点（优先处理最下方一行）
 * 【参    数】：
 *            row - 搜线的目标行（通常传UVC_HEIGHT-1，即图像最下方一行）
 *            side - 边界选择标志：0=查找左边界，1=查找右边界
 * 【返 回 值】：
 *            成功 - 边界点的列坐标；失败 - -1（未找到有效边界点）
 * 【备    注】：
 *            1. 适配斑马线场景：通过多列间隔采样（-15/-10/-5/+5/+10/+15）避免斑马线干扰
 *            2. 越界防护：通过三元运算符（col-15>=0 ? col-15 : col-1）避免列坐标越界
 *            3. 核心逻辑：左边界是“黑→白”跳变，右边界是“白→黑”跳变
 *--------------------------------------------------------------------------*/
int edge_point_ornot(uint8 row, uint8 side)
{
    // 第一步：多位置采样，找到该行中大概率的赛道区域起点（first_point）
    // 初始采样点：图像中线（UVC_WIDTH/2），赛道中心最可能的位置
    int first_point = IMAGE_WIDTH / 2;
    
    // 采样逻辑：若当前点是黑色（背景），则依次尝试其他采样点，直到找到白色（赛道）点
    if (arr[row][first_point] == Black) // 中线是黑的→试左1/3处
    {
        first_point = (int)IMAGE_WIDTH / 3;
        if (arr[row][first_point] == Black) // 左1/3是黑的→试右2/3处
        {
            first_point = (int)IMAGE_WIDTH * 2 / 3;
            if (arr[row][first_point] == Black) // 右2/3是黑的→试左1/4处
            {
                first_point = (int)IMAGE_WIDTH / 4;
                if (arr[row][first_point] == Black) // 左1/4是黑的→试右3/4处
                {
                    first_point = (int)IMAGE_WIDTH * 3 / 4;
                    if (arr[row][first_point] == Black) // 右3/4是黑的→试左1/8处
                    {
                        first_point = (int)IMAGE_WIDTH / 8;
                        if (arr[row][first_point] == Black) // 左1/8是黑的→试右7/8处
                        {
                            first_point = (int)IMAGE_WIDTH * 7 / 8;
                        }
                    }
                }
            }
        }
    }

    // 第二步：查找左边界点（side=0）
    if (side == 0)
    {
        // 从first_point向左遍历列，找“黑→白”的跳变（左边界特征：赛道外黑，赛道内白）
        for (uint8 col = first_point; col > 0; col--) 
        {
            // 左边界判定条件（抗斑马线干扰，多列验证）：
            // 左侧连续多列（col-15/-10/-5/-1）是黑 → 右侧连续多列（col+1/+2/+5/+12）是白
            // 三元运算符：避免col-15<0导致数组越界，越界则取col-1
            if ((arr[row][col - 15 >= 0 ? col - 15 : col - 1]) == Black 
                && (arr[row][col - 10 >= 0 ? col - 10 : col - 1]) == Black 
                && (arr[row][col - 5 >= 0 ? col - 5 : col - 1]) == Black 
                && (arr[row][col - 1]) == Black 
                && (arr[row][col]) == Black 
                && (arr[row][col + 1]) == White 
                && (arr[row][col + 2]) == White 
                && (arr[row][col + 5]) == White 
                && (arr[row][col + 12]) == White)
            {
                return col; // 找到左边界，返回列坐标
            }
        }
        // 容错判定：若图像最左侧是白+黑组合，判定为无有效左边界，返回-1
        if ((arr[row][3]) == White && (arr[row][2]) == White && (arr[row][1]) == Black && (arr[row][0]) == Black)
        {
            return -1;
        }
    }

    // 第三步：查找右边界点（side=1）
    if (side == 1)
    {
        // 从first_point向右遍历列，找“白→黑”的跳变（右边界特征：赛道内白，赛道外黑）
        for (uint8 col = first_point; col < IMAGE_WIDTH - 1; col++)
        {
            // 右边界判定条件（抗斑马线干扰，多列验证）：
            // 左侧连续多列（col-12/-5/-2/-1）是白 → 右侧连续多列（col+1/+5/+10/+15）是黑
            // 三元运算符：避免col+15>IMAGE_WIDTH-1导致数组越界，越界则取col+1
            if ((arr[row][col - 12]) == White 
                && (arr[row][col - 5]) == White 
                && (arr[row][col - 2]) == White 
                && (arr[row][col - 1]) == White 
                && (arr[row][col]) == Black 
                && (arr[row][col + 1]) == Black 
                && (arr[row][col + 5 <= IMAGE_WIDTH - 1 ? col + 5 : col + 1]) == Black 
                && (arr[row][col + 10 <= IMAGE_WIDTH - 1 ? col + 10 : col + 1]) == Black 
                && (arr[row][col + 15 <= IMAGE_WIDTH - 1 ? col + 15 : col + 1]) == Black)
            {
                return col; // 找到右边界，返回列坐标
            }
        }
        // 容错判定：若图像最右侧是白+黑组合，返回右边界列坐标（UVC_WIDTH-2）
        if ((arr[row][IMAGE_WIDTH - 4]) == White && (arr[row][IMAGE_WIDTH - 3]) == White && (arr[row][IMAGE_WIDTH - 2]) == Black && (arr[row][IMAGE_WIDTH - 1]) == Black)
        {
            return IMAGE_WIDTH - 2;
        }
    }

    // 第四步：未找到有效边界点，返回-1
    return -1;
}
/*--------------------------------------------------------------------------
 * 【函数功能】：八邻域算法提取赛道左右边界点（核心爬线函数）
 * 【参数】：无（依赖全局变量：arr[IMAGE_HEIGHT][IMAGE_WIDTH] 二值化图像、End_Line 搜线上限行等）
 * 【返回值】：无
 * 【核心逻辑】：
 *  1. 先获取图像最下方一行的左右边界初始点；
 *  2. 从初始点开始，按7个方向的黑白跳变规则逐点向上/周边搜索赛道边界；
 *  3. 记录所有有效边界点到L_edge/R_edge结构体数组；
 *  4. 过滤边界点，提取有效边界的起止行/索引（剔除贴图像边缘的无效点）；
 * 【关键全局变量说明】：
 *  - L_edge/R_edge：存储左右边界点的结构体数组（成员：row行坐标、col列坐标、flag有效标志）；
 *  - L_edge_count/R_edge_count：左右边界点的有效数量；
 *  - dire_left/dire_right：记录上一个边界点的搜索方向（避免反向搜线）；
 *  - End_Line：搜线停止行（由find_longest_col()计算，超出则停止搜线）；
 *  - left_effect_row_start/end：左边界有效点的起止行；
 *  - right_effect_row_start/end：右边界有效点的起止行；
 * 【搜线方向规则】：
 *  方向编码（0~7）：0=正上、2=右下、3=左下、4=正左、5=正右、6=右上、7=左上；
 *  核心跳变：左边界找“黑(赛道外)→白(赛道内)”，右边界找“白(赛道内)→黑(赛道外)”；
 --------------------------------------------------------------------------*/
struct LEFT_EDGE L_edge[Max_Point]; //左边界结构体
struct RIGHT_EDGE R_edge[Max_Point]; //右边界结构体
uint8 L_edge_count = 0, R_edge_count = 0; //左右边点的个数
uint8 dire_left, dire_right; //记录上一个点的相对位置
uint8 L_search_amount = 180, R_search_amount = 180; //左右边界搜点时最多允许的点
int left = 0;
int right = 0;
int End_Line = 0;//结束行
int left_effect_row_start = 0;
int left_effect_index_start = 0;
int right_effect_index_start = 0;
int right_effect_row_start = 0;
int left_effect_row_end = 0;
int right_effect_row_end = 0;
int left_effect_index_end = 0;
int right_effect_index_end = 0;
void search_neighborhood()
{
    // ===================== 第一步：初始化边界初始点 =====================
    // 获取图像最下方一行（UVC_HEIGHT-1，靠近车头）的左边界初始列坐标
    left = edge_point_ornot(IMAGE_HEIGHT - 1, 0); 
    // 获取图像最下方一行的右边界初始列坐标
    right = edge_point_ornot(IMAGE_HEIGHT - 1, 1); 
    L_edge_count = 0;  // 左边界有效点数量清零（统计实际搜到的边界点个数）
    R_edge_count = 0;  // 右边界有效点数量清零
    // 计算搜线上限行：End_Line = 图像高度 - 最长白列长度（超出该行列停止搜线）
    End_Line = IMAGE_HEIGHT - find_longest_col();

    // ===================== 第二步：八邻域搜索左边界 =====================
    // 若左边界初始点有效（非-1），开始左边界爬线
    if (left) 
    {
        // 重新确认左边界初始点（避免首次赋值异常）
        left = edge_point_ornot(IMAGE_HEIGHT - 1, 0);
        // 初始化第0个左边界点（图像最下方一行）
        L_edge[0].row = IMAGE_HEIGHT - 1;                // 初始点行坐标：最下方一行
        L_edge[0].col = edge_point_ornot(IMAGE_HEIGHT - 1, 0); // 初始点列坐标：左边界初始点
        L_edge[0].flag = 1;                           // 标记该点有效（1=有效，0=无效）
        
        // 初始化当前搜线的行/列坐标（从初始点开始）
        int16 curr_row = IMAGE_HEIGHT - 1;              // 当前行坐标（初始为最下方一行）
        int16 curr_col = edge_point_ornot(IMAGE_HEIGHT - 1, 0); // 当前列坐标
        dire_left = 0; // 初始化左边界上一个搜线方向（0=正上，避免反向）

        // 循环搜线：最多搜索L_search_amount个点（避免死循环，嵌入式需限制次数）
        for (int i = 1; i < L_search_amount; i++) 
        {
            // ===================== 越界判定：满足任一条件则停止搜线 =====================
            // 1. curr_row+1 < 2：行坐标过顶（避开图像上侧2像素黑框）；
            // 2. curr_row > IMAGE_HEIGHT - 1：行坐标超出图像下边界；
            // 3. curr_row <= End_Line：行坐标超出搜线上限（无赛道区域）；
            if (curr_row + 1 < 2 || curr_row > IMAGE_HEIGHT - 1 || curr_row <= End_Line)
                break;

            // ===================== 左边界7方向搜线规则（黑→白跳变） =====================
            // 方向1：左上（7）- 需满足：上一方向≠2（避免反向）、左上像素黑、当前点右侧白
            if (dire_left != 2 && arr[curr_row - 1][curr_col - 1] == Black && arr[curr_row - 1][curr_col] == White) 
            {
                curr_row = curr_row - 1;  // 行坐标上移1行
                curr_col = curr_col - 1;  // 列坐标左移1列
                L_edge_count++;           // 左边界点数量+1
                dire_left = 7;            // 记录当前搜线方向为“左上”
                L_edge[i].row = curr_row; // 存储当前点行坐标
                L_edge[i].col = curr_col; // 存储当前点列坐标
                L_edge[i].flag = 1;       // 标记该点有效
            }
            // 方向2：右上（6）- 需满足：上一方向≠3、右上像素黑、当前点下侧白
            else if (dire_left != 3 && arr[curr_row - 1][curr_col + 1] == Black && arr[curr_row][curr_col + 1] == White) 
            {
                curr_row = curr_row - 1;
                curr_col = curr_col + 1;
                L_edge_count++;
                dire_left = 6;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 方向3：正上（0）- 无方向限制、正上像素黑、正上右侧白
            else if (arr[curr_row - 1][curr_col] == Black && arr[curr_row - 1][curr_col + 1] == White)
            {
                curr_row = curr_row - 1;
                L_edge_count++;
                dire_left = 0;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 方向4：正左（4）- 需满足：上一方向≠5、正左像素黑、左上侧白
            else if (dire_left != 5 && arr[curr_row][curr_col - 1] == Black && arr[curr_row - 1][curr_col - 1] == White) 
            {
                curr_col = curr_col - 1;
                L_edge_count++;
                dire_left = 4;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 方向5：正右（5）- 需满足：上一方向≠4、正右像素黑、左下侧白
            else if (dire_left != 4 && arr[curr_row][curr_col + 1] == Black && arr[curr_row + 1][curr_col + 1] == White) 
            {
                curr_col = curr_col + 1;
                L_edge_count++;
                dire_left = 5;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 方向6：左下（3）- 需满足：上一方向≠6、左下像素黑、上侧白
            else if (dire_left != 6 && arr[curr_row + 1][curr_col - 1] == Black && arr[curr_row][curr_col - 1] == White) 
            {
                curr_row = curr_row + 1;
                curr_col = curr_col - 1;
                L_edge_count++;
                dire_left = 3;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 方向7：右下（2）- 需满足：上一方向≠7、右下像素黑、左侧白
            else if (dire_left != 7 && arr[curr_row + 1][curr_col + 1] == Black && arr[curr_row + 1][curr_col] == White) 
            {
                curr_row = curr_row + 1;
                curr_col = curr_col + 1;
                L_edge_count++;
                dire_left = 2;
                L_edge[i].row = curr_row;
                L_edge[i].col = curr_col;
                L_edge[i].flag = 1;
            }
            // 所有方向都不满足：无有效边界点，停止左边界搜线
            else
                break;
        }
    }

    // ===================== 第三步：八邻域搜索右边界 =====================
    // 若右边界初始点有效（非-1），开始右边界爬线
    if (right) 
    {
        // 重新确认右边界初始点（避免首次赋值异常）
        right = edge_point_ornot(IMAGE_HEIGHT - 1, 1);
        // 初始化第0个右边界点（图像最下方一行）
        R_edge[0].row = IMAGE_HEIGHT - 1;                // 初始点行坐标：最下方一行
        R_edge[0].col = edge_point_ornot(IMAGE_HEIGHT - 1, 1); // 初始点列坐标：右边界初始点
        R_edge[0].flag = 1;                           // 标记该点有效
        
        // 初始化当前搜线的行/列坐标（从初始点开始）
        int16 curr_row = IMAGE_HEIGHT - 1;              // 当前行坐标
        int16 curr_col = edge_point_ornot(IMAGE_HEIGHT - 1, 1); // 当前列坐标
        dire_right = 0; // 初始化右边界上一个搜线方向

        // 循环搜线：最多搜索R_search_amount个点
        for (int i = 1; i < R_search_amount; i++)
        {
            // ===================== 越界判定：满足任一条件则停止搜线 =====================
            // 1. curr_row < 2：行坐标过顶；2. curr_row>IMAGE_HEIGHT-1：行坐标超下边界；3. curr_row <= End_Line：超搜线上限
            if (curr_row < 2 || curr_row > IMAGE_HEIGHT - 1 || curr_row <= End_Line) 
                break;

            // ===================== 右边界7方向搜线规则（白→黑跳变） =====================
            // 方向1：右上（6）- 列不越界、上一方向≠3、右上像素黑、左侧白
            if (curr_col < IMAGE_WIDTH && dire_right != 3 && arr[curr_row - 1][curr_col + 1] == Black && arr[curr_row - 1][curr_col] == White) 
            {
                curr_row = curr_row - 1;
                curr_col = curr_col + 1;
                R_edge_count++;
                dire_right = 6;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向2：左上（7）- 上一方向≠2、左上像素黑、下侧白
            else if (dire_right != 2 && arr[curr_row - 1][curr_col - 1] == Black && arr[curr_row][curr_col - 1] == White) 
            {
                curr_row = curr_row - 1;
                curr_col = curr_col - 1;
                R_edge_count++;
                dire_right = 7;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向3：正上（0）- 正上像素黑、左侧白
            else if (arr[curr_row - 1][curr_col] == Black && arr[curr_row - 1][curr_col - 1] == White)
            {
                curr_row = curr_row - 1;
                R_edge_count++;
                dire_right = 0;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向4：正右（5）- 上一方向≠4、正右像素黑、上侧白
            else if (dire_right != 4 && arr[curr_row][curr_col + 1] == Black && arr[curr_row - 1][curr_col + 1] == White) 
            {
                curr_col = curr_col + 1;
                R_edge_count++;
                dire_right = 5;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向5：正左（4）- 上一方向≠5、正左像素黑、下侧白
            else if (dire_right != 5 && arr[curr_row][curr_col - 1] == Black && arr[curr_row + 1][curr_col - 1] == White) 
            {
                curr_col = curr_col - 1;
                R_edge_count++;
                dire_right = 4;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向6：左下（3）- 上一方向≠6、左下像素黑、右侧白
            else if (dire_right != 6 && arr[curr_row + 1][curr_col - 1] == Black && arr[curr_row + 1][curr_col] == White) 
            {
                curr_row = curr_row + 1;
                curr_col = curr_col - 1;
                R_edge_count++;
                dire_right = 3;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 方向7：右下（2）- 上一方向≠7、右下像素黑、上侧白
            else if (dire_right != 7 && arr[curr_row + 1][curr_col + 1] == Black && arr[curr_row][curr_col + 1] == White) 
            {
                curr_row = curr_row + 1;
                curr_col = curr_col + 1;
                R_edge_count++;
                dire_right = 2;
                R_edge[i].row = curr_row;
                R_edge[i].col = curr_col;
                R_edge[i].flag = 1;
            }
            // 所有方向都不满足：无有效边界点，停止右边界搜线
            else
                break;
        }
    }

    // ===================== 第四步：过滤边界点，提取有效边界的起止行/索引 =====================
    // 初始化左边界有效点的起始行/索引（0=未初始化）
    left_effect_row_start = 0;
    left_effect_index_start = 0;
    // 初始化右边界有效点的起始行/索引
    right_effect_index_start = 0;
    right_effect_row_start = 0;

    // -------------------- 处理左边界有效点：剔除贴图像左侧边缘（col≤1）的无效点 --------------------
    for (uint8 i = 0; i < L_edge_count; i++)
    {
        // 遍历到最后一个左边界点：直接记为有效结束行/索引
        if (i == L_edge_count - 1)
        {
            left_effect_row_end = L_edge[i].row;      // 左边界有效结束行
            left_effect_index_end = i;                // 左边界有效结束索引
            break;
        }
        // 场景1：首次找到“从贴边缘→离开边缘”的点：记为有效起始行/索引
        if (left_effect_row_start == 0 && L_edge[i].col <= 1 && L_edge[i + 1].col > 1)
        {
            left_effect_row_start = L_edge[i + 1].row;    // 左边界有效起始行
            left_effect_index_start = i + 1;              // 左边界有效起始索引
        }
        // 场景2：初始点就不贴边缘：直接以初始点为有效起始
        else if (left_effect_row_start == 0 && L_edge[0].col > 1)
        {
            left_effect_row_start = L_edge[0].row;        // 左边界有效起始行
            left_effect_index_start = 0;                  // 左边界有效起始索引
        }
        // 场景3：找到“从离开边缘→贴边缘”的点：记为有效结束行/索引
        if (L_edge[i].col > 1 && L_edge[i + 1].col <= 1)
        {
            left_effect_row_end = L_edge[i].row;          // 左边界有效结束行
            left_effect_index_end = i;                    // 左边界有效结束索引
            break;
        }
    }

    // -------------------- 处理右边界有效点：剔除贴图像右侧边缘（col≥UVC_WIDTH-2）的无效点 --------------------
    for (uint8 i = 0; i < R_edge_count; i++)
    {
        // 遍历到最后一个右边界点：直接记为有效结束行/索引
        if (i == R_edge_count - 1)
        {
            right_effect_row_end = R_edge[i].row;     // 右边界有效结束行
            right_effect_index_end = i;               // 右边界有效结束索引
            break;
        }
        // 场景1：首次找到“从贴边缘→离开边缘”的点：记为有效起始行/索引
        if (right_effect_row_start == 0 && R_edge[i].col >= IMAGE_WIDTH - 2 && R_edge[i + 1].col < IMAGE_WIDTH - 2)
        {
            right_effect_index_start = i + 1;         // 右边界有效起始索引
            right_effect_row_start = R_edge[i + 1].row; // 右边界有效起始行
        }
        // 场景2：初始点就不贴边缘：直接以初始点为有效起始
        else if (right_effect_row_start == 0 && R_edge[0].col < IMAGE_WIDTH - 2)
        {
            right_effect_index_start = 0;             // 右边界有效起始索引
            right_effect_row_start = R_edge[0].row;   // 右边界有效起始行
        }
        // 场景3：找到“从离开边缘→贴边缘”的点：记为有效结束行/索引
        if (R_edge[i].col < IMAGE_WIDTH - 2 && R_edge[i + 1].col >= IMAGE_WIDTH - 2)
        {
            right_effect_row_end = R_edge[i].row;     // 右边界有效结束行
            right_effect_index_end = i;               // 右边界有效结束索引
            break;
        }
    }
}
//扫出赛道边线
/*--------------------------------------------------------------------------
 * 【函数功能】：标准化赛道边界坐标（爬线后数据整理）
 * 【核心目标】：
 *  1. 为图像每一行匹配对应的左右边界列坐标；
 *  2. 修正越界的列坐标，确保落在合法范围；
 *  3. 统一左右边界的最高有效行，确定赛道整体最高行top_line；
 * 【输入依赖】：
 *  - L_edge/R_edge：八邻域爬线得到的左右边界点数组（存储行/列坐标）；
 * 【输出变量】：
 *  - current_left_col[i]：第i行的左边界列坐标（标准化后）；
 *  - current_right_col[i]：第i行的右边界列坐标（标准化后）；
 *  - left_top/right_top：左右边界的最高有效行（统一后的值）；
 *  - top_line：赛道整体的最高有效行（后续路径规划的上限）；
 * 【关键规则】：
 *  1. 只处理行范围：3 < i < IMAGE_HEIGHT-1（避开图像上下侧黑框）；
 *  2. 列坐标合法范围：1 ≤ col ≤ IMAGE_WIDTH-2（避开图像左右边缘）；
 *  3. 最高有效行统一：取左右边界最高行的最大值，确保两者一致；
 --------------------------------------------------------------------------*/
uint8 current_left_col[IMAGE_HEIGHT];
uint8 current_right_col[IMAGE_HEIGHT];
uint8 current_left_row[IMAGE_HEIGHT];
uint8 current_right_row[IMAGE_HEIGHT];
int left_top=0;
int right_top=0;
int left_top_old=0;
int right_top_old=0;
int top_line;
void current_wide()
{
    // 第一步：遍历图像每一行（从最下方UVC_HEIGHT-1到第4行，避开上侧3行黑框）
    for(int i=IMAGE_HEIGHT-1;i>3;i--)
    {
        // -------------------- 处理左边界：匹配第i行的左边界列坐标 --------------------
        // 遍历L_edge数组（最多180个点），找行坐标等于i的左边界点
        for(int t=0;t<180;t++)
        {
            if(L_edge[t].row==i)  // 找到第i行对应的左边界点
            {
                current_left_col[i] = L_edge[t].col;  // 存储第i行左边界列坐标
                current_left_row[i] = i;              // 存储第i行左边界行坐标（冗余，行号就是i）
                left_top_old = i;                     // 更新左边界当前最高有效行
                break;  // 找到后立即退出循环，避免重复匹配
            }
        }
        // 左边界列坐标越界修正：确保落在1 ~ IMAGE_WIDTH-2范围内
        if(current_left_col[i]<1)  // 列坐标小于1（贴左边缘），修正为1
        {
            current_left_col[i] = 1;
        }
        else if(current_left_col[i]>IMAGE_WIDTH - 2)  // 列坐标大于UVC_WIDTH-2（贴右边缘），修正为UVC_WIDTH-2
        {
            current_left_col[i] = IMAGE_WIDTH - 2;
        }

        // -------------------- 处理右边界：匹配第i行的右边界列坐标 --------------------
        // 遍历R_edge数组（最多180个点），找行坐标等于i的右边界点
        for(int q=0;q<180;q++)//140（注释为历史值，实际遍历180个点）
        {
            if(R_edge[q].row==i)  // 找到第i行对应的右边界点
            {
                current_right_col[i] = R_edge[q].col;  // 存储第i行右边界列坐标
                current_right_row[i] = i;              // 存储第i行右边界行坐标（冗余）
                right_top_old = i;                     // 更新右边界当前最高有效行
                break;  // 找到后立即退出循环
            }
        }
        // 右边界列坐标越界修正：确保落在1 ~ IMAGE_WIDTH-2范围内
        if(current_right_col[i]<1)  // 列坐标小于1，修正为1
        {
            current_right_col[i] = 1;
        }
        else if(current_right_col[i]>IMAGE_WIDTH - 2)  // 列坐标大于UVC_WIDTH-2，修正为UVC_WIDTH-2
        {
            current_right_col[i] = IMAGE_WIDTH - 2;
        }
    }

    // 第二步：统一左右边界的最高有效行（left_top/right_top）
    right_top = right_top_old;  // 先赋值为右边界原始最高行
    left_top = left_top_old;    // 先赋值为左边界原始最高行
    
    // 若左右边界的最高行不一致，统一为两者的最大值（确保赛道整体最高行一致）
    if(left_top_old != right_top_old)
    {
        // 取左右最高行的最大值，赋值给left_top和right_top
        left_top = left_top_old < right_top_old ? right_top_old : left_top_old;
        right_top = left_top_old < right_top_old ? right_top_old : left_top_old;
    }
    
    // 第三步：确定赛道整体的最高有效行（top_line）
    top_line = left_top;  // top_line等于统一后的最高有效行（后续路径规划只处理到该行为止）
}
// 绕行出界反打判定:
// 按底部连续10行赛道宽度判断,宽度整体变窄才置位,避免单行噪声导致BNF乱跳。
int bottom_narrow_flag = 0;
static int bottom_narrow_clear_cnt = 0;
void bottom_width_judge()
{
    if (!((card_evade_phase == CARD_EVADE_MERGE ||
           card_evade_phase == CARD_EVADE_SETTLE) &&
          (turn_action == 0 || turn_action == 2)))
    {
        bottom_narrow_flag = 0;
        bottom_narrow_clear_cnt = 0;
        return;
    }

    const int check_rows = 10;
    const int narrow_width = 50;
    int narrow_count = 0;
    int valid_count = 0;
    int width_sum = 0;
    int max_width = 0;

    const int start_row = IMAGE_HEIGHT - 1;
    const int end_row = IMAGE_HEIGHT - check_rows;

    for (int i = start_row; i >= end_row; i--)
    {
        int width = current_right_col[i] - current_left_col[i];
        if (width < 0)
        {
            width = 0;
        }

        width_sum += width;
        valid_count++;
        if (width > max_width)
        {
            max_width = width;
        }

        if (width < narrow_width)
        {
            narrow_count++;
        }
    }

    int avg_width = (valid_count > 0) ? (width_sum / valid_count) : IMAGE_WIDTH;
    bool danger = (narrow_count >= 8 && avg_width < narrow_width && max_width < narrow_width + 12);

    if (danger)
    {
        bottom_narrow_flag = 1;
        bottom_narrow_clear_cnt = 0;
    }
    else if (bottom_narrow_flag == 1 && bottom_narrow_clear_cnt < 2)
    {
        // 保持2帧,避免边界刚好抖回一帧就立刻解除反打。
        bottom_narrow_clear_cnt++;
        bottom_narrow_flag = 1;
    }
    else
    {
        bottom_narrow_flag = 0;
        bottom_narrow_clear_cnt = 0;
    }
}
//寻找左右边界有效行
/*--------------------------------------------------------------------------
 * 【函数功能】：统计左右边界丢线行数，计算有效赛道行数
 * 【核心逻辑】：
 *  1. 判定近景（最下方一行）是否初始丢线（边界贴边缘）；
 *  2. 分两类统计丢线行数：
 *     - 无效丢线：近景连续贴边缘的行数（无实际赛道意义）；
 *     - 有效丢线：有效赛道区域内贴边缘的行数（真实丢线）；
 *  3. 计算左右边界的总丢线数和有效行数；
 * 【输入依赖】：
 *  - current_left_col/current_right_col：每行标准化后的左右边界列坐标；
 *  - left_top_old/right_top_old：爬线得到的左右边界原始最高行；
 * 【输出变量】：
 *  - diuxianshu[0/1]：左/右边界总丢线行数（无效+有效）；
 *  - L_all_effect_line/R_all_effect_line：左/右边界有效赛道行数；
 *  - 各类丢线标记/计数：用于后续丢线补偿逻辑；
 * 【关键规则】：
 *  1. 只处理行范围 >15（避开远景无效区域）；
 *  2. 丢线判定：左边界列=1、右边界列=IMAGE_WIDTH-2（贴修正后的边缘）；
 --------------------------------------------------------------------------*/
int no_effect_lost_right=0;
int no_effect_lost_left=0;
int effect_lost_right=0;
int effect_lost_left=0;
int left_effect_row;
int right_effect_row;
int right_line_lost_first_Flag=0;
int left_line_lost_first_Flag=0;
int L_all_effect_line;
int R_all_effect_line;
static uint8 diuxianshu[2];
void effect_line()
{
    // ===================== 第一步：初始化所有统计变量 =====================
    right_effect_row=0;          // 右边界有效行（最终未使用，预留变量）
    left_effect_row=0;           // 左边界有效行（最终未使用，预留变量）
    right_line_lost_first_Flag=0;// 右边界近景初始丢线标志：1=丢线，0=正常
    left_line_lost_first_Flag=0; // 左边界近景初始丢线标志：1=丢线，0=正常
    no_effect_lost_right=0;      // 右边界无效丢线行数（近景连续贴边缘）
    no_effect_lost_left=0;       // 左边界无效丢线行数（近景连续贴边缘）
    effect_lost_right=0;         // 右边界有效丢线行数（有效赛道区贴边缘）
    effect_lost_left=0;          // 左边界有效丢线行数（有效赛道区贴边缘）

    // ===================== 第二步：判定近景初始丢线（最下方一行） =====================
    // 右边界：最下方一行列坐标=IMAGE_WIDTH-2 → 近景丢线（贴右边缘）
    if(current_right_col[IMAGE_HEIGHT-1]==IMAGE_WIDTH-2)
    {
        right_line_lost_first_Flag=1;
    }
    // 左边界：最下方一行列坐标=1 → 近景丢线（贴左边缘）
    if(current_left_col[IMAGE_HEIGHT-1]==1)
    {
        left_line_lost_first_Flag=1;
    }

    // ===================== 第三步：统计右边界无效丢线行数 =====================
    // 若近景初始丢线，统计从最下方开始连续贴右边缘的行数（直到行=15或列≠UVC_WIDTH-2）
    if(right_line_lost_first_Flag==1)
    {
        for(int p=IMAGE_HEIGHT-1;p>15;p--)
        {
            // 该行右边界贴边缘 → 无效丢线行数+1
            if(current_right_col[p]==IMAGE_WIDTH-2)
            {
                no_effect_lost_right++;
            }
            // 该行右边界离开边缘 → 停止统计（无效丢线结束）
            else
                break;
        }
    }

    // ===================== 第四步：统计左边界无效丢线行数 =====================
    // 若近景初始丢线，统计从最下方开始连续贴左边缘的行数（直到行=15或列≠1）
    if(left_line_lost_first_Flag==1)
    {
        for(int p=IMAGE_HEIGHT-1;p>15;p--)
        {
            // 该行左边界贴边缘 → 无效丢线行数+1
            if(current_left_col[p]==1)
            {
                no_effect_lost_left++;
            }
            // 该行左边界离开边缘 → 停止统计
            else
                break;
        }
    }

    // ===================== 第五步：统计左边界有效丢线行数 =====================
    // 遍历范围：无效丢线结束后 ~ 行=15（有效赛道区），统计贴左边缘的行数
    for(int i=IMAGE_HEIGHT-1-no_effect_lost_left-1;i>15;i--)
    {
        if(current_left_col[i]==1)
        {
            effect_lost_left++;
        }
    }

    // ===================== 第六步：统计右边界有效丢线行数 =====================
    // 遍历范围：无效丢线结束后 ~ 行=15（有效赛道区），统计贴右边缘的行数
    for(int i=IMAGE_HEIGHT-1-no_effect_lost_right-1;i>15;i--)
    {
        if(current_right_col[i]==IMAGE_WIDTH-2)
        {
            effect_lost_right++;
        }
    }

    // ===================== 第七步：计算总丢线行数与有效行数 =====================
    diuxianshu[0] = effect_lost_left + no_effect_lost_left;  // 左边界总丢线行数
    diuxianshu[1] = effect_lost_right + no_effect_lost_right;// 右边界总丢线行数

    // 左边界有效行数 = 图像总高度 - 左边界最高行 - 左边界总丢线行数
    L_all_effect_line = IMAGE_HEIGHT - left_top_old - effect_lost_left - no_effect_lost_left;
    // 右边界有效行数 = 图像总高度 - 右边界最高行 - 右边界总丢线行数
    R_all_effect_line = IMAGE_HEIGHT - right_top_old - effect_lost_right - no_effect_lost_right;
}
/*--------------------------------------------------------------------------
 * 【函数功能】：赛道左右边界拐点检测（核心：基于边界点坐标的角度+位置特征判定拐点）
 * 【核心逻辑】：
 *  1. 初始化拐点相关变量，默认无拐点；
 *  2. 遍历有效边界点，通过“三点角度计算+位置特征”检测左右边界拐点；
 *  3. 过滤无效拐点（左右拐点重叠、贴图像边缘、边界点数量不足等）；
 *  4. 结合边界点数量和位置特征，修正拐点有效性及边界点计数；
 * 【输入依赖】：
 *  - L_edge/R_edge：八邻域爬线得到的左右边界点数组；
 *  - left/right_effect_index_end：左右边界有效点的结束索引；
 *  - left/right_findflag：左右边界拐点查找标志（1=允许查找，0=禁止）；
 * 【输出变量】：
 *  - L/R_corner_flag：左右拐点存在标志（1=存在，0=不存在）；
 *  - L/R_corner_row/col：左右拐点的行/列坐标；
 *  - L/R_corner_angle：左右拐点的角度（向量法计算）；
 *  - L/R_edge_count：修正后的左右边界点数量（剔除拐点后无效点）；
 * 【关键规则】：
 *  1. 拐点判定：三点角度≤125°+对应位置像素为白 → 判定为锐角/直角拐点；
 *  2. 无效拐点过滤：贴边缘、左右拐点重叠、边界点数量不足等情况标记为无拐点；
 *  3. 边界点修正：拐点附近无效边界点剔除，保证后续路径规划精度；
 --------------------------------------------------------------------------*/
int L_corner_flag;                      // 左拐点存在标志
int L_corner_row;                       // 左拐点所在行
int L_corner_col;                       // 左拐点所在列
int L_corner_angle;                     // 左拐点角度
int R_corner_flag;                      // 右拐点存在标志
int R_corner_row;                       // 右拐点所在行
int R_corner_col;                       // 右拐点所在列
int R_corner_angle;                     // 右拐点角度
int L_corner_count;                     // 八邻域索引
int R_corner_count;
int left_findflag = 0;
int right_findflag = 0;
void find_line()
{
    left_findflag = 1;
    right_findflag = 1;

}
static int16 XiuZLZdiuxianshu[2];
void DIUXIAN_XIUZLZ()
{
    XiuZLZdiuxianshu[0]=diuxianshu[0];
    XiuZLZdiuxianshu[1]=diuxianshu[1];
    for(uint8 y=IMAGE_HEIGHT-2;y>IMAGE_HEIGHT-25;y--)
    {
        if(current_left_col[y]==range_left)
        {
            XiuZLZdiuxianshu[0]--;
        }
        if(current_right_col[y]==range_right)
        {

            XiuZLZdiuxianshu[1]--;
        }
    }
}
void search_corner()
{
    // ===================== 第一步：初始化拐点相关变量（默认无拐点） =====================
    L_corner_flag = 0;                      // 左拐点存在标志：1=检测到拐点，0=无拐点
    L_corner_row = 0;                       // 左拐点所在行坐标（垂直方向）
    L_corner_col = 0;                       // 左拐点所在列坐标（水平方向）
    L_corner_angle = 0;                     // 左拐点的角度（向量法计算，单位：度）
    R_corner_flag = 0;                      // 右拐点存在标志：1=检测到拐点，0=无拐点
    R_corner_row = 0;                       // 右拐点所在行坐标
    R_corner_col = 0;                       // 右拐点所在列坐标
    R_corner_angle = 0;                     // 右拐点的角度（向量法计算）
    L_corner_count = 0, R_corner_count = 0; // 左右拐点对应的边界点数组索引

    // ===================== 第二步：检测左边界拐点 =====================
    // 左边界有效点数量足够（结束索引>20），才进行拐点检测（避免点数不足导致误判）
    if (left_effect_index_end > 20)
    {
        // 遍历左边界有效点：i最大为left_effect_index_end-12（保证i+12不越界）
        for (uint8 i = 0; i < left_effect_index_end - 12; i++)
        {
            // 过滤远景无效点：i+12位置的边界点行坐标>5（避开图像最上方模糊区域）
            if (L_edge[i + 12].row > 5)
            {
                // 拐点位置特征判定：i+5列 > i+12列 且 i+5列 ≥ i列（左边界向内收缩，符合拐点特征）
                if (L_edge[i + 5].col > L_edge[i + 12].col && L_edge[i + 5].col >= L_edge[i].col)
                {
                    // 过滤伪拐点：i+6行 < i+5行 且 列≥i+5列，或 i+6行≤i+5行 且 列>i+5列（边界点小幅波动）
                    if ((L_edge[i + 6].row < L_edge[i + 5].row && L_edge[i + 6].col >= L_edge[i + 5].col) || (L_edge[i + 6].row <= L_edge[i + 5].row && L_edge[i + 6].col > L_edge[i + 5].col))
                        continue; // 跳过伪拐点，继续遍历

                    // 向量法计算三点角度：(i点, i+5点, i+12点) → 判定是否为拐点
                    L_corner_angle = Get_angle(L_edge[i].row, L_edge[i].col, L_edge[i + 5].row, L_edge[i + 5].col, L_edge[i + 12].row, L_edge[i + 12].col);
                    
                    // 最终拐点判定：角度≤125°（锐角/直角）+ i+12点上一行像素为白（确认为赛道内拐点）
                    if (L_corner_angle <= 125 && arr[L_edge[i + 12].row - 1][L_edge[i + 12].col] == White) 
                    {
                        L_corner_flag = 1;        // 标记左拐点存在
                        L_corner_row = L_edge[i + 5].row; // 记录左拐点行坐标（i+5为拐点核心位置）
                        L_corner_col = L_edge[i + 5].col; // 记录左拐点列坐标
                        L_corner_count = i + 5;   // 记录左拐点在边界点数组中的索引
                        break;                    // 找到拐点，退出遍历
                    }
                }
            }
        }
    }

    // ===================== 第三步：检测右边界拐点 =====================
    // 右边界有效点数量足够（结束索引>20），才进行拐点检测
    if (right_effect_index_end > 20)
    {
        // 遍历右边界有效点：i最大为right_effect_index_end-12（保证i+12不越界）
        for (uint8 i = 0; i < right_effect_index_end - 12; i++)
        {
            // 过滤远景无效点：i+12位置的边界点行坐标>5
            if (R_edge[i + 12].row > 5)
            {
                // 拐点位置特征判定：i+12列 > i+5列 且 i列 ≥ i+5列（右边界向内收缩，符合拐点特征）
                if (R_edge[i + 12].col > R_edge[i + 5].col && R_edge[i].col >= R_edge[i + 5].col)
                {
                    // 过滤伪拐点：i+6行 < i+5行 且 列≤i+5列，或 i+6行≤i+5行 且 列<i+5列（边界点小幅波动）
                    if ((R_edge[i + 6].row < R_edge[i + 5].row && R_edge[i + 6].col <= R_edge[i + 5].col) || (R_edge[i + 6].row <= R_edge[i + 5].row && R_edge[i + 6].col < R_edge[i + 5].col))
                        continue; // 跳过伪拐点，继续遍历

                    // 向量法计算三点角度：(i点, i+5点, i+12点) → 判定是否为拐点
                    R_corner_angle = Get_angle(R_edge[i].row, R_edge[i].col, R_edge[i + 5].row, R_edge[i + 5].col, R_edge[i + 12].row, R_edge[i + 12].col);
                    
                    // 最终拐点判定：角度≤125°（锐角/直角）+ i+12点上一行像素为白（确认为赛道内拐点）
                    if (R_corner_angle <= 125 && arr[R_edge[i + 12].row - 1][R_edge[i + 12].col] == White) 
                    {
                        R_corner_flag = 1;        // 标记右拐点存在
                        R_corner_row = R_edge[i + 5].row; // 记录右拐点行坐标
                        R_corner_col = R_edge[i + 5].col; // 记录右拐点列坐标
                        R_corner_count = i + 5;   // 记录右拐点在边界点数组中的索引
                        break;                    // 找到拐点，退出遍历
                    }
                }
            }
        }
    }

    // ===================== 第四步：过滤无效拐点（左右拐点重叠） =====================
    if (R_corner_flag && L_corner_flag)
    {
        // 注释段：若左右拐点行坐标差值>20，判定为伪拐点（保留逻辑，未启用）
        /*if (myabs_Int(R_corner_row - L_corner_row) > 20)
        {
            if (R_corner_row > L_corner_row)
                L_corner_flag = 0;
            else
                R_corner_flag = 0;
        }*/
        // 核心过滤：左拐点列坐标 ≥ 右拐点列坐标 → 左右拐点重叠，标记为无拐点
        if (L_corner_col >= R_corner_col)
        {
            L_corner_flag = 0;
            R_corner_flag = 0;
        }
    }

    // ===================== 第五步：过滤贴图像边缘的无效拐点 =====================
    if (L_corner_col < 10)          // 左拐点列坐标<10（贴图像左边缘）→ 标记为无拐点
        L_corner_flag = 0;
    if (R_corner_col > IMAGE_WIDTH - 10) // 右拐点列坐标>IMAGE_WIDTH-10（贴图像右边缘）→ 标记为无拐点
        R_corner_flag = 0;

    // ===================== 第六步：左拐点有效性二次校验（结合边界点数量） =====================
    if (left_findflag && L_corner_flag)
    {
        // 左边界点数量<20（点数不足）→ 标记无拐点，关闭左拐点查找标志
        if (L_edge_count < 20)
        {
            left_findflag = 0;
            L_corner_flag = 0;
        }
        // 边界点数量充足 + 拐点与最后一个边界点位置接近（行差≤4，列差≤5）→ 修正边界点数量
        if (L_edge_count >= Max_Point - 1 && myabs_Int(L_corner_row - L_edge[L_edge_count - 1].row) <= 4 && myabs_Int(L_corner_col - L_edge[L_edge_count - 1].col) <= 5)
        {
            // 遍历边界点，剔除拐点后无效的边界点（保留到有效拐点前的点数）
            for (uint8 i = 0; i < L_edge_count; i++)
            {
                // 判定条件：当前点行坐标=最后一个边界点行坐标 + 中间点行差≤3（无效点判定）
                if (L_edge[i].row == L_edge[L_edge_count - 1].row && myabs_Int(L_edge[(i + L_edge_count) / 2].row - L_edge[i].row) <= 3)
                {
                    L_edge_count = i; // 修正左边界点数量为当前索引（剔除后续无效点）
                }
            }
        }
    }

    // ===================== 第七步：右拐点有效性二次校验（结合边界点数量） =====================
    if (right_findflag && R_corner_flag)
    {
        // 右边界点数量<20（点数不足）→ 标记无拐点，关闭右拐点查找标志
        if (R_edge_count < 20)
        {
            R_corner_flag = 0;
            right_findflag = 0;
        }
        // 边界点数量充足 + 拐点与最后一个边界点位置接近（行差≤4，列差≤4）→ 修正边界点数量
        if (R_edge_count >= Max_Point - 1 && myabs_Int(R_corner_row - R_edge[R_edge_count - 1].row) <= 4 && myabs_Int(R_corner_col - R_edge[R_edge_count - 1].col) <= 4)
        {
            // 遍历边界点，剔除拐点后无效的边界点（保留到有效拐点前的点数）
            for (uint8 i = 0; i < R_edge_count; i++)
            {
                // 判定条件：当前点行坐标=最后一个边界点行坐标 + 中间点行差≤3（无效点判定）
                if (R_edge[i].row == R_edge[R_edge_count - 1].row && myabs_Int(R_edge[(i + R_edge_count) / 2].row - R_edge[i].row) <= 3)
                {
                    R_edge_count = i; // 修正右边界点数量为当前索引（剔除后续无效点）
                }
            }
        }
    }
}
float add_line_k = 0.0; 
float add_line_b = 0.0;
void ADD_Edge_Line(int edge, int startX, int startY, int endX, int endY, int START_ROW, int END_ROW)
{
    if (edge == _left)
    {
        int i = 0;
        float k = 0.0f, b = 0.0f;
        k = (float)((float)endX - (float)startX) / (float)((float)endY - (float)startY);
        b = (float)startX - (float)startY * k;

        int col;
        for (i = START_ROW; i >= END_ROW; i--)
        {
            if (i < 0 || i > IMAGE_HEIGHT - 1) continue;   // 行下标越界保护,防止写穿 current_left_col
            col = k * i + b;
            if (col < 0)
                col = 1;
            if (col > IMAGE_WIDTH - 2)
                col = IMAGE_WIDTH - 2;
            current_left_col[i] = col;
        }
        add_line_k = k;
        add_line_b = b;
        left_findflag = 1;
        if (START_ROW == IMAGE_HEIGHT - 1)
            left_effect_row_start = IMAGE_HEIGHT - 2;
        if (END_ROW < endY)
            left_effect_row_end = END_ROW;
    }
    else if (edge == _right)
    {
        int i = 0;
        float k = 0.0f, b = 0.0f;
        k = (float)((float)endX - (float)startX) / (float)((float)endY - (float)startY);
        b = (float)startX - (float)startY * k;

        int col;
        for (i = START_ROW; i >= END_ROW; i--)
        {
            if (i < 0 || i > IMAGE_HEIGHT - 1) continue;   // 行下标越界保护,防止写穿 current_right_col
            col = k * i + b;
            if (col < 1)
                col = 1;
            if (col > IMAGE_WIDTH - 2)
                col = IMAGE_WIDTH - 2;
            current_right_col[i] = col;
        }
        add_line_k = k;
        add_line_b = b;
        right_findflag = 1;
        if (START_ROW == IMAGE_HEIGHT - 1)
            right_effect_row_start = IMAGE_HEIGHT - 2;
        if (END_ROW < endY)
            right_effect_row_end = END_ROW;
    }
}
/*-------------------------------------------------------------------------------------------------------------------
  @brief      十字判断函数pro（智能车赛道十字交叉路口检测核心函数）
  @param      arr[IMAGE_HEIGHT][IMAGE_WIDTH] - 输入的二值化图像数组（White=赛道，Black=背景）
  @return     void
  @message    支持三种十字场景检测：
              1. 正向十字（直行通过）；
              2. 左转接斜入十字；
              3. 右转接斜入十字；
              核心逻辑：基于拐点特征+丢线特征+斑马线过滤+边界坐标，判定是否为十字路口，并配置十字通行参数。
  Sample usage: 调用前需确保爬线、拐点检测、丢线统计等前置函数已执行，保证全局变量有效。
-------------------------------------------------------------------------------------------------------------------*/
// 全局变量：左右拐点临时标志（函数内未使用，预留扩展）
uint8 lc_flag = 0, rc_flag = 0;
int if_cross_flag=1;                  // 决定可不可以进入十字补线
int enable_element_search_flag = 1;   // 元素判断使能标志位
int left_halo_flag = 0;
int right_halo_flag = 0;   
int buxian_crossl_row;                // 十字左上拐点
int buxian_crossl_col;                // 十字左上拐点
int buxian_crossr_row;                // 十字右上拐点
int buxian_crossr_col;               // 十字右上拐点
int bx_lastl_exist_flag = 0;
int bx_lastr_exist_flag = 0;          // 十字补线记忆存在标志位
int element=0;
int cross_cnt = 0;                 // 十字状态机标志位
int cross_gesture = 0;             // 进入十字的姿势左斜入十字右斜入十字中入十字
int crossl_end_flag = 0; 
int crossr_end_flag = 0;              // 十字即将结束标志位
int End_Line_col = 0;
int xieshi = 1;
// void shizi_judge(const uint8 arr[IMAGE_HEIGHT][IMAGE_WIDTH])//十字判断
// {
//     // ===================== 前置条件：十字检测总开关 + 元素搜索使能 =====================
//     // if_cross_flag：十字检测总开关（1=允许检测，0=禁止）；
//     // enable_element_search_flag：赛道元素搜索使能（1=允许检测十字/弯道等元素）；
//     if (if_cross_flag && enable_element_search_flag)
//     {
//         // ===================== 分支1：正向十字路口检测（核心判定） =====================
//         // 正向十字判定条件（多维度特征组合，避免误判）：
//         // 1. 左右边界有效行结束索引>15（有效赛道区域足够，避开远景模糊区）；
//         // 2. 左右光晕标志为0（无光晕干扰，边界检测准确）；
//         // 3. 左右边界有效起始行≥UVC_HEIGHT-30（近景30行内有有效边界，赛道未完全出镜）；
//         // 4. 左右拐点均存在（L_corner_flag/R_corner_flag=1）；
//         // 5. 左右拐点行坐标差值<20（拐点在同一水平区域，符合十字路口对称特征）；
//         // 6. 拐点平均行坐标>18且单行坐标≥30（避开远景无效点，近景拐点有效）；
//         // 7. 拐点后边界点向外扩展（R_edge[R_corner_count+20].col > R_corner_col+5 → 右边界拐点后右扩；L_edge同理左扩）；
//         // 8. 左右拐点列间距>30（赛道宽度足够，排除窄道伪十字）；
//         if (left_effect_row_end > 15 && right_effect_row_end > 15 && right_halo_flag == 0 && left_halo_flag == 0 && 
//             right_effect_row_start >= IMAGE_HEIGHT - 30 && left_effect_row_start >= IMAGE_HEIGHT - 30 && 
//             L_corner_flag && R_corner_flag && myabs_Int(R_corner_row - L_corner_row) < 20 && 
//             (L_corner_row + R_corner_row) / 2 > 18 && R_corner_row >= 30 && L_corner_row >= 30 && 
//             left_effect_row_end > 15 && right_effect_row_end > 15 && 
//             R_edge[R_corner_count + 20].col > R_corner_col + 5 && L_edge[L_corner_count + 20].col < L_corner_col - 5 && 
//             R_corner_col - L_corner_col > 30)
//         {
//             // 子条件：左右拐点行坐标差值≥3（非完全水平对齐，需进一步验证）
//             if (myabs_Int(L_corner_row - R_corner_row) >= 3)
//             {
//                 // 初始化遍历范围：start_row=行坐标较大值（上边界），end_row=行坐标较小值（下边界）
//                 int start_row = L_corner_row > R_corner_row ? L_corner_row : R_corner_row;
//                 int end_row = L_corner_row > R_corner_row ? R_corner_row : L_corner_row;
//                 // start_col：对应start_row的拐点列坐标（右拐点/左拐点）
//                 int start_col = L_corner_row > R_corner_row ? L_corner_col : R_corner_col;

//                 // 遍历拐点行范围：从上到下（start_row → end_row），验证是否为斑马线（排除伪十字）
//                 for (uint8 i = start_row; i > end_row; i -= 1) 
//                 {
//                     uint8 banma_count1 = 0, banma_count2 = 0; // 斑马线黑白跳变计数（1=白→黑，2=黑→白）

//                     // 场景1：start_col为右拐点列坐标 → 向左遍历验证斑马线
//                     if (start_col == R_corner_col)
//                     {
//                         // 遍历范围：R_corner_col-1 → 赛道左边界（最小列=1，避免越界）
//                         for (uint8 j = R_corner_col - 1; j > ((R_corner_col - straightwidth[R_corner_row] < 1) ? 1 : (R_corner_col - straightwidth[R_corner_row])); j -= 1)
//                         {
//                             // 计数1：当前像素白，下一个像素黑（白→黑跳变，斑马线特征）
//                             if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
//                                 banma_count1++;
//                             // 计数2：当前像素黑，下一个像素白，且上一行+2列像素白（黑→白跳变，且赛道内有效）
//                             if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
//                                 banma_count2++;
                            
//                             // 提前终止：遍历到赛道中线且跳变计数不足2 → 无斑马线，退出
//                             if (j < (R_corner_col + R_corner_col - straightwidth[R_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
//                                 break;
//                             // 终止判定：跳变计数≥4 → 存在斑马线（非十字路口），直接返回
//                             if (banma_count1 >= 4 && banma_count2 >= 4)
//                             {
//                                 return;
//                             }
//                         }
//                     }

//                     // 场景2：start_col为左拐点列坐标 → 向右遍历验证斑马线
//                     if (start_col == L_corner_col)
//                     {
//                         // 遍历范围：L_corner_col+1 → 赛道右边界（最大列=IMAGE_WIDTH-2，避免越界）
//                         for (uint8 j = L_corner_col + 1; j < (((L_corner_col + straightwidth[L_corner_row]) > IMAGE_WIDTH - 2) ? IMAGE_WIDTH - 2 : (L_corner_col + straightwidth[L_corner_row])); j += 1)
//                         {
//                             // 计数1：白→黑跳变
//                             if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
//                                 banma_count1++;
//                             // 计数2：黑→白跳变（赛道内有效）
//                             if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
//                                 banma_count2++;
                            
//                             // 提前终止：遍历到赛道中线且跳变计数不足2 → 无斑马线，退出
//                             if (j > (L_corner_col + L_corner_col + straightwidth[L_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
//                                 break;
//                             // 终止判定：跳变计数≥4 → 存在斑马线，直接返回
//                             if (banma_count1 >= 4 && banma_count2 >= 4)
//                             {
//                                 return;
//                             }
//                         }
//                     }
//                 }
//             }

//             // 正向十字最终判定条件：
//             // 1. 左边界有效丢线>0（十字路口左边界丢失特征）；
//             // 2. 拐点行坐标≥30（近景有效）；
//             // 3. 拐点上3行边界坐标向外扩展（十字路口边界特征）；
//             // 4. 拐点行坐标>搜线上限行+8（有效赛道区域内）；
//             // 5. 拐点角度<91°（直角，符合十字路口特征）；
//             // 6. 拐点列间距<近景赛道宽度（十字路口宽度特征）；
//             if (effect_lost_left>0 && effect_lost_left>0 && R_corner_row >= 30 && L_corner_row >= 30 && 
//                 current_left_col[L_corner_row - 3] < L_corner_col - 5 && current_right_col[R_corner_row - 3] > R_corner_col + 5 && 
//                 L_corner_row > End_Line + 8 && R_corner_row > End_Line + 8 && 
//                 L_corner_angle < 91 && R_corner_angle < 91 && 
//                 R_corner_col - L_corner_col < current_right_col[IMAGE_HEIGHT - 2] - current_left_col[IMAGE_HEIGHT - 2])
//             {
//                 // 初始化十字路口补偿参数（清零，避免历史值干扰）
//                 buxian_crossl_row = 0;    // 左十字补偿行坐标
//                 buxian_crossl_col = 0;    // 左十字补偿列坐标
//                 buxian_crossr_col = 0;    // 右十字补偿列坐标
//                 buxian_crossr_row = 0;    // 右十字补偿行坐标
//                 bx_lastl_exist_flag = 0;  // 左补偿最后点存在标志
//                 bx_lastr_exist_flag = 0;  // 右补偿最后点存在标志
                
//                 // 配置十字路口核心参数
//                 element = 3;              // 赛道元素类型：3=十字路口
//                 cross_cnt = 1;            // 十字路口计数（标记已检测到十字）
//                 cross_gesture = 0;        // 十字通行姿态：0=正向直行
//                 enable_element_search_flag = 0; // 关闭元素搜索（避免重复检测）
//                 crossl_end_flag = 0;      // 左十字结束标志
//                 crossr_end_flag = 0;      // 右十字结束标志

//                 // 补充左边界线：从近景有效起始点到左拐点，延伸至搜线上限行
//                 ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
//                 // 补充右边界线：从近景有效起始点到右拐点，延伸至搜线上限行
//                 ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
//             }
//         }

//         // ===================== 分支2：左转接斜入十字路口检测 =====================
//         // 左转斜入十字判定条件：
//         // 1. 左最长白列<右拐点列-30（赛道宽度特征）；
//         // 2. xieshi=1（斜入十字使能）；
//         // 3. 右拐点角度<65°（锐角，斜入十字特征）；
//         // 4. 左边界总丢线>15（十字路口左边界大幅丢失）；
//         // 5. 右拐点后边界点右扩（斜入十字边界特征）；
//         // 6. 左边界有效行起始<50 或 结束>50（赛道位置特征）；
//         // 7. 近景右边界与右拐点列间距>20（宽度特征）；
//         // 8. 无光晕干扰，近景15行内有有效右边界；
//         // 9. 右拐点行≥30，搜线上限行<23（有效赛道区域）；
//         // 10. 右拐点行-搜线上限行≥5（有效拐点）；
//         // 11. 右拐点列>图像1/3（赛道右侧）；
//         // 12. 右拐点后30个点列扩>15（斜入特征）；
//         // 13. 左边界有效行起始<IMAGE_HEIGHT-10 或 结束>IMAGE_HEIGHT-10（位置特征）；
//         // 14. 搜线上限列<右拐点列-20（宽度特征）；
//         if (Longest_White_Column_Left[1]<R_corner_col-30&&xieshi&&R_corner_angle < 65 && 
//             (effect_lost_left + no_effect_lost_left >15) && R_edge[R_corner_count + 6].col > R_corner_col  &&
//             (left_effect_row_start < 50 || left_effect_row_end > 50)&&
//             current_right_col[right_effect_row_start] - R_corner_col > 20 && 
//             (effect_lost_left>0 || no_effect_lost_left>0) && 
//             right_halo_flag == 0 && left_halo_flag == 0 && 
//             right_effect_row_start >= IMAGE_HEIGHT - 15 && R_corner_row >= 30 && 
//             End_Line < 23 && R_corner_flag && R_corner_row - End_Line >= 5 && 
//             R_corner_col > IMAGE_WIDTH / 3 && current_right_col[right_effect_row_start] - R_corner_col > 20 && 
//             R_edge[R_corner_count + 30].col > R_corner_col + 15 && 
//             (left_effect_row_start < IMAGE_HEIGHT - 10 || left_effect_row_end > IMAGE_HEIGHT - 10)&& 
//             End_Line_col < R_corner_col - 20)
//         {
//             // 遍历验证：右拐点下3行 → 搜线上限行+3，检测是否有斑马线（排除伪十字）
//             for (uint8 i = R_corner_row - 3; i > End_Line + 3; i -= 1) 
//             {
//                 uint8 banma_count1 = 0, banma_count2 = 0; // 斑马线跳变计数
//                 // 遍历范围：右拐点列-1 → 赛道左边界（避免越界）
//                 for (uint8 j = R_corner_col - 1; j > ((R_corner_col - straightwidth[R_corner_row] < 1) ? 1 : (R_corner_col - straightwidth[R_corner_row])); j -= 1)
//                 {
//                     // 白→黑跳变计数
//                     if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
//                         banma_count1++;
//                     // 黑→白跳变计数（赛道内有效）
//                     if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
//                         banma_count2++;
                    
//                     // 提前终止：遍历到中线且计数不足2 → 无斑马线
//                     if (j < (R_corner_col + R_corner_col - straightwidth[R_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
//                         break;
//                     // 终止判定：计数≥6 → 有斑马线，返回（非十字）
//                     if (banma_count1 >= 6 && banma_count2 >= 6)
//                     {
//                         return;
//                     }
//                 }
//             }

//             // 配置左转斜入十字参数
//             element = 3;                  // 赛道元素：十字路口
//             cross_cnt = 1;                // 十字计数
//             cross_gesture = 1;            // 十字通行姿态：1=左转斜入
//             // 清零补偿参数
//             buxian_crossl_row = 0;
//             buxian_crossl_col = 0;
//             buxian_crossr_col = 0;
//             buxian_crossr_row = 0;
//             bx_lastl_exist_flag = 0;
//             bx_lastr_exist_flag = 0;
//             enable_element_search_flag = 0; // 关闭元素搜索
//             crossl_end_flag = 0;
//             crossr_end_flag = 0;

//             // 补充右边界线：近景最下方→右拐点，延伸至搜线上限行
//             ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
            
//             // 左边界修复：近景10行内有有效起始点但结束点靠上 → 补偿左边界线
//             if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_end < IMAGE_HEIGHT - 10)
//             {
//                 // 遍历左边界行：从起始点-5 → 搜线上限行，找列坐标突变点（边界丢失位置）
//                 for (uint8 i = left_effect_row_start - 5; i > End_Line; i--)
//                 {
//                     // 列坐标差值>5 → 边界突变，补充边界线
//                     if (myabs_Int(current_left_col[i] - current_left_col[i + 1]) > 5)
//                     {
//                         ADD_Edge_Line(_left, current_left_col[i + 5], i + 5, current_left_col[i + 1], i + 1, i, End_Line);
//                         break;
//                     }
//                 }
//             }
//             else
//             {
//                 left_findflag = 0; // 左边界无效，关闭左拐点查找
//             }
//             return; // 左转斜入十字判定完成，返回
//         }

//         // ===================== 分支3：右转接斜入十字路口检测 =====================
//         // 右转斜入十字判定条件（对称于左转，核心为左拐点特征）：
//         // 1. 右最长白列>左拐点列+30（宽度特征）；
//         // 2. xieshi=1（斜入使能）；
//         // 3. 左拐点角度<65°（锐角，斜入特征）；
//         // 4. 右边界总丢线>15（十字路口右边界丢失）；
//         // 5. 左拐点后边界点左扩（斜入特征）；
//         // 6. 右边界有效行起始<50 或 结束>50（位置特征）；
//         // 7. 左拐点与近景左边界列间距>20（宽度特征）；
//         // 8. 无光晕干扰，近景15行内有有效左边界；
//         // 9. 左拐点行≥30，搜线上限行<23；
//         // 10. 左拐点列<图像2/3（赛道左侧）；
//         // 11. 左拐点行-搜线上限行≥5（有效拐点）；
//         // 12. 左拐点后30个点列扩>15（斜入特征）；
//         // 13. 搜线上限列>左拐点列+20（宽度特征）；
//         // 14. 右边界有效行起始<IMAGE_HEIGHT-10 或 结束>IMAGE_HEIGHT-10（位置特征）；
//         if (Longest_White_Column_Right[1]>L_corner_col+30&&xieshi&&L_corner_angle < 65 && 
//             (effect_lost_right + no_effect_lost_right >15) && L_edge[L_corner_count + 6].col < L_corner_col  &&
//             (right_effect_row_start < 50 || right_effect_row_end > 50) && 
//             L_corner_col - current_left_col[left_effect_row_start] > 20 && 
//             (effect_lost_right>0 || no_effect_lost_right>0)&&
//             right_halo_flag == 0 && left_halo_flag == 0 && 
//             left_effect_row_start >= IMAGE_HEIGHT - 15 && L_corner_row >= 30 && 
//             End_Line < 23 && L_corner_flag && L_corner_col < IMAGE_WIDTH * 2 / 3 && 
//             L_corner_row - End_Line >= 5 && L_corner_col - current_left_col[left_effect_row_start] > 20 && 
//             L_edge[L_corner_count + 30].col < L_corner_col - 15 && 
//             End_Line_col > L_corner_col + 20 && 
//             (right_effect_row_start < IMAGE_HEIGHT - 10 || (right_effect_row_end > IMAGE_HEIGHT - 10)))
//         {
//             // 遍历验证：左拐点下3行 → 搜线上限行+3，检测是否有斑马线
//             for (uint8 i = L_corner_row - 3; i > End_Line + 3; i -= 1) 
//             {
//                 uint8 banma_count1 = 0, banma_count2 = 0; // 斑马线跳变计数
//                 // 遍历范围：左拐点列+1 → 赛道右边界（避免越界）
//                 for (uint8 j = L_corner_col + 1; j < (((L_corner_col + straightwidth[L_corner_row]) > IMAGE_WIDTH - 2) ? IMAGE_WIDTH - 2 : (L_corner_col + straightwidth[L_corner_row])); j += 1)
//                 {
//                     // 白→黑跳变计数
//                     if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
//                         banma_count1++;
//                     // 黑→白跳变计数（赛道内有效）
//                     if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
//                         banma_count2++;
                    
//                     // 提前终止：遍历到中线且计数不足2 → 无斑马线
//                     if (j > (L_corner_col + L_corner_col + straightwidth[L_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
//                         break;
//                     // 终止判定：计数≥6 → 有斑马线，返回
//                     if (banma_count1 >= 6 && banma_count2 >= 6)
//                     {
//                         return;
//                     }
//                 }
//             }

//             // 配置右转斜入十字参数
//             element = 3;                  // 赛道元素：十字路口
//             cross_cnt = 1;                // 十字计数
//             buxian_crossl_row = 0;        // 清零补偿参数
//             buxian_crossl_col = 0;
//             buxian_crossr_col = 0;
//             buxian_crossr_row = 0;
//             bx_lastl_exist_flag = 0;
//             bx_lastr_exist_flag = 0;
//             cross_gesture = 2;            // 十字通行姿态：2=右转斜入
//             enable_element_search_flag = 0; // 关闭元素搜索
//             crossl_end_flag = 0;
//             crossr_end_flag = 0;

//             // 补充左边界线：近景最下方→左拐点，延伸至搜线上限行
//             ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
            
//             // 右边界修复：近景10行内有有效起始点但结束点靠上 → 补偿右边界线
//             if (right_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_end < IMAGE_HEIGHT - 10)
//             {
//                 // 遍历右边界行：从起始点-5 → 搜线上限行，找列坐标突变点
//                 for (uint8 i = right_effect_row_start - 5; i > End_Line; i--)
//                 {
//                     // 列坐标差值>5 → 边界突变，补充边界线
//                     if (myabs_Int(current_right_col[i] - current_right_col[i + 1]) > 5)
//                     {
//                         ADD_Edge_Line(_right, current_right_col[i + 5], i + 5, current_right_col[i + 1], i + 1, i, End_Line);
//                         break;
//                     }
//                 }
//             }
//             else
//             {
//                 right_findflag = 0; // 右边界无效，关闭右拐点查找
//             }
//             return; // 右转斜入十字判定完成，返回
//         }
//     }
// }
void shizi_judge(const uint8 arr[IMAGE_HEIGHT][IMAGE_WIDTH])//十字判断
{
    if (if_cross_flag && enable_element_search_flag)
    {
        if (left_effect_row_end > 15 && right_effect_row_end > 15 && right_halo_flag == 0 && left_halo_flag == 0 && right_effect_row_start >= IMAGE_HEIGHT - 30 && left_effect_row_start >= IMAGE_HEIGHT - 30 && L_corner_flag && R_corner_flag && myabs_Int(R_corner_row - L_corner_row) < 20 && (L_corner_row + R_corner_row) / 2 > 18 && R_corner_row >= 30 && L_corner_row >= 30 && left_effect_row_end > 15 && right_effect_row_end > 15 && R_edge[R_corner_count + 20].col > R_corner_col + 5 && L_edge[L_corner_count + 20].col < L_corner_col - 5 && R_corner_col - L_corner_col > 30)
        {
            if (myabs_Int(L_corner_row - R_corner_row) >= 3)
            {
                int start_row = L_corner_row > R_corner_row ? L_corner_row : R_corner_row, end_row = L_corner_row > R_corner_row ? R_corner_row : L_corner_row;
                int start_col = L_corner_row > R_corner_row ? L_corner_col : R_corner_col;
                for (uint8 i = start_row; i > end_row; i -= 1) // 遍历
                {
                    uint8 banma_count1 = 0, banma_count2 = 0;
                    if (start_col == R_corner_col)
                    {
                        for (uint8 j = R_corner_col - 1; j > ((R_corner_col - straightwidth[R_corner_row] < 1) ? 1 : (R_corner_col - straightwidth[R_corner_row])); j -= 1)
                        {
                            if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                                banma_count1++;
                            if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                                banma_count2++;
                            if (j < (R_corner_col + R_corner_col - straightwidth[R_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                                break;
                            if (banma_count1 >= 4 && banma_count2 >= 4)
                            {
                                return;
                            }
                        }
                    }
                    if (start_col == L_corner_col)
                    {
                        for (uint8 j = L_corner_col + 1; j < (((L_corner_col + straightwidth[L_corner_row]) > IMAGE_WIDTH - 2) ? IMAGE_WIDTH - 2 : (L_corner_col + straightwidth[L_corner_row])); j += 1)
                        {
                            if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                                banma_count1++;
                            if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                                banma_count2++;
                            if (j > (L_corner_col + L_corner_col + straightwidth[L_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                                break;
                            if (banma_count1 >= 4 && banma_count2 >= 4)
                            {
                                return;
                            }
                        }
                    }
                }
            }//(no_effect_lost_right>0 || effect_lost_right>0) && (no_effect_lost_left>0 || effect_lost_left>0) &&
            if (effect_lost_left>0 && effect_lost_left>0 && R_corner_row >= 30 && L_corner_row >= 30 && current_left_col[L_corner_row - 3] < L_corner_col - 5 && current_right_col[R_corner_row - 3] > R_corner_col + 5 && L_corner_row > End_Line + 8 && R_corner_row > End_Line + 8 && L_corner_angle < 91 && R_corner_angle < 91 && R_corner_col - L_corner_col < current_right_col[IMAGE_HEIGHT - 2] - current_left_col[IMAGE_HEIGHT - 2])
            {
                buxian_crossl_row = 0;
                buxian_crossl_col = 0;
                buxian_crossr_col = 0;
                buxian_crossr_row = 0;
                bx_lastl_exist_flag = 0;
                bx_lastr_exist_flag = 0;
                element = 3;
                cross_cnt = 1;
                cross_gesture = 0;
                enable_element_search_flag = 0;
                crossl_end_flag = 0;
                crossr_end_flag = 0;
                ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
            }
        }
        // 左转接斜入十字
        if (Longest_White_Column_Left[1]<R_corner_col-30&&xieshi&&R_corner_angle < 65 && (effect_lost_left + no_effect_lost_left >15) && R_edge[R_corner_count + 6].col > R_corner_col  &&(left_effect_row_start < 50 || left_effect_row_end > 50)&&current_right_col[right_effect_row_start] - R_corner_col > 20 && (effect_lost_left>0 || no_effect_lost_left>0) && right_halo_flag == 0 && left_halo_flag == 0 && right_effect_row_start >= IMAGE_HEIGHT - 15 && R_corner_row >= 30 && End_Line < 23 && R_corner_flag && R_corner_row - End_Line >= 5 && R_corner_col > IMAGE_WIDTH / 3 && current_right_col[right_effect_row_start] - R_corner_col > 20 && R_edge[R_corner_count + 30].col > R_corner_col + 15 && (left_effect_row_start < IMAGE_HEIGHT - 10 || left_effect_row_end > IMAGE_HEIGHT - 10)&& End_Line_col < R_corner_col - 20)
        {
            for (uint8 i = R_corner_row - 3; i > End_Line + 3; i -= 1) // 遍历
            {
                uint8 banma_count1 = 0, banma_count2 = 0;
                for (uint8 j = R_corner_col - 1; j > ((R_corner_col - straightwidth[R_corner_row] < 1) ? 1 : (R_corner_col - straightwidth[R_corner_row])); j -= 1)
                {
                    if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                        banma_count1++;
                    if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                        banma_count2++;
                    if (j < (R_corner_col + R_corner_col - straightwidth[R_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                        break;
                    if (banma_count1 >= 6 && banma_count2 >= 6)
                    {
                        return;
                    }
                }
            }
            element = 3;
            cross_cnt = 1;
            cross_gesture = 1;
            buxian_crossl_row = 0;
            buxian_crossl_col = 0;
            buxian_crossr_col = 0;
            buxian_crossr_row = 0;
            bx_lastl_exist_flag = 0;
            bx_lastr_exist_flag = 0;
            enable_element_search_flag = 0;
            crossl_end_flag = 0;
            crossr_end_flag = 0;
            ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
            // 修复左线回返
            if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_end < IMAGE_HEIGHT - 10)
            {
                for (uint8 i = left_effect_row_start - 5; i > End_Line; i--)
                {
                    if (myabs_Int(current_left_col[i] - current_left_col[i + 1]) > 5)
                    {
                        ADD_Edge_Line(_left, current_left_col[i + 5], i + 5, current_left_col[i + 1], i + 1, i, End_Line);
                        break;
                    }
                }
            }
            else
            {
                left_findflag = 0;
            }
            return;
        }
        // 右转接斜入十字
        if (Longest_White_Column_Right[1]>L_corner_col+30&&xieshi&&L_corner_angle < 65 && (effect_lost_right + no_effect_lost_right >15) && L_edge[L_corner_count + 6].col < L_corner_col  &&(right_effect_row_start < 50 || right_effect_row_end > 50) && L_corner_col - current_left_col[left_effect_row_start] > 20 && (effect_lost_right>0 || no_effect_lost_right>0)&&right_halo_flag == 0 && left_halo_flag == 0 && left_effect_row_start >= IMAGE_HEIGHT - 15 && L_corner_row >= 30 && End_Line < 23 && L_corner_flag && L_corner_col < IMAGE_WIDTH * 2 / 3 && L_corner_row - End_Line >= 5 && L_corner_col - current_left_col[left_effect_row_start] > 20 && L_edge[L_corner_count + 30].col < L_corner_col - 15 && End_Line_col > L_corner_col + 20 && (right_effect_row_start < IMAGE_HEIGHT - 10 || (right_effect_row_end > IMAGE_HEIGHT - 10)))
        {
            for (uint8 i = L_corner_row - 3; i > End_Line + 3; i -= 1) // 遍历
            {
                uint8 banma_count1 = 0, banma_count2 = 0;
                for (uint8 j = L_corner_col + 1; j < (((L_corner_col + straightwidth[L_corner_row]) > IMAGE_WIDTH - 2) ? IMAGE_WIDTH - 2 : (L_corner_col + straightwidth[L_corner_row])); j += 1)
                {
                    if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                        banma_count1++;
                    if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                        banma_count2++;
                    if (j > (L_corner_col + L_corner_col + straightwidth[L_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                        break;
                    if (banma_count1 >= 6 && banma_count2 >= 6)
                    {
                        return;
                    }
                }
            }
            element = 3;
            cross_cnt = 1;
            buxian_crossl_row = 0;
            buxian_crossl_col = 0;
            buxian_crossr_col = 0;
            buxian_crossr_row = 0;
            bx_lastl_exist_flag = 0;
            bx_lastr_exist_flag = 0;
            cross_gesture = 2;
            enable_element_search_flag = 0;
            crossl_end_flag = 0;
            crossr_end_flag = 0;
            ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
            if (right_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_end < IMAGE_HEIGHT - 10)
            {
                for (uint8 i = right_effect_row_start - 5; i > End_Line; i--)
                {
                    if (myabs_Int(current_right_col[i] - current_right_col[i + 1]) > 5)
                    {
                        ADD_Edge_Line(_right, current_right_col[i + 5], i + 5, current_right_col[i + 1], i + 1, i, End_Line);
                        break;
                    }
                }
            }
            else
             {
                 right_findflag = 0;
             }
             return;
        }
    }

}
/*-------------------------------------------------------------------------------------------------------------------
  @brief      十字路口通行阶段处理函数（核心：十字检测确认后的边界补线+异常修正）
  @param      void
  @return     void
  @message    函数核心作用：
              1. 当检测到十字路口（element=3）时，启用斑马线检测并关闭光晕干扰标志；
              2. 针对不同十字通行姿态（正向/左转斜入/右转斜入），执行边界补线逻辑；
              3. 修复丢失的左右边界线，计算补偿拐点坐标，确保十字通行时边界连续；
              4. 过滤异常补线（宽度不符、坐标重叠等），保证路径规划稳定性；
  依赖前置：需先执行shizi_judge()完成十字判定，确保element、cross_cnt、cross_gesture等全局变量有效。
-------------------------------------------------------------------------------------------------------------------*/
int enable_banmaflag = 0;
int my_l_count = 0; 
int my_r_count = 0;
int max_colsrow;                                    // 存储最胖点的行
int max_colindex;                                   // 存储最胖点的八邻域代号
Point my_L_edge[100]; // 遍历法得到的边线
Point my_R_edge[100]; // 遍历法得到的边线
int black_(int x) // 判断黑
{
    if (x == Black)
        return 1;
    else
        return 0;
}

int white_(int x) // 判断白
{
    if (x == White)
        return 1;
    else
        return 0;
}
int get_maxline(int edge)
{
    int max_col = 0;
    int min_col = IMAGE_WIDTH - 1;
    if (edge == 0)
    {
        if (left_effect_index_start < left_effect_index_end)
            for (int i = left_effect_index_start; i < left_effect_index_end; i++)
            {
                if (L_edge[i].col >= max_col)
                {
                    max_col = L_edge[i].col;
                    max_colsrow = L_edge[i].row;
                    max_colindex = i;
                }
            }
        return max_col;
    }
    if (edge == 1)
    {
        if (right_effect_index_start < right_effect_index_end)
            for (int i = right_effect_index_start; i < right_effect_index_end; i++)
            {
                if (R_edge[i].col <= min_col)
                {
                    max_colsrow = R_edge[i].row;
                    min_col = R_edge[i].col;
                    max_colindex = i;
                }
            }
        return min_col;
    }
    return 0;
}
int bai_count(int start_row, int end_row, int yuzhi)
{
    // int bai_flag=0;
    int bai_leftcount=0;
    int bai_rightcount=0;
    for(int i=start_row;i>end_row;i--)
    {
        if(arr[i][IMAGE_WIDTH-3]==White)
        {
            bai_rightcount++;
        }
        if(arr[i][2]==White)
        {
            bai_leftcount++;
        }
    }
    if((bai_rightcount>yuzhi && bai_leftcount>yuzhi)||(bai_rightcount>yuzhi+2 && bai_leftcount>yuzhi-2)||(bai_rightcount>yuzhi-2 && bai_leftcount>yuzhi+2))
    {
        // bai_flag=1;
        return 1;
    }
    else
        return 0;
}
/*--------------------------------------------------------------------------
 * 【函数功能】：获取赛道的横向信息
 * 【参    数】：无
 * 【返 回 值】：无
 * 【备    注】：通过从中间向两边遍历获取赛道突变元素已经丢线位置，用于补线，判断,结束行不要小于十
 *--------------------------------------------------------------------------*/
void bianli(int start_row, int end_row, int start_col, int end_col, int edge)
{
    if (edge == 0)
        my_l_count = 0;
    else if (edge == 1)
        my_r_count = 0;
    if (start_row >= IMAGE_HEIGHT - 1)
        start_row = IMAGE_HEIGHT - 1;
    for (int i = start_row; i >= end_row; i--)
    {
        if (edge == 0)
        {
            for (int j1 = start_col; j1 >= end_col; j1 -= 1)
            {
                if (j1 <= 4)
                {
                    my_L_edge[my_l_count].y = i;
                    my_L_edge[my_l_count++].x = 1;
                    break;
                }
                else if (black_(arr[i][j1]) && black_(arr[i][j1 + 1]) && white_(arr[i][j1 + 2]))
                {
                    my_L_edge[my_l_count].y = i;
                    my_L_edge[my_l_count++].x = j1 + 2;
                    break;
                }
            }
        }
        else if (edge == 1)
        {
            for (int j2 = start_col; j2 <= end_col; j2 += 1)
            {

                if (j2 >= IMAGE_WIDTH - 5)
                {
                    my_R_edge[my_r_count].y = i;
                    my_R_edge[my_r_count++].x = IMAGE_WIDTH - 1;
                    break;
                }
                else if (black_(arr[i][j2]) && black_(arr[i][j2 - 1]) && white_(arr[i][j2 - 2]))
                {
                    my_R_edge[my_r_count].y = i;
                    my_R_edge[my_r_count++].x = j2 - 2;
                    break;
                }
            }
        }
    }
}
// void shizi_one()
// {
//     // ===================== 前置条件：已检测到十字路口（element=3） =====================
//     if (element == 3)
//     {
//         enable_banmaflag = 1;    // 启用斑马线检测标志
//         right_halo_flag = 0;     // 关闭右边界光晕标志
//         left_halo_flag = 0;      // 关闭左边界光晕标志

//         // ===================== 分支1：正向十字通行（cross_gesture=0） =====================
//         if (cross_cnt == 1)
//         {
//             if (cross_gesture == 0)
//             {
//                 if (bai_count(IMAGE_HEIGHT-2, IMAGE_HEIGHT-30, 4)==0 && left_effect_row_start >= 50 && right_effect_row_start >= 50)
//                 {
//                     if (L_corner_flag && L_corner_row < IMAGE_HEIGHT - 15 && R_corner_flag && R_corner_row < IMAGE_HEIGHT - 15)
//                     {
//                         ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
//                         ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line + 1);
//                     }
//                     else
//                     {
//                         left_findflag = 0; 
//                     }
//                     right_findflag = 0;    

//                     // -------------------- 左边界补偿拐点查找与补线 --------------------
//                     lc_flag = 0, rc_flag = 0; 
//                     if (left_findflag == 0)  
//                     {
//                         if (bx_lastl_exist_flag == 0) 
//                         {
//                             int mid_col = 0; 
//                             if (L_corner_flag && R_corner_flag)
//                             {
//                                 mid_col = (L_corner_col + R_corner_col) / 2;
//                             }
//                             else if (L_corner_flag && R_corner_flag == 0)
//                             {
//                                 if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || End_Line_col - L_corner_col < 20)
//                                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                                 else
//                                     mid_col = End_Line_col;
//                             }
//                             else if (L_corner_flag == 0 && R_corner_flag)
//                             {
//                                 if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_col - End_Line_col < 20)
//                                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                                 else
//                                     mid_col = End_Line_col;
//                             }
//                             else
//                             {
//                                 int coll = get_maxline(0); 
//                                 int colr = get_maxline(1); 
//                                 if (End_Line_col > coll + 2 && End_Line_col < colr - 2)
//                                     mid_col = End_Line_col;
//                                 else
//                                     mid_col = (coll + colr) / 2;
//                             }
                            
//                             // ！！！扫描黑岛边缘，存入 my_L_edge ！！！
//                             bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, 0, 0);
//                         }
//                         else 
//                         {
//                             bianli(buxian_crossl_row + 15, buxian_crossl_row - 10, buxian_crossl_col + 20, 0, 0);
//                         }

//                         // 【修好的 Bug】：使用 my_L_edge 替代 L_edge！
//                         for (uint8 i = 5; i < my_l_count - 3; i++)
//                         {
//                             if (my_L_edge[i].x - my_L_edge[i - 3].x > 15 && myabs_Int(my_L_edge[i + 2].x - my_L_edge[i].x) < 10)
//                             {
//                                 if (my_L_edge[i + 1].y < IMAGE_HEIGHT - 10) 
//                                 {
//                                     // 增加保护：黑岛在路中间，所以它的x坐标不能太靠左，防止扫到左边墙
//                                     if(my_L_edge[i + 1].x > 30) 
//                                     {
//                                         buxian_crossl_col = my_L_edge[i + 1].x; 
//                                         buxian_crossl_row = my_L_edge[i + 1].y; 
//                                         lc_flag = 1;                         
//                                         break;
//                                     }
//                                 }
//                             }
//                         }

//                         if (lc_flag)
//                         {
//                             // if (left_effect_row_start >= IMAGE_HEIGHT - 15)
//                             //     ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
//                             // else
//                             // {
//                             //     ADD_Edge_Line(_left, 1, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);
//                             // }
//                             // ====== 修改开始：加上拐点优先连线 ======
//                            /// 优先：有拐点连拐点（图1图2正常状态）
//                            // ====== 1. 优先：有拐点，连拐点 ======
//                             if (L_corner_flag) 
//                             {
//                                 ADD_Edge_Line(_left, L_corner_col, L_corner_row, buxian_crossl_col, buxian_crossl_row, L_corner_row, buxian_crossl_row);
//                             }
//                             // ====== 2. 兜底：你原来的完美逻辑 ======
//                             else 
//                             {
//                                 if (left_effect_row_start >= IMAGE_HEIGHT - 15)
//                                     ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
//                                 else
//                                     ADD_Edge_Line(_left, 1, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);
//                             }
//                             // ====== 修改结束 ======

//                             if (buxian_crossl_row > End_Line + 1)
//                             {
//                                 for (int j = buxian_crossl_row; j >= End_Line; j--)
//                                 {
//                                     if (myabs_Int(current_left_col[j] - current_left_col[j - 1]) > 8)
//                                     {
//                                         uint8 find_success_flag = 0; 
//                                         if (arr[j - 1][current_left_col[j]] == Black && End_Line_col > current_left_col[j])
//                                         {
//                                             for (uint8 i = current_left_col[j]; i < End_Line_col; i++)
//                                             {
//                                                 if (arr[j - 1][i] == Black && arr[j - 1][i + 1] == White)
//                                                 {
//                                                     current_left_col[j - 1] = i; 
//                                                     find_success_flag = 1;
//                                                     break;
//                                                 }
//                                             }
//                                         }
//                                         if (arr[j - 1][current_left_col[j]] == White && current_left_col[j] > 30)
//                                         {
//                                             for (uint8 i = current_left_col[j]; i > current_left_col[j] - 20; i--)
//                                             {
//                                                 if (arr[j - 1][i] == White && arr[j - 1][i - 1] == Black)
//                                                 {
//                                                     current_left_col[j - 1] = i - 1; 
//                                                     find_success_flag = 1;
//                                                     break;
//                                                 }
//                                             }
//                                         }
//                                         if (find_success_flag == 0 && arr[j - 1][current_left_col[j]] == Black)
//                                             current_left_col[j - 1] = current_left_col[j] + 1;
//                                     }
//                                     left_effect_row_end = j; 
//                                 }
//                                 if (L_corner_flag && L_corner_angle < 91 && buxian_crossl_row > L_corner_row && L_corner_col > buxian_crossl_col)
//                                     ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
//                             }
//                         }
//                         else
//                         {
//                             left_findflag = 0; 
//                             //---------------
//                             // ====== 新增：防跳动兜底 ======
//                             if (L_corner_flag) 
//                                 ADD_Edge_Line(_left, L_corner_col, L_corner_row, current_left_col[End_Line], End_Line, L_corner_row, End_Line);
//                             else 
//                                 ADD_Edge_Line(_left, 2, IMAGE_HEIGHT - 1, current_left_col[End_Line], End_Line, IMAGE_HEIGHT - 1, End_Line);
//                         }
//                             //---------------
                        
//                     }

//                     // -------------------- 右边界补偿拐点查找与补线 --------------------
//                     if (right_findflag == 0)
//                     {
//                         if (bx_lastr_exist_flag == 0) 
//                         {
//                             int mid_col = 0;
//                             if (L_corner_flag && R_corner_flag)
//                             {
//                                 mid_col = (L_corner_col + R_corner_col) / 2;
//                             }
//                             else if (L_corner_flag && R_corner_flag == 0)
//                             {
//                                 if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || End_Line_col - L_corner_col < 20)
//                                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                                 else
//                                     mid_col = End_Line_col;
//                             }
//                             else if (L_corner_flag == 0 && R_corner_flag)
//                             {
//                                 if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_col - End_Line_col < 20)
//                                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                                 else
//                                     mid_col = End_Line_col;
//                             }
//                             else
//                             {
//                                 int coll = get_maxline(0);
//                                 int colr = get_maxline(1);
//                                 if (End_Line_col > coll + 2 && End_Line_col < colr - 2)
//                                     mid_col = End_Line_col;
//                                 else
//                                     mid_col = (coll + colr) / 2;
//                             }
                            
//                             // ！！！扫描黑岛边缘，存入 my_R_edge ！！！
//                             bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, IMAGE_WIDTH - 1, 1);
//                         }
//                         else 
//                         {
//                             bianli(buxian_crossr_row + 15, buxian_crossr_row - 15, buxian_crossr_col - 20, IMAGE_WIDTH - 1, 1);
//                         }

//                         // 【修好的 Bug】：使用 my_R_edge 替代 R_edge！
//                         for (uint8 i = 5; i < my_r_count - 3; i++)
//                         {
//                             if (my_R_edge[i - 3].x - my_R_edge[i].x > 15 && myabs_Int(my_R_edge[i].x - my_R_edge[i + 2].x) < 10)
//                             {
//                                 if (my_R_edge[i + 3].y < IMAGE_HEIGHT - 10) 
//                                 {
//                                     // 增加保护：黑岛在路中间，它的x坐标不能太靠右
//                                     if(my_R_edge[i + 1].x < IMAGE_WIDTH - 30) 
//                                     {
//                                         buxian_crossr_col = my_R_edge[i + 1].x; 
//                                         buxian_crossr_row = my_R_edge[i + 1].y; 
//                                         rc_flag = 1;                         
//                                         break;
//                                     }
//                                 }
//                             }
//                         }

//                         if (rc_flag)
//                         {
//                             // if (right_effect_row_start >= IMAGE_HEIGHT - 15)
//                             //     ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
//                             // else
//                             //     ADD_Edge_Line(_right, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);
//                             // ====== 修改开始：加上拐点优先连线 ===================================================
//                             // ====== 1. 优先：有拐点，连拐点 ======
//                             if (R_corner_flag)
//                             {
//                                 ADD_Edge_Line(_right, R_corner_col, R_corner_row, buxian_crossr_col, buxian_crossr_row, R_corner_row, buxian_crossr_row);
//                             }
//                             // ====== 2. 兜底：你原来的完美逻辑 ======
//                             else
//                             {
//                                 if (right_effect_row_start >= IMAGE_HEIGHT - 15)
//                                     ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
//                                 else
//                                     ADD_Edge_Line(_right, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);
//                             }
//                             // ====== 修改结束 ======================================================
//                             if (buxian_crossr_row > End_Line + 1)
//                             {
//                                 for (int j = buxian_crossr_row; j >= End_Line; j--)
//                                 {
//                                     if (myabs_Int(current_right_col[j] - current_right_col[j - 1]) > 8)
//                                     {
//                                         uint8 find_success_flag = 0; 
//                                         if (arr[j - 1][current_right_col[j]] == Black && End_Line_col < current_right_col[j])
//                                         {
//                                             for (uint8 i = current_right_col[j]; i > End_Line_col; i--)
//                                             {
//                                                 if (arr[j - 1][i] == Black && arr[j - 1][i - 1] == White)
//                                                 {
//                                                     current_right_col[j - 1] = i; 
//                                                     find_success_flag = 1;
//                                                     break;
//                                                 }
//                                             }
//                                         }
//                                         if (arr[j - 1][current_right_col[j]] == White && current_right_col[j] < IMAGE_WIDTH - 30)
//                                         {
//                                             for (uint8 i = current_right_col[j]; i < current_right_col[j] + 20; i++)
//                                             {
//                                                 if (arr[j - 1][i] == White && arr[j - 1][i + 1] == Black)
//                                                 {
//                                                     current_right_col[j - 1] = i + 1; 
//                                                     find_success_flag = 1;
//                                                     break;
//                                                 }
//                                             }
//                                         }
//                                         if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == Black)
//                                             current_right_col[j - 1] = current_right_col[j] - 1;
//                                         else if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == White)
//                                             current_right_col[j - 1] = current_right_col[j];
//                                     }
//                                     right_effect_row_end = j; 
//                                 }
//                                 if (R_corner_flag && R_corner_angle < 91 && buxian_crossr_row > R_corner_row && R_corner_col < buxian_crossr_col)
//                                     ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
//                             }
//                         }
//                         else
//                         {
//                             right_findflag = 0; 
//                             // ====== 新增：防跳动兜底 ======
//                             if (R_corner_flag) 
//                                 ADD_Edge_Line(_right, R_corner_col, R_corner_row, current_right_col[End_Line], End_Line, R_corner_row, End_Line);
//                             else 
//                                 ADD_Edge_Line(_right, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, current_right_col[End_Line], End_Line, IMAGE_HEIGHT - 1, End_Line);
//                         }
//                     }

//                     // -------------------- 补偿拐点状态更新及异常处理 --------------------
//                     bx_lastl_exist_flag = lc_flag ? 1 : 0; 
//                     bx_lastr_exist_flag = rc_flag ? 1 : 0; 

//                     if (lc_flag && rc_flag && buxian_crossr_col < buxian_crossl_col && buxian_crossr_row < IMAGE_HEIGHT - 15 && buxian_crossl_row < IMAGE_HEIGHT - 15)
//                     {
//                         bx_lastl_exist_flag = 0;
//                         bx_lastr_exist_flag = 0;
//                         left_findflag = 0;
//                         right_findflag = 0;
//                     }
//                     if (lc_flag && rc_flag && buxian_crossr_col - buxian_crossl_col > straightwidth[(buxian_crossr_row + buxian_crossl_row) / 2] * 1.4) 
//                     {
//                         bx_lastl_exist_flag = 0;
//                         bx_lastr_exist_flag = 0;
//                         left_findflag = 0;
//                         right_findflag = 0;
//                     }
//                 }
//                 else 
//                 {
//                     cross_cnt = 2; // 前置条件不满足（有斑马线/边界不足）→ 十字计数=2，退出补线
//                 }
//             }

//             // ===================== 子分支2：左转斜入十字通行（由于篇幅，保留你原来的原封不动） =====================
//             else if (cross_gesture == 1)
//             {
//                 if (R_corner_flag && R_corner_row < IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10)
//                 {
//                     if (left_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10 && 
//                         left_effect_row_end > L_corner_row - 10  && L_corner_flag && R_corner_flag && 
//                         myabs_Int(L_corner_row - R_corner_row) < 10 && L_corner_row > 23 && R_corner_row > 23)
//                     {
//                         cross_gesture = 0;
//                     }

//                     ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                    
//                     if (L_corner_flag && left_effect_row_start > IMAGE_HEIGHT - 10)
//                         ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
//                     else
//                     {
//                         if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_end < IMAGE_HEIGHT - 10)
//                         {
//                             for (uint8 i = left_effect_row_start - 5; i > End_Line; i--)
//                             {
//                                 if (myabs_Int(current_left_col[i] - current_left_col[i + 1]) > 5)
//                                 {
//                                     ADD_Edge_Line(_left, current_left_col[i + 5], i + 5, current_left_col[i + 1], i + 1, i, End_Line);
//                                     break;
//                                 }
//                             }
//                         }
//                         else
//                         {
//                             left_findflag = 0; 
//                         }
//                     }
//                 }
//                 else
//                 {
//                     cross_cnt = 2; 
//                 }
//             }

//             // ===================== 子分支3：右转斜入十字通行（由于篇幅，保留你原来的原封不动） =====================
//             else if (cross_gesture == 2)
//             {
//                 if (L_corner_flag && L_corner_row < IMAGE_HEIGHT - 10 && left_effect_row_start > IMAGE_HEIGHT - 10 )
//                 {
//                     if (left_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10 && 
//                         right_effect_row_end > R_corner_row - 10 && L_corner_flag && R_corner_flag && 
//                         myabs_Int(L_corner_row - R_corner_row) < 10 && L_corner_row > 18 && R_corner_row > 18)
//                     {
//                         cross_gesture = 0;
//                     }

//                     ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                    
//                     if (R_corner_flag && right_effect_row_start > IMAGE_HEIGHT - 10)
//                         ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
//                     else
//                     {
//                         if (right_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_end < IMAGE_HEIGHT - 10)
//                         {
//                             for (uint8 i = right_effect_row_start - 5; i > End_Line; i--)
//                             {
//                                 if (myabs_Int(current_right_col[i] - current_right_col[i + 1]) > 5)
//                                 {
//                                     ADD_Edge_Line(_right, current_right_col[i + 5], i + 5, current_right_col[i + 1], i + 1, i, End_Line);
//                                     break;
//                                 }
//                             }
//                         }
//                         else
//                         {
//                             right_findflag = 0; 
//                         }
//                     }
//                 }
//                 else
//                 {
//                     cross_cnt = 2; 
//                 }
//             }
//         }
//     }
// }
void shizi_one()
{
    if (element == 3)
        {
            enable_banmaflag=1;
            right_halo_flag = 0;
            left_halo_flag = 0;
            if (cross_cnt == 1)
            {
                if (myabs_Int(L_corner_row - R_corner_row) >= 3)
                {
                    int kstart_row = L_corner_row > R_corner_row ? L_corner_row : R_corner_row, end_row = L_corner_row > R_corner_row ? R_corner_row : L_corner_row;
                    int kstart_col = L_corner_row > R_corner_row ? L_corner_col : R_corner_col;
                    if(end_row<=End_Line)end_row = End_Line+1;
                    for (uint8 i = kstart_row; i > end_row; i -= 1) // 遍历
                    {
                        uint8 banma_count1 = 0, banma_count2 = 0;
                        if (kstart_col == R_corner_col)
                        {
                            for (uint8 j = R_corner_col + 1; j > ((R_corner_col - straightwidth[R_corner_row] < 1) ? 1 : (R_corner_col - straightwidth[R_corner_row])); j -= 1)
                            {
                                if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                                    banma_count1++;
                                if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                                    banma_count2++;
                                if (j < (R_corner_col + R_corner_col - straightwidth[R_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                                    break;
                                if (banma_count1 >= 6 && banma_count2 >= 6)
                                {
                                    element = 0;
                                    enable_element_search_flag = 1;
                                    cross_cnt = 0;
                                    crossl_end_flag = 0;
                                    crossr_end_flag = 0;
                                    return;
                                }
                            }
                        }
                        if (kstart_col == L_corner_col)
                        {
                            for (uint8 j = L_corner_col + 1; j < (((L_corner_col + straightwidth[L_corner_row]) > IMAGE_WIDTH - 2) ? IMAGE_WIDTH - 2 : (L_corner_col + straightwidth[L_corner_row])); j += 1)
                            {
                                if ((arr[i][j]) == White && (arr[i][j + 1]) == Black)
                                    banma_count1++;
                                if ((arr[i][j]) == Black && (arr[i][j + 1]) == White && (arr[i - 1][j + 2]) == White)
                                    banma_count2++;
                                if (j > (L_corner_col + L_corner_col + straightwidth[L_corner_row]) / 2 && banma_count2 < 2 && banma_count1 < 2)
                                    break;
                                if (banma_count1 >= 6 && banma_count2 >= 6)
                                {
                                    element = 0;
                                    enable_element_search_flag = 1;
                                    cross_cnt = 0;
                                    crossl_end_flag = 0;
                                    crossr_end_flag = 0;
                                    return;
                                }
                            }
                        }
                    }
                }
                if (cross_gesture == 0)
                {
                    // 正入十字补线
                    /*if (L_corner_col != 0)
                        L_corner_flag = 1;
                    if (R_corner_col != 0)
                        R_corner_flag = 1;*/

                    if (bai_count(IMAGE_HEIGHT-2, IMAGE_HEIGHT-30, 4)==0 && left_effect_row_start >= 50 && right_effect_row_start >= 50)//left_effect_row_end <= IMAGE_HEIGHT - 25 && right_effect_row_end <= IMAGE_HEIGHT - 25
                    {
                        if (L_corner_flag && L_corner_row < IMAGE_HEIGHT - 15&& R_corner_flag && R_corner_row < IMAGE_HEIGHT - 15)
                        {
                            ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                            ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line + 1);
                        }
                        else
                            //ADD_Edge_Line(_left, current_right_col[IMAGE_HEIGHT - 1] - straightwidth[IMAGE_HEIGHT - 1] , IMAGE_HEIGHT - 1, R_corner_col - straightwidth[R_corner_row] ,R_corner_row , IMAGE_HEIGHT - 1 , End_Line);//current_right_col[IMAGE_HEIGHT - 1] - straightwidth[IMAGE_HEIGHT - 1]  R_corner_col - straightwidth[R_corner_row]
                              left_findflag = 0;
                       /* if ()
                        {

                        }
                        else*/
                              right_findflag = 0;

                        // 寻找上拐点
                        lc_flag = 0, rc_flag = 0;
                        if (left_findflag == 0)
                          {
                              if (bx_lastl_exist_flag == 0)
                              {
                                  int mid_col = 0;
                                  if (L_corner_flag && R_corner_flag)
                                  {
                                      //                                if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
                                      //                                    mid_col = End_Line_col;
                                      //                                else
                                      mid_col = (L_corner_col + R_corner_col) / 2;
                                  }
                                  else if (L_corner_flag && R_corner_flag == 0)
                                  {
                                      if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || End_Line_col - L_corner_col < 20)
                                          mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                                      else
                                          mid_col = End_Line_col;
                                  }
                                  else if (L_corner_flag == 0 && R_corner_flag)
                                  {
                                      if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_col - End_Line_col < 20)
                                          mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                                      else
                                          mid_col = End_Line_col;
                                  }
                                  else
                                  {
                                      int coll = get_maxline(0);
                                      int colr = get_maxline(1);
                                      if (End_Line_col > coll + 2 && End_Line_col < colr - 2)
                                          mid_col = End_Line_col;
                                      else
                                          mid_col = (coll + colr) / 2;
                                  }
                                  bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, 0, 0);
                              }
                              else
                                  bianli(buxian_crossl_row + 15, buxian_crossl_row - 10, buxian_crossl_col + 20, 0, 0);
                              for (uint8 i = 5; i < my_l_count - 3; i++)
                              {
                                  if (L_edge[i].col - L_edge[i - 3].col > 20 && myabs_Int(L_edge[i + 2].col - L_edge[i].col) < 10)
                                  {
                                      if (L_edge[i + 1].row < IMAGE_HEIGHT - 10)
                                      {
                                          buxian_crossl_col = L_edge[i + 1].col;
                                          buxian_crossl_row = L_edge[i + 1].row;
                                          lc_flag = 1;
                                          break;
                                      }
                                  }
                              }
                              if (lc_flag)
                              {
                                  /*if(buxian_crossl_col == 0 && buxian_crossl_row == 0)
                                  {
                                      buxian_crossl_col = current_right_col[R_corner_row] - straightwidth[R_corner_row];
                                      buxian_crossl_row = R_corner_row;
                                  }*/
                                  if (left_effect_row_start >= IMAGE_HEIGHT - 15)
                                      ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
                                  else
                                  {
                                      ADD_Edge_Line(_left, 1, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);
                                  }


                                  if (buxian_crossl_row > End_Line + 1)
                                  {
                                      for (int j = buxian_crossl_row; j >= End_Line; j--)
                                      {
                                          if (myabs_Int(current_left_col[j] - current_left_col[j - 1]) > 8)
                                          {
                                              uint8 find_success_flag = 0;
                                              if (arr[j - 1][current_left_col[j]] == Black && End_Line_col > current_left_col[j])
                                              {
                                                  for (uint8 i = current_left_col[j]; i < End_Line_col; i++)
                                                  {
                                                      if (arr[j - 1][i] == Black && arr[j - 1][i + 1] == White)
                                                      {
                                                          current_left_col[j - 1] = i;
                                                          find_success_flag = 1;
                                                          break;
                                                      }
                                                  }
                                              }
                                              if (arr[j - 1][current_left_col[j]] == White && current_left_col[j] > 30)
                                              {
                                                  for (uint8 i = current_left_col[j]; i > current_left_col[j] - 20; i--)
                                                  {
                                                      if (arr[j - 1][i] == White && arr[j - 1][i - 1] == Black)
                                                      {
                                                          current_left_col[j - 1] = i - 1;
                                                          find_success_flag = 1;
                                                          break;
                                                      }
                                                  }
                                              }
                                              if (find_success_flag == 0 && arr[j - 1][current_left_col[j]] == Black)
                                                  current_left_col[j - 1] = current_left_col[j] + 1;
                                          }
                                          left_effect_row_end = j;
                                      }
                                      if (L_corner_flag && L_corner_angle < 91 && buxian_crossl_row > L_corner_row && L_corner_col > buxian_crossl_col)
                                          ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                                  }
                              }
                              else
                                  left_findflag = 0;
                          }
                        if (right_findflag == 0)
                        {
                            if (bx_lastr_exist_flag == 0)
                            {
                                int mid_col = 0;
                                if (L_corner_flag && R_corner_flag)
                                {
                                    //                                if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
                                    //                                    mid_col = End_Line_col;
                                    //                                else
                                    mid_col = (L_corner_col + R_corner_col) / 2;
                                }
                                else if (L_corner_flag && R_corner_flag == 0)
                                {
                                    if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || End_Line_col - L_corner_col < 20)
                                        mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                                    else
                                        mid_col = End_Line_col;
                                }
                                else if (L_corner_flag == 0 && R_corner_flag)
                                {
                                    if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_col - End_Line_col < 20)
                                        mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                                    else
                                        mid_col = End_Line_col;
                                }
                                else
                                {
                                    int coll = get_maxline(0);
                                    int colr = get_maxline(1);
                                    if (End_Line_col > coll + 2 && End_Line_col < colr - 2)
                                        mid_col = End_Line_col;
                                    else
                                        mid_col = (coll + colr) / 2;
                                }
                                bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, IMAGE_WIDTH - 1, 1);
                            }
                            else
                                bianli(buxian_crossr_row + 15, buxian_crossr_row - 15, buxian_crossr_col - 20, IMAGE_WIDTH - 1, 1);
                            for (uint8 i = 5; i < my_r_count - 3; i++)
                            {
                                if (R_edge[i - 3].col - R_edge[i].col > 20 && myabs_Int(R_edge[i].col - R_edge[i + 2].col) < 10)
                                {
                                    if (R_edge[i + 3].row < IMAGE_HEIGHT - 10)
                                    {
                                        buxian_crossr_col = R_edge[i + 1].col;
                                        buxian_crossr_row = R_edge[i + 1].row;
                                        rc_flag = 1;
                                        break;
                                    }
                                }
                            }
                            if (rc_flag)
                            {
                                if (right_effect_row_start >= IMAGE_HEIGHT - 15)
                                    ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
                                else
                                    ADD_Edge_Line(_right, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);

                                if (buxian_crossr_row > End_Line + 1)
                                {
                                    for (int j = buxian_crossr_row; j >= End_Line; j--)
                                    {
                                        if (myabs_Int(current_right_col[j] - current_right_col[j - 1]) > 8)
                                        {
                                            uint8 find_success_flag = 0;
                                            if (arr[j - 1][current_right_col[j]] == Black && End_Line_col < current_right_col[j])
                                            {
                                                for (uint8 i = current_right_col[j]; i > End_Line_col; i--)
                                                {
                                                    if (arr[j - 1][i] == Black && arr[j - 1][i - 1] == White)
                                                    {
                                                        current_right_col[j - 1] = i;
                                                        find_success_flag = 1;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (arr[j - 1][current_right_col[j]] == White && current_right_col[j] < IMAGE_WIDTH - 30)
                                            {
                                                for (uint8 i = current_right_col[j]; i < current_right_col[j] + 20; i++)
                                                {
                                                    if (arr[j - 1][i] == White && arr[j - 1][i + 1] == Black)
                                                    {
                                                        current_right_col[j - 1] = i + 1;
                                                        find_success_flag = 1;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == Black)
                                                current_right_col[j - 1] = current_right_col[j] - 1;
                                        }
                                        right_effect_row_end = j;
                                    }
                                    if (R_corner_flag && R_corner_angle < 91 && buxian_crossr_row > R_corner_row && R_corner_col < buxian_crossr_col)
                                        ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                                }
                            }
                            else
                                right_findflag = 0;
                        }
                        if (lc_flag)
                            bx_lastl_exist_flag = 1;
                        else
                            bx_lastl_exist_flag = 0;
                        if (rc_flag)
                            bx_lastr_exist_flag = 1;
                        else
                            bx_lastr_exist_flag = 0;
                        if (lc_flag && rc_flag && buxian_crossr_col < buxian_crossl_col && buxian_crossr_row < IMAGE_HEIGHT - 15 && buxian_crossl_row < IMAGE_HEIGHT - 15) // 异常情况，左拐点在有拐点右侧
                        {
                            bx_lastl_exist_flag = 0;
                            bx_lastr_exist_flag = 0;
                            left_findflag = 0;
                            right_findflag = 0;
                        }
                        if (lc_flag && rc_flag && buxian_crossr_col - buxian_crossl_col > straightwidth[(buxian_crossr_row + buxian_crossl_row) / 2] * 1.4) // 异常情况，补线宽度严重不对，不符合直道的赛道宽度
                        {
                            bx_lastl_exist_flag = 0;
                            bx_lastr_exist_flag = 0;
                            left_findflag = 0;
                            right_findflag = 0;
                        }
                    }
                    else //if(left_effect_row_end >= IMAGE_HEIGHT - 25 || right_effect_row_end >= IMAGE_HEIGHT - 25)
                    {
                        cross_cnt = 2;
                    }
                }
                else if (cross_gesture == 1)
                {
                    if (R_corner_flag && R_corner_row < IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10)
                    {
                        if (left_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10 && left_effect_row_end > L_corner_row - 10  && L_corner_flag && R_corner_flag && myabs_Int(L_corner_row - R_corner_row) < 10 && L_corner_row > 23 && R_corner_row > 23)
                        {
                            cross_gesture = 0;
                        }
                        ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                        if (L_corner_flag && left_effect_row_start > IMAGE_HEIGHT - 10)
                            ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                        else
                        {
                            // 修复左线回返
                            if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_end < IMAGE_HEIGHT - 10)
                            {
                                for (uint8 i = left_effect_row_start - 5; i > End_Line + 3; i--)
                                {
                                    if (myabs_Int(current_left_col[i - 2] - current_left_col[i + 1]) > 8)
                                    {
                                        ADD_Edge_Line(_left, current_left_col[i + 5], i + 5, current_left_col[i + 1], i + 1, i, End_Line);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                left_findflag = 0;
                            }
                        }
                    }
                    else
                    {
                        cross_cnt = 2;
                    }
                }
                else if (cross_gesture == 2)
                {
                    if (L_corner_flag && L_corner_row < IMAGE_HEIGHT - 10 && left_effect_row_start > IMAGE_HEIGHT - 10)
                    {
                        if (left_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_start > IMAGE_HEIGHT - 10 &&  right_effect_row_end > R_corner_row - 10 && L_corner_flag && R_corner_flag && myabs_Int(L_corner_row - R_corner_row) < 10 && L_corner_row > 18 && R_corner_row > 18)
                        {
                            cross_gesture = 0;
                        }
                        ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                        if (R_corner_flag && right_effect_row_start > IMAGE_HEIGHT - 10)
                            ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                        else
                        {
                            if (right_effect_row_start > IMAGE_HEIGHT - 10 && right_effect_row_end < IMAGE_HEIGHT - 10)
                            {
                                for (uint8 i = right_effect_row_start - 5; i > End_Line + 3; i--)
                                {
                                    if (myabs_Int(current_right_col[i - 2] - current_right_col[i + 1]) > 8)
                                    {
                                        ADD_Edge_Line(_right, current_right_col[i + 5], i + 5, current_right_col[i + 1], i + 1, i, End_Line);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                right_findflag = 0;
                            }
                        }
                    }
                    else
                    {
                        cross_cnt = 2;
                    }
                }
            }
        }
    }

int diu=0;
int shi_end_flag=0;
int shi_tioachuflag=0;
// void shizi_two()
// {
//     diu=0;
//     shi_end_flag=0;
//     //进行丢线检测，防止过早跳出
//     for(int i=50;i>35;i--)
//     {
//         if(current_right_col[i]==148)
//         {
//             diu++;
//         }
//         if(current_left_col[i]==1)
//         {
//                 diu++;
//         }
//         if(diu>20)
//             shi_tioachuflag = 1;
//     }
//     if(shi_tioachuflag==1 && diu<10)
//         shi_end_flag=1;
//     if (cross_cnt == 2)
//     {
//             if (shi_end_flag && left_effect_row_start >= IMAGE_HEIGHT - 4 && right_effect_row_start >= IMAGE_HEIGHT - 4 && (left_effect_row_end < IMAGE_HEIGHT - 25 || (L_corner_flag && L_corner_angle < 91)) && (right_effect_row_end < IMAGE_HEIGHT - 25 || (R_corner_flag && R_corner_angle < 91)))
//             {
//                 element = 0;
//                 enable_element_search_flag = 1;
//                 cross_cnt = 0;
//                 cross_gesture = 0;
//                 crossl_end_flag = 0;
//                 crossr_end_flag = 0;
//                 shi_tioachuflag = 0;
//                 return;
//             }
//             if (shi_end_flag&&left_effect_row_start >= IMAGE_HEIGHT - 4 && ((L_corner_flag && L_corner_row < 15) || (left_effect_row_end < 15 && L_corner_flag == 0 && L_corner_row == 0)))
//             {
//                 element = 0;
//                 enable_element_search_flag = 1;
//                 cross_cnt = 0;
//                 cross_gesture = 0;
//                 crossl_end_flag = 0;
//                 crossr_end_flag = 0;
//                 shi_tioachuflag = 0;
//                 return;
//             }
//             if (shi_end_flag&&right_effect_row_start >= IMAGE_HEIGHT - 4 && ((R_corner_flag && R_corner_row < 15) || (right_effect_row_end < 15 && R_corner_flag == 0 && R_corner_row == 0)))
//             {
//                 element = 0;
//                 enable_element_search_flag = 1;
//                 cross_cnt = 0;
//                 cross_gesture = 0;
//                 crossl_end_flag = 0;
//                 crossr_end_flag = 0;
//                 shi_tioachuflag = 0;
//                 return;
//             }
//             if (shi_end_flag&&End_Line > 25 && right_effect_row_start > IMAGE_HEIGHT - 5 && left_effect_row_start > IMAGE_HEIGHT - 5 && right_effect_row_end == End_Line + 1 && left_effect_row_end == End_Line + 1 && L_corner_flag == 0 && R_corner_flag == 0)
//             {
//                 element = 0;
//                 enable_element_search_flag = 1;
//                 cross_cnt = 0;
//                 cross_gesture = 0;
//                 crossl_end_flag = 0;
//                 crossr_end_flag = 0;
//                 shi_tioachuflag = 0;
//                 return;
//             }
//         // 寻找上拐点
//         lc_flag = 0, rc_flag = 0;
//         if (bx_lastl_exist_flag == 0)
//         {
//             int mid_col = 0;
//             if (L_corner_flag && R_corner_flag)
//             {
//                 if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
//                 {
//                     if (cross_gesture == 1)
//                         mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                     else if (cross_gesture == 2)
//                         mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                     else
//                         mid_col = End_Line_col;
//                 }
//                 else
//                     mid_col = (L_corner_col + R_corner_col) / 2;
//             }
//             else if (L_corner_flag && R_corner_flag == 0)
//             {
//                 if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || L_corner_row > IMAGE_HEIGHT - 20)
//                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//                 if (cross_gesture == 2)
//                 {
//                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                 }
//             }
//             else if (L_corner_flag == 0 && R_corner_flag)
//             {
//                 if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_row > IMAGE_HEIGHT - 20)
//                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//                 if (cross_gesture == 1)
//                 {
//                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                 }
//             }
//             else
//             {
//                 if (bx_lastr_exist_flag)
//                     mid_col = buxian_crossr_col - straightwidth[buxian_crossr_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//             }
//             bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, 0, 0);
//         }
//         else
//             bianli(buxian_crossl_row + 30, buxian_crossl_row - 10, buxian_crossl_col + 20, 0, 0);
//         for (uint8 i = 5; i < my_l_count - 3; i++)
//         {
//             if (my_L_edge[i].x - my_L_edge[i - 3].x > 10 && myabs_Int(my_L_edge[i + 2].x - my_L_edge[i].x) < 10)
//             {
//                 if (my_L_edge[i + 1].y < IMAGE_HEIGHT - 10 )
//                 {
//                     buxian_crossl_col = my_L_edge[i + 1].x;
//                     buxian_crossl_row = my_L_edge[i + 1].y;
//                     lc_flag = 1;
//                     break;
//                 }
//             }
//         }
//         if (bx_lastr_exist_flag == 0)
//         {
//             int mid_col = 0;
//             if (L_corner_flag && R_corner_flag)
//             {
//                 if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
//                 {
//                     if (cross_gesture == 1)
//                         mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                     else if (cross_gesture == 2)
//                         mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                     else
//                         mid_col = End_Line_col;
//                 }
//                 else
//                     mid_col = (L_corner_col + R_corner_col) / 2;
//             }
//             else if (L_corner_flag && R_corner_flag == 0)
//             {
//                 if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || L_corner_row > IMAGE_HEIGHT - 20)
//                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//                 if (cross_gesture == 2)
//                 {
//                     mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
//                 }
//             }
//             else if (L_corner_flag == 0 && R_corner_flag)
//             {
//                 if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_row > IMAGE_HEIGHT - 20)
//                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//                 if (cross_gesture == 1)
//                 {
//                     mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
//                 }
//             }
//             else
//             {
//                 if (bx_lastl_exist_flag)
//                     mid_col = buxian_crossl_col + straightwidth[buxian_crossl_row] / 2;
//                 else
//                     mid_col = End_Line_col;
//             }
//             bianli(IMAGE_HEIGHT - 1, End_Line+1, mid_col, IMAGE_WIDTH - 1, 1);
//         }
//         else
//             bianli(buxian_crossr_row + 15, buxian_crossr_row - 15, buxian_crossr_col - 20, IMAGE_WIDTH - 1, 1);
//         for (uint8 i = 5; i < my_r_count - 3; i++)
//         {
//             if (my_R_edge[i - 3].x - my_R_edge[i].x > 10 && myabs_Int(my_R_edge[i].x - my_R_edge[i + 2].x) < 10)
//             {
//                 if (my_R_edge[i + 3].y < IMAGE_HEIGHT - 10 )
//                 {
//                     buxian_crossr_col = my_R_edge[i + 1].x;
//                     buxian_crossr_row = my_R_edge[i + 1].y;
//                     rc_flag = 1;
//                     break;
//                 }
//             }
//         }
//         if (lc_flag)
//         {
//             // if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_start > buxian_crossl_row + 3)
//             //     ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
//             // else
//             //     ADD_Edge_Line(_left, 2, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);
//             //修改开始=================================================================
//             // ====== 完全恢复你原来的神仙逻辑，删掉 start_r ======
//             if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_start > buxian_crossl_row + 3)
//                 ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
//             else
//                 ADD_Edge_Line(_left, 2, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);


//             //修改结束=================================================================
//             if (buxian_crossl_row > End_Line + 1)
//             {
//                 for (int j = buxian_crossl_row; j >= End_Line; j--)
//                 {
//                     if (myabs_Int(current_left_col[j] - current_left_col[j - 1]) > 8)
//                     {
//                         uint8 find_success_flag = 0;
//                         if (arr[j - 1][current_left_col[j]] == Black && End_Line_col > current_left_col[j])
//                         {
//                             for (uint8 i = current_left_col[j]; i < End_Line_col; i++)
//                             {
//                                 if (arr[j - 1][i] == Black && arr[j - 1][i + 1] == White)
//                                 {
//                                     current_left_col[j - 1] = i;
//                                     break;
//                                 }
//                             }
//                         }
//                         if (arr[j - 1][current_left_col[j]] == White && current_left_col[j] > 30)
//                         {
//                             for (uint8 i = current_left_col[j]; i > current_left_col[j] - 20; i--)
//                             {
//                                 if (arr[j - 1][i] == White && arr[j - 1][i - 1] == Black)
//                                 {
//                                     current_left_col[j - 1] = i - 1;
//                                     break;
//                                 }
//                             }
//                         }
//                         if (find_success_flag == 0 && arr[j - 1][current_left_col[j]] == Black)
//                             current_left_col[j - 1] = current_left_col[j] + 1;
//                     }
//                     left_effect_row_end = j;
//                 }
//                 if (L_corner_flag && L_corner_angle < 91 && buxian_crossl_row > L_corner_row && L_corner_col > buxian_crossl_col)
//                     ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
//             }
//         }
//         //---------------------
//         else
//         {
//             left_findflag = 0; 
//             // ====== 新增：防跳动兜底 ======
//             // 哪怕黑岛丢了，也从左下角连到视野最远端，死死盖住垂直墙壁！
//             ADD_Edge_Line(_left, 2, IMAGE_HEIGHT - 1, current_left_col[End_Line], End_Line, IMAGE_HEIGHT - 1, End_Line);
//         }
//         //--------------------

//         if (rc_flag==1)
//         {
//             // if (right_effect_row_start >= IMAGE_HEIGHT - 10 && right_effect_row_start > buxian_crossr_row + 3)
//             //     ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
//             // else
//             //     ADD_Edge_Line(_right, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);
//             //修改开始================================================================= 
//             // ====== 完全恢复你原来的神仙逻辑，删掉 start_r ======
//             if (right_effect_row_start >= IMAGE_HEIGHT - 10 && right_effect_row_start > buxian_crossr_row + 3)
//                 ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
//             else
//                 ADD_Edge_Line(_right, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);


//             //修改结束=====================================================================
//             if (buxian_crossr_row > End_Line + 1)
//             {
//                     for (int j = buxian_crossr_row; j >= End_Line; j--)
//                 {
//                     if (myabs_Int(current_right_col[j] - current_right_col[j - 1]) > 8)
//                     {
//                         uint8 find_success_flag = 0;
//                         if (arr[j - 1][current_right_col[j]] == Black && End_Line_col < current_right_col[j])
//                         {
//                             for (uint8 i = current_right_col[j]; i > End_Line_col; i--)
//                             {
//                                 if (arr[j - 1][i] == Black && arr[j - 1][i - 1] == White)
//                                 {
//                                     current_right_col[j - 1] = i;
//                                     break;
//                                 }
//                             }
//                         }
//                         if (arr[j - 1][current_right_col[j]] == White && current_right_col[j] < IMAGE_WIDTH - 30)
//                         {
//                             for (uint8 i = current_right_col[j]; i < current_right_col[j] + 20; i++)
//                             {
//                                 if (arr[j - 1][i] == White && arr[j - 1][i + 1] == Black)
//                                 {
//                                     current_right_col[j - 1] = i + 1;
//                                     break;
//                                 }
//                             }
//                         }
//                         if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == Black)
//                             current_right_col[j - 1] = current_right_col[j] - 1;
//                         else if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == White)
//                             current_right_col[j - 1] = current_right_col[j];
//                     }
//                     right_effect_row_end = j;
//                 }
//                 if (R_corner_flag && R_corner_angle < 91 && buxian_crossr_row > R_corner_row && R_corner_col < buxian_crossr_col)
//                     ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
//             }
//         }
//         //-----------
//         else
//         {
//             right_findflag = 0; 
//             // ====== 新增：防跳动兜底 ======
//             ADD_Edge_Line(_right, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, current_right_col[End_Line], End_Line, IMAGE_HEIGHT - 1, End_Line);
//         }
//         //-----------------
//         if (lc_flag)
//             bx_lastl_exist_flag = 1;
//         else
//             bx_lastl_exist_flag = 0;
//         if (rc_flag)
//             bx_lastr_exist_flag = 1;
//         else
//             bx_lastr_exist_flag = 0;
//         if (lc_flag && rc_flag && buxian_crossr_col < buxian_crossl_col) // 异常情况，左拐点在有拐点右侧
//             {
//                 bx_lastl_exist_flag = 0;
//                 bx_lastr_exist_flag = 0;
//                 left_findflag = 0;
//                 right_findflag = 0;
//             }
//             if (lc_flag && rc_flag && buxian_crossr_col - buxian_crossl_col > straightwidth[(buxian_crossr_row + buxian_crossl_row) / 2] * 1.4) // 异常情况，补线宽度严重不对，不符合直道的赛道宽度
//             {
//                 bx_lastl_exist_flag = 0;
//                 bx_lastr_exist_flag = 0;
//                 left_findflag = 0;
//                 right_findflag = 0;
//             }
//             if (bx_lastl_exist_flag && buxian_crossl_row > IMAGE_HEIGHT - 15)
//             {
//                 crossl_end_flag = 1;
//             }
//             if (bx_lastr_exist_flag && buxian_crossr_row > IMAGE_HEIGHT - 15)
//             {
//                 crossr_end_flag = 1;
//             }
//             if (crossl_end_flag && buxian_crossl_row < IMAGE_HEIGHT - 20)
//                 left_findflag = 0;
//             if (crossr_end_flag && buxian_crossr_row < IMAGE_HEIGHT - 20)
//                 right_findflag = 0;
//             if (left_findflag && right_findflag == 0)
//             {
//                 ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, 2, IMAGE_HEIGHT - 1, IMAGE_HEIGHT - 1, buxian_crossl_row);
//             }
//             else if (left_findflag == 0 && right_findflag)
//             {
//                 ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, IMAGE_HEIGHT - 1, buxian_crossr_row);
//             }
//         }
// }
void shizi_two()
        {
            diu=0;
            shi_end_flag=0;
            //进行丢线检测，防止过早跳出
            for(int i=50;i>35;i--)
            {
                if(current_right_col[i]==148)
                {
                    diu++;
                }
                if(current_left_col[i]==1)
                {
                     diu++;
                }
                if(diu>20)
                    shi_tioachuflag = 1;
            }
            if(shi_tioachuflag==1 && diu<10)
                shi_end_flag=1;
            if (cross_cnt == 2)
            {
                    if (shi_end_flag && left_effect_row_start >= IMAGE_HEIGHT - 4 && right_effect_row_start >= IMAGE_HEIGHT - 4 && (left_effect_row_end < IMAGE_HEIGHT - 25 || (L_corner_flag && L_corner_angle < 91)) && (right_effect_row_end < IMAGE_HEIGHT - 25 || (R_corner_flag && R_corner_angle < 91)))
                    {
                        element = 0;
                        enable_element_search_flag = 1;
                        cross_cnt = 0;
                        cross_gesture = 0;
                        crossl_end_flag = 0;
                        crossr_end_flag = 0;
                        shi_tioachuflag = 0;
                        return;
                    }
                    if (shi_end_flag&&left_effect_row_start >= IMAGE_HEIGHT - 4 && ((L_corner_flag && L_corner_row < 15) || (left_effect_row_end < 15 && L_corner_flag == 0 && L_corner_row == 0)))
                    {
                        element = 0;
                        enable_element_search_flag = 1;
                        cross_cnt = 0;
                        cross_gesture = 0;
                        crossl_end_flag = 0;
                        crossr_end_flag = 0;
                        shi_tioachuflag = 0;
                        return;
                    }
                    if (shi_end_flag&&right_effect_row_start >= IMAGE_HEIGHT - 4 && ((R_corner_flag && R_corner_row < 15) || (right_effect_row_end < 15 && R_corner_flag == 0 && R_corner_row == 0)))
                    {
                        element = 0;
                        enable_element_search_flag = 1;
                        cross_cnt = 0;
                        cross_gesture = 0;
                        crossl_end_flag = 0;
                        crossr_end_flag = 0;
                        shi_tioachuflag = 0;
                        return;
                    }
                    if (shi_end_flag&&End_Line > 25 && right_effect_row_start > IMAGE_HEIGHT - 5 && left_effect_row_start > IMAGE_HEIGHT - 5 && right_effect_row_end == End_Line + 1 && left_effect_row_end == End_Line + 1 && L_corner_flag == 0 && R_corner_flag == 0)
                    {
                        element = 0;
                        enable_element_search_flag = 1;
                        cross_cnt = 0;
                        cross_gesture = 0;
                        crossl_end_flag = 0;
                        crossr_end_flag = 0;
                        shi_tioachuflag = 0;
                        return;
                    }
                // 寻找上拐点
                lc_flag = 0, rc_flag = 0;
                if (bx_lastl_exist_flag == 0)
                {
                    int mid_col = 0;
                    if (L_corner_flag && R_corner_flag)
                    {
                        if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
                        {
                            if (cross_gesture == 1)
                                mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                            else if (cross_gesture == 2)
                                mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                            else
                                mid_col = End_Line_col;
                        }
                        else
                            mid_col = (L_corner_col + R_corner_col) / 2;
                    }
                  else if (L_corner_flag && R_corner_flag == 0)
                    {
                        if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || L_corner_row > IMAGE_HEIGHT - 20)
                            mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                        else
                            mid_col = End_Line_col;
                        if (cross_gesture == 2)
                        {
                            mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                        }
                    }
                    else if (L_corner_flag == 0 && R_corner_flag)
                    {
                        if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_row > IMAGE_HEIGHT - 20)
                            mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                        else
                            mid_col = End_Line_col;
                        if (cross_gesture == 1)
                        {
                            mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                        }
                    }
                    else
                    {
                        if (bx_lastr_exist_flag)
                            mid_col = buxian_crossr_col - straightwidth[buxian_crossr_row] / 2;
                        else
                            mid_col = End_Line_col;
                    }
                    bianli(IMAGE_HEIGHT - 2, End_Line+1, mid_col, 0, 0);
                }
                else
                    bianli(buxian_crossl_row + 30, buxian_crossl_row - 10, buxian_crossl_col + 20, 0, 0);
                for (uint8 i = 5; i < my_l_count - 3; i++)
                {
                    if (my_L_edge[i].x - my_L_edge[i - 3].x > 10 && myabs_Int(my_L_edge[i + 2].x - my_L_edge[i].x) < 10)
                    {
                        if (my_L_edge[i + 1].y < IMAGE_HEIGHT - 10 )
                        {
                            buxian_crossl_col = my_L_edge[i + 1].x;
                            buxian_crossl_row = my_L_edge[i + 1].y;
                            lc_flag = 1;
                            break;
                        }
                    }
                }
                if (bx_lastr_exist_flag == 0)
                {
                    int mid_col = 0;
                    if (L_corner_flag && R_corner_flag)
                    {
                        if (End_Line_col > L_corner_col && End_Line_col < R_corner_col)
                        {
                            if (cross_gesture == 1)
                                mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                            else if (cross_gesture == 2)
                                mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                            else
                                mid_col = End_Line_col;
                        }
                        else
                            mid_col = (L_corner_col + R_corner_col) / 2;
                    }
                    else if (L_corner_flag && R_corner_flag == 0)
                    {
                        if (End_Line_col < L_corner_col || End_Line_col > L_corner_col + straightwidth[L_corner_row] || L_corner_row > IMAGE_HEIGHT - 20)
                            mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                        else
                            mid_col = End_Line_col;
                        if (cross_gesture == 2)
                        {
                            mid_col = L_corner_col + straightwidth[L_corner_row] / 2;
                        }
                    }
                    else if (L_corner_flag == 0 && R_corner_flag)
                    {
                        if (End_Line_col > R_corner_col || End_Line_col < R_corner_col - straightwidth[R_corner_row] || R_corner_row > IMAGE_HEIGHT - 20)
                            mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                        else
                            mid_col = End_Line_col;
                        if (cross_gesture == 1)
                        {
                            mid_col = R_corner_col - straightwidth[R_corner_row] / 2;
                        }
                    }
                    else
                    {
                        if (bx_lastl_exist_flag)
                            mid_col = buxian_crossl_col + straightwidth[buxian_crossl_row] / 2;
                        else
                            mid_col = End_Line_col;
                    }
                    bianli(IMAGE_HEIGHT - 1, End_Line+1, mid_col, IMAGE_WIDTH - 1, 1);
                }
                else
                    bianli(buxian_crossr_row + 15, buxian_crossr_row - 15, buxian_crossr_col - 20, IMAGE_WIDTH - 1, 1);
                for (uint8 i = 5; i < my_r_count - 3; i++)
                {
                    if (my_R_edge[i - 3].x - my_R_edge[i].x > 10 && myabs_Int(my_R_edge[i].x - my_R_edge[i + 2].x) < 10)
                    {
                        if (my_R_edge[i + 3].y < IMAGE_HEIGHT - 10 )
                        {
                            buxian_crossr_col = my_R_edge[i + 1].x;
                            buxian_crossr_row = my_R_edge[i + 1].y;
                            rc_flag = 1;
                            break;
                        }
                    }
                }
                if (lc_flag)
                {
                    if (left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_start > buxian_crossl_row + 3)
                        ADD_Edge_Line(_left, current_left_col[left_effect_row_start], left_effect_row_start, buxian_crossl_col, buxian_crossl_row, left_effect_row_start, buxian_crossl_row);
                    else
                        ADD_Edge_Line(_left, 2, IMAGE_HEIGHT - 1, buxian_crossl_col, buxian_crossl_row, IMAGE_HEIGHT - 1, buxian_crossl_row);

                   if (buxian_crossl_row > End_Line + 1)
                    {
                        for (int j = buxian_crossl_row; j >= End_Line; j--)
                        {
                            if (myabs_Int(current_left_col[j] - current_left_col[j - 1]) > 8)
                            {
                                uint8 find_success_flag = 0;
                                if (arr[j - 1][current_left_col[j]] == Black && End_Line_col > current_left_col[j])
                                {
                                    for (uint8 i = current_left_col[j]; i < End_Line_col; i++)
                                    {
                                        if (arr[j - 1][i] == Black && arr[j - 1][i + 1] == White)
                                        {
                                            current_left_col[j - 1] = i;
                                            break;
                                        }
                                    }
                                }
                                if (arr[j - 1][current_left_col[j]] == White && current_left_col[j] > 30)
                                {
                                    for (uint8 i = current_left_col[j]; i > current_left_col[j] - 20; i--)
                                    {
                                        if (arr[j - 1][i] == White && arr[j - 1][i - 1] == Black)
                                        {
                                            current_left_col[j - 1] = i - 1;
                                            break;
                                        }
                                    }
                                }
                                if (find_success_flag == 0 && arr[j - 1][current_left_col[j]] == Black)
                                    current_left_col[j - 1] = current_left_col[j] + 1;
                            }
                            left_effect_row_end = j;
                        }
                        if (L_corner_flag && L_corner_angle < 91 && buxian_crossl_row > L_corner_row && L_corner_col > buxian_crossl_col)
                            ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, L_corner_col, L_corner_row, L_corner_row - 1, End_Line);
                    }
                }

                if (rc_flag==1)
                {
                    if (right_effect_row_start >= IMAGE_HEIGHT - 10 && right_effect_row_start > buxian_crossr_row + 3)
                        ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, buxian_crossr_col, buxian_crossr_row, right_effect_row_start, buxian_crossr_row);
                    else
                        ADD_Edge_Line(_right, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, buxian_crossr_col, buxian_crossr_row, IMAGE_HEIGHT - 1, buxian_crossr_row);

                    if (buxian_crossr_row > End_Line + 1)
                    {
                         for (int j = buxian_crossr_row; j >= End_Line; j--)
                        {
                            if (myabs_Int(current_right_col[j] - current_right_col[j - 1]) > 8)
                            {
                                uint8 find_success_flag = 0;
                                if (arr[j - 1][current_right_col[j]] == Black && End_Line_col < current_right_col[j])
                                {
                                    for (uint8 i = current_right_col[j]; i > End_Line_col; i--)
                                    {
                                        if (arr[j - 1][i] == Black && arr[j - 1][i - 1] == White)
                                        {
                                            current_right_col[j - 1] = i;
                                            break;
                                        }
                                    }
                                }
                                if (arr[j - 1][current_right_col[j]] == White && current_right_col[j] < IMAGE_WIDTH - 30)
                                {
                                    for (uint8 i = current_right_col[j]; i < current_right_col[j] + 20; i++)
                                    {
                                        if (arr[j - 1][i] == White && arr[j - 1][i + 1] == Black)
                                        {
                                            current_right_col[j - 1] = i + 1;
                                            break;
                                        }
                                    }
                                }
                                if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == Black)
                                    current_right_col[j - 1] = current_right_col[j] - 1;
                                else if (find_success_flag == 0 && arr[j - 1][current_right_col[j]] == White)
                                    current_right_col[j - 1] = current_right_col[j];
                            }
                            right_effect_row_end = j;
                        }
                        if (R_corner_flag && R_corner_angle < 91 && buxian_crossr_row > R_corner_row && R_corner_col < buxian_crossr_col)
                            ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                    }
                }

                if (lc_flag)
                    bx_lastl_exist_flag = 1;
                else
                    bx_lastl_exist_flag = 0;
                if (rc_flag)
                    bx_lastr_exist_flag = 1;
                else
                    bx_lastr_exist_flag = 0;
                if (lc_flag && rc_flag && buxian_crossr_col < buxian_crossl_col) // 异常情况，左拐点在有拐点右侧
                 {
                     bx_lastl_exist_flag = 0;
                     bx_lastr_exist_flag = 0;
                     left_findflag = 0;
                     right_findflag = 0;
                 }
                 if (lc_flag && rc_flag && buxian_crossr_col - buxian_crossl_col > straightwidth[(buxian_crossr_row + buxian_crossl_row) / 2] * 1.4) // 异常情况，补线宽度严重不对，不符合直道的赛道宽度
                 {
                     bx_lastl_exist_flag = 0;
                     bx_lastr_exist_flag = 0;
                     left_findflag = 0;
                     right_findflag = 0;
                 }
                 if (bx_lastl_exist_flag && buxian_crossl_row > IMAGE_HEIGHT - 15)
                 {
                     crossl_end_flag = 1;
                 }
                 if (bx_lastr_exist_flag && buxian_crossr_row > IMAGE_HEIGHT - 15)
                 {
                     crossr_end_flag = 1;
                 }
                 if (crossl_end_flag && buxian_crossl_row < IMAGE_HEIGHT - 20)
                     left_findflag = 0;
                 if (crossr_end_flag && buxian_crossr_row < IMAGE_HEIGHT - 20)
                     right_findflag = 0;
                 if (left_findflag && right_findflag == 0)
                 {
                     ADD_Edge_Line(_left, buxian_crossl_col, buxian_crossl_row, 2, IMAGE_HEIGHT - 1, IMAGE_HEIGHT - 1, buxian_crossl_row);
                 }
                 else if (left_findflag == 0 && right_findflag)
                 {
                     ADD_Edge_Line(_right, buxian_crossr_col, buxian_crossr_row, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 1, IMAGE_HEIGHT - 1, buxian_crossr_row);
                 }
             }
          }


int linewideth[IMAGE_HEIGHT];
int centralline[IMAGE_HEIGHT];
void reNiHe()
{
    uint8 i=0;
    // 遍历行：从第1行到UVC_HEIGHT-1行（注意i从1开始，跳过第0行）
    for(i=1;i<=IMAGE_HEIGHT-1;i++)
    {
        // 1. 计算初始中心线（左右列的平均值）和行宽度（左右列的差值）
        centralline[i]=(current_left_col[i]+current_right_col[i])/2;
        linewideth[i]=(current_right_col[i]-current_left_col[i]);

        // 2. 根据光晕标志修正中心线位置
        // 右光晕标志=1时，中心线取左列 + 直宽度/2
        if(right_halo_flag==1 )
            centralline[i]=current_left_col[i]+straightwidth[i]/2 ;
        // 左光晕标志=1时，中心线取右列 - 直宽度/2
        if(left_halo_flag==1 )
            centralline[i]=current_right_col[i]-straightwidth[i]/2 ; // 注：原代码此处写的是current_right_col，疑似笔误？
        // 右光晕标志=6时，同标志1的修正逻辑（注释里有未启用的条件）
        if(right_halo_flag==6 )//&& halo_six_right_row==0
            centralline[i]=current_left_col[i]+straightwidth[i]/2  ;
        // 左光晕标志=6时，同标志1的修正逻辑
        if(left_halo_flag==6 )//&& halo_six_left_row==0
            centralline[i]=current_right_col[i]-straightwidth[i]/2  ;

        // 左圆环状态5：中线 = 左边界 + 2/5 标定赛道宽
        if(left_halo_flag==5)
            centralline[i]=current_left_col[i]+straightwidth[i]*2/5;
        // 右圆环状态5：中线 = 右边界 - 2/5 标定赛道宽
        if(right_halo_flag==5)
            centralline[i]=current_right_col[i]-straightwidth[i]*2/5;

        if(brick_dis_isp == 1) // 🚨 红砖避障补线：中线贴到远离砖的一侧(距该侧边界 1/3 赛道宽)
        {
            // 用真实赛道宽度(兜底 straightwidth)算 1/3
            int track_width = straightwidth[i];
            // if(track_width < 10) track_width = straightwidth[i]; // 兜底保护

            if(Brick_pos_flag == 1)       // 🧱 左红砖，向右避：中线 = 右边界 - 1/3 赛道宽
            {
                centralline[i] = current_right_col[i] - track_width / 4;
            }
            else if(Brick_pos_flag == 2)  // 🧱 右红砖，向左避：中线 = 左边界 + 1/3 赛道宽
            {
                centralline[i] = current_left_col[i] + track_width / 4;
            }
        }

        //圆环中靠近中线处理
        // if(left_halo_flag>=2&&left_halo_flag<=5)
        // {
        //     centralline[i]=centralline[i]*0.6;
        // }
        // if(right_halo_flag>=2&&right_halo_flag<=5)
        // {
        //     centralline[i]=centralline[i]*1.2;
        // }
        // 3. 限制中心线的取值范围，避免越界
        if(centralline[i]<1)
        {
            centralline[i]=1;
        }
        if(centralline[i]>IMAGE_WIDTH - 2)
         {
             centralline[i]=IMAGE_WIDTH - 2;
         }
    }
}
//-------------------------------------------------------------------------------------------------------------------
//  @brief      搜索边线并获取赛道中线信息
//  @param      void
//  @return     void
//  @message
//  Sample usage:
//-------------------------------------------------------------------------------------------------------------------
int re_flag;  // 全局变量，未在该函数中使用
int DIFFERROW = 0;
int Aim_Line = IMAGE_HEIGHT - 5;
int turn_propect = 0;//前瞻行

float col_Differ;
float Last_col_Differ;
/**********************************************************************
 * 函数名称：get_track_bias
 * 功能描述：根据指定的前瞻行，计算赛道中心线偏差值（用于循迹转向控制）
 * 输入参数：
 *      const uint8 image[IMAGE_HEIGHT][IMAGE_WIDTH]  —— 图像数组（未使用，保留接口）
 *      uint8 forward_row                              —— 【输入】前瞻行号（看第几行）
 * 输出参数：无
 * 返回值：无
 * 全局输出：col_Differ —— 最终赛道偏差（正数=车在左，负数=车在右，0=居中）
 * 优化说明：全整数运算，无浮点，算力极低，适合嵌入式小车
 **********************************************************************/
void getdiffer( uint8 forward_row)
{
    // 局部变量，存储3行中心线总和
    int total_center = 0;

    // ==========================
    // 步骤1：行号安全保护（必须）
    // 防止前瞻行超出图像有效区域，避免数组越界死机
    // 如果前瞻行比结束行 End_Line 还小，就强制用 End_Line
    // ==========================
    if (forward_row < End_Line)
    {
        forward_row = End_Line;
    }
    // 上限保护：下面要读取 forward_row、forward_row+1、forward_row+2 三行，
    // 必须保证 forward_row+2 <= IMAGE_HEIGHT-1，否则会越界读 centralline[60]/[61]。
    // 所以 forward_row 最大只能到 IMAGE_HEIGHT-3。
    if (forward_row > IMAGE_HEIGHT - 3)
    {
        forward_row = IMAGE_HEIGHT - 3;
    }

    // ==========================
    // 步骤2：累加 前瞻行 + 下两行 的中心线
    // 取连续3行平均 → 平滑噪声，让转向更稳定
    // ==========================
    for (int i = forward_row; i <= forward_row + 2; i++)
    {
        // 累加第 i 行的中心线位置
        total_center += centralline[i];
    }

    // ==========================
    // 步骤3：求3行平均值（★浮点除法：亚像素分辨率）
    //   原来是整数除法 total_center/3 → col_Differ 只能取整数(1像素量化),直道上真实偏差
    //   0.3px 也会被量化成 0/1 来回跳,每跳1经方向环+角速度环放大成 ±5 的轮速目标台阶 →
    //   高频小抖动(量化极限环)。改浮点后分辨率 1/3 像素,量化台阶降到 1/3,抖动幅度随之砍掉约 2/3。
    //   注意:这只是把 col_Differ 变"细",数值本身不变(5.0 还是 5.0),不影响已调好的 Dir/Gyro 增益。
    // ==========================
    float avg_center = total_center / 3.0f;

    // ==========================
    // 步骤4：计算最终赛道偏差
    // 图像宽度 150，中心基准为 75
    // col_Differ = 0   → 居中
    // col_Differ > 0   → 偏右
    // col_Differ < 0   → 偏左
    // ==========================
    col_Differ = avg_center - 75.0f;

    // 新卡片绕行由 TURN_IN/CLEAR/MERGE/SETTLE 状态机接管。
    // bottom_narrow_flag仅保留为诊断量，不再用固定±40覆盖col_Differ，
    // 避免在平滑并线阶段突然反打，破坏虚拟中线向真实中线的连续衰减。

    // 环岛出环状态5方向保护:防止出环时偏差反向拉车。
    // 左环出环只允许负偏差;右环出环只允许正偏差。
    if (left_halo_flag == 5 && col_Differ > 0)
    {
        col_Differ = 0;
    }
    if (right_halo_flag == 5 && col_Differ < 0)
    {
        col_Differ = 0;
    }

    // ===== 绕行丢线反打(治"出界后回不来"的正反馈)——【暂时禁用,只留BNF反打】 =====
    // 绕行补线是 中线=贴边侧边界:车骑上/越过该侧边线后边界被 current_wide 夹到图像边缘
    // (左=range_left/右=range_right),col_Differ 顶到 ±极值 → 方向环朝出界方向打死,越出越狠。
    // 处理:绕行执行期+冷却期,前瞻采样的3行里贴边侧边界≥2行夹边 → 不用塌掉的中线,
    // 直接给反向定值把车拉回(与BNF反打同量级、互补:BNF靠底行夹边,这里靠前瞻行贴边侧夹边)。
    // if ((recog_staute == 1 || evade_cooldown_active == 1) && (turn_action == 0 || turn_action == 2))
    // {
    //     // 反打量用 image.hpp 里的宏 LOST_COL_DIFFER(默认40,与底部窄道反打同量级)
    //     int lost_cnt = 0;
    //     for (int i = forward_row; i <= forward_row + 2; i++)
    //     {
    //         // 只看贴的那一侧是否夹到图像边缘
    //         bool row_lost = (turn_action == 0 && current_left_col[i]  <= range_left)    // 贴左,看左边界
    //                      || (turn_action == 2 && current_right_col[i] >= range_right);  // 贴右,看右边界
    //         if (row_lost) lost_cnt++;
    //     }
    //     if (lost_cnt >= 2)   // 3行里至少2行丢 → 判定贴边侧丢线(抗单行噪声)
    //     {
    //         if (turn_action == 0) col_Differ =  LOST_COL_DIFFER;   // 左绕行丢线:向右拉回
    //         else                  col_Differ = -LOST_COL_DIFFER;   // 右绕行丢线:向左拉回
    //     }
    // }
    // ===== 绕行丢线反打结束 =====

    // // ===== col_Differ 一阶低通滤波(测试时可整段注释掉，注释后即为原始值) =====
    // static float col_Differ_lpf = 0.0f;
    // const float COL_LPF_ALPHA = 0.5f;   // 0~1：越小越平滑(滞后越大)，越接近1越接近原始无滤波
    // col_Differ_lpf = COL_LPF_ALPHA * col_Differ + (1.0f - COL_LPF_ALPHA) * col_Differ_lpf;
    // col_Differ = col_Differ_lpf;
    // // ===== 滤波结束 =====
}
/*--------------------------------------------------------------------------
 * 【函数功能】：判断直道和入弯减速
 * 【参    数】：无
 * 【返 回 值】：无
 * 【备    注】：无
 *--------------------------------------------------------------------------*/
// int qulv_left = 0;
// int qulv_right = 0; 
// int zhi_flag = 0; 
// int ruwan_flag = 0;
// int bigwan_flag = 0; 
// int wd_flag = 0; 
// int ruku_cnt = 0; 
// void qulv_characteriristic()
// {
//     //    if(ruwan_flag)BEEP_ON
//     //    else    BEEP_OFF
//     if (element == 0 )
//     {
//         int qulv_saver[8] = {0};
//         int k = 0;
//         // 中线的倾斜判断
//         for (uint8 i = End_Line + 10; i < UVC_HEIGHT - 1; i += 4)
//         {
//             qulv_left = 0, qulv_right = 0;
//             for (uint8 j = i; j < i + 5; ++j)
//             {
//                 if (j == 59)
//                     break;
//                 if (centralline[j] - centralline[j + 1] > 0)
//                     qulv_right += centralline[j] - centralline[j + 1];
//                 else if (centralline[j + 1] - centralline[j] > 0)
//                     qulv_left += centralline[j + 1] - centralline[j];
//             }
//             qulv_saver[k++] = myabs_Int(qulv_right - qulv_left);
//         }
//         int sum_qulv = 0;
//         for (int i = 0; i < k; i++)
//             sum_qulv += qulv_saver[i];
//         int clo = 0, rem = 0;
//         for (int i = 0; i < k/2; i++)
//             rem += qulv_saver[i];
//         for (int i = k/2; i < k; i++)
//             clo += qulv_saver[i];
//         int max_k = myabs_Int(qulv_saver[1] - qulv_saver[0]), max_k_position = 0, max_value = qulv_saver[0], max_value_position = 0;
//         for (int i = 0; i < 5; i++)
//         {
//             if (myabs_Int(qulv_saver[i + 1] - qulv_saver[i]) > max_k)
//             {
//                 max_k = myabs_Int(qulv_saver[i + 1] - qulv_saver[i]);
//                 max_k_position = i;
//             }

//             if (qulv_saver[i + 1] > max_value)
//             {
//                 max_value = qulv_saver[i + 1];
//                 max_value_position = i + 1;
//             }
//         }
//         if (!(L_corner_flag && R_corner_flag))
//         {
//             if (End_Line >= 25 && myabs_Int(col_Differ) > 20)
//             {
//                 wd_flag = 1;
//                 zhi_flag = 0;
//                 ruwan_flag = 0;
//             }
//             else if (End_Line < 25 && clo + rem > 30 && clo >= 15)
//             {
//                 wd_flag = 1;
//                 zhi_flag = 0;
//                 ruwan_flag = 0;
//             }
//             else
//                 wd_flag = 0;
//             if (wd_flag == 0 && max_value_position <= 1 && max_value > 5 && End_Line <= 25 && clo < 20)
//             {
//                 wd_flag = 0;
//                 zhi_flag = 0;
//                 ruwan_flag = 1;
//             }
//             else
//                 ruwan_flag = 0;
//         }
//         else
//         {
//             wd_flag = 0;
//             ruwan_flag = 0;
//         }
//         if ( ruwan_flag == 0 && End_Line <= 14 && max_value <= 5 && max_k <= 3 )
//         {
//             zhi_flag = 1;
//             ruwan_flag = 0;
//             wd_flag = 0;
//         }
//         else
//             zhi_flag = 0;
//         if (element == 3)
//         {
//             wd_flag = 0;
//             if (End_Line <= 17)
//             {
//                 zhi_flag = 1;
//                 ruwan_flag = 0;
//                 wd_flag = 0;
//             }
//         }
//     }
//     else
//     {
//         ruwan_flag = 0;
//         zhi_flag = 0;
//         wd_flag = 0;
//         bigwan_flag = 0;
//         if (ruku_cnt == 2)
//             ruwan_flag = 1;
//     }
// }
int qulv_left = 0;
int qulv_right = 0;

int zhi_flag = 0;       // 直道标志
int ruwan_flag = 0;     // 入弯标志
int bigwan_flag = 0;    // 大弯标志
int wd_flag = 0;        // 弯道/弯道状态标志
int wd_flag_stable = 0; // wd_flag 去抖后的稳定值(供绕行起始/距离判定用,防单帧噪声导致绕行距离乱跳)
int ruku_cnt = 0;       // 入库计数
static int wd_stable_candidate = 0;
static int wd_stable_count = 0;

// wd_flag 去抖：只有当 wd_flag 连续 WD_STABLE_N 帧保持同一值，才更新稳定值 wd_flag_stable。
// 单帧噪声不会改变稳定值 → 解决"同一地点 wd_flag 偶发翻脸,导致 EvaD/EvaSD 选错、绕行距离时大时小"。
// 每帧在 qulv_characteriristic 之后调用一次(见 main)。
void wd_flag_debounce(void)
{
    const int WD_STABLE_N = 4;   // 连续 N 帧一致才认定(10FPS≈0.4s,可调)
    if (wd_flag == wd_stable_candidate)
    {
        if (wd_stable_count < WD_STABLE_N) wd_stable_count++;
    }
    else
    {
        wd_stable_candidate = wd_flag;
        wd_stable_count = 1;
    }
    if (wd_stable_count >= WD_STABLE_N) wd_flag_stable = wd_stable_candidate;
}


// void qulv_characteriristic()
// {
//     /*
//         函数作用：
//         通过 centralline[] 中线数组的变化趋势，判断当前赛道状态。

//         主要输出：
//         zhi_flag   = 1：认为当前是直道
//         ruwan_flag = 1：认为当前正在入弯
//         wd_flag    = 1：认为当前处于弯道 / 弯道丢线类状态

//         核心思路：
//         从 End_Line + 10 开始，往图像底部方向采样中线。
//         每隔 4 行取一个小窗口，每个窗口统计连续 5 行中线的横向变化。
//         如果中线变化小，说明比较直；
//         如果中线变化大，说明可能入弯或处于弯道。
//     */


//     /************************************************************
//      * 一、普通赛道元素处理
//      *
//      * element == 0 表示当前没有特殊元素干扰，
//      * 此时正常根据中线曲率判断直道、入弯、弯道。
//      ************************************************************/
//     if (element == 0)
//     {
//         /*
//             主逻辑开始时，先清理 bigwan_flag。

//             原代码在 element == 0 分支里面没有处理 bigwan_flag，
//             如果外部曾经把 bigwan_flag 置 1，这里可能会残留旧状态。
//         */
//         bigwan_flag = 0;


//         /*
//             QULV_MAX_SIZE 是曲率采样数组容量。

//             如果 UVC_HEIGHT = 240，
//             从 End_Line + 10 开始，每隔 4 行采样一次，
//             最坏情况下可能采样 50 多个点。

//             原来的 qulv_saver[30] 偏小，虽然有 k >= 30 的保护，
//             但是会提前截断后半段中线信息。

//             这里设置成 64，适合 240 高度。
//             如果你以后图像高度改成 320，可以改成 96 或 128。
//         */
//         const int QULV_MAX_SIZE = 64;
//         int qulv_saver[QULV_MAX_SIZE] = {0};

//         /*
//             k 表示实际采集到了多少个曲率采样点。
//         */
//         int k = 0;


//         /************************************************************
//          * 二、采集中线局部变化量
//          ************************************************************/

//         /*
//             从 End_Line + 10 开始分析。

//             End_Line 一般表示有效赛道终止行。
//             +10 是为了避开过近或不稳定区域，从更靠后的区域开始判断趋势。

//             注意：
//             原代码使用 uint8 i，这里改成 int。
//             因为 uint8 最大只有 255，如果图像高度变大，容易溢出。
//         */
//         for (int i = End_Line + 10; i < UVC_HEIGHT - 1; i += 4)
//         {
//             /*
//                 防止 qulv_saver 越界。

//                 一旦采样数量达到数组容量，就停止采样。
//             */
//             if (k >= QULV_MAX_SIZE)
//             {
//                 break;
//             }

//             /*
//                 qulv_right：
//                 统计当前 5 行窗口内，中线向一个方向变化的累计量。

//                 qulv_left：
//                 统计当前 5 行窗口内，中线向另一个方向变化的累计量。

//                 注意：
//                 这里沿用你的原始命名。
//                 实际上 left/right 的物理方向需要结合你的坐标系理解。
//             */
//             qulv_left = 0;
//             qulv_right = 0;


//             /*
//                 在当前采样点 i 附近，向下看 5 行。

//                 比较 centralline[j] 和 centralline[j + 1]。
//                 如果两行中线位置发生变化，就累计变化量。
//             */
//             for (int j = i; j < i + 5; ++j)
//             {
//                 /*
//                     访问 centralline[j + 1] 前必须保证 j + 1 不越界。
//                     所以 j 最大只能到 UVC_HEIGHT - 2。
//                 */
//                 if (j >= UVC_HEIGHT - 1)
//                 {
//                     break;
//                 }

//                 /*
//                     diff > 0：
//                     当前行中线坐标大于下一行中线坐标。

//                     diff < 0：
//                     当前行中线坐标小于下一行中线坐标。

//                     用 diff 写法比反复写：
//                     centralline[j] - centralline[j + 1]
//                     更清楚，也能减少重复计算。
//                 */
//                 int diff = centralline[j] - centralline[j + 1];

//                 if (diff > 0)
//                 {
//                     qulv_right += diff;
//                 }
//                 else if (diff < 0)
//                 {
//                     qulv_left += -diff;
//                 }
//             }

//             /*
//                 保存当前小窗口的中线变化特征。

//                 这里使用 abs(qulv_right - qulv_left)，沿用你原来的逻辑。

//                 它表示：
//                 当前 5 行窗口内，中线主要向某一侧偏移的净变化量。

//                 注意：
//                 它不是严格数学意义的曲率。
//                 它更像是“局部中线斜率强度”。

//                 如果写成：
//                     qulv_right + qulv_left
//                 那表示总摆动量，会对中线抖动更敏感。

//                 这里为了保持原算法判断习惯，不改成相加。
//             */
//             qulv_saver[k] = myabs_Int(qulv_right - qulv_left);
//             k++;
//         }


//         /************************************************************
//          * 三、没有采样数据时，直接清空状态并退出
//          ************************************************************/

//         /*
//             如果 k == 0，说明没有成功采集到有效曲率段。

//             常见原因：
//             1. End_Line 太靠近图像底部；
//             2. UVC_HEIGHT 太小；
//             3. End_Line + 10 已经超过有效分析范围。

//             这种情况下不能继续使用 qulv_saver[0]，
//             否则会出现非法逻辑。

//             同时这里清空状态，避免保留上一帧状态。
//         */
//         if (k == 0)
//         {
//             zhi_flag = 0;
//             ruwan_flag = 0;
//             wd_flag = 0;
//             bigwan_flag = 0;
//             return;
//         }


//         /************************************************************
//          * 四、统计曲率整体特征
//          ************************************************************/

//         /*
//             sum_qulv：
//             所有采样窗口的变化量总和。

//             目前后续逻辑没有直接用它，
//             但是调试时非常有价值，可以打印出来观察不同赛道状态。
//         */
//         int sum_qulv = 0;

//         for (int i = 0; i < k; i++)
//         {
//             sum_qulv += qulv_saver[i];
//         }


//         /*
//             rem：
//             前半段曲率累计值。

//             clo：
//             后半段曲率累计值。

//             你的原始代码是这样分的：
//             前半段加到 rem；
//             后半段加到 clo。

//             这里保持一致。
//         */
//         int rem = 0;
//         int clo = 0;

//         for (int i = 0; i < k / 2; i++)
//         {
//             rem += qulv_saver[i];
//         }

//         for (int i = k / 2; i < k; i++)
//         {
//             clo += qulv_saver[i];
//         }


//         /*
//             max_k：
//             相邻两个采样窗口之间的最大变化差。

//             它反映的是：
//             中线变化是否突然增大或突然减小。

//             直道情况下，max_k 一般较小；
//             入弯或弯道突变情况下，max_k 会变大。
//         */
//         int max_k = 0;

//         /*
//             max_k_position：
//             最大曲率突变出现的位置。

//             目前后续逻辑没有用到，
//             但调试时可以打印观察。
//         */
//         int max_k_position = 0;

//         if (k > 1)
//         {
//             max_k = myabs_Int(qulv_saver[1] - qulv_saver[0]);
//             max_k_position = 0;

//             for (int i = 1; i < k - 1; i++)
//             {
//                 int delta = myabs_Int(qulv_saver[i + 1] - qulv_saver[i]);

//                 if (delta > max_k)
//                 {
//                     max_k = delta;
//                     max_k_position = i;
//                 }
//             }
//         }


//         /*
//             max_value：
//             所有采样窗口中，最大的局部变化量。

//             max_value_position：
//             最大变化量出现的位置。

//             如果 max_value 很小，说明整体比较直。
//             如果 max_value 很大，说明某处中线变化明显。
//         */
//         int max_value = qulv_saver[0];
//         int max_value_position = 0;

//         for (int i = 1; i < k; i++)
//         {
//             if (qulv_saver[i] > max_value)
//             {
//                 max_value = qulv_saver[i];
//                 max_value_position = i;
//             }
//         }


//         /*
//             避免编译器在开启 -Wall 时提示变量未使用。

//             如果你后面要串口打印调试这些值，可以删掉这两行。
//         */
//         (void)sum_qulv;
//         (void)max_k_position;


//         /************************************************************
//          * 五、根据曲率特征判断弯道 / 入弯 / 直道
//          ************************************************************/

//         /*
//             如果同时检测到左右角点，
//             说明可能是十字、环岛、特殊元素等情况，
//             此时不使用普通弯道逻辑。
//         */
//         if (!(L_corner_flag && R_corner_flag))
//         {
//             /***********************
//              * 1. 弯道状态判断
//              ***********************/

//             /*
//                 条件一：
//                 End_Line >= 25 且左右列差很大。

//                 含义：
//                 有效赛道线终止位置比较靠下，
//                 同时左右边界差异明显，
//                 可能处于弯道或大偏移状态。
//             */
//             if (End_Line >= 25 && myabs_Int(col_Differ) > 20)
//             {
//                 wd_flag = 1;
//                 zhi_flag = 0;
//                 ruwan_flag = 0;
//             }

//             /*
//                 条件二：
//                 End_Line < 25，说明前方可视区域较长；
//                 但是整体曲率 clo + rem > 30，
//                 且后半段 clo >= 15。

//                 含义：
//                 虽然前方看得远，但是中线已经出现明显变化，
//                 可能进入弯道状态。
//             */
//             else if (End_Line < 25 && clo + rem > 30 && clo >= 15)
//             {
//                 wd_flag = 1;
//                 zhi_flag = 0;
//                 ruwan_flag = 0;
//             }

//             /*
//                 不满足弯道条件，则先认为不是弯道。
//             */
//             else
//             {
//                 wd_flag = 0;
//             }


//             /***********************
//              * 2. 入弯状态判断
//              ***********************/

//             /*
//                 入弯判断条件：

//                 wd_flag == 0：
//                 当前不能已经被判成弯道。

//                 max_value_position <= 1：
//                 最大局部变化出现在比较靠前的位置。
//                 说明刚开始出现明显转弯趋势。

//                 max_value > 5：
//                 局部变化量超过阈值。

//                 End_Line <= 25：
//                 前方赛道相对完整，说明不是严重丢线。

//                 clo < 20：
//                 后半段变化不要太大。
//                 如果后半段已经很大，更像已经进入弯道，而不是刚入弯。
//             */
//             if (wd_flag == 0 &&
//                 max_value_position <= 1 &&
//                 max_value > 5 &&
//                 End_Line <= 25 &&
//                 clo < 20)
//             {
//                 ruwan_flag = 1;
//                 zhi_flag = 0;
//                 wd_flag = 0;
//             }
//             else
//             {
//                 ruwan_flag = 0;
//             }
//         }
//         else
//         {
//             /*
//                 如果左右角点同时存在，不走普通弯道/入弯判断。
//             */
//             wd_flag = 0;
//             ruwan_flag = 0;
//         }


//         /***********************
//          * 3. 直道状态判断
//          ***********************/

//         /*
//             直道判断条件：

//             ruwan_flag == 0：
//             当前不是入弯。

//             End_Line <= 14：
//             前方有效赛道较完整。

//             max_value <= 5：
//             最大局部中线变化很小。

//             max_k <= 3：
//             相邻曲率变化也很小，没有突然转弯趋势。
//         */
//         if (ruwan_flag == 0 &&
//             End_Line <= 14 &&
//             max_value <= 5 &&
//             max_k <= 3)
//         {
//             zhi_flag = 1;
//             ruwan_flag = 0;
//             wd_flag = 0;
//         }
//         else
//         {
//             zhi_flag = 0;
//         }
//     }


//     /************************************************************
//      * 六、特殊元素 element == 3 的处理
//      *
//      * 注意：
//      * 原代码把 element == 3 写在 element == 0 的内部，
//      * 导致这段逻辑永远不会执行。
//      *
//      * 这里单独拿出来处理。
//      ************************************************************/
//     else if (element == 3)
//     {
//         /*
//             element == 3 时，不使用普通弯道判断。
//         */
//         wd_flag = 0;
//         ruwan_flag = 0;
//         bigwan_flag = 0;

//         /*
//             原始逻辑：
//             如果 End_Line <= 17，则认为是直道。
//         */
//         if (End_Line <= 17)
//         {
//             zhi_flag = 1;
//         }
//         else
//         {
//             zhi_flag = 0;
//         }
//     }


//     /************************************************************
//      * 七、其他特殊元素处理
//      ************************************************************/
//     else
//     {
//         /*
//             非普通元素时，默认清空普通赛道状态。
//         */
//         ruwan_flag = 0;
//         zhi_flag = 0;
//         wd_flag = 0;
//         bigwan_flag = 0;

//         /*
//             保留你的原始逻辑：
//             当 ruku_cnt == 2 时，强制认为处于入弯状态。
//         */
//         if (ruku_cnt == 2)
//         {
//             ruwan_flag = 1;
//         }
//     }
// } 
// int TiaoBianDian=0;
// int TiaoBianDianMax=50;
// int banmaxian_flag = 0;
// int banmaxian_start(const uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH]) //斑马线检测
// {
//     uint8 i=0,k=0;
//     uint8 b_W_flag=6;
//     uint8 count=0;
//     TiaoBianDian = 0;
//     count = End_Line+1 < 30 ? 30 : End_Line;
//     for(k=58;k>=count;k--)
//     {
//         uint8 black_count=0;//连续黑点计数防止将环岛等圆环误判成斑马线
//         for(i=current_left_col[k]+5;i<current_right_col[k]-5;i++)
//         {


//             b_W_flag=arr[k][i];

//             if(arr[k][i+1]!=b_W_flag)
//             {
//                 black_count++;
//             }

//         }
//         if(black_count<=8)
//         {
//             black_count=0;
//         }
//         TiaoBianDian+=black_count;
//         if(End_Line < 30 && TiaoBianDian>=TiaoBianDianMax && enable_banmaflag==1)//&& enable_banmaflag==0
//         {
//             return 1;
//         }

//     }

//     return 0;
// }
// 全局标志位（你工程里已经有了就不用重复定义）
int banmaxian_flag = 0;

// 斑马线检测：void 函数，结果存在 banmaxian_flag 里
void banmaxian_start(
    const uint8_t arr[IMAGE_HEIGHT][IMAGE_WIDTH],
    const uint8_t left_border[IMAGE_HEIGHT],
    const uint8_t right_border[IMAGE_HEIGHT],
    uint16_t end_row
)
{
    // 默认：没检测到
    banmaxian_flag = 0;

    // 如果已经找到黑色矩形 → 直接不检测斑马线
    if (br_found != 0)
    {
        banmaxian_flag = 0;
        return;
    }

    int TiaoBianDian = 0;
    const int TiaoBianDianMax = 14;

    if (end_row >= IMAGE_HEIGHT)
        end_row = IMAGE_HEIGHT - 1;

    // 从 End_Line 往下搜索到图片底部
    for (int k = end_row; k <= IMAGE_HEIGHT - 1; k++)
    {
        int L = left_border[k];
        int R = right_border[k];

        if (L < 0 || R >= IMAGE_WIDTH || L >= R - 10)
            continue;

        int track_width = R - L;  // 赛道真实宽度

        L += 5;
        R -= 5;
        if (L >= R - 2)
            continue;

        // ===================== 记录：第一次跳变 & 最后一次跳变 =====================
        int first_trans = -1;
        int last_trans = -1;
        int transition_count = 0;
        uint8_t last = arr[k][L];

        for (int i = L; i < R - 1; i++)
        {
            if (arr[k][i+1] != last)
            {
                transition_count++;
                last = arr[k][i+1];

                if (first_trans == -1)
                    first_trans = i;  // 最左侧跳变位置

                last_trans = i;  // 不断更新，最后就是最右侧
            }
        }

        // 没有有效跳变
        if (first_trans == -1 || last_trans == -1)
            continue;

        // ===================== 新增关键判断：条纹宽度必须 >= 赛道一半 =====================
        int stripe_region_width = last_trans - first_trans;
        if (stripe_region_width < track_width / 2)
        {
            continue; // 太窄，不是斑马线
        }

        // 过滤黑块：跳变太少
        if (transition_count <= 3)
            continue;

        // 过滤整块黑色矩形
        int black_num = 0;
        int total = R - L;
        for (int i = L; i < R; i++)
        {
            if (arr[k][i] == 0)
                black_num++;
        }

        // 黑色 > 80% 不是斑马线
        if (black_num > total * 8 / 10)
            continue;

        // 累计有效跳变
        TiaoBianDian += transition_count;

        // 满足条件 → 检测到斑马线
        if (TiaoBianDian >= TiaoBianDianMax)
        {
            if (br_found == 0)
                banmaxian_flag = 1;
            else
                banmaxian_flag = 0;
            return;
        }
    }
}
// //@停车判断
// int stop_flag_judge;
// int boundary=125;
// int stop_count;
// int stop_judge()
// {
//      stop_flag_judge=0;
//      stop_count=0;
//         for(int t=2;t<IMAGE_WIDTH-2;t++)
//         {
//             if(arr[115][t]==Black)
//                 stop_count++;
//         }
//     if(stop_count>0 )
//         stop_flag_judge=1;
//     return stop_flag_judge;
// }
//@出界判断
int out_flag_judge;       // 出界标志
int out_boundary = 150;   // 出界阈值：底部一行黑点超过这个数则判定出界（根据IMAGE_WIDTH调整）
int out_count;            // 黑点计数器

int out_of_bounds_judge()
{
    out_flag_judge = 0;
    out_count = 0;

    //扫描第59行，图像最下方
    for(int t = 2; t < IMAGE_WIDTH - 2; t++)
    {
        if(arr[59][t] == Black) 
        {
            out_count++;
        }
    }

    // 判断逻辑：如果这一行的黑点数量超过了设定的阈值
    if(out_count > out_boundary)
    {
        out_flag_judge = 1;
    }

    return out_flag_judge;
}

//圆环统计白点数目
int baiyuan_count(uint8 start_row, uint8 end_row, uint8 edge)
{
    int count = 0;
    if(edge == 0)
    {
        for(int i=start_row;i>end_row;i--)
        {
            if(arr[i][2]==White)
            {
                count++;
            }
        }
    }
    if(edge == 1)
    {
        for(int i=start_row;i>end_row;i--)
        {
            if(arr[i][145]==White)
            {
                count++;
            }
        }
    }

    return count;
}


// 定义环岛大小的枚举/状态值
int big_or_small = 0; // 当前环岛大小状态
int small_ = 1;       // 小环标志
int big_ = 2;         // 大环标志

// 左侧和右侧的整体斜率与局部斜率变量
float k_left = 0;
float k_left_s = 0;
float k_right = 0;
float k_right_s = 0;

// 右侧分段斜率变量（用于计算曲率差）
float k_right1 = 0;
float k_right2 = 0;

// 左右侧“中拐点”（圆环内侧尖角）的检测标志位
int Rzhong_guai_flag=0;
int Lzhong_guai_flag=0;

// 左右侧“中拐点”的坐标记录
int L_zhongguai_x=0;
int L_zhongguai_y=0;
int R_zhongguai_x=0;
int R_zhongguai_y=0;
uint8 halo_up_corner_row, halo_up_corner_col, halo_up_corner_flag;    // 入环岛上拐点
// 左右圆环内侧大面积白色的像素点计数（用于防伪，避免把十字误判为圆环）
int Lhuan_bai=0;
int Rhuan_bai=0;
int enable_left_circle_flag = 1;
int enable_right_circle_flag = 1;
float my_fabs(float my_value)//myabs_Int
{
    if (my_value < 0.0)
        my_value = -my_value;
    return my_value;
}
// 圆环判定主函数 (完美重构对称版)

void circle_judge()
{
    if (enable_left_circle_flag && enable_element_search_flag)
    {
        // halo left
        uint8 end_l = R_corner_flag ? R_corner_row + 5 : right_effect_row_end + 10;
        uint8 row_midl = (right_effect_row_start + end_l) / 2;
        float kr1 = current_right_col[left_effect_row_start] - current_right_col[row_midl];
        float kr2 = current_right_col[row_midl] - current_right_col[end_l];
        //row_mid = (left_effect_row_start + end_r) / 2;
        float kr3 = current_right_col[13] - current_right_col[23];
        float kr4 = current_right_col[35] - current_right_col[45];
        float kr5 = current_right_col[13] - current_right_col[33];
        float kr6 = current_right_col[35] - current_right_col[55];
        // k_right = (current_right_col[end_l] - current_right_col[right_effect_row_start]) / ((end_l - right_effect_row_start) * 1.0);
        // k_right_s = (current_right_col[(end_l + right_effect_row_start)/2] - current_right_col[right_effect_row_start]) / (((end_l + right_effect_row_start)/2) - right_effect_row_start);
         // 3. 带有【除零保护】的整体和局部斜率计算
        int den1 = end_l - right_effect_row_start;
        int den2 = ((end_l + right_effect_row_start) / 2) - right_effect_row_start;
        
        if (den1 == 0) {
            k_right = 0.0;
        } else {
            k_right = (current_right_col[end_l] - current_right_col[right_effect_row_start]) / (den1 * 1.0);
        }

        if (den2 == 0) {
            k_right_s = 0.0;
        } else {
            k_right_s = (current_right_col[(end_l + right_effect_row_start)/2] - current_right_col[right_effect_row_start]) * 1.0 / den2;
        }
        if (current_right_col[L_corner_row-3] - current_left_col[L_corner_row-3]>straightwidth[L_corner_row-3] && current_right_col[L_corner_row-4] - current_left_col[L_corner_row-4]>straightwidth[L_corner_row-4] && current_right_col[L_corner_row-2] - current_left_col[L_corner_row-2]>straightwidth[L_corner_row-2] && right_halo_flag == 0 && left_halo_flag == 0 && L_corner_flag && L_corner_angle < 85 && L_corner_row > IMAGE_HEIGHT / 2 - 5 && L_corner_row - End_Line > 10 && right_effect_row_end <= 15 && End_Line < 15 && (R_corner_flag==0 || R_corner_row <= 16))
        {//baiyuan_count(L_corner_row-1,L_corner_row-15,0)>5 &&
            Lzhong_guai_flag = 0;
            Lhuan_bai=0;
            bianli(L_corner_row - 2, End_Line+1, (current_right_col[right_effect_row_start] + current_left_col[left_effect_row_start])/2, 0 , 0);
            for(int i = 0;i<my_l_count - 6;i++)
            {
                if(my_L_edge[i+3].y < L_corner_row - 8 && my_L_edge[i+3].x > my_L_edge[i+8].x && my_L_edge[i+3].x + 3 > L_corner_col && my_L_edge[i+6].x < my_L_edge[i+3].x && my_L_edge[i+3].x > my_L_edge[i+1].x && my_L_edge[i+3].x > my_L_edge[i].x && my_L_edge[i+3].x > my_L_edge[i-1].x && my_L_edge[i+2].x > my_L_edge[i-1].x)
                {
                    Lzhong_guai_flag = 1;
                    L_zhongguai_x = my_L_edge[i+3].x;
                    L_zhongguai_y = my_L_edge[i+3].y;
                }
            }//Lzhong_guai_flag == 1 &&
            if(Lzhong_guai_flag == 1)
                        {
                            for(int i=L_corner_row;i>L_zhongguai_y;i--)
                            {
                                if(arr[i][2]==White)
                                {
                                    Lhuan_bai++;
                                }
                            }
                        }//Lhuan_bai>5 &&
            if (Lhuan_bai>10 && Lzhong_guai_flag == 1 && ((left_effect_row_start >= IMAGE_HEIGHT - 30 && right_effect_row_start >= IMAGE_HEIGHT - 30 ) || (left_effect_row_start >= IMAGE_HEIGHT - 40 && right_effect_row_start >= IMAGE_HEIGHT - 25 ) || (left_effect_row_start >= IMAGE_HEIGHT - 25 && right_effect_row_start >= IMAGE_HEIGHT - 40 )) && k_right > 0.0 && k_right <= 2.3 && my_fabs(kr2 - kr1) <= 8.0&& my_fabs(kr3 - kr4) <= 4.0 && my_fabs(kr5 - kr6) <= 5.0) //&& left_effect_row_start - left_effect_row_end >= 5)
            {
                left_halo_flag = 1;
                left_findflag = 0;
                halo_up_corner_row = 0, halo_up_corner_col = 0, halo_up_corner_flag = 0;
                return;
            }
        }
        if (left_halo_flag == 1)
        {
            if_cross_flag=0;
            element = 4;
            left_findflag = 1;
                 if (my_fabs(kr2 - kr1) > 9.0 && k_right > 2.4 && 0)//此时认为边线严重不对，为误判圆环应直接退出
                 {
                     left_halo_flag = 0;
                     if_cross_flag=1;
                     element = 0;
                     return;
                 }

             // halo_speed = Speed_Now;
             //if(right_effect_row_end<= 30 + halo_similar_b && enable_big_or_small)
                 //{
                     //big_or_small = _big;
                 //}
             //else
             big_or_small = _small;
             return;
        }
    }
    uint8 end_r = R_corner_flag ? R_corner_row : right_effect_row_end + 2;
    uint8 row_mid = (right_effect_row_start + end_r) / 2;
    row_mid = (left_effect_row_start + end_r) / 2;
    if (enable_right_circle_flag && enable_element_search_flag)
    {
        end_r = L_corner_flag ? L_corner_row + 5 : left_effect_row_end + 10;
        row_mid = (left_effect_row_start + end_r) / 2;
        float k1 = current_left_col[row_mid] - current_left_col[left_effect_row_start];
        float k2 = current_left_col[end_r] - current_left_col[row_mid];
        float k3 = current_left_col[23] - current_left_col[13];
        float k4 = current_left_col[45] - current_left_col[35];
        float k5 = current_left_col[33] - current_left_col[13];
        float k6 = current_left_col[55] - current_left_col[35];
        k_left = (current_left_col[end_r] - current_left_col[left_effect_row_start]) / ((left_effect_row_start - end_r) * 1.0);
        k_left_s = (current_left_col[(end_r + left_effect_row_start)/2] - current_left_col[left_effect_row_start]) / ((left_effect_row_start - (end_r + left_effect_row_start)/2) * 1.0);
        // int diu_line = 0;
        if (current_right_col[R_corner_row-3] - current_left_col[R_corner_row-3]>straightwidth[R_corner_row-3] && current_right_col[R_corner_row-4] - current_left_col[R_corner_row-4]>straightwidth[R_corner_row-4] && current_right_col[R_corner_row-2] - current_left_col[R_corner_row-2]>straightwidth[R_corner_row-2] && baiyuan_count(R_corner_row-1,R_corner_row-15,1)>5 && right_halo_flag == 0 && left_halo_flag == 0 && R_corner_row > IMAGE_HEIGHT / 2 - 5 && R_corner_flag && R_corner_angle < 85 && R_corner_row - End_Line > 10 && End_Line < 15 && left_effect_row_end <= 15 && (L_corner_flag == 0 || L_corner_row <= 16))
        {//
            Rzhong_guai_flag = 0;
            Rhuan_bai=0;
            bianli(R_corner_row - 2, End_Line+1, (current_right_col[right_effect_row_start] + current_left_col[left_effect_row_start])/2, IMAGE_WIDTH-1 , 1);
            //k_right1 = (current_right_col[right_effect_row_start] - current_right_col[(max_colsrow + right_effect_row_start) / 2]) / (1.0 * (right_effect_row_start - (max_colsrow + right_effect_row_start) / 2));&& baiyuan_count(29,15,1)>5
            //k_right2 = (current_right_col[(max_colsrow + right_effect_row_start) / 2] - current_right_col[max_colsrow]) / (1.0 * ((max_colsrow + right_effect_row_start) / 2 - max_colsrow));
            for(int i = 0;i<my_r_count - 6;i++)
            {
                if(my_R_edge[i+3].y < R_corner_row - 8 &&my_R_edge[i+3].x - 3 < R_corner_col && my_R_edge[i+3].x < my_R_edge[i+8].x && my_R_edge[i+6].x > my_R_edge[i+3].x && my_R_edge[i+3].x < my_R_edge[i+1].x && my_R_edge[i+3].x < my_R_edge[i].x && my_R_edge[i+3].x < my_R_edge[i-1].x && my_R_edge[i+2].x < my_R_edge[i-1].x)
                {
                    Rzhong_guai_flag = 1;
                    R_zhongguai_x = my_R_edge[i+3].x;
                    R_zhongguai_y = my_R_edge[i+3].y;
                }
            }//
            if(Rzhong_guai_flag == 1)
                      {
                          for(int i=R_corner_row;i>R_zhongguai_y;i--)
                          {
                              if(arr[i][147]==White)
                              {
                                  Rhuan_bai++;
                              }
                          }
                      }//Rhuan_bai>5 &&
            if (Rhuan_bai>5 &&Rzhong_guai_flag == 1 &&((left_effect_row_start >= IMAGE_HEIGHT - 30 &&right_effect_row_start >= IMAGE_HEIGHT - 30 ) || (left_effect_row_start >= IMAGE_HEIGHT - 40 &&right_effect_row_start >= IMAGE_HEIGHT - 25 ) || (left_effect_row_start >= IMAGE_HEIGHT - 25 &&right_effect_row_start >= IMAGE_HEIGHT - 40 ))&& k_left > 0.0 && k_left <= 2.3 && my_fabs(k2 - k1) <= 8.0 && my_fabs(k3 - k4) <= 4.0 && my_fabs(k5 - k6 ) <= 5.0) //
            {
                right_halo_flag = 1;
                right_findflag = 0;
                halo_up_corner_row = 0, halo_up_corner_col = 0, halo_up_corner_flag = 0;
                return;
            }
        }
        if (right_halo_flag == 1)
        {

            element = 5;
            if_cross_flag=0;
            right_findflag = 1;
                if (my_fabs(k2 - k1) > 9.0 && k_left > 2.4 && 0)//此时认为边线严重不对，为误判圆环，应直接跳出
                {
                    right_halo_flag = 0;
                    element = 0;
                    if_cross_flag=1;
                    return;
                }

            // halo_speed = Speed_Now;
            //if(right_effect_row_end<= 30 + halo_similar_b && enable_big_or_small)
                //{
                    //big_or_small = _big;
                //}
            //else
            big_or_small = _small;
            return;
        }
    }
}
// ==========================================
// 全局变量定义与状态标志位
// ==========================================
int right_diu = 0; // 右侧边线丢失计数器（目前代码中被注释未使用）
int jilu_col = IMAGE_WIDTH / 2; // 记录某个关键列坐标，初始化为图像中线

// 状态1切换到状态2的标志位（判断车头是否抵达入环临界点）
int right_one_change_two_flag = 0;
int left_one_change_two_flag=0;

// 状态4切换到状态5的标志位（确认发现出环的大面积白色特征）
int halo_five_change_flag = 0;
int L_halo_five_change_flag = 0;

// 彻底出环结束标志位（状态6完成时置1）
int R_end_flag=0;
int L_end_flag=0;

// 作者的备忘录/TODO列表
//直道判断高度改成60
//加陀螺仪积分
//修改停车判断
//编码器积分距离

// 记录环岛出/入阶段左右边界的斜率
float k_halo_left = 0;
float k_halo_right = 0;

// 记录左右边界的最高有效行号
int Lmax_colsrow=0;
int Rmax_colsrow=0;
int right_halo_six=0;
int left_halo_six=0;

int four_mid_flag = 0;
uint8 halo_six_left_row = 0;
uint8 halo_six_right_row = 0 ;

int enable_gyro_flag = 0;
float gyro_angle_z_add = 0.0;
// ==========================================
// 圆环处理主逻辑函数
// ==========================================
void circle_process()
{
    // 左环岛
    if (element == 4)
    {
        right_halo_flag = 0;
        if (left_effect_row_start<35 && (current_left_col[IMAGE_HEIGHT - 2] == 1) && (current_left_col[IMAGE_HEIGHT - 1] == 1) && (current_left_col[IMAGE_HEIGHT - 6] == 1))
        {
            left_one_change_two_flag = 1;
            enable_element_search_flag = 0;
            left_findflag = 1;
        }
        if(left_one_change_two_flag == 1 && left_halo_flag == 1 && left_effect_row_start > 50 && current_left_col[IMAGE_HEIGHT - 20] > 1)
            left_halo_flag = 2;
        if (left_halo_flag == 2)
        {
            enable_element_search_flag = 0;
            get_maxline(0);
            halo_up_corner_flag = 0;
            Lmax_colsrow=0;
            Lmax_colsrow = max_colsrow<35?max_colsrow:35;
            ADD_Edge_Line(_left, current_right_col[IMAGE_HEIGHT - 1]-straightwidth[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, current_right_col[Lmax_colsrow+1] - straightwidth[Lmax_colsrow+1], Lmax_colsrow+1, IMAGE_HEIGHT - 1, Lmax_colsrow+1);
            //halo_up_corner_col = 0;
            //halo_up_corner_row = 0;
            /*for (int i = left_effect_row_start; i > left_effect_row_end; i--)
            {
                if (current_left_col[i] >= L_max_col)
                {
                    L_max_col = current_left_col[i];
                    Lmax_colsrow = current_left_row[i];
                }
            }*/
            if(left_effect_row_start>40)
            {
                bianli(IMAGE_HEIGHT - 2, End_Line, End_Line_col, 0, 0);

                for (int i = 14; i < L_edge_count; i++)
                {
                    if (my_L_edge[i - 1].x - my_L_edge[i - 4].x > 30 && myabs_Int(my_L_edge[i].x - my_L_edge[i - 1].x) <= 5)
                    {

                        if (my_L_edge[i - 1].x < current_right_col[IMAGE_HEIGHT - 2] && my_L_edge[i - 1].y < IMAGE_HEIGHT - 10)
                        {
                            halo_up_corner_col = my_L_edge[i - 1].x;
                            halo_up_corner_row = my_L_edge[i -  1].y;
                            halo_up_corner_flag = 1;
                            break;
                        }
                    }
                }
            }
            if (halo_up_corner_flag )
            {
                

                // ADD_Edge_Line(_right, current_left_col[IMAGE_HEIGHT - 1]+straightwidth[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, current_left_col[Rmax_colsrow+1] + straightwidth[Rmax_colsrow+1], Rmax_colsrow+1, IMAGE_HEIGHT - 1, Rmax_colsrow+1);
                // ADD_Edge_Line(_left, current_left_col[Rmax_colsrow], Rmax_colsrow, halo_up_corner_col, halo_up_corner_row, Rmax_colsrow, 20);
                ADD_Edge_Line(_left, current_right_col[IMAGE_HEIGHT - 1]-straightwidth[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, current_right_col[Lmax_colsrow+1]-straightwidth[Lmax_colsrow+1], Lmax_colsrow+1, IMAGE_HEIGHT - 1, Lmax_colsrow+1);
                ADD_Edge_Line(_right, current_right_col[Lmax_colsrow], Lmax_colsrow, halo_up_corner_col, halo_up_corner_row, Lmax_colsrow, 20);
            }
            else
                right_findflag = 0;
            if (halo_up_corner_flag && (halo_up_corner_row > IMAGE_HEIGHT - 30 || max_colsrow >= IMAGE_HEIGHT - 20)) // 大小平衡
                left_halo_flag = 3;
            //if (my_fabs(gyro_angel_z_add) > 50.0 && right_effect_row_start >= IMAGE_HEIGHT - 5)
                //left_halo_flag = 3;
            //if (halo_up_corner_flag && ((halo_up_corner_col >= IMAGE_WIDTH / 2 + 12) || (current_right_col[right_effect_row_start] > IMAGE_WIDTH - 5 && right_effect_row_start > IMAGE_HEIGHT - 10))) // 大小平衡
                //left_halo_flag = 3;
            if(big_or_small==_big)
            {
                if(halo_up_corner_flag && halo_up_corner_row > IMAGE_HEIGHT - 30 && max_colsrow > IMAGE_HEIGHT - 15)left_halo_flag = 3;
            }
        }
        if (left_halo_flag == 3)
        {
            enable_gyro_flag=1;
            get_maxline(0);
            bianli(IMAGE_HEIGHT - 2, End_Line, End_Line_col, 0, 0);
            halo_up_corner_flag = 0;
            for (int i = 14; i < L_edge_count; i++)
            {
                if (my_L_edge[i - 1].x - my_L_edge[i - 4].x > 30 && myabs_Int(my_L_edge[i].x - my_L_edge[i - 1].x) <= 5)
                {
                    if (my_L_edge[i - 1].x < current_right_col[IMAGE_HEIGHT - 2] && my_L_edge[i - 1].y < IMAGE_HEIGHT - 10)
                    {
                        halo_up_corner_col = my_L_edge[i - 1].x;
                        halo_up_corner_row = my_L_edge[i - 1].y;
                        halo_up_corner_flag = 1;
                        break;
                    }
                }
            }
            if (halo_up_corner_flag && halo_up_corner_col < IMAGE_WIDTH *0.8 )
            {
                k_halo_right = (current_right_col[right_effect_row_start] - current_right_col[right_effect_row_end]) / ((right_effect_row_start - right_effect_row_end) * 1.0);
                int temp_col = 0.0;
                if(big_or_small == _small)
                    temp_col = halo_up_corner_col - (halo_up_corner_col - IMAGE_WIDTH / 3) * 0.45;
                else
                    temp_col = halo_up_corner_col;
                if (temp_col < IMAGE_WIDTH / 4 || temp_col < current_left_col[halo_up_corner_row])
                    temp_col = IMAGE_WIDTH / 4 > current_left_col[halo_up_corner_row] ? IMAGE_WIDTH / 4 : current_left_col[halo_up_corner_row] + 1;
                if(big_or_small == _small)
                {
                    if (left_findflag&&left_effect_row_start>IMAGE_HEIGHT-5&&left_effect_row_end<IMAGE_HEIGHT-5)
                    {
                        jilu_col = current_left_col[IMAGE_HEIGHT - 2];
                        ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT - 2], IMAGE_HEIGHT - 2, temp_col-15, halo_up_corner_row+1, IMAGE_HEIGHT - 2, 20);
                    }
                    else
                        ADD_Edge_Line(_right, IMAGE_WIDTH - 10, IMAGE_HEIGHT - 2, temp_col-15  , halo_up_corner_row+1, IMAGE_HEIGHT - 2, 20);
                }
                /*else
                {
                    if (left_findflag&&left_effect_row_start>IMAGE_HEIGHT-5&&left_effect_row_end<IMAGE_HEIGHT-5)
                    {
                        jilu_col = current_left_col[IMAGE_HEIGHT - 2];
                        if(right_effect_row_start>IMAGE_HEIGHT-5&&right_effect_row_end<IMAGE_HEIGHT-5&&jilu_col>current_right_col[right_effect_row_start])
                            ADD_Edge_Line(_right, current_right_col[right_effect_row_start], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                        else
                            ADD_Edge_Line(_right, current_left_col[IMAGE_HEIGHT - 2] + straightwidth[1], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                    }
                    else
                    {
                        if(right_effect_row_start>IMAGE_HEIGHT-5&&right_effect_row_end<IMAGE_HEIGHT-5&&jilu_col>current_right_col[right_effect_row_start])
                            ADD_Edge_Line(_right, current_right_col[right_effect_row_start], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                        else
                            ADD_Edge_Line(_right, jilu_col + straightwidth[1], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                    }
                }*/

            }
            else
            {
                k_halo_right = (current_right_col[right_effect_row_start] - current_right_col[right_effect_row_end]) / ((right_effect_row_start - right_effect_row_end) * 1.0);
                ADD_Edge_Line(_right, IMAGE_WIDTH - 10, IMAGE_HEIGHT - 2, IMAGE_WIDTH - 110, 20, IMAGE_HEIGHT - 2, 20);
                if (gyro_angle_z_add < -25.0 && right_effect_row_start >= IMAGE_HEIGHT - 2 && right_effect_row_start >= IMAGE_HEIGHT-2 && current_right_col[right_effect_row_end] < current_right_col[right_effect_row_start])
                {
                    left_halo_flag = 4;
                    k_halo_right = 0.0;
                    enable_gyro_flag=0;
                    gyro_angle_z_add = 0;
                    return;
                }
                else
                {
                    left_findflag = 0;
                    right_findflag = 0;
                }
            }

            //float k_right = (current_right_col[right_effect_row_start] - current_right_col[right_effect_row_end]) / ((right_effect_row_start - right_effect_row_end) * 1.0);
            if (k_halo_right>2.0 && right_effect_row_start > IMAGE_HEIGHT/2 && R_corner_flag == 0 && right_effect_row_end == End_Line + 1 && current_right_col[right_effect_row_end] < current_right_col[(right_effect_row_end + right_effect_row_start) / 2] && current_right_col[(right_effect_row_end + right_effect_row_start) / 2] < current_right_col[right_effect_row_start])
            {
                left_halo_flag = 4;
                k_halo_right = 0.0;
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                return;
            }
            if (gyro_angle_z_add < -40.0 && right_effect_row_start >= IMAGE_HEIGHT - 10 )
            {
                left_halo_flag = 4;
                k_halo_right = 0.0;
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                return;
            }
            if (gyro_angle_z_add < -25.0 && right_effect_row_start >= IMAGE_HEIGHT-2  && current_right_col[right_effect_row_end] < current_right_col[right_effect_row_start] )
            {
                left_halo_flag = 4;
                k_halo_right = 0.0;
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                return;
            }
        }
        if (left_halo_flag == 4)
        {
            int L_four_change_five_flag=0;
            L_four_change_five_flag=0;
            get_maxline(0);
            for(int i=IMAGE_HEIGHT-3;i>20;i--)
            {
                if(arr[i][147] == 255)
                {
                    L_four_change_five_flag++;
                }
            }
            if (R_corner_flag == 0&&right_findflag&&right_effect_row_end<IMAGE_HEIGHT-5)
                End_Line = right_effect_row_end - 1;
            if (R_corner_flag && (R_corner_row < IMAGE_HEIGHT - 40||R_corner_col<IMAGE_WIDTH/4))
            {
                int col_left = left_effect_row_start >= IMAGE_HEIGHT - 10 ? get_maxline(0) : 0;

                ADD_Edge_Line(_right, R_corner_col, R_corner_row, col_left + (R_corner_col - col_left)*0.4, R_corner_row - 16, R_corner_row - 1, R_corner_row - 40 < 10 ? 10 : R_corner_row - 40);
                if (left_effect_row_start < IMAGE_HEIGHT - 10 )
                    left_findflag = 0;
            }
            else if (R_corner_flag && R_corner_angle < 91 && R_corner_col < R_edge[0].col)
            {
                if(L_four_change_five_flag>15)
                {
                    left_halo_flag = 5;
                    L_halo_five_change_flag = 0;
                }
                int col_left = left_effect_row_start >= IMAGE_HEIGHT - 10 ? get_maxline(0) : 0;
                ADD_Edge_Line(_right, R_corner_col, R_corner_row, col_left + (R_corner_col - col_left)*0.4 , R_corner_row - 15, R_corner_row - 1, R_corner_row - 40 < 10?10:R_corner_row - 40);
                //if (left_effect_row_start < IMAGE_HEIGHT - 10 || lost_l_endrow != 0)
                    //left_findflag = 0;
                return;
            }
        }
        if (left_halo_flag == 5)
        {
            enable_gyro_flag=1;
            if (right_effect_row_start < 45 && current_right_col[IMAGE_HEIGHT - 1] == 148 && current_right_col[IMAGE_HEIGHT - 2] == 148 && current_right_col[IMAGE_HEIGHT - 3] == 148 && current_right_col[IMAGE_HEIGHT - 4] == 148 && current_right_col[IMAGE_HEIGHT - 5] == 148 && current_right_col[IMAGE_HEIGHT - 6] == 148 && current_right_col[IMAGE_HEIGHT - 7] == 148 && current_right_col[IMAGE_HEIGHT - 8] == 148
                && current_right_col[IMAGE_HEIGHT - 9] == 148 && current_right_col[IMAGE_HEIGHT - 10] == 148)
                L_halo_five_change_flag = 1;

            /*if (R_corner_flag && right_findflag && R_corner_row < IMAGE_HEIGHT - 10 && R_corner_row >= End_Line + 4 && R_corner_row > 25 && R_edge[R_corner_count + 20 < R_edge_count ? R_corner_count + 20 : R_edge_count - 1].col>R_corner_col+10)
            {
                // ADD_Edge_Line(_right, current_right_col[R_corner_row+10], R_corner_row+10, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
                int col_left = 0;
                if (left_findflag && left_effect_row_start >= IMAGE_HEIGHT - 10 && left_effect_row_end < IMAGE_HEIGHT - 10)
                    col_left = get_maxline(0);
                if (col_left > R_corner_col)
                    col_left = 0;*/
                ADD_Edge_Line(_right, 140, IMAGE_HEIGHT - 1, 75, 20, 42, 30);
                //return;
            //}
            // 65 30
                if(L_halo_five_change_flag == 1)
                    ADD_Edge_Line(_right, IMAGE_WIDTH/3*2, IMAGE_HEIGHT - 1, current_right_col[End_Line + 1]>IMAGE_WIDTH/2?IMAGE_WIDTH/2:current_right_col[End_Line + 1], IMAGE_HEIGHT/3, 42, 30);
            if (gyro_angle_z_add < -11 ||(gyro_angle_z_add < -11 && End_Line <= 16 && (right_effect_row_start >= IMAGE_HEIGHT - 14 ||(left_effect_row_end < 43 && left_effect_row_end > 31 )) && L_halo_five_change_flag==1))
            {
                left_halo_flag = 6;
                halo_six_left_row = 0;   // ★进状态6先清零:该值只在第二循环检到近处出环角点时才写,从不复位。
                                         //  若残留上次环岛的旧值(>34),下面 if(halo_six_left_row>34) 会在
                                         //  本次刚进6时就误置 L_end_flag=1 → 退出门槛少了"必须先看到近处角点"
                                         //  这一条 → 第二次出环提前退出、路径变差。
                return;
            }

            /*else
            {
                int start_col = 0;
                if (left_effect_row_start > IMAGE_HEIGHT - 10)
                    start_col = current_left_col[IMAGE_HEIGHT - 1] + straightwidth[0];
                else
                    start_col = IMAGE_WIDTH * 2 / 3;
                start_col < IMAGE_WIDTH * 2 / 3 ? IMAGE_WIDTH * 2 / 3 : start_col;
                if (End_Line <= 20 && End_Line_col < IMAGE_WIDTH / 2 && right_effect_row_start > IMAGE_HEIGHT / 2 + 15 && right_effect_row_end < IMAGE_HEIGHT - 20)
                {
                    ADD_Edge_Line(_right, start_col, IMAGE_HEIGHT - 1, current_right_col[(right_effect_row_start + End_Line) / 2], (right_effect_row_start + End_Line) / 2, IMAGE_HEIGHT - 1, (right_effect_row_start + End_Line) / 2);
                    left_findflag = 0;
                }
                else
                {
                    int choose_row = End_Line < 20 ? 20 : End_Line;
                    if (current_left_col[choose_row] < start_col)
                    {
                        if(big_or_small == _small)
                            ADD_Edge_Line(_right, start_col, IMAGE_HEIGHT - 1, current_left_col[choose_row] + straightwidth[IMAGE_HEIGHT - 1 - choose_row], choose_row, IMAGE_HEIGHT - 1, choose_row);
                        else
                        {
                            int col1 = current_left_col[choose_row] + straightwidth[IMAGE_HEIGHT - 1 - choose_row]<IMAGE_WIDTH/3?IMAGE_WIDTH/3:current_left_col[choose_row] + straightwidth[IMAGE_HEIGHT - 1 - choose_row];
                            ADD_Edge_Line(_right, start_col, IMAGE_HEIGHT - 1, col1, choose_row, IMAGE_HEIGHT - 1, choose_row);
                        }
                    }
                    else
                    {
                        if(big_or_small == _small)
                            ADD_Edge_Line(_right, start_col, IMAGE_HEIGHT - 1, straightwidth[IMAGE_HEIGHT - 1 - choose_row], choose_row, IMAGE_HEIGHT - 1, choose_row);
                        else
                        {
                            int col1 = straightwidth[IMAGE_HEIGHT - 1 - choose_row]<IMAGE_WIDTH/3?IMAGE_WIDTH/3:straightwidth[IMAGE_HEIGHT - 1 - choose_row];
                            ADD_Edge_Line(_right, start_col, IMAGE_HEIGHT - 1, col1, choose_row, IMAGE_HEIGHT - 1, choose_row);
                        }
                    }
                }
            }*/
        }
        if (left_halo_flag == 6)
        {
            uint8 bux_flag = 0;
            if(right_effect_row_start<=58 && left_halo_six==0)
                ADD_Edge_Line(_right, current_right_col[IMAGE_HEIGHT-1], IMAGE_HEIGHT-1, current_right_col[right_effect_row_end], right_effect_row_end, IMAGE_HEIGHT-1, 10);
            if (End_Line < IMAGE_HEIGHT - 20)
            {
                for (uint8 i = IMAGE_HEIGHT - 20; i > End_Line + 2; i--)
                 {
                     if (current_left_col[i] - current_left_col[i + 5] > 15)
                     {
                         //bux_flag = 1;
                         left_halo_six = 1;
                         //halo_six_left_row = i;
                         break;
                     }
                 }
                if(left_halo_six==1)
                    bux_flag = 1;
                // 只在近处带(i>34,与下方 L_end_flag 阈值一致)找出环角点:否则远处十字的角点也会
                // 把 bux_flag 顶成2,近处角点消失后远处角点仍在→bux_flag永远回不到1→卡在状态6出不来。
                for (uint8 i = IMAGE_HEIGHT - 10; i > End_Line + 2 && i > 34; i--)
                  {
                     if ((current_left_col[i] - current_left_col[i + 5] > 15) && left_halo_six==1 && (current_left_col[i+2]==1||current_left_col[i+4]==1||current_left_col[i+6]==1))
                         {
                          bux_flag = 2;
                          halo_six_left_row = i;
                          break;

                         }
                  }
                //if(L_end_flag==0&&bux_flag==2)
                //ADD_Edge_Line(_left, current_left_col[halo_six_left_row-3], halo_six_left_row-3, current_left_col[halo_six_left_row-7], halo_six_left_row-7, IMAGE_HEIGHT-1, 10);
                if(halo_six_left_row>34)
                     L_end_flag=1;
            }


            //if (R_corner_flag && R_corner_col < IMAGE_WIDTH - 20 && Aim_Line <= R_corner_row)
            //{
               // Aim_Line = R_corner_row + 1;
               // ADD_Edge_Line(_right, current_right_col[right_effect_row_start], right_effect_row_start, R_corner_col, R_corner_row, R_corner_row - 1, End_Line);
            //}
            left_findflag = 0;
            if (L_end_flag==1&&bux_flag == 1 && left_effect_row_start >= IMAGE_HEIGHT - 5 && right_effect_row_start >= IMAGE_HEIGHT - 5)
            {
                enable_element_search_flag = 1;
                element = 0;
                L_end_flag = 0;
                left_findflag = 1;
                right_findflag = 1;
                left_one_change_two_flag=0;
                left_halo_six = 0;
                left_halo_flag = 0;
                if_cross_flag=1;
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                return;
            }
        }
        right_halo_flag = 0;
    }
    // 右环岛
    if (element == 5)
    {

        //right_one_change_two_flag=0;
        left_halo_flag = 0;
        //ADD_Edge_Line(_right, current_left_col[IMAGE_HEIGHT - 5]+straightwidth[IMAGE_HEIGHT-5], IMAGE_HEIGHT - 5, current_left_col[IMAGE_HEIGHT - 10]+straightwidth[IMAGE_HEIGHT-10], IMAGE_HEIGHT - 10, IMAGE_HEIGHT - 1, 0);
        right_diu = 0;
        //if (right_effect_row_start > IMAGE_HEIGHT - 10) //&&(R_corner_row>40||R_corner_flag==0))
        //{
            if (right_effect_row_start<35 && (current_right_col[IMAGE_HEIGHT - 2] == 148) && (current_right_col[IMAGE_HEIGHT - 1] == 148) && (current_right_col[IMAGE_HEIGHT - 6] == 148))
            {
                right_one_change_two_flag = 1;
                enable_element_search_flag = 0;
                right_findflag = 1;
            }
        //}
        /*for(int i=IMAGE_HEIGHT-1;i>IMAGE_HEIGHT-28;i--)
        {
            if(current_right_col[i]==148)
            {
                right_diu++;
            }
        }*/
        if (right_one_change_two_flag == 1 && right_halo_flag == 1 && right_effect_row_start > 50 && current_right_col[IMAGE_HEIGHT - 20] < 148)//(R_corner_flag==0||R_corner_row>50)&&&& current_right_col[IMAGE_HEIGHT - 10] <= 148
            right_halo_flag = 2;
        if (right_halo_flag == 2)
        {
            get_maxline(1);
            halo_up_corner_flag = 0;
            enable_element_search_flag=0;
            Rmax_colsrow=0;
            Rmax_colsrow = max_colsrow<35?max_colsrow:35;
            ADD_Edge_Line(_right, current_left_col[IMAGE_HEIGHT - 1]+straightwidth[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, current_left_col[Rmax_colsrow+1] + straightwidth[Rmax_colsrow+1], Rmax_colsrow+1, IMAGE_HEIGHT - 1, Rmax_colsrow+1);
            if (right_effect_row_start > 40)
            {
                bianli(IMAGE_HEIGHT - 2, End_Line, End_Line_col, IMAGE_WIDTH - 1, 1);//IMAGE_HEIGHT - 2
                for (int i = 14; i < my_r_count; i++)
                {
                    if (my_R_edge[i - 4].x - my_R_edge[i - 1].x > 30 && myabs_Int(my_R_edge[i - 1].x - my_R_edge[i].x) <= 5)
                    {
                        if (my_R_edge[i - 1].x > current_left_col[IMAGE_HEIGHT - 2] && my_R_edge[i - 1].y < IMAGE_HEIGHT - 10)
                        {
                            halo_up_corner_col = my_R_edge[i - 1].x;
                            halo_up_corner_row = my_R_edge[i - 1].y;
                            halo_up_corner_flag = 1;
                            break;
                        }
                    }
                }
                //if(halo_up_corner_row > right_effect_row_end)
                //{
                    //halo_up_corner_flag=0;
                //}
            }
            if (halo_up_corner_flag )
            {
                ADD_Edge_Line(_right, current_left_col[IMAGE_HEIGHT - 1]+straightwidth[IMAGE_HEIGHT - 1], IMAGE_HEIGHT - 1, current_left_col[Rmax_colsrow+1] + straightwidth[Rmax_colsrow+1], Rmax_colsrow+1, IMAGE_HEIGHT - 1, Rmax_colsrow+1);
                ADD_Edge_Line(_left, current_left_col[Rmax_colsrow], Rmax_colsrow, halo_up_corner_col, halo_up_corner_row, Rmax_colsrow, 20);
            }

            else
                left_findflag = 0;
            if (halo_up_corner_flag && (halo_up_corner_row > IMAGE_HEIGHT - 30 || max_colsrow > IMAGE_HEIGHT - 20)) // 大小平衡
                right_halo_flag = 3;
            /*if (big_or_small == _big)
            {
                if (halo_up_corner_flag && halo_up_corner_row > IMAGE_HEIGHT - 30 && max_colsrow > IMAGE_HEIGHT - 15)right_halo_flag = 3;
            }*/
        }
        if (right_halo_flag == 3)
        {
            get_maxline(1);
            enable_gyro_flag = 1;
            bianli(IMAGE_HEIGHT - 2, End_Line, End_Line_col, IMAGE_WIDTH - 1, 1);
            halo_up_corner_flag = 0;
            for (int i = 14; i < my_r_count; i++)
            {
                if (my_R_edge[i - 4].x - my_R_edge[i - 1].x > 30 && myabs_Int(my_R_edge[i - 1].x - my_R_edge[i].x) <= 5)
                {
                    if (my_R_edge[i - 1].x > current_left_col[IMAGE_HEIGHT - 2] && my_R_edge[i - 1].y < IMAGE_HEIGHT - 10)
                    {
                        halo_up_corner_col = my_R_edge[i - 1].x;
                        halo_up_corner_row = my_R_edge[i - 1].y;
                        halo_up_corner_flag = 1;
                        break;
                    }
                }
            }
           // enable_gyro_flag=1;
            if (halo_up_corner_flag && halo_up_corner_col > IMAGE_WIDTH/3)
            {
                k_halo_left = (current_left_col[left_effect_row_end] - current_left_col[left_effect_row_start]) / ((left_effect_row_start - left_effect_row_end) * 1.0);
                int temp_col = 0.0;
                if (big_or_small == _small)
                    temp_col = halo_up_corner_col + (IMAGE_WIDTH * 2 / 3 - halo_up_corner_col) * 0.45;
                else
                    temp_col = halo_up_corner_col;
                if (temp_col > IMAGE_WIDTH * 3 / 4 || temp_col > current_right_col[halo_up_corner_row])
                    temp_col = IMAGE_WIDTH * 3 / 4 < current_right_col[halo_up_corner_row] ? IMAGE_WIDTH * 3 / 4 : current_right_col[halo_up_corner_row] + 1;
                if (big_or_small == _small)
                {
                    if (right_findflag && right_effect_row_start > IMAGE_HEIGHT - 5 && right_effect_row_end < IMAGE_HEIGHT - 5)
                    {
                        jilu_col = current_right_col[IMAGE_HEIGHT - 2];
                        ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT - 2] , IMAGE_HEIGHT - 2, temp_col+10, halo_up_corner_row+1, IMAGE_HEIGHT - 2, 20);
                    }
                    else
                        ADD_Edge_Line(_left, 10, IMAGE_HEIGHT - 2, temp_col+10, halo_up_corner_row+1, IMAGE_HEIGHT - 2, 20);
                }
                /*else
                {
                    if (right_findflag&&right_effect_row_start>IMAGE_HEIGHT-5&&right_effect_row_end<IMAGE_HEIGHT-5)
                    {
                        jilu_col = current_right_col[IMAGE_HEIGHT - 2];
                        if(left_effect_row_start>IMAGE_HEIGHT-5&&left_effect_row_end<IMAGE_HEIGHT-5&&jilu_col<current_left_col[left_effect_row_start])
                            ADD_Edge_Line(_left, current_left_col[left_effect_row_start], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                        else
                            ADD_Edge_Line(_left, current_right_col[IMAGE_HEIGHT - 2] - straightwidth[1], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                    }
                    else
                    {
                        if(left_effect_row_start>IMAGE_HEIGHT-5&&left_effect_row_end<IMAGE_HEIGHT-5&&jilu_col<current_left_col[left_effect_row_start])
                            ADD_Edge_Line(_left, current_left_col[left_effect_row_start], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                        else
                            ADD_Edge_Line(_left, jilu_col - straightwidth[1], IMAGE_HEIGHT - 2, temp_col, halo_up_corner_row, IMAGE_HEIGHT - 1, 0);
                    }

                }*/

            }
            else
            {//my_fabs(gyro_angel_z_add) > 25.0 &&
                k_halo_left = (current_left_col[left_effect_row_end] - current_left_col[left_effect_row_start]) / ((left_effect_row_start - left_effect_row_end) * 1.0);
                ADD_Edge_Line(_left, 10, IMAGE_HEIGHT - 2, 110, 20, IMAGE_HEIGHT - 2, 30);
                if (k_halo_left>2.0 && left_effect_row_start > IMAGE_HEIGHT - 30 &&  current_left_col[left_effect_row_end] > current_left_col[left_effect_row_start])//halo_up_corner_col > current_left_col[left_effect_row_start] &&
                {
                    right_halo_flag = 4;
                    enable_gyro_flag=0;
                    gyro_angle_z_add = 0;
                    k_halo_left = 0;
                    return;
                }
                else
                {
                    left_findflag = 0;
                    right_findflag = 0;
                }
            }
            //float k_left = (current_left_col[left_effect_row_end] - current_left_col[left_effect_row_start]) / ((left_effect_row_start - left_effect_row_end) * 1.0);
            //my_fabs(gyro_angel_z_add) > 50.0 &&
            if (k_halo_left>2.0 && L_corner_flag == 0 && left_effect_row_start > IMAGE_HEIGHT - 30 && left_effect_row_end == End_Line + 1 && current_left_col[left_effect_row_end] > current_left_col[(left_effect_row_end + left_effect_row_start) / 2] && current_left_col[(left_effect_row_end + left_effect_row_start) / 2] > current_left_col[left_effect_row_start])
            {
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                right_halo_flag = 4;
                k_halo_left = 0;
                return;
            }
            if (gyro_angle_z_add > 25.0 && left_effect_row_start >= IMAGE_HEIGHT - 2 && current_left_col[left_effect_row_end] > current_left_col[left_effect_row_start] )
            {
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                right_halo_flag = 4;
                k_halo_left = 0;
                return;
            }
            if (gyro_angle_z_add > 40.0 && left_effect_row_start >= IMAGE_HEIGHT - 10)
            {
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                right_halo_flag = 4;
                k_halo_left = 0;
                return;
            }
        }
        if (right_halo_flag == 4)
        {
            int R_four_change_five_flag = 0;
            R_four_change_five_flag = 0;
            get_maxline(1);
            for (int i = IMAGE_HEIGHT - 3; i > 20; i--)
            {
                if (arr[i][2] == 255)
                {
                    R_four_change_five_flag++;
                }
            }
            //if(left_effect_row_end > 35)
                //R_four_change_five_flag=2;

            if (L_corner_flag == 0 && left_findflag && left_effect_row_end < IMAGE_HEIGHT - 5)
                End_Line = left_effect_row_end - 1;
            if (L_corner_flag && (L_corner_row < IMAGE_HEIGHT - 40 || L_corner_col>IMAGE_WIDTH * 3 / 4))
            {
                //R_four_change_five_flag=1;

                int col_right = right_effect_row_start >= IMAGE_HEIGHT - 10 ? get_maxline(1) : IMAGE_WIDTH - 1;
                four_mid_flag = 1;
                ADD_Edge_Line(_left, L_corner_col, L_corner_row, L_corner_col + (col_right - L_corner_col) , L_corner_row - 16, L_corner_row - 1, L_corner_row - 40 < 10 ? 10 : L_corner_row - 40);
                if (right_effect_row_start < IMAGE_HEIGHT - 10)//|| lost_r_endrow != 0
                    right_findflag = 0;
            }
            else if (L_corner_flag && L_corner_angle < 91 && L_corner_col > L_edge[0].col)//&& L_edge[L_corner_count + 20 < L_edge_count ? L_corner_count + 20 : L_edge_count - 1].col < L_corner_col - 8
            {
                if (R_four_change_five_flag > 15)
                {
                    right_halo_flag = 5;
                    halo_five_change_flag = 0;
                }
                int col_right = right_effect_row_start >= IMAGE_HEIGHT - 10 ? get_maxline(1) : IMAGE_WIDTH - 1;
                ADD_Edge_Line(_left, L_corner_col, L_corner_row, L_corner_col + (col_right - L_corner_col) , L_corner_row - 16, L_corner_row - 1, L_corner_row - 40 < 10 ? 10 : L_corner_row - 40);
                //if (right_effect_row_start < IMAGE_HEIGHT - 10 || lost_r_endrow != 0)
                    //right_findflag = 0;
                return;
            }
        }
        if (right_halo_flag == 5)
        {
            //halo_five_change_flag = 0;
            enable_gyro_flag=1;
            if (left_effect_row_start < 45 && current_left_col[IMAGE_HEIGHT - 1] == 1 && current_left_col[IMAGE_HEIGHT - 2] == 1 && current_left_col[IMAGE_HEIGHT - 3] == 1 && current_left_col[IMAGE_HEIGHT - 4] == 1 && current_left_col[IMAGE_HEIGHT - 5] == 1 && current_left_col[IMAGE_HEIGHT - 6] == 1 && current_left_col[IMAGE_HEIGHT - 7] == 1 && current_left_col[IMAGE_HEIGHT - 8] == 1
                && current_left_col[IMAGE_HEIGHT - 9] == 1 && current_left_col[IMAGE_HEIGHT - 10] == 1)
                halo_five_change_flag = 1;
            ADD_Edge_Line(_left, 10, IMAGE_HEIGHT - 1, 90 , 30, 52, 30);
            /*get_maxline(1);
            if (L_corner_flag && left_findflag && L_corner_row < IMAGE_HEIGHT - 10 && L_corner_row >= End_Line + 4 && L_corner_row > 25 && L_edge[L_corner_count + 20 < L_edge_count ? L_corner_count + 20 : L_edge_count - 1].col < L_corner_col-10)
            {
                int col_left = IMAGE_WIDTH - 1;
                if (right_findflag && right_effect_row_start >= IMAGE_HEIGHT - 10 && right_effect_row_end < IMAGE_HEIGHT - 10)
                    col_left = get_maxline(1);
                if (col_left < L_corner_col)
                    col_left = IMAGE_WIDTH - 1;
                int col_right = right_effect_row_start >= IMAGE_HEIGHT - 10 ? get_maxline(1) : IMAGE_WIDTH - 1;
                ADD_Edge_Line(_left, L_corner_col, L_corner_row, L_corner_col + (col_right - L_corner_col) * 2 / 3, L_corner_row - 15, L_corner_row - 1, L_corner_row - 25<10?10:L_corner_row - 25);
                return;
            }*/

            if(halo_five_change_flag == 1)
                ADD_Edge_Line(_left,IMAGE_WIDTH/3, IMAGE_HEIGHT - 1, current_left_col[End_Line + 1]<IMAGE_WIDTH/2?IMAGE_WIDTH/2:current_left_col[End_Line + 1] , IMAGE_HEIGHT/3 , IMAGE_HEIGHT - 8, 30);
            // 放宽需求，避免多余动作
            //my_fabs(gyro_angel_z_add) > 65.0&&(End_Line <= 17) ||    my_fabs(gyro_angel_z_add) >= 40.0 &&
            if (gyro_angle_z_add > 11 || (gyro_angle_z_add > 11 && End_Line <= 16 && (left_effect_row_start >= IMAGE_HEIGHT - 14 || (right_effect_row_end<48 && right_effect_row_end>32)) && halo_five_change_flag == 1 ))//&& current_left_col[left_effect_row_start] > 15
            {
                //encounter_add = 0;
                right_halo_flag = 6;
                halo_six_right_row = 0;  // ★进状态6先清零:理由同左环岛(防上次环岛旧值导致 R_end_flag 提前置1)
                //right_findflag = 0;
                return;
            }
            /*else
            {
                int start_col = 0;
                if (right_effect_row_start > IMAGE_HEIGHT - 10)
                    start_col = current_right_col[IMAGE_HEIGHT - 1] - straightwidth[30];
                else
                    start_col = IMAGE_WIDTH * 1 / 3;
                start_col > IMAGE_WIDTH * 1 / 3 ? IMAGE_WIDTH * 1 / 3 : start_col;
                if (End_Line <= 20 && End_Line_col > IMAGE_WIDTH / 2 && left_effect_row_start > IMAGE_HEIGHT / 2 + 15 && left_effect_row_end < IMAGE_HEIGHT - 20)
                {
                    ADD_Edge_Line(_left, start_col, IMAGE_HEIGHT - 1, current_left_col[(left_effect_row_start + End_Line) / 2], (left_effect_row_start + End_Line) / 2, IMAGE_HEIGHT - 1, (left_effect_row_start + End_Line) / 2);
                    right_findflag = 0;
                }
                else
                {
                    int choose_row = End_Line < 20 ? 20 : End_Line;
                    if (current_right_col[choose_row] > start_col)
                    {
                        if(big_or_small==_small)
                            ADD_Edge_Line(_left, start_col, IMAGE_HEIGHT - 1, current_right_col[choose_row] - straightwidth[IMAGE_HEIGHT - 1 - choose_row], choose_row, IMAGE_HEIGHT - 1, choose_row);
                        else
                        {
                            int col2 = current_right_col[choose_row] - straightwidth[IMAGE_HEIGHT - 1 - choose_row]>IMAGE_WIDTH*2/3?IMAGE_WIDTH*2/3:current_right_col[choose_row] - straightwidth[IMAGE_HEIGHT - 1 - choose_row];
                            ADD_Edge_Line(_left, start_col, IMAGE_HEIGHT - 1, col2, choose_row, IMAGE_HEIGHT - 1, choose_row);
                        }
                    }
                    else
                    {
                        if(big_or_small==_small)
                            ADD_Edge_Line(_left, start_col, IMAGE_HEIGHT - 1, IMAGE_WIDTH - straightwidth[IMAGE_HEIGHT - 1 - choose_row], choose_row, IMAGE_HEIGHT - 1, choose_row);
                        else
                        {
                            int col2 = IMAGE_WIDTH - straightwidth[IMAGE_HEIGHT - 1 - choose_row]>IMAGE_WIDTH*2/3?IMAGE_WIDTH*2/3:IMAGE_WIDTH - straightwidth[IMAGE_HEIGHT - 1 - choose_row];
                            ADD_Edge_Line(_left, start_col, IMAGE_HEIGHT - 1, col2, choose_row, IMAGE_HEIGHT - 1, choose_row);
                        }
                    }
                }
            }*/
        }
        if (right_halo_flag == 6)
        {
            // 与左环岛6状态逐字段镜像:左边界(==1)↔右边界(==148),跳变方向取反,补线补对侧。
            uint8 bux_flag = 0;
            if(left_effect_row_start<=58 && right_halo_six==0)
                ADD_Edge_Line(_left, current_left_col[IMAGE_HEIGHT-1], IMAGE_HEIGHT-1, current_left_col[left_effect_row_end], left_effect_row_end, IMAGE_HEIGHT-1, 10);
            if (End_Line < IMAGE_HEIGHT - 20)
            {
                for (uint8 i = IMAGE_HEIGHT - 20; i > End_Line + 2; i--)
                 {
                     if (current_right_col[i + 5] - current_right_col[i] > 15)
                     {
                         right_halo_six = 1;
                         break;
                     }
                 }
                if(right_halo_six==1)
                    bux_flag = 1;
                // 只在近处带(i>34,与下方 R_end_flag 阈值一致)找出环角点:否则远处十字的角点也会
                // 把 bux_flag 顶成2,近处角点消失后远处角点仍在→bux_flag永远回不到1→卡在状态6出不来。
                for (uint8 i = IMAGE_HEIGHT - 10; i > End_Line + 2 && i > 34; i--)
                  {
                     if ((current_right_col[i + 5] - current_right_col[i] > 15) && right_halo_six==1 && (current_right_col[i+2]==148||current_right_col[i+4]==148||current_right_col[i+6]==148))
                         {
                          bux_flag = 2;
                          halo_six_right_row = i;
                          break;
                         }
                  }
                if(halo_six_right_row>34)
                     R_end_flag=1;
            }
            right_findflag = 0;
            if (R_end_flag==1 && bux_flag == 1 && left_effect_row_start >= IMAGE_HEIGHT - 5 && right_effect_row_start >= IMAGE_HEIGHT - 5)
            {
                enable_element_search_flag = 1;
                four_mid_flag = 0;
                element = 0;
                R_end_flag = 0;
                right_findflag = 1;
                left_findflag = 1;
                right_one_change_two_flag=0;
                right_halo_six = 0;
                right_halo_flag = 0;
                if_cross_flag=1;
                enable_gyro_flag=0;
                gyro_angle_z_add = 0;
                return;
            }
        }
        left_halo_flag = 0;
    }
}
/*--------------------------------------------------------------------------
 * 【函数功能】：寻找最大封顶行，防止左右八邻域边线相互覆盖
 * 【参    数】：无
 * 【返 回 值】：End_Line
 * 【备    注】：无
 *--------------------------------------------------------------------------*/
// 定义：分配内存，必须写在函数外（全局作用域）
int End_line = 0; 
uint8 get_min_value(uint8 a[], int n)
{
    int min, min_col = 0;
    min = a[0];
    for (int i = 1; i < n; i++)
    {
        if (min > a[i])
        {
            min = a[i];
            min_col = i;
        }
    }
    int left_cnt = 0, right_cnt = 0, sum = 0;
    for (int i = min_col; i > 0; --i)
    {
        if (myabs_Int(a[i] - min) > 1)
            break;
        --left_cnt;
    }
    for (int i = min_col; i < n; ++i)
    {
        if (myabs_Int(a[i] - min) > 1)
            break;
        ++right_cnt;
    }
    sum = left_cnt + right_cnt;
    End_Line_col = END_LINE_GAP + END_LINE_GAP * (min_col + sum * 0.5);
    return min;
}
int16 white_col_max[2] = {0};
uint8 End_Line_Count[END_LINE_NUM]; // 储存搜索封顶行的值
int16 white_col[2][END_LINE_NUM] = {0}; //[0]行 [1]列
void Find_End_Line()
{
    End_Line = 0;
    white_col_max[0] = IMAGE_HEIGHT - 2;
    for (int j = END_LINE_GAP; j < IMAGE_WIDTH - 1; j += END_LINE_GAP)
    {
        for (int i = IMAGE_HEIGHT - 2; i > 5; i--)
        {
            if (!white_col[1][j / END_LINE_GAP - 1])
                white_col[1][j / END_LINE_GAP - 1] = j;
            if (arr[i][j] == Black /*&& arr[i - 1][j] == Black*/)
            {
                End_Line_Count[j / END_LINE_GAP - 1] = i;
                white_col[0][j / END_LINE_GAP - 1] = i;
                break;
            }
            else
                End_Line_Count[j / END_LINE_GAP - 1] = IMAGE_HEIGHT - 5;
        }
        // 最长白列
        if (white_col[0][j / END_LINE_GAP - 1] < white_col_max[0])
        {
            white_col_max[0] = white_col[0][j / END_LINE_GAP - 1];
            white_col_max[1] = white_col[1][j / END_LINE_GAP - 1];
        }
    }
    //---比较出最小封顶点，作为最终封顶点End_Line,即这个封顶为最远点
    End_Line = get_min_value(End_Line_Count, END_LINE_NUM);
    End_Line = (End_Line >= 10) ? End_Line : 10; // 小于12则直接封顶
  /*  if (element == 8 && luzhang_cnt >= 2)
    {
        if (End_Line < IMAGE_HEIGHT - 5 && End_Line > 18)
            End_Line = 18;
    }
    if (element == 1 && ruku_cnt == 2)
    {
        End_Line = (End_Line >= 18) ? End_Line : 18; // 小于4则直接封顶
    }*/
}
/*--------------------------------------------------------------------------
 * 【函数功能】：判断直道、入弯、弯道
 * 【核心思路】：
 *      按原版 qulv_characteriristic 的思想：
 *      每 5 行统计一次中线左右变化量，
 *      用 qulv_saver 保存每段弯曲强度，
 *      再用 max_value / max_k / clo / rem 判断状态。
 *
 * 【改动】：
 *      只分析 End_Line 到图像底部有效区域的上面三分之一，
 *      不再把近处图像底部全部算进去。
 *--------------------------------------------------------------------------*/
void qulv_characteriristic()
{
    if (element == 0)
    {
        const int BOTTOM_SAFE_ROW = IMAGE_HEIGHT - 2;
        const int START_OFFSET = 10;
        const int SEG_LEN = 5;
        const int STEP = 4;
        const int MAX_SEG = 30;

        int qulv_saver[MAX_SEG] = {0};      // 弯曲强度
        int qulv_dir_saver[MAX_SEG] = {0};  // 弯曲方向，保留正负号
        int k = 0;

        int start_base = End_Line;

        if (start_base < 0)
        {
            start_base = 0;
        }

        if (start_base > BOTTOM_SAFE_ROW)
        {
            start_base = BOTTOM_SAFE_ROW;
        }

        // 只看 End_Line 到底部的上面三分之一
        int total_range = BOTTOM_SAFE_ROW - start_base + 1;

        if (total_range <= 0)
        {
            ruwan_flag = 0;
            zhi_flag = 0;
            wd_flag = 0;
            return;
        }

        int scan_bottom = start_base + total_range / 3;

        if (scan_bottom > BOTTOM_SAFE_ROW)
        {
            scan_bottom = BOTTOM_SAFE_ROW;
        }

        int start_row = start_base + START_OFFSET;

        // 如果 End_Line + 10 超过了上三分之一区域，就退一点，避免 k=0
        if (start_row + SEG_LEN > scan_bottom)
        {
            start_row = start_base + 2;
        }

        if (start_row < 0)
        {
            start_row = 0;
        }

        if (start_row + 1 >= IMAGE_HEIGHT)
        {
            ruwan_flag = 0;
            zhi_flag = 0;
            wd_flag = 0;
            return;
        }

        // ==================== 1. 分段统计弯曲强度和方向 ====================

        for (int i = start_row; i <= scan_bottom; i += STEP)
        {
            if (k >= MAX_SEG)
            {
                break;
            }

            qulv_left = 0;
            qulv_right = 0;

            for (int j = i; j < i + SEG_LEN; ++j)
            {
                if (j + 1 >= IMAGE_HEIGHT)
                {
                    break;
                }

                if (j >= scan_bottom)
                {
                    break;
                }

                int diff = centralline[j] - centralline[j + 1];

                if (diff > 0)
                {
                    qulv_right += diff;
                }
                else if (diff < 0)
                {
                    qulv_left += -diff;
                }
            }

            /*
             * 原来只存 abs，方向丢失。
             * 现在多存一个带方向的值。
             *
             * qulv_right > qulv_left：认为是右弯趋势
             * qulv_left  > qulv_right：认为是左弯趋势
             */
            int signed_qulv = qulv_right - qulv_left;

            qulv_dir_saver[k] = signed_qulv;
            qulv_saver[k] = myabs_Int(signed_qulv);

            k++;
        }

        if (k <= 0)
        {
            ruwan_flag = 0;
            zhi_flag = 0;
            wd_flag = 0;
            return;
        }

        // ==================== 2. 统计 rem / clo ====================

        int rem = 0;
        int clo = 0;

        for (int i = 0; i < k / 2; i++)
        {
            rem += qulv_saver[i];
        }

        for (int i = k / 2; i < k; i++)
        {
            clo += qulv_saver[i];
        }

        // ==================== 3. 统计 max_value / max_k ====================

        int max_value = qulv_saver[0];
        int max_value_position = 0;

        int max_k = 0;
        int max_k_position = 0;

        for (int i = 0; i < k; i++)
        {
            if (qulv_saver[i] > max_value)
            {
                max_value = qulv_saver[i];
                max_value_position = i;
            }

            if (i + 1 < k)
            {
                int dk = myabs_Int(qulv_saver[i + 1] - qulv_saver[i]);

                if (dk > max_k)
                {
                    max_k = dk;
                    max_k_position = i;
                }
            }
        }

        // ==================== 4. 统计整体方向 ====================

        long dir_sum = 0;
        long dir_weight_sum = 0;

        for (int i = 0; i < k; i++)
        {
            int strength = qulv_saver[i];

            // 太小的段当噪声，不参与方向判断
            if (strength <= 1)
            {
                continue;
            }

            /*
             * 权重用弯曲强度。
             * 哪些段弯得明显，哪些段对方向影响更大。
             */
            dir_sum += (long)qulv_dir_saver[i] * strength;
            dir_weight_sum += strength;
        }

        int curve_side = 0;

        if (dir_weight_sum > 0)
        {
            if (dir_sum > 0)
            {
                curve_side = 1;      // 右弯
            }
            else if (dir_sum < 0)
            {
                curve_side = -1;     // 左弯
            }
            else
            {
                curve_side = 0;
            }
        }

        /*
         * 如果实车发现左右反了，就把这里打开：
         *
         * curve_side = -curve_side;
         */

        // ==================== 5. 判断弯道 wd_flag ====================

        if (!(L_corner_flag && R_corner_flag))
        {
            bool is_curve = false;

            if (End_Line >= 25 && myabs_Int(col_Differ) > 20)
            {
                is_curve = true;
            }
            else if (End_Line < 25 && clo + rem > 10 && clo >= 5)
            {
                is_curve = true;
            }

            if (is_curve)
            {
                /*
                 * 这里是你要的效果：
                 * 左弯 wd_flag = -1
                 * 右弯 wd_flag = 1
                 * 方向不明确 wd_flag = 0
                 */
                wd_flag = curve_side;
                zhi_flag = 0;
                ruwan_flag = 0;
            }
            else
            {
                wd_flag = 0;
            }

            // ==================== 6. 判断入弯 ruwan_flag ====================

            if (wd_flag == 0 &&
                max_value_position <= 1 &&
                max_value > 5 &&
                End_Line <= 25 &&
                clo < 20)
            {
                wd_flag = 0;
                zhi_flag = 0;
                ruwan_flag = 1;
            }
            else
            {
                ruwan_flag = 0;
            }
        }
        else
        {
            wd_flag = 0;
            ruwan_flag = 0;
        }

        // ==================== 7. 判断直道 zhi_flag ====================

        if (ruwan_flag == 0 &&
            End_Line <= 14 &&
            max_value <= 5 &&
            max_k <= 3)
        {
            zhi_flag = 1;
            ruwan_flag = 0;
            wd_flag = 0;
        }
        else
        {
            zhi_flag = 0;
        }
    }
    else
    {
        ruwan_flag = 0;
        zhi_flag = 0;
        wd_flag = 0;
        bigwan_flag = 0;

        if (ruku_cnt == 2)
        {
            ruwan_flag = 1;
        }
    }
}

// 发车前恢复图像处理的“首帧”状态。
// 这里只清运行态，不改 straightwidth、out_boundary 等赛道标定/菜单参数。
void reset_image_runtime_state(void)
{
    image_threshold = 0;
    Search_Stop_Line = 0;
    min_efect_row_end = 0;
    memset(White_Column, 0, sizeof(White_Column));
    memset(Longest_White_Column_Left, 0, sizeof(Longest_White_Column_Left));
    memset(Longest_White_Column_Right, 0, sizeof(Longest_White_Column_Right));
    memset(Image, 0, sizeof(Image));
    memset(arr, 0, sizeof(arr));

    memset(L_edge, 0, sizeof(L_edge));
    memset(R_edge, 0, sizeof(R_edge));
    L_edge_count = 0;
    R_edge_count = 0;
    dire_left = 0;
    dire_right = 0;
    left = 0;
    right = 0;
    End_Line = 0;
    left_effect_row_start = 0;
    left_effect_index_start = 0;
    right_effect_index_start = 0;
    right_effect_row_start = 0;
    left_effect_row_end = 0;
    right_effect_row_end = 0;
    left_effect_index_end = 0;
    right_effect_index_end = 0;
    left_top = 0;
    right_top = 0;
    left_top_old = 0;
    right_top_old = 0;
    top_line = 0;

    for (int row = 0; row < IMAGE_HEIGHT; row++)
    {
        current_left_col[row] = 0;
        current_right_col[row] = IMAGE_WIDTH - 1;
        current_left_row[row] = row;
        current_right_row[row] = row;
        linewideth[row] = IMAGE_WIDTH - 1;
        centralline[row] = IMAGE_WIDTH / 2;
    }
    bottom_narrow_flag = 0;
    bottom_narrow_clear_cnt = 0;

    no_effect_lost_right = 0;
    no_effect_lost_left = 0;
    effect_lost_right = 0;
    effect_lost_left = 0;
    left_effect_row = 0;
    right_effect_row = 0;
    right_line_lost_first_Flag = 0;
    left_line_lost_first_Flag = 0;
    L_all_effect_line = 0;
    R_all_effect_line = 0;
    memset(diuxianshu, 0, sizeof(diuxianshu));
    memset(XiuZLZdiuxianshu, 0, sizeof(XiuZLZdiuxianshu));

    L_corner_flag = 0;
    L_corner_row = 0;
    L_corner_col = 0;
    L_corner_angle = 0;
    R_corner_flag = 0;
    R_corner_row = 0;
    R_corner_col = 0;
    R_corner_angle = 0;
    L_corner_count = 0;
    R_corner_count = 0;
    left_findflag = 0;
    right_findflag = 0;
    add_line_k = 0.0f;
    add_line_b = 0.0f;

    lc_flag = 0;
    rc_flag = 0;
    if_cross_flag = 1;
    enable_element_search_flag = 1;
    left_halo_flag = 0;
    right_halo_flag = 0;
    buxian_crossl_row = 0;
    buxian_crossl_col = 0;
    buxian_crossr_row = 0;
    buxian_crossr_col = 0;
    bx_lastl_exist_flag = 0;
    bx_lastr_exist_flag = 0;
    element = 0;
    cross_cnt = 0;
    cross_gesture = 0;
    crossl_end_flag = 0;
    crossr_end_flag = 0;
    End_Line_col = 0;
    xieshi = 1;

    enable_banmaflag = 0;
    my_l_count = 0;
    my_r_count = 0;
    max_colsrow = 0;
    max_colindex = 0;
    memset(my_L_edge, 0, sizeof(my_L_edge));
    memset(my_R_edge, 0, sizeof(my_R_edge));
    diu = 0;
    shi_end_flag = 0;
    shi_tioachuflag = 0;

    re_flag = 0;
    DIFFERROW = 0;
    Aim_Line = IMAGE_HEIGHT - 5;
    turn_propect = 0;
    col_Differ = 0.0f;
    Last_col_Differ = 0.0f;
    qulv_left = 0;
    qulv_right = 0;
    zhi_flag = 0;
    ruwan_flag = 0;
    bigwan_flag = 0;
    wd_flag = 0;
    wd_flag_stable = 0;
    wd_stable_candidate = 0;
    wd_stable_count = 0;
    ruku_cnt = 0;

    banmaxian_flag = 0;
    out_flag_judge = 0;
    out_count = 0;

    big_or_small = 0;
    k_left = 0.0f;
    k_left_s = 0.0f;
    k_right = 0.0f;
    k_right_s = 0.0f;
    k_right1 = 0.0f;
    k_right2 = 0.0f;
    Rzhong_guai_flag = 0;
    Lzhong_guai_flag = 0;
    L_zhongguai_x = 0;
    L_zhongguai_y = 0;
    R_zhongguai_x = 0;
    R_zhongguai_y = 0;
    Lhuan_bai = 0;
    Rhuan_bai = 0;
    enable_left_circle_flag = 1;
    enable_right_circle_flag = 1;
    halo_up_corner_row = 0;
    halo_up_corner_col = 0;
    halo_up_corner_flag = 0;
    right_diu = 0;
    jilu_col = IMAGE_WIDTH / 2;
    right_one_change_two_flag = 0;
    left_one_change_two_flag = 0;
    halo_five_change_flag = 0;
    L_halo_five_change_flag = 0;
    R_end_flag = 0;
    L_end_flag = 0;
    k_halo_left = 0.0f;
    k_halo_right = 0.0f;
    Lmax_colsrow = 0;
    Rmax_colsrow = 0;
    right_halo_six = 0;
    left_halo_six = 0;
    four_mid_flag = 0;
    halo_six_left_row = 0;
    halo_six_right_row = 0;
    enable_gyro_flag = 0;
    gyro_angle_z_add = 0.0f;
    End_line = 0;
    memset(End_Line_Count, 0, sizeof(End_Line_Count));
}
