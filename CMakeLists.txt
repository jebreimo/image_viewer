##****************************************************************************
## Copyright © 2022 Jan Erik Breimo. All rights reserved.
## Created by Jan Erik Breimo on 2022-08-18.
##
## This file is distributed under the BSD License.
## License text is included with the source distribution.
##****************************************************************************
cmake_minimum_required(VERSION 3.23)
project(image_viewer)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(tungsten
    GIT_REPOSITORY "https://github.com/jebreimo/Tungsten.git"
    GIT_TAG v0.2.73)
FetchContent_Declare(yimage
    GIT_REPOSITORY "https://github.com/jebreimo/Yimage.git"
    GIT_TAG v0.1.1)
FetchContent_Declare(argos
    GIT_REPOSITORY "https://github.com/jebreimo/Argos.git"
    GIT_TAG v1.1.267)
FetchContent_MakeAvailable(argos tungsten yimage)

list(APPEND CMAKE_MODULE_PATH ${tungsten_SOURCE_DIR}/tools/cmake)

include(TungstenTargetEmbedShaders)

add_executable(image_viewer
    src/image_viewer/main.cpp
    src/image_viewer/Render2DShaderProgram.cpp
    src/image_viewer/Render2DShaderProgram.hpp
    )

target_link_libraries(image_viewer
    PRIVATE
        Argos::Argos
        Tungsten::Tungsten
        Yimage::Yimage
    )

tungsten_target_embed_shaders(image_viewer
    FILES
        src/image_viewer/Render2D-frag.glsl
        src/image_viewer/Render2D-vert.glsl
    )
