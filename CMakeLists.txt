cmake_minimum_required(VERSION 3.15)
project(NameExchanger VERSION 2.2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "_x64")
    set(ARCH_DIR "x64")
else()
    set(ARCH_SUFFIX "")
    set(ARCH_DIR "x86")
endif()

# Add ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp
    ${IMGUI_DIR}/imgui_impl_dx11.cpp
)

# Application sources
set(APP_SOURCES
    main.cpp
    src/app.cpp
    src/d3d_helpers.cpp
    src/i18n.cpp
    src/tray.cpp
    src/utils.cpp
)

# Add resource file
set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/res.rc)

# Executable
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${IMGUI_SOURCES} ${RC_FILE})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Ensure UTF-8 source encoding for all compilers
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/name_exchanger${ARCH_SUFFIX}.lib
    d3d11
    d3dcompiler
    dxgi
    dwmapi
    shlwapi
    shell32
    user32
    gdi32
    advapi32
    ole32
    ntdll
    ws2_32
    userenv
    bcrypt
)

# Set output name
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "name_exchanger")
