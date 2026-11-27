cmake_minimum_required(VERSION 3.10)

project(SMARTCAR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CROSSTOOL_DIR "/opt/loongson-gnu-toolchain-13.2")
set(CMAKE_C_COMPILER ${CROSSTOOL_DIR}/bin/loongarch64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${CROSSTOOL_DIR}/bin/loongarch64-unknown-linux-gnu-g++)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR loongarch64)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

include_directories(${PROJECT_SOURCE_DIR}/smartCar/include)
include_directories(${PROJECT_SOURCE_DIR}/cross_lib/opencv/include/opencv4)
include_directories(${PROJECT_SOURCE_DIR}/cross_lib/json/include)

aux_source_directory(${PROJECT_SOURCE_DIR}/smartCar/code MYCODE)
aux_source_directory(${PROJECT_SOURCE_DIR}/smartCar/src MYSRC)

link_directories(${PROJECT_SOURCE_DIR}/cross_lib/opencv/lib)
link_directories(${PROJECT_SOURCE_DIR}/cross_lib/json/lib)

add_executable(icar ${MYCODE} ${MYSRC})
target_link_libraries(icar PRIVATE 
                        opencv_core 
                        opencv_imgcodecs
                        opencv_imgproc
                        opencv_videoio
                        jsoncpp)