#!/bin/bash

# ~/car/run 智能车项目的shell脚本
# 此脚本自动化CMake构建过程

# 删除旧的build目录
if [ -d "build" ]; then
    rm -rf build
    echo "已删除旧的build目录"
fi

# 创建新的build目录
mkdir build
echo "已创建新的build目录"

# 进入build目录
cd build

# 运行cmake
echo "正在运行cmake..."
cmake ..

# 运行make
echo "正在运行make..."
make -j12

echo "构建完成！可执行文件 main 位于build目录中"

# 已创建 build.sh 脚本并测试成功
# 现在只需运行 ./build.sh 即可自动完成清理、cmake配置和编译的全过程