cmake_minimum_required(VERSION 3.16)
project(pvc_line_track C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(PVC_TRACK_HOST_DEMO "Build host demo with stub HAL" ON)

add_library(pvc_track_core
  track_pvc.c
  element_fsm.c
  control_steer.c
)
target_include_directories(pvc_track_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(PVC_TRACK_HOST_DEMO)
  add_executable(pvc_track_host
    app_track_main.c
    hal_stub.c
  )
  target_compile_definitions(pvc_track_host PRIVATE PVC_TRACK_HOST_DEMO=1)
  target_link_libraries(pvc_track_host PRIVATE pvc_track_core)
  if(MSVC)
    target_compile_options(pvc_track_core PRIVATE /W3)
    target_compile_options(pvc_track_host PRIVATE /W3)
  else()
    target_compile_options(pvc_track_core PRIVATE -Wall -Wextra)
    target_compile_options(pvc_track_host PRIVATE -Wall -Wextra)
    target_link_libraries(pvc_track_host PRIVATE m)
  endif()
endif()
