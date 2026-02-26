cmake_minimum_required(VERSION 3.16)
project(Valorant-Vision)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tell CMake where Qt6 is installed
set(CMAKE_PREFIX_PATH "$ENV{CMAKE_COMPILER_DIR}")

# Find Qt6
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Multimedia)
find_package(Qt6 REQUIRED COMPONENTS MultimediaWidgets)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Your executable
add_executable(Valorant-Vision
    "cpp/main.cpp"
 "cpp/videoplayer.cpp" "headers/videoplayer.h" "headers/mainwindow.h" "cpp/mainwindow.cpp")

# Link Qt + OpenCV
target_link_libraries(Valorant-Vision
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
)

# Optional debug info
message(STATUS "Qt6_DIR: ${Qt6_DIR}") 
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
