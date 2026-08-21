cmake_minimum_required(VERSION 4.1.0)
project(NameExchanger LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "_x64")
else()
    set(ARCH_SUFFIX "_x86")
endif()

# Detect compiler
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "GNU/MinGW compilers are not supported. Please use MSVC.")
endif()

# Fetch Dear ImGui at configure time (change GIT_TAG to update)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY  https://github.com/ocornut/imgui.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# Application sources
set(APP_SOURCES
    main.cpp
    src/app.cpp
    src/d3d_helpers.cpp
    src/i18n.cpp
    src/tray.cpp
    src/utils.cpp
    src/exchange_name_lib.hpp
)

# Add resource file
set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/res.rc)

# Executable
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${IMGUI_SOURCES} ${RC_FILE})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Ensure UTF-8 source encoding for all compilers
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /utf-8 /W4 /permissive- /EHsc
        $<$<CONFIG:Release>:/O2 /Gy /Gw>
        $<$<CONFIG:MinSizeRel>:/O1 /Gy /Gw>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:/OPT:REF /OPT:ICF>
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR LANGUAGES CXX)
    if(IPO_SUPPORTED)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
    else()
        message(WARNING "Interprocedural optimization is unavailable: ${IPO_ERROR}")
    endif()
endif()

# Use static CRT for MSVC in Release (/MT), dynamic in Debug (/MDd)
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/name_exchanger${ARCH_SUFFIX}.lib
    advapi32
    d3d11
    d3dcompiler
    dbghelp
    dwmapi
    dxgi
    gdi32
    ntdll
    ole32
    shell32
    shlwapi
    user32
    userenv
    ws2_32
)

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "name_exchanger${ARCH_SUFFIX}")
