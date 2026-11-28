cmake_minimum_required(VERSION 3.16)
project(board_stc32_stub C)

set(CMAKE_C_STANDARD 99)

add_executable(board_stc32_stub_host
  app/main.c
  app/isr_tick.c
  user/control.c
  user/track_line.c
  user/element_fsm.c
  user/safety.c
  hal/hal_host_stub.c
)

target_include_directories(board_stc32_stub_host PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hal
  ${CMAKE_CURRENT_SOURCE_DIR}/user
  ${CMAKE_CURRENT_SOURCE_DIR}/app
)

if(MSVC)
  target_compile_options(board_stc32_stub_host PRIVATE /utf-8)
endif()

