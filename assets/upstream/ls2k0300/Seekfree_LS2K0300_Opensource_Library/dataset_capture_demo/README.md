# LS2K0300 数据集自动采集教程

本目录是一套独立的数据集采集程序，包含板端程序和电脑/虚拟机接收脚本。它不会修改或链接现有的 `project` 主程序，采集时也不会把图片保存在板卡上。

## 一、程序工作方式

板端程序会完成以下工作：

1. 初始化摄像头、IMU、左右电机、编码器和 TCP 客户端。
2. 车辆静止时读取 100 次陀螺仪 Z 轴数据，每次间隔 5 ms，计算零漂偏移量。
3. 左右轮反向差速，依次转到 `+20°`、`-20°`，再慢速归中到 `0°`；每次到达极限角后先将电机停止 `250 ms`，再切换旋转方向。
4. 归中后按当前方向行驶 150 编码器计数，然后再次执行完整旋转和归中。
5. 红块底边行号达到或超过输入的最大行号时向后行驶，达到或低于输入的最小行号时向前行驶。
6. 在输入的行号区间内保持当前方向逐段采集；相对启动位置的 `-4500~4500` 编码器计数作为安全限位。
7. 每 500 ms 使用当前摄像头帧尝试截取一张模型输入图片。
8. 将图片无损编码为 PNG，并立即通过 TCP 发送到电脑或虚拟机。

路程只在前进和后退阶段累计，原地旋转时的编码器变化不会计入位置。左编码器使用原方向，右编码器按反号后参与速度闭环和路程计算。

当前裁剪逻辑与主工程保持一致：

- 在 `160×120` 处理坐标中检测红色标识物。
- 只在运行时输入的“最小行号 < 红块底边行号 < 最大行号”区间内拍照。
- 横向对准红色外接框中心，纵向让 ROI 底边对齐红框底边，生成 `32×32` ROI。
- 映射到 `320×240` 彩色原图后，截取得到 `64×64` 彩色图片。
- 没有检测到合法 ROI 时跳过本次采集，不发送整帧，也不重复上一张图片。

板端内存中只保留当前摄像头帧、当前 ROI 和 PNG 编码缓冲区，不会不断积累图片。

## 二、目录说明

```text
dataset_capture_demo/
├── pc/
│   ├── receive_dataset.py    Windows/虚拟机通用接收脚本
│   └── start_windows.bat     Windows 接收端启动脚本
├── user/
│   ├── main.cpp              LS2K0300 板端采集程序
│   ├── CMakeLists.txt        交叉编译配置
│   ├── cross.cmake           LoongArch 工具链配置
│   └── build.sh              编译和上传脚本
└── README.md                 本教程
```

## 三、使用前准备

使用前确认以下条件：

- 摄像头连接到板卡并对应 `/dev/video0`。
- IMU、左右电机和编码器连接方式与当前主工程一致。
- 板卡使用电池或满足电机功率要求的电源供电。
- Windows 安装了 Python 3，并在安装时勾选了 `Add Python to PATH`。
- 板卡与 Windows 网络互通。
- 本机已经安装项目使用的 LoongArch 交叉编译工具链和 OpenCV 环境。

本教程使用 Windows 直接接收图片，虚拟机只负责编译和上传板端程序。这样图片会直接保存在 Windows，不经过虚拟机磁盘。

## 四、确认 Windows IP

在 Windows 中按 `Win+R`，输入 `cmd` 并回车，然后执行：

```bat
ipconfig
```

找到 Windows 当前连接网络对应的“无线局域网适配器 WLAN”或“以太网适配器”下的 IPv4 地址。例如：

```text
IPv4 地址 . . . . . . . . . . . : 192.168.1.100
```

这里需要传给板端程序的 IP 是 `192.168.1.100`。实际使用时请替换成你当前查到的 Windows 地址。

不要使用下面这些地址：

- `127.0.0.1`。
- `VMware Network Adapter VMnet1` 的地址。
- `VMware Network Adapter VMnet8` 的地址。
- 虚拟机自己的 IP。

应选择与板卡处于同一个路由器或热点网络中的 Windows Wi-Fi/以太网地址。

可以从板卡测试网络是否连通：

```bash
ping 192.168.1.100
```

