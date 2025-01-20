
include(FetchContent)

set(ONNX_VERSION 1.20.0)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL ARM64)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-arm64-${ONNX_VERSION}.zip")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-x64-${ONNX_VERSION}.zip")
    else()
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-win-x86-${ONNX_VERSION}.zip")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/downloadv${ONNX_VERSION}/onnxruntime-osx-x86_64-${ONNX_VERSION}.tgz")
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
    file(COPY ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
    target_link_libraries(onnxruntime_lib INTERFACE ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.a)
    file(COPY ${onnxruntime_SOURCE_DIR}/lib/onnxruntime.so DESTINATION ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()