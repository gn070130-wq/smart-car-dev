# 1. 包含交叉编译工具链 (必须在 project 之前)
include(cross.cmake)

# 2. 基础设置
cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)

# 3. 编译优化选项 (针对龙芯2K0300架构优化)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=loongarch64 -pthread -Wall -rdynamic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=loongarch64 -Wall -rdynamic")

# 可选：静态链接 OpenMP (取消注释以启用)
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgomp")

# 4. 自动获取项目名称
set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(PARENT_PARENT_DIR ${CURRENT_DIR}/.. ABSOLUTE)
get_filename_component(PARENT_PARENT_FOLDER_NAME ${PARENT_PARENT_DIR} NAME)
message("Project Name: ${PARENT_PARENT_FOLDER_NAME}")

project(${PARENT_PARENT_FOLDER_NAME} VERSION 0.1.0 LANGUAGES C CXX)

# 5. 配置 libjpeg-turbo 路径 (手动指定安装目录)
# 确保此路径指向你含有 include 和 lib 的那个文件夹
set(MY_LIBJPEG_PATH "${CMAKE_CURRENT_LIST_DIR}/../../libraries/libjpeg")
# 定义具体的库文件全路径，强制避开系统 /lib 搜索
set(LIBJPEG_LIB_FILE      "${MY_LIBJPEG_PATH}/lib/libjpeg.so")
set(LIBTURBOJPEG_LIB_FILE "${MY_LIBJPEG_PATH}/lib/libturbojpeg.so")

# 包含 libjpeg 头文件
include_directories(${MY_LIBJPEG_PATH}/include)

# 6. 配置 ncnn 路径 (手动指定安装目录)
set(MY_NCNN_PATH "${CMAKE_CURRENT_LIST_DIR}/../../libraries/ncnn")
# 定义具体的库文件全路径
set(NCNN_LIB_FILE "${MY_NCNN_PATH}/lib/libncnn.so")

# 包含 ncnn 头文件
include_directories(${MY_NCNN_PATH}/include)

# 7. 配置 giflib 路径
set(MY_GIFLIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../../libraries/giflib")
set(GIFLIB_LIB_FILE "${MY_GIFLIB_PATH}/lib/libgif.so")
include_directories(${MY_GIFLIB_PATH}/include)

# 7. 配置 OpenCV
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../../libraries/opencv")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 8. 包含项目业务代码路径
include_directories(../main)
include_directories(../include)
include_directories(../web_service/include)
include_directories(../../libraries)
include_directories(../../libraries/zf_common)
include_directories(../../libraries/zf_device)
include_directories(../../libraries/zf_driver)
include_directories(../../libraries/zf_components)

# 9. 收集所有源代码
file(GLOB_RECURSE DIR_SRCS
    ../main/*.c
    ../main/*.cpp
    ../src/*.c
    ../src/*.cpp
    ../web_service/src/*.c
    ../web_service/src/*.cpp
    ../../libraries/zf_common/*.c
    ../../libraries/zf_common/*.cpp
    ../../libraries/zf_device/*.c
    ../../libraries/zf_device/*.cpp
    ../../libraries/zf_driver/*.c
    ../../libraries/zf_driver/*.cpp
    ../../libraries/zf_components/*.c
    ../../libraries/zf_components/*.cpp
)

# 10. 创建可执行文件
add_executable(${PROJECT_NAME} ${DIR_SRCS})

# 11. 链接库文件 (核心修改)
# 直接使用变量指向的 .so 绝对路径，确保链接的是 Turbo 版本
target_link_libraries(${PROJECT_NAME}
    pthread
    ${LIBJPEG_LIB_FILE}
    ${LIBTURBOJPEG_LIB_FILE}
    ${OpenCV_LIBS}
    ${NCNN_LIB_FILE}
    ${GIFLIB_LIB_FILE}
)