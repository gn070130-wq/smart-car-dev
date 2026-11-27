# 代码复查与离线证据

结论只针对各manifest固定版本。修复建议作为迁移/重构时的方案，未修改或提交上游代码。详细原始测量见 [offline-evidence.json](offline-evidence.json)，实际模型调用见 [tflite-smoke.json](tflite-smoke.json)。

## 已确认并有离线反例

### F01 smart2026 默认模型与解码器不匹配

位置：`smartCar/param/config.json`的YOLO配置、`code/yolov5_detect.cc`的 `YoloDetector::detect` 与 `tiny_classifier_fp32.ncnn.param`。
条件：沿默认配置启用YOLO。调用要求in0及out0/out1/out2，按3尺度anchors生成框；图文件仅17层、单输出分类路径。契约工具明确报出out1、out2缺失。
影响：不能期待当前默认模型产生YOLO检测框。解决：更换匹配检测图/权重/后处理，或改ROI分类适配器及语义动作映射。不得忽略extract返回值后继续把未知张量当检测头。

### F02 Loognson 阈值偏移作用在二值图

位置：`project/user/main.cpp:L64—66`。
先Otsu原地改写gray_original，再对其做adjusted_threshold，已经失去原始灰度层次。合成输入Otsu=130，偏移20后，观察到第二次输出与第一次完全相同；对原灰度用110阈值则相差10像素。
解决：保留原灰度，计算阈值时输出到独立临时图或仅取阈值，最终从原灰度进行一次二值化。用固定失败帧对照，再调整偏移，不把反例等同所有场景都会失败。

### F03 duteryan fast_exp 注释精度界不成立

位置：`project/code/ncnn_model.cpp::fast_exp`及其Softmax使用。
按原float32位近似公式，在[-8,0]共10001点最大相对误差约6.1476%，超过注释的2%。这验证数值近似误差，不验证板端“快3—5倍”。
解决：当概率参与拒识/动作门限时，比较真实Softmax与近似概率的阈值穿越、校准和错例；保留减max。argmax不变不等于置信度语义不变。

## 静态成立，尚无并发/实车复现

### F04 RunACCM2025 的局部锁没有保护共享预测vector

位置：`thread/thread.cpp::AIConsumer/consumer`。
AIConsumer自己创建局部mutex后赋值predict_result；consumer读取并传入Standard.run时不获取该mutex。两个线程对同一vector读写，队列锁也不覆盖此对象。应采用共同保护的不可变快照，含源帧号、捕获/推理时刻及有效期。
另一个独立问题：同一图像分流到两个队列，不保证消费时拿到同源AI结果。需要时间/帧绑定，不能只因队列有锁就称感知同步已完成。

### F05 smart2026 缺少新帧时效门控

位置：`smartCar/main.cc::capture_callback/control_callback`。
采集置g_frame_ready，控制检查true和非空并clone，缺少序号/捕获时刻的更新验证。若相机停更，旧图可能继续进入控制。建议记录源帧序号、单调捕获时刻和最大年龄，并定义减速/停止策略。普通相机丢一帧不应被误判为所有帧都失效。

## 条件性风险或配置核对项

- Abandoned：高频LADRC回调在onto_pd_control_enable=false时没有刷新缓存PWM却继续输出。若把此开关用作停车门会保留旧命令；它本来的“方向调试使能”语义须先确认。不要未经说明直接改成全车急停。
- Loognson：静态boundary_mask首建尺寸后未随ROI重建，若ROI尺寸改变将有形状/索引风险；实际是否改变需跟踪调用。未减max的exp(logits)在大logit合成输入下产生非有限值；不据此断言实车logit已溢出。
- Loognson：red_miss_count仅在失败分支累计；若需求是连续失败10次，成功时也应清零。当前可能表示累积失败而非连续失败，应与任务意图对照。
- THL001：Motor_Diff_Pid1的P倍率0.3/6.5/1在阈值处不连续；注释百分比与代码不同。回放阈值两侧控制输出和饱和，再决定平滑/调参，不保证更复杂的参数表更优。
- ls2k0300：TFLite样例是40×40 float三类，行车NCNN适配器是64×64三类。相同类别数不能证明模型、归一化、算子与标签可互换。
- wjlai/test：识别demo和循迹RAR尚不能证明识别结果已进入绕行；README也说明目标板转向未完成。

## 已排除或不能成立的误报

- loong_SmartCar：阿克曼函数单独看有除零风险，但真实ackerman_control对零半径已有直行分支；正常链路“直道一定除零”不成立。单独迁移函数仍需带前置条件。
- car-chao：queue_adjust当前循环为i<rear，不能拿注释描述的历史<=rear越界当当前缺陷。
- THL001：注释写“已注销转向”，实际仍调用Motor_Diff_Pid1；判断运行行为以代码为准。
- Abandoned：源码文件名tflm_model_process_lq并不能证明实现TFLM；所读实现为NCNN。类存在或初始化注释不能证明已启用。
- han-user镜像：提交与旧CoConutMilkSoy项目相同，不是独立验证，也不能凭选择文件数不同推断功能差异。
- star-link：目录里的电路文件没有被逐页实测，既不能宣传“硬件无问题”，也不根据模板README虚构算法缺陷。

后续复查流程：固定版本→找到活跃调用和配置→验证前置条件→构造最小反例/读取日志→定级→只修改责任模块→回归。同样重视排除误报，不做仅匹配危险字符串的“深度审计”。
