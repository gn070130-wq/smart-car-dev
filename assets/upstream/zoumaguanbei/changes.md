添加的文件：
- project/web_service/include/web_service_main.h（网页服务主入口声明。）
- project/web_service/include/dataset_test_service.h（网页模型测试及模型管理服务接口。）
- project/web_service/src/web_service_main.cpp（网页服务主程序。）
- project/web_service/src/dataset_test_service.cpp（数据集测试、文件夹识别和网页模型管理实现。）
- changes.md（修改记录。）
删除的文件：
- project/include/capture.h（已迁移并重命名。）
- project/src/capture.cpp（已迁移并重命名。）
修改的文件：
- project/cmake/CMakeLists.txt（加入 web_service 目录构建配置。）
- project/include/classifier.h（新增 current_model_name 只读接口，用于准确显示当前成功加载的模型。）
- project/src/classifier.cpp（记录当前成功加载的模型名；model_load 成功后更新，model_unload 时清空；未修改模型推理算法。）
- project/main/main.cpp（使用 web_service_menu。）
- project/web_service/include/dataset_test_service.h（新增模型目录查询、运行状态查询和切换模型接口。）
- project/web_service/include/dataset_test_service.h（新增可取消测试任务的 request_stop_dataset_test 和 stop_dataset_test 接口，供网页退出流程安全回收测试线程。）
- project/web_service/src/dataset_test_service.cpp（将全量测试和文件夹识别从 detached 线程改为受控 std::thread；增加原子取消标志，每张图片处理前检查取消状态；提供退出时 join 测试线程，避免网页退出后后台测试继续占用 CPU。）
- project/web_service/src/web_service_main.cpp（/api/stop 先请求取消测试、停止采集和网页服务；web_service_menu() 退出前等待 HTTP 线程和测试线程结束；不调用 model_unload；只有 web_service_menu() 运行期间启动 HTTP 服务；将“退出网页服务”按钮移动到服务选择首页，数据集拍摄页移除该按钮。）
- project/web_service/src/web_service_main.cpp（模型测试页新增当前模型显示、自动发现模型下拉框和切换按钮；测试运行时禁用模型切换；新增 GET /api/models 和 POST /api/models/switch 接口。）
- project/include/camera.h（新增 CameraExposureConfig 及 MMAP 摄像头曝光参数查询、设置、持久化公开接口；增加 auto_priority_available 能力标志，兼容未实现动态帧率优先控件的摄像头。）
- project/src/camera.cpp（新增 V4L2 自动/手动曝光、手动曝光时间及自动曝光帧率/亮度优先控制；兼容 EXPOSURE_ABSOLUTE/EXPOSURE；启动时读取 camera_config.json 应用参数；camera_cfg_save 和曝光保存均先合并原 JSON，并通过临时文件原子替换，保留逆透视矩阵与未知字段；实机验证后将 AUTO_PRIORITY 作为可选能力，不支持时不阻断其他曝光参数保存；摄像头自动发现只选择可直接流式采集并支持 YUYV 的节点，排除需媒体管线配置的 IO_MC 节点，避免误选 Intel IPU6。）
- project/web_service/src/web_service_main.cpp（首页新增摄像头参数调整入口和独立页面；新增曝光参数 API、g_yuyv 实时 YUYV 转 JPEG 预览；预览按页面心跳启停并与数据集采集互斥，页面离开、采集启动或网页服务退出后停止取帧；修复预览心跳每 1.5 秒重新渲染参数表单、覆盖用户尚未保存的曝光输入，现仅首次加载及明确应用后刷新表单。）
- project/src/camera.cpp（修复 DHZJ 摄像头从手动切换自动曝光失败：不再固定发送设备不支持的 V4L2_EXPOSURE_AUTO(0)，改为查询 V4L2_CID_EXPOSURE_AUTO 菜单，优先使用驱动支持的 AUTO，否则使用 APERTURE_PRIORITY(3)；手动模式仍使用 MANUAL，读取时非 MANUAL 均视为自动曝光。重新编译通过；第二轮实机切换验证时摄像头已从系统断开，未修改设备参数。）
- project/src/camera.cpp（修改 get_camera_mmap_message()：删除 Contrast 和 Sharpness 的读取与显示；复用 camera_exposure_get() 正确读取 EXPOSURE_ABSOLUTE/EXPOSURE；Auto Exposure 显示为英文 Automatic 或 Manual，控件不可用时显示 Unavailable。）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（新增 find_barrier_accurate，精确拟合红色矩形近侧长边并推算卡片中心、角度和距离）；project/include/find_road.h（新增函数声明）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（find_barrier_accurate 改为近侧红色前沿扫描与 RANSAC 直线拟合，避免远侧红色色块粘连干扰；保留两个 print_image 测试调用）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（将 find_barrier_accurate 的 angle_barrier_save 改为卡片有向法线相对屏幕竖直向上的夹角，顺时针为正，范围 [-180, +180]；未改动测试函数）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（仅优化 find_barrier_accurate 的 RANSAC 候选线生成，改为固定横向间隔采样，保留原内点和近侧优先判定；其他检测逻辑未修改）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（优化近侧前沿扫描为增量坐标和连续红点计数；RANSAC 改为无内层动态分配、最终一次性收集最佳内点；保持 cut_image 逻辑和测试 return -1 不变）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（红色 ROI 改为扫描像素按需 HSV 转换并缓存；一次调用仅处理沿中线最近红区，尝试后不再处理更远红区；保持成功时无条件 cut_image，并保留中心、barrier_far、angle_barrier_save 更新及测试 return -1）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（先由种子中心扫描线估计近侧边缘深度，其余横向线仅搜索深度附近，并修正 RANSAC 内层筛选控制流）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（修正最近红区语义：候选红区精定位失败后跳过该连续红区并继续搜索后续红区；找到首张有效卡片后立即更新参数、裁图并结束；保留测试 return -1）

