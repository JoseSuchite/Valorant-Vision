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

# Download eigen (I love this library
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

target_link_libraries(motcpp PUBLIC yaml-cpp)

add_library(onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime PROPERTIES
    IMPORTED_IMPLIB "$ENV{ONNXRuntime_DIR}/lib/onnxruntime.lib"
    IMPORTED_LOCATION "$ENV{ONNXRuntime_DIR}/lib/onnxruntime.dll"
    INTERFACE_INCLUDE_DIRECTORIES "$ENV{ONNXRuntime_DIR}/include"
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
    # Why are only these two .h files here when the directories are included down below, you ask?
    # Why, that's actually quite simple! It's because Qt is weird and breaks without it
    # (The real answer is that it involves generation of MOC files or something, and it can't find the .h files by just including the directories
    # Only comes up for these two since they're being connected through Qt connect
    "headers/videoplayer.h" 
    "headers/minimap.h"
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
# For adding logo to the app window
qt_add_resources(APP_RESOURCES resources.qrc)
target_sources(Valorant-Vision PRIVATE ${APP_RESOURCES})
# Optional debug info
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV included libs: ${OpenCV_LIBS}")