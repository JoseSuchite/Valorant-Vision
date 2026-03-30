cmake_minimum_required(VERSION 3.16)
project(Valorant-Vision)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets MultimediaWidgets Multimedia REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(Valorant-Vision
    "cpp/main.cpp"
    "cpp/videoplayer.cpp"
    "cpp/mainwindow.cpp"
    "cpp/LogBar.cpp"
    "cpp/minimap.cpp"
    "cpp/mapdetector.cpp"
)

target_include_directories(Valorant-Vision PRIVATE
    ${CMAKE_SOURCE_DIR}/headers
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(Valorant-Vision
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/map_layouts"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/map_layouts"
)

#qt_add_resources(APP_RESOURCES resources.qrc)
#target_sources(Valorant-Vision PRIVATE ${APP_RESOURCES})

message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")