cmake_minimum_required(VERSION 3.16)
project(board_cyt4bb7_stub C)
set(CMAKE_C_STANDARD 99)

add_executable(board_cyt4bb7_host
  app/main.c
  cm7_0/control_core.c
  cm7_1/vision_core.c
  shared/vision_ipc.c
)

target_include_directories(board_cyt4bb7_host PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/shared
  ${CMAKE_CURRENT_SOURCE_DIR}/cm7_0
  ${CMAKE_CURRENT_SOURCE_DIR}/cm7_1
)

if(MSVC)
  target_compile_options(board_cyt4bb7_host PRIVATE /utf-8)
endif()