## 五、在 Windows 启动接收端

虚拟机中的 Windows 接收包位于：

```text
/home/saki/hgfs/vm/LS2K0300_dataset_receiver_windows
```

这是 VMware 的 Windows 共享目录。Windows 接收包包含：

```text
LS2K0300_dataset_receiver_windows/
├── pc/
│   ├── receive_dataset.py
│   └── start_windows.bat
└── README.md
```

在 Windows 中打开这个共享文件夹，在地址栏输入 `cmd` 并回车，然后进入 `pc` 目录。

也可以直接双击 `start_windows.bat`，然后按照提示输入类别编号和图片数量。

采集物资类别 `supplies`，保存 100 张：

```bat
start_windows.bat supplies 100
```

采集车辆类别 `vehicle`：

```bat
start_windows.bat vehicle 100
```

采集武器类别 `weapon`：

```bat
start_windows.bat weapon 100
```

图片数量设为 `0` 时持续采集，直到按 `Ctrl+C`：

```bat
start_windows.bat supplies 0
```

也可以不使用批处理，直接运行 Python：

```bat
py -3 receive_dataset.py --label supplies --output ..\dataset --port 8087 --count 100
```

第一次运行时，如果 Windows 防火墙弹出网络访问提示，应允许 Python 在“专用网络”中通信。

如果没有弹出提示且板卡无法连接，可以使用管理员 PowerShell 添加入站规则：

```powershell
New-NetFirewallRule -DisplayName "LS2K0300 Dataset TCP 8087" -Direction Inbound -Protocol TCP -LocalPort 8087 -Action Allow -Profile Private
```

看到以下提示后，表示 Windows 正在等待板卡连接：

```text
Listening on 0.0.0.0:8087; label=supplies
```

必须保持这个窗口运行，再去启动板端程序。

接收脚本会自动关闭当前 CMD 窗口的“快速编辑模式”，防止鼠标点击或拖选文字时暂停 Python，造成板端 TCP 缓冲区写满。

### 可选：仍然使用 Linux/虚拟机接收

如果以后需要改回虚拟机接收，可以在虚拟机执行：

```bash
cd Seekfree_LS2K0300_Opensource_Library/dataset_capture_demo
python3 pc/receive_dataset.py --label supplies --port 8087 --count 100
```

可用参数如下：

| 参数 | 作用 | 默认值 |
| --- | --- | --- |
| `--label` | 当前采集类别，只能是 `supplies`、`vehicle` 或 `weapon` | 必填 |
| `--output` | 数据集输出目录 | `dataset` |
| `--bind` | 接收脚本监听地址 | `0.0.0.0` |
| `--port` | TCP 端口 | `8087` |
| `--count` | 保存多少张后自动停止，`0` 表示不限数量 | `0` |


## 六、编译并上传板端程序

打开另一个终端并进入板端源码目录：

```bash
cd Seekfree_LS2K0300_Opensource_Library/dataset_capture_demo/user
```

只编译、不上传：

```bash
./build.sh
```

生成的 LoongArch 程序位于：

```text
../out/dataset_capture_demo
```

编译并上传到板卡：

```bash
./build.sh <板卡IP>
```

例如板卡 IP 是 `192.168.152.204`：

```bash
./build.sh 192.168.152.204
```

上传目标路径为：

```text
/home/root/dataset_capture_demo
```

如果脚本没有执行权限，先执行一次：

```bash
chmod +x build.sh
```

## 七、在板卡上开始采集

必须先启动 Windows 接收脚本，再运行板端程序。板端程序启动后会立即连接指定的 Windows IP 和端口。

通过 SSH 登录板卡：

```bash
ssh root@<板卡IP>
```

运行采集程序：

```bash
/home/root/dataset_capture_demo <Windows-IP> <最小行号> <最大行号> [端口]
```

行号直接使用 `160×120` 处理画面坐标，画面顶部为 `0`、底部为 `120`。最小行号必须小于最大行号，两者都可在 `0~120` 之间设置。为了与当前主程序保持一致，建议把最小行号设为 `60`。

例如，在 `60 < 红块底边行号 < 110` 时拍照，并使用默认端口 `8087`：

