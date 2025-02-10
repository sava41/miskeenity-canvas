
include(FetchContent)

set(ONNX_VERSION 1.20.0)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL ARM64)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-arm64-${ONNX_VERSION}.zip")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-x64-${ONNX_VERSION}.zip")
    else()
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-x86-${ONNX_VERSION}.zip")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL x86_64)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-osx-x86_64-${ONNX_VERSION}.tgz")
    else()
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-osx-arm64-${ONNX_VERSION}.tgz")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL aarch64)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-linux-aarch64-${ONNX_VERSION}.tgz")
    else()
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-linux-x64-${ONNX_VERSION}.tgz")
    endif()
endif()

FetchContent_Declare(
    onnxruntime
    URL ${ONNXRUNTIME_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(onnxruntime)

add_library(onnxruntime_lib INTERFACE)

target_include_directories(onnxruntime_lib INTERFACE ${onnxruntime_SOURCE_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries(onnxruntime_lib INTERFACE ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib)
    install(FILES ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll COMPONENT miskeenity-canvas DESTINATION bin)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_link_libraries(onnxruntime_lib INTERFACE ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${ONNX_VERSION}.dylib)
    install(FILES ${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${ONNX_VERSION}.dylib COMPONENT miskeenity-canvas DESTINATION bin)
else()
    target_link_libraries(onnxruntime_lib INTERFACE ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.a)
    install(FILES ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.so COMPONENT miskeenity-canvas DESTINATION bin)
endif()