# chao-8xx/car

用途：基于Wuwu库的多线程竞赛应用。固定提交 `cd8c64c6d7fc75f783264b3006d961b767300541`。

来源：[仓库](https://gitee.com/chao-8xx/car)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：WuwuSama_Icar_Project提供底层库，run才是应用；run/src划分pthread_control、preprocess、vision_tracking_base、vision_element_state、vision_model、control_decision、element_control、PID/LADRC和菜单。读取README及模型/队列等关键交界，未覆盖全部分支。

算法组织：采集与视觉通过条件变量传帧；视觉产生道路状态和模型动作，编码器/IMU提供运动量，电机线程进行速度及控制决策。模型具有识别触发和菜单标定队列触发两条路径，不能把后者产生的动作当作实时神经网络识别证据。

模型：vision_model中按三类supplies/transport/weapon组织NCNN及红色候选、ROI、动作和距离恢复；应分别核查文件可用性、分类标签、触发来源和控制上下文恢复。

复查排误：vision_model_queue.cpp的queue_adjust已使用i<rear搬移有效区间；源码注释提到旧版<=rear问题，不表示当前仍存在该越界。只根据历史注释报错属于误报。

可借鉴：状态上下文保存/恢复、模型触发路径可观察性、模块化现场工具。初始API树曾被截断，后通过Git得到完整树并校验应用快照；未审计内核及整个交叉库。

本次保存79份文件/成员的定位与校验信息，索引得到472个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
