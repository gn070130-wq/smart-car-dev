# 21届智能车走马观碑开源仓库

由B站up:WwuSama 主导的开源仓库 旨在为智能车竞赛添砖加瓦  

## 介绍

- 开源仓库用于21届全国大学生智能车竞赛走马观碑赛道，硬件平台基于龙芯片2K0300久久派，同时仓库也将提供开源拓展板PCB和原理图设计`(已验证)`
- 开源硬件外设清单：  
  1. DRV8701双路电机驱动（外接）
  2. ST7735 128x160 LCD屏幕（板载）
  3. ICM42688 六轴陀螺仪（外接）
  4. VL53L0X 激光测距模块（外接）
  5. 五向开关（板载）
  6. 无源蜂鸣器（板载）  
- __开源交流群：681603150__
---

### 引脚分布及硬件预览
| 引脚分布图| 拓展板预览图 |
|-------|-------|
| ![引脚分布图](/res/引脚分布图.png "引脚分布图") | ![拓展板预览图](/res/Loongson_Aquarii_1.png "拓展板预览图") |

---

### 我们的优势

1. `内核级驱动` 将硬件全权由内核主导 极大提高硬件响应速度 应用崩溃触发内核保护 极大提高系统稳定性和安全性  

2. `全公开源码` 欢迎所有同学参与 感受从零开始的BSP开发流程  

3. `提供外设购买方案` 极致性价比首选！！！

4. `极致简单的应用层开发框架` 确保新手小白零基础也可以很好入门智能车竞赛 全身心投入应用层开发轻松学习Linux  

5. `uboot + 赛事定制内核 + 赛事定制系统` 拥有更快启动时间上电到进入系统实测实测仅需 ___25秒___  

6. `开发环境集成NCNN静态库` 拥有龙架构优化的模型推理引擎 上手即用 无须额外搭建环境

7. `高集成度拓展板` 板载外设提高稳定性 避免杜邦线松动导致数据丢失 轻量化设计减少空间占用比


## 仓库结构
```
├── res/                             # 资源文件
├── system/
    ├── uImage                       # 定制内核
    ├── wuwusama_buildrootfs.tar     # 定制跟文件系统  

├── example/                         # 库例程

├── WuwuSama_Icar_Project/           # 应用层开源库
    ├── cross_lib/
        ├── json                     # jsoncpp动态库
        ├── ncnn                     # ncnn静态库
        ├── opencv                   # opencv动态库
    ├── smartCar/
        ├── code                     # 用户代码添加在此处
        ├── include                  # 开源库头文件路径
        ├── wuwu_library             # 开源库源文件路径
        main.cc                      # 程序入口.cpp文件
    CMakeLists.txt                   # 项目CMake文件

└── Hardware/                        # 基于龙芯2K0300的拓展板工程
    ├──ProPrj_Loongson_Aquarii.epro
```
## 快速开始

### 1.烧写U-BOOT
___免责声明___ : 本项目不提供U-BOOT烧写教程,因烧写问题导致的板子变砖一律不负责  

___U-BOOT烧写参考资料___ : (作者也是参考下面教程进行的烧写)  
【龙芯2K300-99派更新u-boot固件】https://www.bilibili.com/video/BV1FTqPYLER2?vd_source=9e5530c6fa951aaadc66d0d7b35a6103  
【久久派新世界系统烧录】https://www.bilibili.com/video/BV12tsJeXEWK?vd_source=9e5530c6fa951aaadc66d0d7b35a6103 （2:33开始）  

- 成功案例分享  
https://blog.csdn.net/hhhaaa___/article/details/155676117?fromshare=blogdetail&sharetype=blogdetail&sharerId=155676117&sharerefer=PC&sharesource=Hao0129&sharefrom=from_link

---

### 2.使用配套虚拟机镜像开发
通过网盘分享的文件：21界智能车走马观碑-WuwuSama配套Vmware  
链接: https://pan.baidu.com/s/17RVDXLaOuwwSA4aWFliFOQ  
提取码: ka53  
--来自百度网盘超级会员v3的分享

虚拟机密码：`123456`

---

