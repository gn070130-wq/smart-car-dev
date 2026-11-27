cmake_minimum_required(VERSION 3.5.0)

include(cross.cmake)

get_filename_component(PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
get_filename_component(PROJECT_NAME ${PROJECT_DIR} NAME)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(COMMON_FLAGS "-O3 -march=loongarch64 -mtune=loongarch64 -fPIC -ffunction-sections -fdata-sections -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -pthread -Wno-delete-non-virtual-dtor")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build")
find_package(OpenCV REQUIRED)

set(LIBRARY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../libraries)
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${LIBRARY_ROOT}/zf_common
    ${LIBRARY_ROOT}/zf_device
    ${LIBRARY_ROOT}/zf_driver
)

set(SOURCES
    main.cpp
    ${LIBRARY_ROOT}/zf_device/zf_device_imu.cpp
    ${LIBRARY_ROOT}/zf_device/zf_device_uvc.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_delay.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_encoder.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_file_buffer.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_file_string.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_gpio.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_pit.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_pwm.cpp
    ${LIBRARY_ROOT}/zf_driver/zf_driver_tcp_client.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "/opt/ls_2k0300_env/opencv_4_10_build/lib"
    BUILD_WITH_INSTALL_RPATH ON
)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} pthread m)
