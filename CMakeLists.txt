## This file is a quick tutorial on writing CMakeLists for targeting the Vita
cmake_minimum_required(VERSION 3.16)


## This includes the Vita toolchain must go before project definition
# It is a convenience so you do not have to type
# -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake for cmake. It is
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


## Define project parameters here
# Name of the project
project(TH07)
# This line adds Vita helper macros must go after project definition in order
# to build Vita specific artifacts (self/vpk).
include("${VITASDK}/share/vita.cmake" REQUIRED)

## Configuration options for this app
# Display name (under bubble in LiveArea)
set(VITA_APP_NAME "TH07")
# Unique ID must be exactly 9 characters. Recommended: XXXXYYYYY where X =
# unique string of developer and Y = a unique number for this app
set(VITA_TITLEID "STAL0TH07")
# Optional version string to show in LiveArea's more info screen
set(VITA_VERSION "01.00")

## Flags and includes for building
# Note that we make sure not to overwrite previous flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fjump-tables -O2 -mcpu=cortex-a9 -mtune=cortex-a9 -mfloat-abi=hard -mfpu=neon -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -fjump-tables -Wno-c++26-extensions -O2 -mcpu=cortex-a9 -mtune=cortex-a9 -mfloat-abi=hard -mfpu=neon -g")
# Optional. You can specify more param.sfo flags this way.
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")


# Add any additional include paths here
include_directories(
  # This is for debugScreenPrintf() you shouldn't need it in your projects
)

# Add any additional library paths here
# ${CMAKE_CURRENT_BINARY_DIR} lets you use any library currently being built
link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)


## Build and link
# Add all the files needed to compile here
add_executable(${PROJECT_NAME}
    src/AnmManager.cpp
    src/AnmVm.cpp
    src/AsciiManager.cpp
    src/BombData.cpp
    src/BulletManager.cpp
    src/Chain.cpp
    src/Controller.cpp
    src/EclManager.cpp
    src/EffectManager.cpp
    src/Ending.cpp
    src/EnemyEclInstr.cpp
    src/EnemyManager.cpp
    src/FileSystem.cpp
    src/GameErrorContext.cpp
    src/GameManager.cpp
    src/GameWindow.cpp
    src/Gui.cpp
    src/ItemManager.cpp
    src/MainMenu.cpp
    src/MidiOutput.cpp
    src/MusicRoom.cpp
    src/Player.cpp
    src/ReplayManager.cpp
    src/ResultScreen.cpp
    src/Rng.cpp
    src/ScreenEffect.cpp
    src/SoundPlayer.cpp
    src/Stage.cpp
    src/Supervisor.cpp
    src/TextHelper.cpp
    src/main.cpp
    src/graphics/Gles.cpp
    src/pbg4/Lzss.cpp
    src/pbg4/Pbg4Archive.cpp
    src/pbg4/Pbg4File.cpp
    src/utils.cpp
    src/thirdparty/sjis_converter.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pbg4"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty"
  "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE

)
# Library to link to (drop the -l prefix). This will mostly be stubs.
target_link_libraries(${PROJECT_NAME}
  SDL2_ttf
  SDL2_image

  -Wl,--start-group
  SDL2
  freetype
  webpdemux
  webp
  png16
  bz2
  z
  pthread
  -Wl,--end-group

  pib
  libScePiglet_stub_weak
  SceShaccCg_stub_weak
  SceShaccCgExt
  SceIme_stub_weak
  SceGxm_stub_weak
  SceLibKernel_stub_weak
  SceDisplay_stub_weak
  SceCtrl_stub_weak
  SceMotion_stub_weak
  SceAudio_stub_weak
  SceAudioIn_stub_weak
  SceTouch_stub_weak
  SceHid_stub_weak
  SceCommonDialog_stub_weak
  SceAppMgr_stub_weak
  taihen_stub_weak



)

## Create Vita files
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
# The FILE directive lets you add additional files to the VPK the syntax is
# FILE src_path dst_path_in_vpk. In this case we add the LiveArea paths.
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME "TH07"

  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
