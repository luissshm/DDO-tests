# -------------------------------------------------
# Minimum CMake version
# -------------------------------------------------
cmake_minimum_required(VERSION 3.22.1)

# -------------------------------------------------
# Project
# -------------------------------------------------
project(ddotest LANGUAGES C CXX)

# -------------------------------------------------
# Library name
# -------------------------------------------------
set(LIB_NAME daas_jni)

# -------------------------------------------------
# C++ standard (REQUIRED by DaaS R0.21.3)
# -------------------------------------------------
set(CMAKE_C_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ANDROID_NATIVE_API_LEVEL 30)

# -------------------------------------------------
# Android log library
# -------------------------------------------------
find_library(log-lib log)

# -------------------------------------------------
# Include directories
# -------------------------------------------------
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# -------------------------------------------------
# Force arm64-v8a
# -------------------------------------------------
set(DAAS_ABI arm64-v8a)
message(STATUS "Using fixed ABI: ${DAAS_ABI}")

# -------------------------------------------------
# Path to native DaaS library
# -------------------------------------------------
set(DAAS_LIB_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${DAAS_ABI}/libdaas.a
)

if(NOT EXISTS ${DAAS_LIB_PATH})
    message(FATAL_ERROR "libdaas.a not found at ${DAAS_LIB_PATH}")
endif()

message(STATUS "Using libdaas.a from: ${DAAS_LIB_PATH}")

# -------------------------------------------------
# JNI library
# -------------------------------------------------
add_library(${LIB_NAME} SHARED
        daas_jni.cpp
)

# -------------------------------------------------
# Imported DaaS static library
# -------------------------------------------------
add_library(daas STATIC IMPORTED)

set_target_properties(daas PROPERTIES
        IMPORTED_LOCATION ${DAAS_LIB_PATH}
)

# -------------------------------------------------
# Link
# -------------------------------------------------
target_link_libraries(
        ${LIB_NAME}
        daas
        ${log-lib}
)
