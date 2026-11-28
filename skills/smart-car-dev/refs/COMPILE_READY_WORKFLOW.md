# Skill + 大模型 → 本地可编译代码（强制）

用户说「按我的要求写代码 / 调 Skill / 大模型分析 / 要能本地编译」时，**必须走本流程**。  
目标：输出的不是片段思路，而是**落盘到工程目录、能过本地编译**的代码。

> 说明：「启动大模型」在本 Skill 中默认指 **Cursor Agent（大模型）调用 Skill 知识写码**。  
> 若用户要 **YOLO/分割等模型训练**，另走 `smartcar-ml-optimize`（数据集→训练→导出→板端），见文末。

---

## 强制步骤

### 1. 读要求 → 定槽位

| 槽位 | 来源 |
|------|------|
| 组别/赛道 | 用户原文 / `smartcar-competition` |
| 任务 | 循迹/元素/控制/避障… |
| 主控与库 | 用户指定；否则假设 + 写清 `hal_*` |
| 输出路径 | 默认 `generated/<topic>/` 或用户工程路径 |

### 2. 调取 Skill（按任务打开，禁止凭空编）

| 任务 | 必读 |
|------|------|
| 总流程 | `FROM_PROBLEM_TO_CODE.md` |
| 算法选型 | `../smartcar-ml-optimize/refs/INGESTED_ALGORITHMS.md` + `TRACK_ALGORITHM_MAP.md` |
| 视觉循迹 | `VISION_LINE_PIPELINE.md` |
| 元素 | `ELEMENT_FSM_LIBRARY.md` |
| 电磁 | `ELECTRO_MIDLINE.md` |
| 路径跟踪 | `STANLEY_PURE_PURSUIT.md` |
| 外设 | `MODULES_BSP.md` / `IMU_INS_PLAYBOOK.md` |
| 对照开源 | 赛道 `code_opensource/RECOMMENDED.md` |

### 3. 大模型分析（输出必须含）

```markdown
【理解】组别/任务/假设硬件
【算法】ID 表（必选/可选）
【结构】目录树
【风险】PVC反光、周期、限幅、超时
```

### 4. 落盘可编译工程（硬性）

- 写入真实文件：`.h/.c`（或 `.cpp`）+ `CMakeLists.txt` 或 `Makefile`
- **板级隔离**：业务代码只调 `hal_*.h`；提供 `hal_stub.c` 以便 PC 本地编译
- 禁止只给聊天窗口里的半截代码就算完成
- 默认输出目录：`generated/<name>/`

### 5. 本地编译验证（硬性）

在输出目录执行其一并报告结果：

```bat
cmake -B build -DPVC_TRACK_USE_STUB_HAL=ON
cmake --build build
```

或：

```bat
gcc -std=c11 -DPVC_TRACK_USE_STUB_HAL -I. *.c -o pvc_track_host.exe
```

- **通过**：告诉用户可执行文件路径 + 如何换成真 HAL  
- **失败**：修到编译通过再结束（缺头文件、重复 main、未定义符号等）

### 6. 交付格式

```markdown
## 交付
- 路径：generated/...
- 算法：V../C../S..
- 编译：成功 / 命令
- 换真机：实现哪些 hal_*
- 测试步骤：…
```

---

## 用户怎么启动（直接复制）

```text
用 smart-car-dev Skill，按 COMPILE_READY_WORKFLOW：
组别：雁过留痕 / PVC 摄像头循迹
要求：……
输出到 generated/xxx，并本地编译通过。
```

---

## 若要「训练」深度学习模型

1. 打开 `smartcar-ml-optimize`：`DL_FROM_CORPUS.md` / `workflow.md`  
2. 定任务：检测/分割 → 数据集格式 → 训练脚本 → ONNX/RKNN/NCNN  
3. 板端推理与 MCU 控制仍用本仓库 `CODE_TEMPLATES` T3 + 赛道 Skill  
4. 训练在用户本机 Python/GPU 环境执行；Agent 负责生成**可运行训练脚本与配置**并尽量试跑 `python train.py --help` 或干跑

---

## 质量底线

- 控制环非阻塞；限幅；超时安全态  
- `hal` 与算法分离  
- **未本地编译成功，不得宣称「可编译」**  
