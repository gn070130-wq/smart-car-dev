# dry-flipping-the-sky/THL001

用途：龙邱库与单循环视觉/电机应用。固定提交 `a545ea5dd3290b791a81043766294d21e6e84722`。

来源：[仓库](https://gitee.com/dry-flipping-the-sky/THL001)；[文件哈希与覆盖范围](manifest.json)；[函数候选索引](symbols.json)。

结构：Loongson_2K300_301_LIB的main通过LQ_APP_TARGET/LQ_DEMO_TARGET选择CarRuntime或外设示例；user_app/car_runtime组织初始化、图像、Motor_Control及相机循环，image.cpp实现边界/中线与元素，motor.cpp实现差速和轮速PID。

算法：ImageProcess含BStyleScanLines及Otsu边界搜索；Motor_Control读取编码器，再调用Motor_Diff_Pid1、左右轮PID并输出。实际存在转向调用，不能根据“已注销转向环”注释认定转向关闭。

时序：main传120fps相机请求，同时RunCameraLoop每次成功处理后sleep20ms。相机请求帧率不等于控制循环频率；仅sleep就限制到至多约50次/秒，另加图像、驱动和日志时间。当前轮速控制随该循环更新，需检查离散增益和编码器窗口。

调参风险：Motor_Diff_Pid1的小误差P乘0.3、中误差乘6.5、大误差回默认；与注释的百分比不一致，并在分界处产生增益跳变。应对阈值两侧回放误差、输出和限幅，不直接复制为通用分段P。

可借鉴：应用/演示编译开关、单一运行入口和分阶段驱动验证；本轮未核验所有内核模块、硬件原理图或各示例的实际设备表现。

本次保存40份文件/成员的定位与校验信息，索引得到131个定义候选；这两个数量都不代表已验证同等数量的算法。源码在本机快照，技能包保存解析和索引。
