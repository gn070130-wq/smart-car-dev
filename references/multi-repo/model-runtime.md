# 深度学习模型调用与验证

这里的能力是根据真实模型、运行时、输入输出契约接入和诊断推理，不是重新训练助手本身。训练新视觉模型需要明确数据、标签、训练代码、计算设备和验证目标；不能把阅读仓库称为已训练权重。

## 先选模型路线

| 路线 | 已见案例 | 调用与输出 | 不可混用的条件 |
|---|---|---|---|
| NCNN ROI分类 | ls2k0300、duteryan、Loognson、wjlai、既有ZouMaGuanBei | ROI→resize/归一化→Extractor→1D类别向量→语义动作 | 输入通道、尺寸、blob名、标签顺序、logits或概率 |
| NCNN检测 | wuwu99 YOLOv5n示例、smart2026检测器 | 整图/letterbox→多输出head→框解码→阈值/NMS→位置与类别 | 导出版本、输出shape、anchors/stride、类别数、坐标回映 |
| TFLM | 部分逐飞示例、Loognson候选类 | FlatBuffer→op resolver→tensor arena→Invoke→张量读取 | schema、注册算子、arena、dtype、scale/zero_point |
| 本机TFLite CPU | 本轮提供的smoke_tflite工具 | 分配当前shape→合成输入→Invoke→输出shape/有限性 | 只作运行冒烟，不证明TFLM和龙芯速度或语义精度 |
| EdgeBoard混合推理 | RunACCM2025 | PPNC/NNA主干→ONNX Runtime后处理→PPNC NMS | aarch64 SDK及模型产物，不是LoongArch NCNN库 |

文件名yolov5_detect、tflm_model_process、model32都不是模型格式与输入尺寸的证据。先看图与调用，再看名字。

## 用显式契约连接模型与车辆

按 [model-contract.json](../../assets/model-contract.json) 记录模型路径/哈希、引擎版本、任务、输入布局/dtype/尺寸/通道、归一化、输出节点/shape、后处理、标签与动作映射。未知字段保留null，不猜标准ImageNet或默认三类。

完整链路：

```text
RawFrame{timestamp, sequence, format}
→ Locator{ROI, geometry, completeness}
→ Preprocessor{color, resize/letterbox, mean/scale, layout, dtype}
→ Runtime{load, input, invoke, output status}
→ Decoder{shape, finite check, logits/probabilities or detection heads}
→ Observation{class, confidence, position, source_frame, model_version}
→ Confirmation/Expiry
→ SemanticAction
→ Existing path/state/controller
```

先保存同一份ROI和模型输出，比较PC与板端。发生错误时区分定位错、通道错、图契约错、模型误判和任务映射错，不能通过提高PID掩盖模型契约错误。

## NCNN实际接入检查

1. param和bin来自同一导出版本；校验load_param/load_model返回值，初始化失败禁止使用旧的“成功”状态。
2. 检查输入连续性/stride、RGB或BGR、resize/letterbox、归一化，保持训练一致。from_pixels中的PIXEL_BGR与PIXEL_BGR2RGB不能只看注释。
3. 检查Extractor.input/extract返回值、输出dims/w/h/c/elempack/elemsize与实际布局；再读数值。只检查out.w不够用于一般多维模型。
4. 分类输出长度匹配标签，拒绝NaN/Inf。只有确认输出为logits时才Softmax，采用减max的稳定形式；图已Softmax时不要重复处理。
5. YOLO先确认导出head数和shape，再选解码器。单输出可能是分类、已解码检测或其他格式，不能仅按输出数量判任务；本轮smart2026的图与具体三head解码器已确认不匹配。
6. 将细类映射成稳定语义枚举，再映射动作。若NCNN更换三/六/八类网络，控制层不应依赖原整数索引。
7. 记录捕获、定位、预处理、推理、后处理、控制消费时间。推理不能长期阻塞快环；异步结果带源帧并检查年龄。

静态工具（不依赖NCNN运行库）：

```text
python "<skill>/scripts/inspect_ncnn_contract.py" "model.param" --input in0 --output out0
python "<skill>/scripts/inspect_ncnn_contract.py" "model.param" --output out0 --output out1 --output out2
```

契约错误返回码1，格式/文件错误返回码2；同时报告层数、blob名、Softmax存在性和本地bin是否可见。该工具不证明权重正确、每个算子可运行或输出shape符合解码器，也不能只凭图中有Softmax认定任意提取节点都是概率。

## TFLite与TFLM

本轮实际调用的 `loong_cnn_model_simple.tflite` 哈希与输入输出见 [tflite-smoke.json](tflite-smoke.json)：默认分配batch1、40×40×3 float32，输出1×3。其shape_signature的batch为动态，冒烟仅使用模型当前默认shape，不扩展到任意batch。

```text
python "<skill>/scripts/smoke_tflite.py" "model.tflite"
```

工具按需导入numpy及tflite_runtime或TensorFlow，采用CPU和合成零张量，不调整相机/电机。会读TFL3标识、分配张量、Invoke并检查输出有限性；浮点0或量化整数0不是约定的真实图片预处理。

环境依赖不兼容时报告具体原因，不自动修改全局Python。本轮TensorFlow 2.21和全局protobuf版本不匹配，测试在任务本地目录使用匹配依赖解决，未升级全局包。Windows非ASCII路径可将已读模型bytes传给解释器，避免原生路径兼容问题。

TFLM另需验证op resolver、tensor arena、schema和目标架构内核。量化输入按 `q=round(real/scale)+zero_point` 并裁剪dtype范围，输出按相反关系解释；scale=0或未知时不能硬算。桌面TFLite成功不能替代这些目标端检查。

## 训练与导出的有效性

- 以采集会话/场景划分训练验证，避免相邻视频帧泄漏。
- 检查实际裁剪、运动模糊、光照、目标远近、无目标和错ROI样本；分类器不天然理解“没有目标”。
- 分别统计定位召回、分类混淆、拒识、任务通过率和端到端时延，不把离线准确率当全圈成功率。
- 训练→导出→转换→板端分别保存版本、哈希、输入输出对照；转换成功不是语义等价证据。
- 量化、FP16、packing、线程数与算子融合以目标硬件支持和测量为准；不把仓库注释的“更快”“无SIMD”泛化到全部平台。
- 没有权重、SDK或有效样本时说明具体缺项并完成静态契约与代码适配检查，不编造已推理、已训练或可直接上车的结论。