### 3.网络启动系统
- 配置虚拟机与久久派在同一网段 并进入U-BOOT(久久派开机一直按 __'C'__ 进入U-BOOT)  
配置过程中需要久久派与电脑在同一局域网

- 设置U-BOOT
    1. 设置久久派IP __(IP根据自己网段修改， 保证局域网内没有其他设备使用)__
    >`setenv ipaddr 192.168.1.99  `
    2. 设置网关 __(根据自己局域网设置)__
    >`setenv gatewayip 192.168.1.1  `
    3. 设置虚拟机当前IP __(需要与久久派在同一网段下)__
    >`setenv serverip 192.168.1.90  `
    4. 设置通过网络挂在跟文件系统 __(实例中IP根据上述自行修改)__
    >`setenv bootargs 'console=ttyS0,115200 root=/dev/nfs nfsroot=192.168.1.90:/home/wuwu/linux/nfs/lsrootfs ip=192.168.1.99:192.168.1.90:192.168.1.1:255.255.255.0::eth0:off'  `
    5. 设置TFTP下载内核
    >`setenv bootcmd tftp ${loadaddr} uImage  `
    6. 保存设置 __(出错不需要重新设置)__
    >`saveenv  `
    7. 查看当前环境设置(可选)  
    >`printenv  `
    8. 启动系统  
    >`boot  `

---
### 4.烧写系统到EMMC

1. 输入密码root进入系统  

2. 在虚拟机中将项目中system/ 拷贝至 ~/linux/nfs/lsroots ___(在虚拟机中操作)___
    >`cp system/ ~/linux/nfs/lsrootfs/root/ -r`

3. 在久久派中使用mkfs.ext4 格式化emmc ___(在久久派中操作)___
    >`mkfs.ext4 /dev/mmcblk0  `

4. 输入y确认格式化
    >`y`

5. 挂载emmc
    >`mount /dev/mmcblk0 /mnt  `

6. 将system/中wuwusama_buildrootfs.tar 解压至emmc根目录中
    >`tar -xvf system/wuwusama_buildrootfs.tar -C /mnt  `

7. 将新内核拷贝到emmc中
    >`cp system/uImage /mnt/boot  `

8. 取消emmc挂载
    >`umount /mnt  `

- 此时已经成功将内核和系统烧写至EMMC

---

### 5.U-BOOT设置EMMC启动

1. reboot重新启动系统  
    >`reboot  `

2. 狂按 ```c``` 进入U-BOOT  

3. 设置跟文件系统路径 __rootdelay__ 参数用于设置U-BOOT等待时间
    >`setenv bootargs root=/dev/mmcblk0  `

4. 设置从EMMC中读取内核
    >`setenv bootcmd 'ext4load mmc 0:0 ${loadaddr} boot/uImage; bootm ${loadaddr}'  `

5. 保存参数  
    >`saveenv  `

6. 启动系统
    >`boot  `

- 如想使用网络启动按照 ___3.网络启动系统___ 配置U-BOOT即可
---

## 硬件简介  （已验证）

### 1.硬件特点
- 使用Type_C接口，集成USB-TTL，可进行供电和串口连接，搭配充电宝可实现不断电换电池。
- 同时预留无线串口接口，可根据各自购买的无线串口自行更改原理图。
- 集成屏幕板，减小空间占用，避免使用杜邦线造成的数据传输错误。
- 考虑温漂及焊接问题，决定拓展板只留I2C接口，可自制外接板或一分二排线连接tof及陀螺仪
- 在GND接口旁有标注“O”丝印
### ___如果发现有问题请联系Aquarii(春风)___

---

### 2.注意
- 注意MOS选型，开关要用PMOS
- 电源管理部分选用RDS(on)低的MOS。测试时发现如BSS84等MOS在栅源压差-2.5V时RDS为欧姆级，导致使用电池供电时压降严重，5V降到3V
- 注意电机接口和无线串口接口不要短路
- 若在智能车中使用，为了美观可让两个编码器接口间隙大一些
- 部分元件焊盘有修改
---

##  **作者信息** 
- 项目作者：WuwuSama、Aquarii(春风)、Blockingsys
- 邮箱：1635202242@qq.com Aquarii706401@163.com
- QQ：1635202242
- b站：3546716263418536