添加的文件：无
删除的文件：无
修改的文件：project/test/test.py（生成 -90 到 +90、间隔 22.5 度的九档旋转映射并加入方向及最大残差校验）；project/include/rotate_array.h（由脚本生成九档 75x75 反向采样表）；project/src/camera.cpp（cut_image 按 11.25 度中点阈值选择最近映射表，正角选 p 表、负角选 n 表）

添加的文件：无
删除的文件：无
修改的文件：
- project/include/classifier.h（新增 add_photo、save_all_photo、clean_ptoto 照片缓存接口声明。）
- project/src/classifier.cpp（新增固定 cv::Mat 数组照片缓存；支持深拷贝暂存 75x75 BGR 图片、按顺序批量保存和清空缓存。）
- project/src/start.cpp（障碍识别时改为缓存照片；main_run 开始时清空缓存，结束或取图失败退出前统一保存。）

添加的文件：无
删除的文件：无
修改的文件：
- project/include/camera.h（在现有曝光接口基础上新增固定九项 CameraImageControls 画面参数结构及查询、整组设置、启动恢复接口。）
- project/src/camera.cpp（仅增加亮度、对比度、饱和度、色相、自动白平衡、伽马、电源频率防闪烁、白平衡色温、锐度九项白名单参数；按驱动范围和步进校验、读回生效值，原子合并保存到 camera_config.json 的 image_controls；启动时先恢复自动白平衡再恢复其余参数。）
- project/web_service/src/web_service_main.cpp（保留现有曝光与实时预览页面，在其下新增九项画面参数、中文说明、实际范围与中文防闪烁菜单；新增 /api/camera/image-settings 查询和整组应用接口；自动白平衡开启时禁用白平衡色温，心跳不覆盖编辑值。）

添加的文件：
- project/web_service/include/web_pages.h（声明共享样式与四个独立界面渲染器。）
- project/web_service/src/web_common_style.cpp（存放所有界面共用的 CSS。）
- project/web_service/src/home_page.cpp（服务选择首页。）
- project/web_service/src/capture_page.cpp（数据集拍摄界面。）
- project/web_service/src/camera_settings_page.cpp（摄像头参数调整界面。）
- project/web_service/src/model_test_page.cpp（模型识别测试界面。）
删除的文件：无
修改的文件：project/web_service/src/web_service_main.cpp（移除内嵌页面与样式，只保留 HTTP/API 路由和业务调度，路由改为调用独立界面渲染器。）

添加的文件：无
删除的文件：无
修改的文件：
- project/src/find_road.cpp（修复 find_zebra 固定从第 79/80 列初始化却按动态赛道边界扫描的问题；改为从每行有效左边界连续采样，限制左右边界到图像范围，跳过宽度不足区间，并用具名常量和独立逐行计数提升可读性；保留原跳变及阈值判定语义。）

添加的文件：无
删除的文件：无
修改的文件：project/src/camera.cpp（将 get_mmap_image 模式 4、5 的白色得分公式改为 Y - (|U-128| + |V-128|) / 2，模式 4 继续执行标准大津二值化，模式 5 输出灰度图）；project/include/camera.h（补充模式 4、5 及白色得分公式说明）

添加的文件：
- project/tools/track_designer.html（新增自包含赛道绘制网页：支持 45 cm 固定宽度的参数化直道/弯道、拖拽旋转、端点吸附、组合锁定、十字与环岛、双口径长度统计、缩放平移、撤销重做及 JSON 导入导出。）
删除的文件：无
修改的文件：无

添加的文件：无
删除的文件：无
修改的文件：
- project/tools/track_designer.html（显示与输入单位统一改为 cm，JSON 导出升级为 cm 制并兼容旧 m 制文件；新增两个接驳节点间的 Dubins 最短赛道连接，遍历 LSL、RSR、LSR、RSL、RLR、LRL 六类路径，自动生成并锁定直道/弯道组合，连接半径强制不低于 50 cm。）

添加的文件：无
删除的文件：无
修改的文件：project/src/find_road.cpp（删除 find_barrier_accurate 中长边跨度上限 45 像素约束，RANSAC 与 fitLine 后均只保留 span >= 12 下限）

添加的文件：无
删除的文件：无
修改的文件：README.md（补充项目背景、系统组成、完整目录结构、project 各目录功能、project/src 每个源文件功能、模型说明、构建流程及运行调试提示。）
