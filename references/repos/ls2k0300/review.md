# vvsakivv/LS2K0300

用途：逐飞示例与三类识别差速应用。固定提交 `b16ed55a9e1f89c3f8f81090901b18865e57a4d9`。

来源：[仓库](https://gitee.com/vvsakivv/LS2K0300)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：Example为外设/TFLite示例；Seekfree_LS2K0300_Opensource_Library/project/user包含main、vision、image_recognition、lq_ncnn、chassis、esc及调试显示。应用按视觉外环、角速度中环、轮速内环组织，pit_callback执行后两环。

模型：image_recognition配置64×64三类NCNN，标签supplies/vehicle/weapon映射右/直/左，LQ_NCNN检查BGR输入类型、输入输出返回值、1D FP32输出维度、非有限值并采用减max的Softmax。优于仅argmax不检查张量的封装，但仍需确认模型是否自带Softmax。

ROI契约：build_model_input要求生成者给出固定64×64；vision.cpp实际设置IMAGE_ROI_SIZE，不能只看到前者不resize就指认缺陷。修改搜索图分辨率或ROI生成后要重新验证尺度。

退出：cleanup先停止pit_timer再关电调/底盘，g_final_stop_requested参与回调门控，是可借鉴的停机次序；仍需检查全部跨线程路径，不把一个atomic当完整快照。

实测边界：另一个随仓库提供的loong_cnn_model_simple.tflite在本机CPU上已完成合成零输入Invoke，真实输入1×40×40×3、float32，输出1×3且有限。它不是上述64×64 NCNN应用模型，不能混用配置；该测试不证明TFLM算子覆盖、识别精度或龙芯时延。

本次保存72份文件/成员的定位与校验信息，索引得到235个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
