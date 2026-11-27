cmake_minimum_required(VERSION 3.5.0)

include(cross.cmake)

get_filename_component(DEMO_DIR ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
get_filename_component(PROJECT_NAME ${DEMO_DIR} NAME)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(COMMON_FLAGS "-O3 -march=loongarch64 -mtune=loongarch64 -fPIC -ffunction-sections -fdata-sections -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -pthread -Wno-delete-non-virtual-dtor")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build")
find_package(OpenCV REQUIRED)

set(LIBRARY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../libraries)
set(PROJECT_USER_DIR ${CMAKE_CURRENT_LIST_DIR}/../../project/user)
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROJECT_USER_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../project/model
    ${LIBRARY_ROOT}/zf_common
    ${LIBRARY_ROOT}/zf_device
    ${LIBRARY_ROOT}/zf_driver
    ${LIBRARY_ROOT}/zf_components/seekfree_assistant
    ${LIBRARY_ROOT}/zf_components/tflm
    ${LIBRARY_ROOT}/zf_components/tflm/third_party/flatbuffers/include
    ${LIBRARY_ROOT}/zf_components/tflm/third_party/gemmlowp
    ${LIBRARY_ROOT}/zf_components/tflm/third_party/gemmlowp/fixedpoint
    ${LIBRARY_ROOT}/zf_components/tflm/third_party/ruy
    ${LIBRARY_ROOT}/zf_components/tflm/third_party/ruy/ruy
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/micro
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/micro/kernels
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/kernels/internal
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/core/api
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/core/c
    ${LIBRARY_ROOT}/zf_components/tflm/tensorflow/lite/schema
    ${LIBRARY_ROOT}/zf_components/ncnn/include
    ${LIBRARY_ROOT}/zf_components/ncnn/include/ncnn
)

set(NCNN_LIBRARY "${LIBRARY_ROOT}/zf_components/ncnn/lib/libncnn.a")
if(NOT EXISTS "${NCNN_LIBRARY}")
    message(FATAL_ERROR "NCNN static library not found: ${NCNN_LIBRARY}")
endif()
add_library(ncnn_local STATIC IMPORTED)
set_target_properties(ncnn_local PROPERTIES IMPORTED_LOCATION "${NCNN_LIBRARY}")

set(SOURCES
    main.cpp
    ${PROJECT_USER_DIR}/image_recognition.cpp
    ${PROJECT_USER_DIR}/lq_ncnn.cpp
    ${LIBRARY_ROOT}/zf_common/zf_common_fifo.cpp
    ${LIBRARY_ROOT}/zf_device/zf_device_uvc.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_tcp_client.cpp
    ${LIBRARY_ROOT}/zf_components/seekfree_assistant/seekfree_assistant.cpp
    ${LIBRARY_ROOT}/zf_components/seekfree_assistant/seekfree_assistant_interface.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "/opt/ls_2k0300_env/opencv_4_10_build/lib"
    BUILD_WITH_INSTALL_RPATH ON
)
target_link_libraries(
    ${PROJECT_NAME}
    ${OpenCV_LIBS}
    ncnn_local
    gomp
    pthread
    m
)
