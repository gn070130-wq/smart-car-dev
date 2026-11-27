export PATH=/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6/bin:$PATH

# ===================== 路径配置 =====================
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${WORK_DIR}/../out"
USER_DIR="${WORK_DIR}/../user"
WIN_DESKTOP="/mnt/d/Users/sun/Desktop"
MAKE_JOBS=$(nproc)
RESERVE_FILE="本文件夹作用.txt"
    
REMOTE_IP="192.168.157.205"                                  # 远程设备IP
REMOTE_USER="root"                                        # 远程登录用户
REMOTE_PATH="/home/root/"                                 # 远程上传路径
POWERSHELL="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"


# ===================== 工具函数 =====================
error_exit() {
    echo -e "\033[31m[ERROR] $1\033[0m"
    exit 1
}

info_echo() {
    echo -e "\033[32m[INFO] $1\033[0m"
}

# ===================== 主流程 =====================
info_echo "进入编译目录：${OUT_DIR}"
cd "${OUT_DIR}" || error_exit "目录不存在"

# ===================== 【关键】增量编译：不清理文件 =====================
# 只在没有 CMakeCache.txt 时才执行 cmake
if [ ! -f "CMakeCache.txt" ]; then
    info_echo "首次编译，执行 CMake..."
    cmake "${USER_DIR}" || error_exit "CMake 失败"
else
    info_echo "增量编译，跳过 CMake"
fi

info_echo "开始编译（增量，只编译修改文件）..."
make -j${MAKE_JOBS} || error_exit "编译失败"

# ===================== 复制最终产物到桌面 =====================
#info_echo "复制 project 到桌面..."
#cp -f "${OUT_DIR}/project" "${WIN_DESKTOP}/" || error_exit "复制失败"

# 5. 获取上级目录名称（原逻辑保留，优化写法更健壮）
parent_dir_name=$(basename "$(dirname "$(pwd)")")
info_echo "待上传文件/目录：${parent_dir_name}"

# 6. SCP上传文件到远程设备，-O 兼容老版本openssh，增加上传校验
info_echo "开始上传文件到 ${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}"
$POWERSHELL scp -O "${parent_dir_name}" "${REMOTE_USER}@${REMOTE_IP}:${REMOTE_PATH}" || error_exit "文件上传失败，请检查网络/远程权限/文件是否存在！"

info_echo "✅ 编译完成！project 已在桌面"
exit 0