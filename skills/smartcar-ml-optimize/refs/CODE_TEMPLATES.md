# 可调用代码模板（完善工程用）

约定：输出时按用户语言（C/MicroPython）改写；必须带限幅、超时、调试开关。  
完整落码时套【原始→修改后→说明→测试】。

## T1 偏差 → 差速（雁过/疯狂）

```c
// in: err(像素或归一), v_base; out: pwm_l, pwm_r
float u = Kp*err + Kd*(err - err_prev);
err_prev = err;
u = clampf(u, -UMAX, UMAX);
float vl = clampf(v_base - u, VMIN, VMAX);
float vr = clampf(v_base + u, VMIN, VMAX);
```

测试：直线 err≈0；阶跃 err 不振荡；限幅有效。

## T2 连通域信标（飞跃）

```c
// 二值 → 最大合法 blob 质心
for each blob:
  if area in [Amin,Amax] && score_best: cx,cy,conf=score
if (now - stamp > T_EXPIRE) conf=0; // 车端停
```

测试：单灯稳；双灯选高分；反光误检抬 T。

## T3 YOLO/检测 → 控制偏差（模型/蚂蚁/卡丁）

```python
# NPU 后处理伪代码
boxes = nms(yolo(img), conf, iou)
tgt = pick_best(boxes, class_priority)
if tgt is None or time.time()-stamp > T:
    send_safe_stop(); return
err_x = (tgt.cx - cx0) / half_w  # [-1,1]
uart_send(err_x, tgt.cls, stamp, crc)
```

测试：无目标超时停车；抖动用 3～5 帧投票。

## T4 CTE 回放（卡丁）

```c
int i = nearest_index(x,y,path,n);
float cte = cross_track_error(x,y,yaw, path,i);
float steer = clampf(Kp_cte*cte + feedforward(path,i), -SMAX, SMAX);
```

测试：直线 CTE→0；弯道不切内；打滑后不爆炸。

## T5 平衡串级骨架（单车/轮腿）

```c
// 1ms: 内环角速度；外环倾角；最外速度
float ang_ref = -Kp_v*(v_ref - v_meas);      // 速度→倾角
float gyro_ref = Kp_ang*(ang_ref - pitch);  // 倾角→角速度
float torque = Kp_g*(gyro_ref - gyro_y) + Kd_g*(-gyro_y);
// 路径转向作为弱叠加，且 |pitch| 大时削减
```

测试：原地站稳→轻推恢复→再加偏航。

## T6 麦轮/全向逆解

```c
// vx,vy,w → w1..w4 (麦克纳姆示例，矩阵按实车改)
w[0]= vx - vy - k*w;
w[1]= vx + vy + k*w;
w[2]= vx + vy - k*w;
w[3]= vx - vy + k*w;
for i: w[i]=clampf(w[i],-WMAX,WMAX);
```

测试：纯平移无自旋；原地转 vx=vy=0。

## T7 通信新鲜度

```c
if (!crc_ok(frame)) return;
memcpy(&cmd, &frame.payload, sizeof(cmd));
cmd_stamp = now;
// 控制环：
if (now - cmd_stamp > T_EXPIRE) { zero_cmd(); safe_mode(); }
```

测试：拔线后 T 内停车；坏 CRC 不更新。

## T8 状态机框架

```c
switch(state){
 case APPROACH: if (dist< D) state=ALIGN; break;
 case ALIGN: if (aligned) state=PUSH; break;
 case PUSH: if (done||timeout) state=RETURN; break;
}
// 每态独立限速表
```

测试：强制超时能退出；禁止非法跳转。
