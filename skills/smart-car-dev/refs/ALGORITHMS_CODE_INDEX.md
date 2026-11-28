# 算法完整代码索引（视觉补强版）

搜集 GitHub/Gitee 开源共识 + 本地赛道仓，写成可粘贴实现。

| 专题 | 文件 | 算法 ID | 主要开源参考 |
|------|------|---------|--------------|
| 摄像头循迹全链路 | `VISION_LINE_PIPELINE.md` | V01–V03 | AuTop RT1064、20th_smart_vision、yunvfeng、2025-Smart-car |
| 元素状态机库 | `ELEMENT_FSM_LIBRARY.md` | S01 | 20th_smart_vision fsm、换边线出环开源、yunvfeng ring |
| 电磁中线 | `ELECTRO_MIDLINE.md` | V03类/电磁 | 逐飞电磁模块、16th-smart-car、差比和/加权质心 |
| Stanley / Pure Pursuit | `STANLEY_PURE_PURSUIT.md` | P05 | PythonRobotics、path-tracking-controllers |
| TOF/编码/红外 | `MODULES_BSP.md` | P01 等 | 通用板级 |
| IMU/惯导/平衡 | `IMU_INS_PLAYBOOK.md` | P02/P03 | 单车/轮腿报告共识 |
| 逐飞核心板 | `SEEKFREE_BOARDS.md` / `BOARD_TEMPLATES.md` | — | `board_libs/`（本机镜像，默认可不入 Git） |
| CYT4BB7 双核 | `CYT4BB7_DUAL_CORE.md` | — | 轮腿；`generated/board_cyt4bb7_stub` |
| 龙芯 LS2K | `LS2K_LINUX_BOARD.md` | — | 走马 `references/repos` |
| RKNN 部署 | `RKNN_DEPLOY.md` | V05/A3 | 智能模型开源 |
| 从赛题到可编译 | `COMPILE_READY_WORKFLOW.md` | — | `generated/pvc_line_track` 等 |

## 分板 host 骨架（generated/）

| 板 | 目录 |
|----|------|
| 通用 Seekfree | `board_seekfree_stub` |
| TC264/ADS | `board_tc264_stub` |
| STC32 | `board_stc32_stub` |
| RT1064 | `board_rt1064_stub` |
| CYT4BB7 双核 | `board_cyt4bb7_stub` |
| PVC 循迹示例 | `pvc_line_track` |

写赛题代码时：先查本表 → 复制对应 `.md` 中的 C 模块 → 改 HAL → 用分板 stub 本地编过再上板。
