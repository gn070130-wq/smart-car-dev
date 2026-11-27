# wuwusama/SmartCar_99Pai_OpenSource

用途：21届龙芯底层库与示例。固定提交 `c0c76f75e8872b2b5506525ebe6d608c9aaf435d`。

来源：[仓库](https://gitee.com/wuwusama/SmartCar_99Pai_OpenSource)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

来源：Wuwu129旧主页跳转到wuwusama；公开仓库列表显示本项目及loong_SmartCar。本次只分析可见公开项目，不推断私人仓库。

结构：WuwuSama_Icar_Project包含ww_camera、ww_motor、ww_icm42688、ww_timerThread、ToF、图传等封装；example目录有电机、编码器、相机、传输及ncnn_yolov5n示例，另有硬件工程文件。

有效性：smartCar/main.cc当前只有空while循环，故这是库和例程集合，不能因为README写“走马观碑”就认定存在完整竞赛行车状态机。应从具体example入口理解接口，不能直接运行主程序期待循迹。

模型：YOLOv5n示例按anchors/stride生成框并做后处理；它与目标板ROI三分类不同。模型param/bin在目录树可见，本文读取了param和示例代码，没有对该NCNN权重作推理验证。

可借鉴：标准设备封装、示例组织、硬件接口和图传；父项目相同或共享库相同不意味着其派生仓库的运行策略也相同。

本次保存45份文件/成员的定位与校验信息，索引得到122个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
