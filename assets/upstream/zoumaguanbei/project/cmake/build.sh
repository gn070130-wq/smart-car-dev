#!/bin/bash

# ====================================================
# 1. 路径锁定与变量定义
# ====================================================
# 获取脚本所在的绝对路径，确保历史文件存放在脚本同级目录
SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
IP_HISTORY_FILE="$SCRIPT_DIR/.ip_history"

# 默认配置
DEFAULT_IP="192.168.8.50"
TARGET_DIR="/home/root/"

# 构建目录路径
BUILD_DIR="$SCRIPT_DIR/../out"

# 获取上级目录名称（即原脚本中的传输目标名）
# 假设结构为 project/scripts/build.sh，则此变量为 project
PARENT_DIR_NAME=$(basename $(dirname "$SCRIPT_DIR"))

# ====================================================
# 2. 编译构建阶段
# ====================================================
cd "$BUILD_DIR" || { echo "无法进入 $BUILD_DIR 目录，请检查是否存在。"; exit 1; }

# 清理目录
find . -mindepth 1 ! -name "本文件夹作用.txt" -exec rm -rf {} +

# 执行 CMake 和 Make
cmake ../cmake || { echo "cmake 命令执行失败。"; exit 1; }
echo "cmake 命令执行成功。"

make -j16 || { echo "编译失败。"; exit 1; }
echo "生成APP: $PARENT_DIR_NAME"

# ====================================================
# 3. IP 选择逻辑
# ====================================================
# 确定初始 IP
if [ -f "$IP_HISTORY_FILE" ]; then
    CURRENT_IP=$(head -n 1 "$IP_HISTORY_FILE")
else
    CURRENT_IP=$DEFAULT_IP
fi

# 定义 SCP 传输函数
do_transfer() {
    local target_ip=$1
    echo "----------------------------------------"
    echo "正在尝试传输 $PARENT_DIR_NAME 至 $target_ip ..."
    # -O 使用旧版协议, -o ConnectTimeout 防止死等
    scp -O -o ConnectTimeout=5 "$PARENT_DIR_NAME" "root@$target_ip:$TARGET_DIR"
    return $?
}

# 第一次尝试
do_transfer "$CURRENT_IP"

# 如果失败，引导用户选择历史 IP 或输入新 IP
if [ $? -ne 0 ]; then
    echo -e "\n[!] 无法连接到 $CURRENT_IP"
    
    # 准备菜单选项
    if [ -f "$IP_HISTORY_FILE" ]; then
        echo "过去使用过的 IP 列表:"
        PS3="请选择编号 (或按 Ctrl+C 退出): "
        select OPT_IP in $(cat "$IP_HISTORY_FILE") "手动输入新 IP"; do
            case $OPT_IP in
                "手动输入新 IP")
                    read -p "请输入 IP: " CURRENT_IP
                    break
                    ;;
                *)
                    if [ -n "$OPT_IP" ]; then
                        CURRENT_IP=$OPT_IP
                        break
                    else
                        echo "无效选择。"
                    fi
                    ;;
            esac
        done
    else
        read -p "无历史记录，请输入新 IP: " CURRENT_IP
    fi

    # 使用新 IP 再次尝试
    do_transfer "$CURRENT_IP"
    if [ $? -ne 0 ]; then
        echo "再次传输失败，请检查网络。"
        exit 1
    fi
fi

# ====================================================
# 4. 传输成功，更新历史记录
# ====================================================
echo "传输完成"

# 成功后将当前 IP 存入历史文件：置顶、去重、保留最近5个
{ 
    echo "$CURRENT_IP"
    [ -f "$IP_HISTORY_FILE" ] && grep -v "^$CURRENT_IP$" "$IP_HISTORY_FILE"
} > "${IP_HISTORY_FILE}.tmp"

head -n 5 "${IP_HISTORY_FILE}.tmp" > "$IP_HISTORY_FILE"
rm "${IP_HISTORY_FILE}.tmp"

echo "当前保存的首选 IP 为: $CURRENT_IP"