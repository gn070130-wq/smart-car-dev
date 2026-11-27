# IcksLin/Abandoned

用途：龙芯差速视觉控制与调试工程。固定提交 `d590c5d09566340d2e3a68171fba37043082b69a`。

来源：[仓库](https://github.com/IcksLin/Abandoned)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：project/user入口、my_global设备/任务组织、imgproc赛道视觉、my_pid与LADRC控制、motor执行、red_rect目标ROI、菜单和上位机。README说明160×120巡线及320×240采集；精确标定以当前工程为准。

已读控制链：my_global.cpp的pid_contol_handle组合方向与角速度控制；hight_frequence_encoder_get_speed_handler使用轮速LADRC。该回调中基准目标被设为1，注释明确用于角速度调试，不能拿来当通用巡航速度。car_config选择LADRC、1ms相关参数，另一PID配置为15ms，不能混用尺度。

my_pid.cpp::PDController::compute包含对称非线性项error*abs(error)，不是无符号error²；四段线性权重当前都为0.70，因此“分段”本身没有不同线性增益。LADRC是可读实现，不等于所有控制模式同时启用。

模型：tflm_model_process_lq.cpp文件名带tflm，实际实现LQ_NCNN，使用PIXEL_BGR2RGB并另做Softmax。my_global的NCNN初始化在注释块内，不能凭文件名或类存在认定默认行车已运行识别。

核对项：高频回调在onto_pd_control_enable为假时没有更新speed_to_pwm_l/r，随后仍下发缓存值。若将此开关解释为停止门，就会复用旧命令；需先厘清开关契约，再给独立停止优先级。跨线程状态、实际执行周期与退出路径需结合实车日志验证。

可借鉴：PID/LADRC分模式、参数菜单、双预瞄与ROI调试；模型接口返回值检查、是否重复Softmax与运行使能属于移植前检查点。未逐行证明整套视觉状态机与惯性导航的实车正确性。

本次保存78份文件/成员的定位与校验信息，索引得到303个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
