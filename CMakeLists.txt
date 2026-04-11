cmake_minimum_required(VERSION 3.10)
project(OCR_Detector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Find Tesseract
find_package(Tesseract CONFIG REQUIRED)

# Include directories
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${Tesseract_INCLUDE_DIRS}
)

# Build executable
add_executable(ocr_detector main.cpp)

# Link libraries
target_link_libraries(ocr_detector
    ${OpenCV_LIBS}
    Tesseract::libtesseract
)
