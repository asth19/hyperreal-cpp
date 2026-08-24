cmake_minimum_required(VERSION 3.15)
project(HyperrealLibrary LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 可执行文件输出到 build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==================== 库 target ====================
# 静态库，可被本项目及其他程序复用
add_library(hyperreal STATIC
    src/Hyperreal.cpp
)

# 公开 include 路径：使用方只需 #include "hyper/Hyperreal.h"
target_include_directories(hyperreal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Windows + MinGW 下启用 UTF-8 运行时，解决中文字符串与路径乱码
if(MINGW)
    target_compile_options(hyperreal PRIVATE
        -Wall -Wextra
        -D__USE_MINGW_ANSI_STDIO=1
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
    )
endif()

if(MSVC)
    target_compile_options(hyperreal PRIVATE /utf-8 /W4)
endif()

# ==================== 测试 target ====================
add_executable(test_hyperreal
    test/test_hyperreal.cpp
)
target_link_libraries(test_hyperreal PRIVATE hyperreal)

# 测试程序同样需要 UTF-8 运行时（继承库的 PUBLIC include 即可）
if(MINGW)
    target_compile_options(test_hyperreal PRIVATE
        -D__USE_MINGW_ANSI_STDIO=1
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
    )
endif()

if(MSVC)
    target_compile_options(test_hyperreal PRIVATE /utf-8)
endif()
