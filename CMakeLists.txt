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
# Your executable
add_executable(Valorant-Vision
    "cpp/main.cpp"
    "cpp/videoplayer.cpp"
    "cpp/mainwindow.cpp"
    "cpp/LogBar.cpp"
    "cpp/minimap.cpp"
    "cpp/mapdetector.cpp"
    "headers/videoplayer.h"
    "headers/mainwindow.h"
    "headers/logbar.h"
    "headers/minimap.h"
    "headers/mapdetector.h"
)
# Link Qt + OpenCV
target_link_libraries(Valorant-Vision
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
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