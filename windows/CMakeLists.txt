# ============================================================================
# Q25 Demo - Windows 专用构建配置
# ============================================================================
cmake_minimum_required(VERSION 3.14)
project(Q25_Demo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# UTF-8 编码支持 (MSVC)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options("/utf-8")
endif()

# ============================================================================
# 输出目录配置
# ============================================================================
set(OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../build/windows)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG_MODE)
    set(OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/debug)
else()
    set(OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/release)
endif()

# 可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/bin)

message("[+] Output directory: ${OUTPUT_DIRECTORY}")

# ============================================================================
# 平台库配置 - Winsock2
# ============================================================================
set(PLATFORM_LIBS ws2_32)

# ============================================================================
# Demo 可执行文件列表
# ============================================================================
set(DEMO_SOURCES
    auto_charge_demo
    axis_control_demo
    emergency_stop_demo
    gait_switch_demo
    height_control_demo
    motion_mode_demo
    power_control_demo
    stand_lie_demo
    status_receiver_demo
)

# ============================================================================
# 为每个 Demo 创建可执行目标
# ============================================================================
foreach(DEMO_NAME ${DEMO_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DEMO_NAME}.cpp")
        add_executable(${DEMO_NAME} ${DEMO_NAME}.cpp)
        target_link_libraries(${DEMO_NAME} PRIVATE ${PLATFORM_LIBS})
        message(STATUS "Added demo target: ${DEMO_NAME}")
    else()
        message(WARNING "Source file not found: ${DEMO_NAME}.cpp")
    endif()
endforeach()

# ============================================================================
# 构建信息
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Q25 Demo Windows 构建配置")
message(STATUS "========================================")
message(STATUS "CMake version:    ${CMAKE_VERSION}")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler:     ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

