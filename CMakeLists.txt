cmake_minimum_required(VERSION 3.16)

project(LightCorrespondence VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
    src/light_comm.cpp
    src/main.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${OpenCV_LIBS}
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()
