# amireux_fish/Loognson

用途：逐飞视觉元素与TFLM/NCNN候选识别。固定提交 `9eba6c613b15b2ad42d60dfbc55e115a0b446aeb`。

来源：[仓库](https://gitee.com/amireux_fish/Loognson)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：project/user/main负责图像与元素；deal_image中边界/中线/元素，direction/speed_control闭环，deal_model含红块、ROI和两种分类封装，get_image/device/menu等提供配套。必须追踪Active_classifier的实际类型/编译设置，不能只因两种类存在就称双引擎同时运行。

定位：RGB565→BGR→HSV双区间红色掩膜→赛道边界约束→形态学→轮廓与面积筛选；后续截取ROI并进行分类。区分原分辨率与灰度半分辨率坐标。

已复现：main先cv::threshold(gray_original,gray_original,...OTSU)原地二值化，再用OTSU减偏移对同一binary图阈值处理，通常不能实现原始灰度上的偏移。合成反例Otsu130、调整到110时第二次图与首次完全相同，却与从原灰度110阈值的结果相差10像素。

其他条件风险：static boundary_mask按首次ROI尺寸构造，ROI变化需重建；NCNN分支使用未减max的exp(logits)，大logit反例会非有限；red_miss_count只在失败分支更新，若目标是连续失败计数，成功分支需清零。实际触发与修复效果还需样本/编译分支确认。

可借鉴：赛道内候选约束、识别与控制状态协作；模型输出6类与另一分支3类不应共享同一隐式映射。当前验证是静态与离线反例，不是整车精度或时序认证。

本次保存62份文件/成员的定位与校验信息，索引得到224个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
