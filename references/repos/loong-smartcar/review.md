# wuwusama/loong_SmartCar

用途：20届极速光电舵机与阿克曼控制。固定提交 `b9973735dc78c9afef3e38e8d5908910fec31586`。

来源：[仓库](https://gitee.com/wuwusama/loong_SmartCar)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：smartCar的preprocess/tracking_base/crossRoad/ring生成视觉方向，control_thread负责舵机、陀螺抑制、速度和风扇，control_pid/FuzzyCtrl提供控制，kernel_driver含相应设备驱动。

底盘：舵机转向加左右后轮阿克曼速度分配，get_turningRadius由舵机控制量估算半径，ackerman_Lmotor/Rmotor计算轮速。该物理结构与三轮F差速转向不同，不能直接搬入F车方向环。

复查结论：单看ackerman_Lmotor有除以abs(turningRadius)，但control_thread.cpp::ackerman_control已分别处理正半径、负半径和零半径，零时直接给两轮相同目标。因此“正常调用链直道必然除零”是误报；单独复用函数时仍要带上前置条件。

可借鉴：转向机构建模、编码器/电机/舵机分层、模糊参数与现场菜单。前瞻、舵机中位、轮距/轴距和速度单位必须标定。本轮没有确认整车阻断性错误，不代表所有驱动和状态分支都已验证。

本次保存58份文件/成员的定位与校验信息，索引得到243个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
