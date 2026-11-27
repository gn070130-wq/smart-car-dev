# damon3964/smart2026

用途：Wuwu设备层上的视觉/场景/检测控制。固定提交 `e271dd5ddde2f007396c28bec8f1c8802d81d981`。

来源：[仓库](https://gitee.com/damon3964/smart2026)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：smartCar/main.cc组织采图10ms、传感器5ms、控制20ms的TimerThread；image_proc负责预处理/IPM，line_detect迷宫巡线，scene_detect处理元素，car_control/pid生成驱动命令。设定周期不等于实测执行周期。

已确认阻断：param/config.json默认启用YOLO并指向tiny_classifier_fp32.ncnn.param/bin；yolov5_detect.cc却要求in0以及out0/out1/out2三尺度检测输出，并用3类anchors解码。所给param是17层单输出分类网络，缺out1/out2，静态契约检查已复现。

修复方向：选择真正匹配的检测模型及预处理/解码，或者改成红色定位+ROI分类+三类任务映射；不能仅改文件名、放宽阈值或忽略extract返回值。

另一个时效风险：采图设置g_frame_ready=true，但控制路径只看bool和非空，缺少源帧序号/捕获时刻消费约束；相机停止更新后旧帧可能重复参与控制。应加新鲜度和故障策略，不能把持续有图像对象当成持续有新帧。

可借鉴：设备/场景模块划分和多速率入口；没有据此给出“可直接上车”的结论，也没有执行原程序电机操作。

本次保存42份文件/成员的定位与校验信息，索引得到145个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
