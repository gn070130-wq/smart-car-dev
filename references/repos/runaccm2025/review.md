# hccc1203/RunACCM2025

用途：EdgeBoard完全模型组上位机。固定提交 `23e60c66605f53d6473dd264f16040f43db0178c`。

来源：[仓库](https://github.com/hccc1203/RunACCM2025)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：main读配置并启动线程；capture生产图像；Factory有界队列分给AIConsumer和consumer；Standard.run组织普通路、十字、圆环及special目录中的业务区域，再经Uart发速度/舵机PWM。它是aarch64 EdgeBoard上位机，不是龙芯F车轮速驱动的直接替代。

模型链：include/detection.hpp的PPNC/NNA主干→ONNX Runtime后处理→PPNC NMS；另依赖相机、libserial、glib。模型目录被排除，需post.onnx、io_paddle.json、nms.tar、label_list.txt；不能当作NCNN分类器加载。

算法组织：生产者同帧进入两个深度3的队列，调试队列深度5；满队列丢最旧项，消费端轮询取FIFO。语义识别结果辅助传统路径/元素处理。队列不保证AI与巡线结果来自同一帧。

确认问题：thread.cpp的AIConsumer仅锁自身局部mutex，consumer对共享predict_result读取及传给Standard.run没有持相同锁，不能保护跨线程vector；另缺少预测结果的源帧时间戳绑定。应发布带帧号、时间戳的不可变预测快照，并定义最大年龄。

边界：Factory::consume为空时轮询等待且无关闭条件，线程退出不能仅依赖普通flag和exit；uartReceive中接收调用被注释且空转。退出可靠性和CPU影响需进一步运行验证。

可借鉴：多阶段推理、队列分流和业务状态模块；本轮深入读取的是入口、队列、AI与控制交界，未逐一覆盖special状态机所有分支，不能宣称所有赛道任务已验证。

本次保存43份文件/成员的定位与校验信息，索引得到95个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
