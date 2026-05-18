cmake_minimum_required(VERSION 3.20)
project(BrickBreaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FreeGLUT REQUIRED)
find_package(OpenGL   REQUIRED)

add_executable(BrickBreaker
    src/main.cpp
    src/game.cpp
)

target_include_directories(BrickBreaker PRIVATE src)

target_link_libraries(BrickBreaker
    PRIVATE
    FreeGLUT::freeglut
    OpenGL::GL
    OpenGL::GLU
)

if(MSVC)
    target_compile_options(BrickBreaker PRIVATE /W3)
endif()

# Automatically copy freeglut.dll next to the exe after each build (Windows only)
if(WIN32)
    add_custom_command(TARGET BrickBreaker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:FreeGLUT::freeglut>
            $<TARGET_FILE_DIR:BrickBreaker>
        COMMENT "Copying freeglut DLL..."
        VERBATIM
    )
endif()
