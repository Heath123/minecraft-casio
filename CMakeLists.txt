# Configure with [fxsdk build-fx] or [fxsdk build-cg], which provide the
# toolchain file and module path of the fxSDK

cmake_minimum_required(VERSION 3.15)
project(Minecraft LANGUAGES ASM C CXX)

include(GenerateG1A)
include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)
find_package(Azur 0.1 REQUIRED)
find_package(LibProf 2.1 REQUIRED)

set(SOURCES
  src/render/main.cpp
  src/render/meshing.cpp
  src/game/world/chunk.cpp
  src/game/world/world.cpp
  src/game/world/raycast.cpp
  src/game/entity/player.cpp
  src/util/boundingBox.cpp
  src/util/libnumToS3L.cpp
  src/util/angle.cpp
  src/triangle2.c
  src/triangle2.S
  # ...
)
# Shared assets, fx-9860G-only assets and fx-CG-50-only assets
set(ASSETS
  # ...
)
set(ASSETS_fx
  assets-fx/example.png
  # ...
)
set(ASSETS_cg
  assets-cg/font5x7.png
  assets-cg/tileset.png
  # ...
)

fxconv_declare_assets(${ASSETS} ${ASSETS_fx} ${ASSETS_cg} WITH_METADATA)

add_executable(minecraft ${SOURCES} ${ASSETS} ${ASSETS_${FXSDK_PLATFORM}})
target_include_directories(minecraft PRIVATE src/)
target_compile_options(minecraft PRIVATE -Wall -Wextra -Wno-comment -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-unused-variable --std=c++20 -O3 -flto -g)
target_link_libraries(minecraft PRIVATE
  Azur::Azur -lnum LibProf::LibProf Gint::Gint -lm -lstdc++)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G)
  generate_g1a(TARGET minecraft OUTPUT "Minecraft.g1a"
    NAME "Minecraft" ICON assets-fx/icon.png)
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET minecraft OUTPUT "Minecraft.g3a"
    NAME "Minecraft" ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
endif()
