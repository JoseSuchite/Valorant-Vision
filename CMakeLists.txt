cmake_minimum_required(VERSION 3.16)
project(Valorant-Vision)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Widgets MultimediaWidgets Multimedia REQUIRED)

# Find OpenCV
find_package(OpenCV REQUIRED)

include(FetchContent)

# Download eigen (I love this library)
FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG master
)
FetchContent_MakeAvailable(Eigen3)

# Download yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
)
FetchContent_MakeAvailable(yaml-cpp)

# Download motcpp
# Note: this is a fork of it I set up because its use of onnx runtime breaks on windows
# If you're looking at this in a future semester, it's in the onnx_backend.hpp file
set(MOTCPP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    motcpp
    GIT_REPOSITORY https://github.com/KiingdomKey/motcpp
    GIT_TAG main
)
FetchContent_MakeAvailable(motcpp)

# Tesseract, Leptonica, and libcurl from Desktop vcpkg
set(VCPKG_DIR "$ENV{VCPKG_DIR}")

add_library(tesseract SHARED IMPORTED)
set_target_properties(tesseract PROPERTIES
    IMPORTED_IMPLIB "${VCPKG_DIR}/lib/tesseract55.lib"
    IMPORTED_LOCATION "${VCPKG_DIR}/bin/tesseract55.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_DIR}/include"
)

add_library(leptonica SHARED IMPORTED)
set_target_properties(leptonica PROPERTIES
    IMPORTED_IMPLIB "${VCPKG_DIR}/lib/leptonica-1.87.0.lib"
    IMPORTED_LOCATION "${VCPKG_DIR}/bin/leptonica-1.87.0.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_DIR}/include"
)

add_library(libcurl SHARED IMPORTED)
set_target_properties(libcurl PROPERTIES
    IMPORTED_IMPLIB "${VCPKG_DIR}/lib/libcurl.lib"
    IMPORTED_LOCATION "${VCPKG_DIR}/bin/libcurl.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_DIR}/include"
)

set(JSON_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(motcpp PUBLIC yaml-cpp)

if(NOT DEFINED ENV{ONNXRuntime_DIR} OR "$ENV{ONNXRuntime_DIR}" STREQUAL "")
    set(ONNXRUNTIME_DIR "C:/libs/onnxruntime")
else()
    set(ONNXRUNTIME_DIR "$ENV{ONNXRuntime_DIR}")
endif()

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
    IMPORTED_IMPLIB "${ONNXRUNTIME_DIR}/lib/onnxruntime.lib"
    IMPORTED_LOCATION "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_DIR}/include"
)

# Your executable
add_executable(Valorant-Vision
    "cpp/main.cpp"
    "cpp/videoplayer.cpp"
    "cpp/mainwindow.cpp"
    "cpp/LogBar.cpp"
    "cpp/minimap.cpp"
    "cpp/neuralnetwork.cpp"
    "cpp/Mapdetector.cpp"
    "cpp/modelwrapper.cpp"
    "cpp/ocr.cpp"
    "cpp/webScraper.cpp"
    "headers/videoplayer.h"
    "headers/minimap.h"
    "headers/mainwindow.h"
    "headers/ocr.h"
)

# Lets us just include the files without having to specify relative path
target_include_directories(Valorant-Vision PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/cpp")
target_include_directories(Valorant-Vision PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/headers")

# Link Qt + OpenCV
target_link_libraries(Valorant-Vision PUBLIC
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
    onnxruntime
    Eigen3::Eigen
    yaml-cpp::yaml-cpp
    motcpp::motcpp
    nlohmann_json::nlohmann_json
    tesseract
    leptonica
    libcurl
)

#Copy model weights to the exe folder
add_custom_command(TARGET Valorant-Vision POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/model/faster_rcnn.onnx"
    "$<TARGET_FILE_DIR:Valorant-Vision>"
)

#Copy onnxruntime.dll to the exe folder
add_custom_command(TARGET Valorant-Vision POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:onnxruntime>"
    "$<TARGET_FILE_DIR:Valorant-Vision>"
)

# adds the minimap folder to the .exe directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/map_layouts"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/map_layouts"
)

# Adds the agent icons folder to the .exe directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/model/agent_icons"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/agent_icons"
)

# Copy tesseract/leptonica/curl DLLs
foreach(DLL
    "${VCPKG_DIR}/bin/tesseract55.dll"
    "${VCPKG_DIR}/bin/leptonica-1.87.0.dll"
    "${VCPKG_DIR}/bin/libcurl.dll"
    "${VCPKG_DIR}/bin/archive.dll"
    "${VCPKG_DIR}/bin/gif.dll"
    "${VCPKG_DIR}/bin/openjp2.dll"
    "${VCPKG_DIR}/bin/libwebpmux.dll"
    "${VCPKG_DIR}/bin/lz4.dll"
    "${VCPKG_DIR}/bin/zstd.dll"
)
    add_custom_command(TARGET Valorant-Vision POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL}" "$<TARGET_FILE_DIR:Valorant-Vision>"
    )
endforeach()

# Copy tessdata folder next to the exe
add_custom_command(TARGET Valorant-Vision POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/tessdata"
    "$<TARGET_FILE_DIR:Valorant-Vision>/tessdata"
)

# Adds id to agent icon file to .exe directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/model/id_to_name.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Adds the REID weights to the .exe directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/model/osnet_x1_0_dukemtmcreid.onnx"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# So we know where the 
target_compile_definitions(Valorant-Vision PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
) 

# For adding logo to the app window
qt_add_resources(APP_RESOURCES resources.qrc)
target_sources(Valorant-Vision PRIVATE ${APP_RESOURCES})
# Optional debug info
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV included libs: ${OpenCV_LIBS}")