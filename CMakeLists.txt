# CMakeLists.txt updates
# Use static CRT for MSVC in Release, dynamic in Debug
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")  # Static CRT
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")    # Dynamic CRT
endif()

# Add -static for GNU/MinGW in Release
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -static")
endif()

# Make sure to keep existing /utf-8 and charset flags
