添加的文件：changes.md（记录本次修改）
删除的文件：无
修改的文件：src/camera.cpp（新增 camera_cfg_load()/camera_cfg_save()，将逆透视矩阵改为从 /home/root/configs/camera_config.json 读取，默认仍使用当前硬编码矩阵；把原摄像头曝光设置函数改名为 camera_exposure_setting() 避免与设置页 camera_setting() 冲突；ImagePerspective2/2_Color/3_Color 使用可配置逆透视矩阵）
修改的文件：src/setting.cpp（新增 camera_setting()，循环显示 get_mmap_image(1)+print_image_toushi(0)，CST816 ESC 退出，GPIO UP/DOWN 微调 M[2][1] 并保存、重建逆透视映射；setting3 改调用 camera_exposure_setting()）
修改的文件：include/camera.h（新增逆透视配置变量与 camera_cfg_load()/camera_cfg_save()/camera_exposure_setting() 声明）
修改的文件：include/setting.h（新增 camera_setting() 声明）
修改的文件：src/camera.cpp（新增 camera_ipm_set_pitch()，调 M[2][1] 时同步补偿 M[0][1]，用底部中心点映射作为锚点，减少微调导致的图像底部左右漂移）
修改的文件：src/setting.cpp（逆透视微调改为调用 camera_ipm_set_pitch()，说明同步补偿 M[0][1]）
修改的文件：include/camera.h（新增 camera_ipm_set_pitch() 声明）
修改的文件：tools/udp_bridge.py（修复遥测广播与操作回执并发写入造成 WebSocket 帧交错断线；地址和参数消息改为明确类型且单条错误不再关闭连接；增加 UDP 发送回执与日志；修正控制帧格式为 2 个 int + 3 个 float）
修改的文件：tools/udp_monitor.html（地址应用和参数发送使用明确消息类型，等待服务端回执后显示结果）
修改的文件：tools/udp_monitor.py（修正控制帧格式为 2 个 int + 3 个 float）
添加的文件：无
删除的文件：无
修改的文件：src/camera.cpp（camera_ipm_set_pitch() 新增纵向补偿：调整 M[2][1] 时同步反算 M[1][1]，固定默认矩阵的底部中心 local_y，抑制数值增大导致图像整体向 y 负方向偏移；保留 M[0][1] 横向补偿）
修改的文件：src/setting.cpp（更新逆透视微调注释，说明同时补偿横向和纵向漂移）
修改的文件：src/setting.cpp（扩展 camera_setting()：默认模式保留 UP/DOWN 透视微调，新增 LEFT/RIGHT 整体缩放；GPIO ENTER 切换平移模式，四方向键整体平移；完整保留蓝色/红色辅助线显示）
修改的文件：src/camera.cpp（新增逆透视整体缩放及 X/Y 平移参数，应用到灰度、彩色和双倍彩色映射表，并写入/读取 camera_config.json）
修改的文件：web_service/src/capture_page.cpp（数据集拍摄页面增加保存目录输入和应用按钮，显示并切换当前图片目录）
修改的文件：web_service/src/web_service_main.cpp（增加可持久化的采集目录配置与 /api/capture-dir；限定目录在 /home/root/image/ 下并自动递归创建；保存、列表、预览、编号计算和删除均使用当前目录）
