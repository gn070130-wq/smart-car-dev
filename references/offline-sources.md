# 已安装源码与完整附件

本技能现在包含实际文件，不需要仅打开上游网址：

- `assets/upstream/<项目ID>/`：可直接浏览和修改工作副本的应用源码、已读归档成员、配置与小型模型文件，保留原始字节。
- `assets/upstream-manifests/*.json.gz`：完整目录、模式、大小、SHA256和展开文件记录。
- `assets/upstream-archives/*.zip`：13份去重后的固定提交完整Git树，加原始53份报告ZIP；不包含上游Git历史和本机凭据。
- [source-assets.json](source-assets.json)：项目ID、镜像对应、实际数量、归档名称与校验值。

## 本机位置

考虑到C盘空间，体积较大的归档实际存储于D盘，通过技能内 `assets/upstream-archives` 文件夹联接访问。读取工具只用技能内的相对路径；应用源码与脚本实际安装于本机skill目录。D盘缓存需要保留，否则归档联接会失效；展开的应用代码仍可读取。

## GitHub中的内容

同一私有GitHub仓库的main分支保存可浏览的应用代码、元数据、说明和工具；完整上游内容与原始报告同时保存于Release `sources-2026-11-27` 的实际ZIP附件。附件位于用户自己的仓库，不依赖上游网址继续可用。大附件没有放入普通Git历史，也未使用收费LFS配额。

在另一台机器clone后，恢复归档：

```text
python tools/download_source_archives.py
python tools/download_source_archives.py --repo-id zoumaguanbei
python tools/download_source_archives.py --verify-only
```

下载器使用GitHub CLI现有登录或本机Git Credential Manager凭据，仅从本仓库Release恢复文件，不执行解压得到的代码；每个归档都按SHA256验证。

## 读取实际代码

```text
python scripts/lookup_open_source.py --repo-id smart2026 --query YoloDetector::load_model
python scripts/read_source.py --repo-id smart2026 --path smartCar/code/yolov5_detect.cc --start 149 --lines 35
python scripts/read_source.py --repo-id zoumaguanbei --path linux4.19/README --lines 40
python scripts/read_source.py --repo-id reports --list
```

符号索引优先显示已校验的本地上下文。索引未覆盖的SDK、内核和其他文件可通过 `read_source.py --list --prefix` 查目录，再按精确路径从完整ZIP中读取。完整快照的新文件尚未逐行审查，不能因已经下载就提升原分析结论的验证等级。

导出某个实际模型、驱动或报告到工作目录：

```text
python scripts/read_source.py --repo-id smart2026 --path smartCar/tiny_classifier_fp32.ncnn.bin --output working/model.bin
```

输出路径必须尚不存在。工具验证原文件SHA256后复制字节，不启动模型、设备或脚本，不将符号链接自动当作可执行目标导出。修改需要在工作副本中进行。

## 完整性的含义

完整快照覆盖固定Git提交的全部已跟踪文件，逐blob校验并保留Unix文件模式；压缩包也通过CRC与整体SHA256校验。未跟踪、私有服务端、上游遗漏、另行分发的模型或设备固件不在此定义内。本次快照未发现需要另行补取的Git子模块或LFS指针。

原分析报告的选择性读取范围保持不变；本次追加完整文件，不声称对所有内核、工具链和硬件设计重新完成审计。上游版权及许可原样保留，自用也不改变来源标记。
