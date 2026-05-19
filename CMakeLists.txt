cmake_minimum_required(VERSION 3.20)
project(BrickBreaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

if(APPLE)
    # Use the macOS system GLUT framework (no freeglut install required)
    find_package(GLUT REQUIRED)
else()
    find_package(FreeGLUT REQUIRED)
endif()

add_executable(BrickBreaker
    src/main.cpp
    src/game.cpp
    src/sound.cpp
)

target_include_directories(BrickBreaker PRIVATE src)

if(APPLE)
    target_link_libraries(BrickBreaker
        PRIVATE
        GLUT::GLUT
        OpenGL::GL
        "-framework AudioToolbox"
    )
else()
    target_link_libraries(BrickBreaker
        PRIVATE
        FreeGLUT::freeglut
        OpenGL::GL
        OpenGL::GLU
    )
endif()

if(MSVC)
    target_compile_options(BrickBreaker PRIVATE /W3)
endif()

if(APPLE)
    target_compile_definitions(BrickBreaker PRIVATE GL_SILENCE_DEPRECATION)
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
