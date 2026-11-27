# duteryan/seekfree_-ls2-k0300_-opensource_-library

用途：逐飞工程上的视觉与六类NCNN应用。固定提交 `6c0342d47478ffbc40b16d5b230c238cc5dde1b9`。

来源：[仓库](https://gitee.com/duteryan/seekfree_-ls2-k0300_-opensource_-library)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：project/user/main串接system_init、ncnn_init、pit_init、图像刷新；project/code包括图像/八邻域/最长白列、红区验证、ROI、模糊控制、conctrol、按键/Flash和模型。launch_request在主循环处理prepare_new_run，避免在周期任务内做长清理。

模型：ncnn_model.cpp为64输入、6类顺序APC/Aidbag/ambulance/grenade/gun/telescope，主入口加载model32.param/bin，文件名不等于输入32。运行前核对实际文件、图预处理、标签和动作映射。

可借鉴：静态分配/缓冲复用、输入输出返回值检查、停车ROI锁存、ROI最小尺寸和识别冷却。FP16、packing选项中的CPU能力说明仅对其板卡有效，不能作为所有龙芯平台的统一限制。

已复现数值问题：fast_exp用位级近似，注释称误差<2%；按代码float32公式在[-8,0]取10001点，最大相对误差约6.15%。使用max类分子1只保证部分计算简化，不保证置信度与真实Softmax一致。应比较拒识门限和实车错例，再决定是否保留近似；未验证“快3—5倍”的速度说法。

本次保存79份文件/成员的定位与校验信息，索引得到300个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
