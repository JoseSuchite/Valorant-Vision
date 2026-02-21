cmake_minimum_required(VERSION 3.16)
project(QtOpenCVExample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tell CMake where Qt6 is installed
set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2019_64")

# Find Qt6
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Your executable
add_executable(QtOpenCVExample
    Test.cpp
)

# Link Qt + OpenCV
target_link_libraries(QtOpenCVExample
    Qt6::Widgets
    ${OpenCV_LIBS}
)

# Optional debug info
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
