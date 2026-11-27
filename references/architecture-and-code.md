# 架构与代码实施

需要源码级对照时读 [ZouMaGuanBei解析](repos/zoumaguanbei/overview.md)：其中将视觉主循环、轮速线程、标签到动作、帧相关保持量与退出清理分别定位到真实函数。只借鉴已核对的机制，不把项目原有耦合当成推荐架构。

## 从入口追到执行器

检索词：`main`、`timerfd`、`pit_callback`、`scheduler`、`pthread`、`image_process`、`set_duty`、`PID`。从实际工程确认调用顺序，不按报告章节顺序猜运行顺序。

典型链路为：设备初始化→参数/模型加载→标定/待机→启动→视觉结果发布→状态决策→参考轨迹→方向控制→轮速混合→轮速闭环→PWM/方向GPIO。停机路径应能从退出、故障、用户停止等入口到达执行器关闭。

### 推荐核对的模块契约

这是一组接口设计建议，优先适配工程已有类型，不强制创建这些类：

| 数据 | 必须明确的语义 |
|---|---|
| Frame | 捕获时刻、处理起止、序号、格式、尺寸、图像内存所有权 |
| TrackResult | 边界/中线、坐标系、有效点数、质量、元素候选、源帧 |
| TargetObservation | ROI/位置、坐标系、标签版本、置信度、观测时刻、源帧 |
| Decision | 活跃元素、动作、事件号、锁定条件、失效/退出原因 |
| ReferencePath | 坐标尺度、采样间距、点序、有效期、生成者 |
| ControlSnapshot | 时间戳、路径误差、基准速度、有效标志、当前状态 |
| ActuatorCommand | 左右轮目标与输出单位、限幅、使能及有效期 |

用 `enum class` 或工程已有枚举表达道路与任务状态；不要让无类型整数同时代表模型标签、绕行方向和状态编号。

## 单板多速率任务

参照03（视觉主循环/5ms控制）、04（2ms控制）、32（5/10ms分层）、34（视觉与控制/按键分离）。这些周期是报告样例，不是技能默认值。

- 控制内环保持自己的时间基准；视觉更新慢时，控制读取最近有效结果并执行项目定义的超时策略。
- 单核多线程不会让计算量消失；高优先级线程不能包含不可控的图传、磁盘写入和长推理。
- 跨线程共享多个字段时发布完整快照。单个 `atomic<bool>` 或 `volatile` 不保证图像、路径、状态和时间戳整体一致。
- 复制发布、互斥锁、双缓冲、序列号方案按开销和已有架构选择。锁保护下只做短时间数据交换；双缓冲需保证消费者持有期间生产者不覆盖该槽。
- 使用单调时钟描述间隔和有效期；相机硬件时间戳与控制时钟不一致时先映射。
- 最新帧策略适用于以新鲜度优先的视觉控制；录像或离线统计是否允许丢帧另行决定。记录丢弃和覆盖数，不能把跳帧当成相机故障。

建议逻辑示意，非特定SDK代码：

```text
on_camera_frame(frame):
    track = extract_track(frame)
    observation = detect_and_classify_if_triggered(frame, track)
    decision = update_state(track, observation, motion)
    reference = choose_reference(track, decision)
    publish_complete_snapshot(reference, decision, frame.capture_time)

on_control_tick(now):
    motion = read_feedback()
    snapshot = read_complete_snapshot()
    if snapshot not usable under project timeout rules:
        apply_configured_degraded_or_stop_policy()
    else:
        turn = direction_and_yaw_control(snapshot, motion, actual_dt)
        wheel_targets = mix_with_base_speed(turn)
        output = wheel_control(wheel_targets, motion, actual_dt)
        write_actuators(output)
```

不要在视觉回调里直接抢写最终PWM，同时另一个定时线程也控制电机。

## 双板与事件通信

43提供事件编号、时间戳、分类锁定、目标位姿绑定和事件结束的具体线索；48、50展示双摄双板分工。双摄本身不能证明双板，双板本身不能证明异构或实时性能。

新增协议时对接现有编码；至少定义版本/长度、校验、消息类型、序号、数据单位、事件ID、有效标志及超时。目标类别和位姿需要属于同一事件，不能拼接不同帧或不同目标的字段。重传要幂等，重复消息不重复开始绕行。对端重启、乱序、丢包和EVENT_END丢失要有可恢复行为。

跨板时钟未同步，不能直接用“本板当前时刻−对板时间戳”求数据年龄；采用本地接收时刻及传输估计，或明确实现时钟同步。决策发生后车辆继续运动，规划应处理观测到执行的位姿变化。

## 状态、仲裁与恢复

把元素识别、目标生命周期和车辆运行模式分清；不强制三套状态机，但所有状态都需有进入、保持、退出、超时和复位语义。

参考组织：搜索→候选→确认→接近→绕行/直行→恢复→冷却→重新搜索。它是可选设计，不是任何队伍的逐字实现。普通路、十字、环岛、目标绕行都可能提供路径，仲裁须明确唯一有效参考、速度上限和保护优先级。

调试重点：新目标能否在旧目标冷却后进入；图像暂时漏检是否错误重建事件；环岛期间误识红块是否抢占控制；停车后旧任务是否还会输出PWM。优先修现有状态过渡，避免无证据增加更多状态。

## 修改与验证

先获得基线构建和现象，再修改责任模块。模型替换、时间基准变更、控制器更换属于不同改动，尽可能分别验证。控制器逻辑离线检查左右对称、误差符号、饱和、复位、失效输入；协议检查重复/乱序/超时；图像检查已保存的失败帧。只编译通过不能证明实车稳定。

不要直接执行报告附录中的 `sudo apt`、系统调度设置、内核修改或电机测试命令。先确认当前项目所需和用户授权范围。
