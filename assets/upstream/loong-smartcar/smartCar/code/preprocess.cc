#include "preprocess.h"

Preprocess::Preprocess(){}

Preprocess::~Preprocess(){}

int Preprocess::Preprocess_init(void)
{
    ifstream file("./preprocess.json");
    if(!file.is_open()) {
        cerr << "Error: Failed to Open json file" << endl;
        return -1;
    }

    // 解析 JSON 数据
    Json::Value jsonvalue;
    Json::Reader reader;
    if(!reader.parse(file, jsonvalue)) { /* 解析file文件中的数据 -> Value jsonvalue */
        cerr << "Error: Failed to parse json file" << endl;
        return -1;
    }
    file.close(); // 关闭输入文件

    // 获取 JSON 数据
    /* string   str = jsonvalue["str_name"].asString();      获取名字为str_name的字符串
        * 
        * int      age = jsonvalue["age_name"].asInt();         获取名字为age_name的整型
        * 
        * bool     is  = jsonvalue["is_name"].asBool();         获取名字为is_name的Bool值  
        * 
        * Value    val = jsonvalue["val"];                      直接获取名字val json数据类型的值
        * 
        */
    rowCutUp = jsonvalue["rowCutUp"].asUInt();
    rowCutBottom = jsonvalue["rowCutBottom"].asUInt();
    imglimiter = jsonvalue["imglimiter"].asInt();
    
    L_motor_kp = jsonvalue["L_motor_kp"].asInt();
    L_motor_ki = jsonvalue["L_motor_ki"].asInt();
    R_motor_kp = jsonvalue["R_motor_kp"].asInt();
    R_motor_ki = jsonvalue["R_motor_ki"].asInt();

    fuzzy.error_max = jsonvalue["fuzzy_error_max"].asInt();
    fuzzy.dedt_max = jsonvalue["fuzzy_dedt_max"].asInt();
    fuzzy.kp_buff = jsonvalue["fuzzy_kp_buff"].asFloat();
    fuzzy.kd_buff = jsonvalue["fuzzy_kd_buff"].asFloat();

    trackAim = jsonvalue["trackAim"].asInt();
    motorspeed_low = jsonvalue["motorspeed_low"].asInt();
    motorspeed_high = jsonvalue["motorspeed_high"].asInt();
    motorspeed_weakhigh = jsonvalue["motorspeed_weakhigh"].asInt();

    
    ring_intoSpeed = jsonvalue["ring_intoSpeed"].asInt();
    ringSpeed = jsonvalue["ringSpeed"].asInt();
    crossSpeed_h = jsonvalue["crossSpeed_h"].asInt();
    crossSpeed_l = jsonvalue["crossSpeed_l"].asInt();
    barrier_speed = jsonvalue["barrier_speed"].asInt();
    bridge_speed = jsonvalue["bridge_speed"].asInt();

    showImg = jsonvalue["showImg"].asBool();
    weakRing = jsonvalue["weakRing"].asBool();
    ICAR_HALFW = jsonvalue["ICAR_HALFW"].asFloat();
    LEFT_HALFW_BUF = jsonvalue["LEFT_HALFW_BUF"].asFloat();
    Gryo_Zkd = jsonvalue["Gryo_Zkd"].asFloat();
    akmThreshold = jsonvalue["akmThreshold"].asInt();
    long_restrain = jsonvalue["long_restrain"].asFloat();
    long_weak_restrain = jsonvalue["long_weak_restrain"].asFloat();
    barrier_buffer = jsonvalue["barrier_buffer"].asFloat();
    fans_Maxduty = jsonvalue["fans_Maxduty"].asInt();


    Ring_into_Size = jsonvalue["Ring_into_Size"].asInt();
    Ring_into_Size_Max = jsonvalue["Ring_into_Size_Max"].asInt();
    Ring_Y = jsonvalue["Ring_Y"].asInt();
    Ring_Count = jsonvalue["Ring_Count"].asInt();

    ring1_X_in = jsonvalue["ring1_X_in"].asInt();
    ring1_X_out = jsonvalue["ring1_X_out"].asInt();
    ring1_out_dis = jsonvalue["ring1_out_dis"].asInt();
    ring1_trackAim = jsonvalue["ring1_trackAim"].asInt();

    ring2_X_in = jsonvalue["ring2_X_in"].asInt();
    ring2_X_out = jsonvalue["ring2_X_out"].asInt();
    ring2_out_dis = jsonvalue["ring2_out_dis"].asInt();
    ring2_trackAim = jsonvalue["ring2_trackAim"].asInt();

    ring3_X_in = jsonvalue["ring3_X_in"].asInt();
    ring3_X_out = jsonvalue["ring3_X_out"].asInt();
    ring3_out_dis = jsonvalue["ring3_out_dis"].asInt();
    ring3_trackAim = jsonvalue["ring3_trackAim"].asInt();

    ring4_X_in = jsonvalue["ring4_X_in"].asInt();
    ring4_X_out = jsonvalue["ring4_X_out"].asInt();
    ring4_out_dis = jsonvalue["ring4_out_dis"].asInt();
    ring4_trackAim = jsonvalue["ring4_trackAim"].asInt();

    ring5_X_in = jsonvalue["ring5_X_in"].asInt();
    ring5_X_out = jsonvalue["ring5_X_out"].asInt();
    ring5_out_dis = jsonvalue["ring5_out_dis"].asInt();
    ring5_trackAim = jsonvalue["ring5_trackAim"].asInt();

    zebraOUT = jsonvalue["zebraOUT"].asInt();
    barrierOUT = jsonvalue["barrierOUT"].asInt();

    Bridge_Enable = jsonvalue["Bridge_Enable"].asBool();

    h_l = jsonvalue["h_l"].asInt();
    s_l = jsonvalue["s_l"].asInt();
    v_l = jsonvalue["v_l"].asInt();
    h_h = jsonvalue["h_h"].asInt();
    s_h = jsonvalue["s_h"].asInt();
    v_h = jsonvalue["v_h"].asInt();

    servoMID = jsonvalue["servoMID"].asInt();
    

    cout << "图像显示 = ";     if(showImg) cout << "true"; else cout << "false" << endl;
    cout << "电机低速:" << motorspeed_low << "电机高速:" << motorspeed_high << endl;

    cout << "预处理结束" << endl;

    return 0;
}

