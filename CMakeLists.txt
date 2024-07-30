cmake_minimum_required(VERSION 3.25)

project(ffmpeg-w32codec)

include_directories(
  include
  ${CMAKE_BINARY_DIR}/ffmpeg/out/include
)

link_directories(
  ${CMAKE_BINARY_DIR}/ffmpeg/out/bin
)

if (MSVC)
  set_property(GLOBAL
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else ()
  add_link_options("-static")
endif()

add_library(acmdrv SHARED
  src/common.cpp
  src/acmdrv/main.cpp
  src/acmdrv/driver.cpp
  src/acmdrv/format.cpp
  src/acmdrv/stream.cpp
  src/acmdrv/acmdrv.def
)
target_link_libraries(acmdrv
  avcodec
  avutil
)
set_target_properties(acmdrv PROPERTIES
  OUTPUT_NAME "ffmpeg"
  PREFIX ""
  SUFFIX ".acm"
)

add_executable(test_msacm32
  src/common.cpp
  tests/msacm32/main.cpp
)
target_link_libraries(test_msacm32
  msacm32
  winmm
)