```bash
/home/root/dataset_capture_demo 192.168.1.100 60 110
```

需要指定端口时：

```bash
/home/root/dataset_capture_demo 192.168.1.100 60 110 8087
```

程序显示下面的内容时，必须让车身保持静止：

```text
Keep the car still: calibrating gyro...
```

大约 0.5 秒后校准完成，车辆开始执行“转到 `+20°`、转到 `-20°`、归中到 `0°`、按当前方向移动 150”的循环。红块底边行号达到区间下边界时向前行驶，达到上边界时向后行驶；只有位于开区间内的合法 ROI 才会发送。

板端成功发送时会打印：

```text
sent image 1, yaw=3.25 deg, bytes=...
```

没有检测到红色目标时会打印：

```text
capture skipped: no valid 64x64 ROI
```

## 八、采集结果

图片直接保存在 Windows 接收包的 `dataset` 目录，默认结构如下：

```text
dataset/
├── manifest.csv
├── supplies/
├── vehicle/
└── weapon/
```

图片文件名包含采集会话时间、序号和拍摄时的偏航角，例如：

```text
20260717_174342_816926_000007_yaw_m12345.png
```

其中：

- `000007` 是本次板端运行发送的第 7 张图片。
- `yaw_m12345` 表示偏航角约为 `-12.345` 度。
- `yaw_p19876` 表示偏航角约为 `+19.876` 度。

`manifest.csv` 会记录图片路径、类别、序号、偏航角和电脑接收时间。

## 九、停止采集

存在两种正常停止方式：

1. 接收端达到 `--count` 指定的张数后自动停止。
2. 在接收端按 `Ctrl+C` 手动停止。

接收端停止时会向板端发送 `S` 命令。板端收到后退出程序，停止控制定时器，并把左右电机 PWM 设置为零。

也可以在板端终端按 `Ctrl+C`，板端同样会执行电机清理逻辑。

## 十、常见问题

### 1. 板端提示 TCP 连接失败

检查以下内容：

- 接收脚本是否已经启动。
- 板端命令中的 IP 是否是 Windows 的 Wi-Fi/以太网 IP，而不是板卡或虚拟机 IP。
- 两端端口是否都是 `8087`。
- 防火墙是否允许 TCP 8087 端口。
- Windows 和板卡是否连接到同一个路由器或热点。
- Windows 防火墙是否允许 Python 监听 TCP 8087。

### 2. 一直没有保存图片

观察板端是否反复打印 `capture skipped`。该提示表示当前帧没有通过主工程相同的红色目标检测和几何筛选。需要检查红色标识物是否在画面内、光照是否合适，以及摄像头是否正常输出彩色图。

程序不会为了凑数量而发送整帧或旧图片。

### 3. 车辆旋转方向与预期相反

首次运行建议架空车轮测试。如果车辆不是原地左右旋转，需要核对电机安装方向、编码器符号，以及板端程序中的以下配置：

```cpp
constexpr int MOTOR_LEFT_DIR_LEVEL = 1;
constexpr int MOTOR_RIGHT_DIR_LEVEL = 1;
constexpr int ENCODER_LEFT_SIGN = 1;
constexpr int ENCODER_RIGHT_SIGN = -1;
```

这些值当前与主工程底盘配置一致。

### 4. 实际旋转超过正负 20 度

程序按照陀螺仪积分角度在边界处反向，电机存在惯性，因此实车可能有少量越界。先确认启动校准期间车身完全静止，再观察文件名和 `manifest.csv` 中记录的偏航角。

### 5. 修改了主工程图片尺寸

当前数据集程序严格按照主工程的 `64×64` 模型输入配置。如果以后再次修改 `IMAGE_ROI_SIZE`、`MODEL_INPUT_WIDTH` 或 `MODEL_INPUT_HEIGHT`，数据集程序和接收脚本的尺寸检查也必须同步修改，之后重新编译板端程序。

## 十一、安全注意事项

- 第一次运行时先把车轮架空，确认左右轮转向正确。
- 陀螺仪校准完成前不要移动车身。
- 先启动接收端，再启动板端，避免车辆启动后没有数据接收方。
- 调试时确保车辆周围有足够空间，并随时准备在板端或接收端按 `Ctrl+C`。