int Preprocess::Preprocess_save() {
    // 创建一个新的 Json::Value 对象并填充当前值
    Json::Value jsonvalue;
    
    // 填充所有参数到 jsonvalue
    jsonvalue["rowCutUp"] = rowCutUp;
    jsonvalue["rowCutBottom"] = rowCutBottom;
    jsonvalue["imglimiter"] = imglimiter;

    jsonvalue["L_motor_kp"] = L_motor_kp;
    jsonvalue["L_motor_ki"] = L_motor_ki;
    jsonvalue["R_motor_kp"] = R_motor_kp;
    jsonvalue["R_motor_ki"] = R_motor_ki;

    jsonvalue["fuzzy_error_max"] = fuzzy.error_max;
    jsonvalue["fuzzy_dedt_max"] = fuzzy.dedt_max;
    jsonvalue["fuzzy_kp_buff"] = fuzzy.kp_buff;
    jsonvalue["fuzzy_kd_buff"] = fuzzy.kd_buff;

    jsonvalue["trackAim"] = trackAim;
    jsonvalue["motorspeed_low"] = motorspeed_low;
    jsonvalue["motorspeed_high"] = motorspeed_high;
    jsonvalue["motorspeed_weakhigh"] = motorspeed_weakhigh;
    
    


    jsonvalue["ring_intoSpeed"] = ring_intoSpeed;
    jsonvalue["ringSpeed"] = ringSpeed;
    jsonvalue["crossSpeed_h"] = crossSpeed_h;
    jsonvalue["crossSpeed_l"] = crossSpeed_l;
    jsonvalue["barrier_speed"] = barrier_speed;
    jsonvalue["bridge_speed"] = bridge_speed;
    
    jsonvalue["showImg"] = showImg;
    jsonvalue["weakRing"] = weakRing;
    jsonvalue["ICAR_HALFW"] = ICAR_HALFW;
    jsonvalue["LEFT_HALFW_BUF"] = LEFT_HALFW_BUF;
    jsonvalue["Gryo_Zkd"] = Gryo_Zkd;
    jsonvalue["akmThreshold"] = akmThreshold;
    jsonvalue["long_restrain"] = long_restrain;
    jsonvalue["long_weak_restrain"] = long_weak_restrain;
    jsonvalue["barrier_buffer"] = barrier_buffer;
    jsonvalue["fans_Maxduty"] = fans_Maxduty;
    

    jsonvalue["Ring_into_Size"] = Ring_into_Size;
    jsonvalue["Ring_into_Size_Max"] = Ring_into_Size_Max;
    jsonvalue["Ring_Y"] = Ring_Y;
    jsonvalue["Ring_Count"] = Ring_Count;

    jsonvalue["ring1_X_in"] = ring1_X_in;
    jsonvalue["ring1_X_out"] = ring1_X_out;
    jsonvalue["ring1_out_dis"] = ring1_out_dis;
    jsonvalue["ring1_trackAim"] = ring1_trackAim;

    jsonvalue["ring2_X_in"] = ring2_X_in;
    jsonvalue["ring2_X_out"] = ring2_X_out;
    jsonvalue["ring2_out_dis"] = ring2_out_dis;
    jsonvalue["ring2_trackAim"] = ring2_trackAim;

    jsonvalue["ring3_X_in"] = ring3_X_in;
    jsonvalue["ring3_X_out"] = ring3_X_out;
    jsonvalue["ring3_out_dis"] = ring3_out_dis;
    jsonvalue["ring3_trackAim"] = ring3_trackAim;

    jsonvalue["ring4_X_in"] = ring4_X_in;
    jsonvalue["ring4_X_out"] = ring4_X_out;
    jsonvalue["ring4_out_dis"] = ring4_out_dis;
    jsonvalue["ring4_trackAim"] = ring4_trackAim;

    jsonvalue["ring5_X_in"] = ring5_X_in;
    jsonvalue["ring5_X_out"] = ring5_X_out;
    jsonvalue["ring5_out_dis"] = ring5_out_dis;
    jsonvalue["ring5_trackAim"] = ring5_trackAim;

    jsonvalue["Bridge_Enable"] = Bridge_Enable;

    jsonvalue["zebraOUT"] = zebraOUT;
    jsonvalue["barrierOUT"] = barrierOUT;

    jsonvalue["h_l"] = h_l;
    jsonvalue["s_l"] = s_l;
    jsonvalue["v_l"] = v_l;
    jsonvalue["h_h"] = h_h;
    jsonvalue["s_h"] = s_h;
    jsonvalue["v_h"] = v_h;

    jsonvalue["servoMID"] = servoMID;
    
    // 打开文件准备写入
    std::ofstream outfile("./preprocess.json");
    if(!outfile.is_open()) {
        std::cerr << "Error: Failed to open file for writing" << std::endl;
        return -1;
    }
    
    // 使用 Json::StyledWriter 生成格式化的 JSON
    Json::StyledWriter writer;
    outfile << writer.write(jsonvalue);
    outfile.close();
    
    std::cout << "参数已成功保存" << std::endl;
    return 0;
}

Mat Preprocess::Cut_and_imageGrayING(Mat &img, int start_y, int len_y)
{
    Mat imageGray;

    Rect roi(0, start_y, COLSIMAGE, len_y);

    Mat cutimg = img(roi);

    Size newSize(COLSIMAGE, ROWSIMAGE);

    resize(cutimg, cutimg, newSize, 0, 0, cv::INTER_LINEAR);

    img = cutimg.clone();

    cvtColor(cutimg, imageGray, COLOR_BGR2GRAY);   //RGB转灰度图像

    return imageGray;
}

Mat Preprocess::binaryzation(Mat img)
{
    Mat imageBinary;

    threshold(img, imageBinary, 0, 255, THRESH_OTSU);

    return imageBinary;
}

float Preprocess::calculate_slopeK(int y1, int y2, int x1, int x2)
{
    if(x1 - x2 == 0) {
        return 0.0f;
    }
    return (y1 - y2) * 1.0 / (x1 - x2) * 1.0;
}

float Preprocess::calculate_slopeB(float k, int y, int x)
{
    return (y * 1.0 - k * x);
}

Preprocess preprocess;
