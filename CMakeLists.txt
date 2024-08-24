cmake_minimum_required(VERSION 3.25)

project(ffmpeg-w32codec)

include_directories(
  include
  ${CMAKE_BINARY_DIR}/ffmpeg/out/include
)

link_directories(
  ${CMAKE_BINARY_DIR}/ffmpeg/out/bin
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/out" CACHE PATH "..." FORCE)
endif()

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
  winmm
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

add_library(vcmdrv SHARED
  src/common.cpp
  src/vcmdrv/main.cpp
  src/vcmdrv/driver.cpp
  src/vcmdrv/ic.cpp
  src/vcmdrv/vcmdrv.def
)
target_link_libraries(vcmdrv
  winmm
  avcodec
  avutil
  swscale
)
set_target_properties(vcmdrv PROPERTIES
  PREFIX ""
)

set(VIDEO_CODECS iv50)
if(MSVC)
  set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>)
else()
  set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif()
foreach(name ${VIDEO_CODECS})
  add_custom_command(TARGET vcmdrv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${OUTPUT_DIR}/$<TARGET_FILE_NAME:vcmdrv> ${OUTPUT_DIR}/vidc${name}.dll)
endforeach()

add_executable(test_msvfw32
  src/common.cpp
  tests/msvfw32/main.cpp
)
target_link_libraries(test_msvfw32
  msvfw32
  vfw32
)
