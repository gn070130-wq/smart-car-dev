# 赛道 → 算法 → 代码接口地图

写代码时按赛道选默认模块组合；符号为逻辑名，接到用户工程时改成其真实文件名。

## 雁过留痕 `yanguo-liuhen`
- 链：`V01→V02→V03→S01→C02→C01` + 激光脉冲安全  
- DL 增强（可选）：元素分类小模型，不进主环  
- 接口：`err, element_id, v_set → pwm_l,pwm_r, laser_en`

## 单车定向 `danche-dingxiang`
- 链：`P02→P03→P05/P07→C04→C01` + `C08`  
- 原则：平衡输出优先于路径误差  
- 接口：`imu, enc → roll_cmd, yaw_rate, speed；path → steer_ff`

## 飞跃雷区 `feiyue-leiqu`
- 链：`V01/V04→C03/C07→S03→C05→C01`  
- DL 增强：信标检测替代纯阈值（需保新鲜度）  
- 接口：Air`(tx,ty,conf,stamp)` → Car`vx,vy,w`

## 疯狂电路 `fengkuang-dianlu`
- 链：同雁过循迹 + `C10` 负压  
- 接口：`err, element → Δ, v；ducted_duty`

## 卡丁快跑 `kading-kuaipao`
- 链：`V05/V06→C09` 或 `P04→CTE→C01` + 语音 FSM `S01`  
- 源码锚：`kading-kuaipao/code_tree/km13`（优先对照改）  
- 接口：`target_err / cte → steer_pid；gate → speed`

## 轮腿穿越 `lunhui-chuanyue`
- 链：`P02→C04→C06→C05/差速` + `S02`  
- 视觉：`V05/锥桶` 只切模式，不直接抢平衡  
- 接口：`leg_len,leg_ang → q_joint[]；balance → wheel_torque`

## 蚂蚁搬家 `mayi-banjia`
- 链：`V05/V06→C05→C01→S01` + `P06/P09` + `S04`  
- 接口：`target_xy → vx,vy,w；push_cmd；collab_frame`

## 智能模型 `zhineng-moxing`
- 链：`V06/V07→部署RKNN→V09→S03→C01`  
- 接口：RK`err/type/stamp` → TC264`steer,speed`

## 智能视觉 `zhineng-shijue`
- 链：`V08→P07→P08→S01→C05→C01` + `V09`  
- 接口：`grid,path,box_plan → omni_cmd`

## 缺工程树时的写法
1. 用本表逻辑模块给出 `.c/.h` 或 `.py` 片段  
2. 注明假设：`frame_hz`、`uart_hz`、坐标系  
3. 用户贴真实工程后，再改名对齐并补测试
