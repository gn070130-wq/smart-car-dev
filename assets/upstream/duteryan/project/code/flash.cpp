#include "zf_common_headfile.hpp" 
#include "menu.hpp"

uint32 parameter_data[6][14]={0};
static const uint32 CARD_SEQUENCE_CONFIG_MAGIC = 20260723;
static const uint32 CAMERA_EXPOSURE_CONFIG_MAGIC = 20260724;

// 模式："r+" 可读可写，文件不存在会失败（需要初始化逻辑兜底）
zf_driver_file_string file_u32(FILENAME,"r+");

char buffer[100] = {0};

// ====================== 保存参数（覆盖写入） ======================
void Parameter_Write(void)
{
    // 1. 把菜单参数最新值拷贝到数组
    for(uint8_t i=0;i<Para1_LENGTH;i++)   parameter_data[0][i] = *(Para1[i].value);
    for(uint8_t i=0;i<Para2_LENGTH;i++)   parameter_data[1][i] = *(Para2[i].value);
    for(uint8_t i=0;i<Para3_LENGTH;i++)   parameter_data[2][i] = *(Para3[i].value);
    for(uint8_t i=0;i<Para4_LENGTH;i++)   parameter_data[3][i] = *(Para4[i].value);   
    for(uint8_t i=0;i<Para5_LENGTH;i++)   parameter_data[4][i] = *(Para5[i].value);
    for(uint8_t i=0;i<Cama1_LENGTH;i++)   parameter_data[5][i] = *(Cama1[i].value);
    // 第83行保存新版曝光菜单格式：0=自动、1=手动；与旧版底层1/3模式区分。
    parameter_data[5][12] = CAMERA_EXPOSURE_CONFIG_MAGIC;
    // 第84行保存五卡片开关的格式标记，不占菜单槽；用于区分旧参数文件的保留零值。
    parameter_data[5][13] = CARD_SEQUENCE_CONFIG_MAGIC;

    // 2. 【关键】复位指针到文件开头
    // 这样写数据会从开头覆盖，而不是追加
    file_u32.rewind_file();

    // 3. 写入全新的 6 页参数
    for (uint8_t i=0;i<6;i++)
    {
        for (uint8_t j=0;j<14;j++)
        {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%d\n", parameter_data[i][j]);
            // 每次写入都会覆盖掉旧数据
            file_u32.write_string(buffer);
        }
    }

    // 4. 写完再次复位，方便下次读取
    file_u32.rewind_file();
}

// ====================== 上电读取参数 ======================
void Parameter_Read(void)
{
    // 上电先复位指针
    file_u32.rewind_file();

    for (uint8_t i=0;i<6;i++)
    {
        for (uint8_t j=0;j<14;j++)
        {
            memset(buffer, 0, sizeof(buffer));
            // 读取每一个数字
            file_u32.read_string(buffer);
            parameter_data[i][j] = atoi(buffer);
        }
    }

    // 5. 把数组里的数据赋值给菜单变量
    for(uint8_t i=0;i<Para1_LENGTH;i++)   *(Para1[i].value) = parameter_data[0][i];
    for(uint8_t i=0;i<Para2_LENGTH;i++)   *(Para2[i].value) = parameter_data[1][i];
    for(uint8_t i=0;i<Para3_LENGTH;i++)   *(Para3[i].value) = parameter_data[2][i];
    for(uint8_t i=0;i<Para4_LENGTH;i++)   *(Para4[i].value) = parameter_data[3][i];   
    for(uint8_t i=0;i<Para5_LENGTH;i++)   *(Para5[i].value) = parameter_data[4][i];
    for(uint8_t i=0;i<Cama1_LENGTH;i++)   *(Cama1[i].value) = parameter_data[5][i];

    // 旧相机菜单直接保存V4L2底层值(1=手动、3=自动)。新版改为直观的
    // 0=自动、1=手动，并用第83行格式标记消除“旧1/新1”的歧义。
    if(parameter_data[5][12] != CAMERA_EXPOSURE_CONFIG_MAGIC)
    {
        if(parameter_data[5][0] == UVC_AUTO_EXPOSURE_ENABLE)
        {
            // 旧文件处于自动曝光时，旧Expo并未实际生效；新手动曝光值从要求的140起步。
            camera_exposure_mode = 0;
            camera_manual_exposure = 140;
        }
        else
        {
            // 旧文件已经使用手动曝光时保留用户调好的曝光值。
            camera_exposure_mode = 1;
            camera_manual_exposure = (parameter_data[5][1] > 0) ?
                                     static_cast<int>(parameter_data[5][1]) : 140;
        }
    }
    else
    {
        camera_exposure_mode = (camera_exposure_mode == 0) ? 0 : 1;
        if(camera_manual_exposure <= 0) camera_manual_exposure = 140;
    }
    sync_camera_exposure_config();

    // 兼容增加 BkSp 之前的旧参数文件：Para5第5槽（文件第61行）原本固定为0。
    // 0不是有效的软减速目标，自动恢复原先代码中的固定值60；菜单保存后会正式写回第61行。
    if(black_soft_speed <= 0) black_soft_speed = BLACK_SOFT_SPEED_DEFAULT;

    // Para5 第9项（parameter_new.txt 第65行）是卡片识别总开关，只接受0/1。
    // 旧参数文件该保留槽通常为0；异常值按默认“开启识别”处理。
    card_recognition_disable = (card_recognition_disable == 1) ? 1 : 0;

    // Card1~Card5 使用 Para5 第10~14项（parameter_new.txt 第66~70行）。
    // 旧参数文件这些位置全是保留零，不能直接解释为“跳过全部卡片”，否则升级后行为会突变。
    // 第84行没有格式标记时按兼容默认值11111加载；首次从菜单保存后写入标记并持久化真实配置。
    if(parameter_data[5][13] != CARD_SEQUENCE_CONFIG_MAGIC)
    {
        for(int i=0;i<CARD_SEQUENCE_COUNT;i++) card_sequence_enable[i] = 1;
    }
    else
    {
        for(int i=0;i<CARD_SEQUENCE_COUNT;i++)
            card_sequence_enable[i] = (card_sequence_enable[i] == 1) ? 1 : 0;
    }
}

void Parameter_Init(void)
{
    // 这里可以加一个判断：如果文件读取失败（读到0），就加载默认参数
    // 但你现在的逻辑是上电直接读，保持空实现即可
}
