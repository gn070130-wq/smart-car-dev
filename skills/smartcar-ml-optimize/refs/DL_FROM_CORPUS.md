# 从报告语料提炼的 DL/部署经验（灌入）

来源：`zhineng-moxing` / `zhineng-shijue` / `mayi-banjia` / `kading` 等 corpus 高频共识。

## 硬件分工

- **RK3588 / OrangePi5 + NPU**：跑 YOLO/分割/OCR；出结构化结果  
- **TC264 / RT1064 / RT1021 / TC387 / CYT4**：实时控制、PID、运动学、安全  
- **OpenArt / OpenMV**：蚂蚁/视觉组常见边端识别，UART 出结果  

## 优化优先级（赛场向）

1. **帧率稳定** > 偶尔更高 mAP（控制更吃周期抖动）  
2. **掉检策略**（保持/降速/停）必须有  
3. **量化后**在真实赛场光照回归，不在只看 val 集  
4. 输入分辨率：320/416/640 做消融；小目标再升分辨率  
5. 类别：只留比赛需要的类，减 NMS 负担  

## 与传统算法混合

| 场景 | 建议 |
|------|------|
| 室内高对比循迹 | 传统边线主环；DL 仅元素 |
| 信标灯 | 连通域优先；DL 抗反光增强 |
| 跟随显示板/人员 | DL 检测 + 多帧投票 |
| 俯视推箱地图 | 颜色分割/建图为主；分类可用小模型 |
| 复杂室外元素 | DL 检锥桶/框；平衡仍传统串级 |

## 写代码时的 DL 模块接口约定

```c
typedef struct {
  float cx, cy;     // 归一或像素，文档标明
  float conf;
  uint8_t cls;
  uint32_t stamp_ms;
  uint8_t valid;
} DetectTarget_t;
```

控制侧只依赖 `DetectTarget_t` + 新鲜度，不直接绑某个 YOLO 版本。
