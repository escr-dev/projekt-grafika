cmake_minimum_required(VERSION 3.20)

project(projekt_testowy LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h" PATH_SUFFIXES stb REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/build/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*\\\\build\\\\.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/x64/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*\\\\x64\\\\.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/Debug/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*\\\\Debug\\\\.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/Release/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*\\\\Release\\\\.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/.vs/.*")
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*\\\\.vs\\\\.*")

add_executable(projekt_testowy
    ${PROJECT_SOURCES}
)

target_include_directories(projekt_testowy PRIVATE
    ${STB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(projekt_testowy PRIVATE
    glad::glad
    glfw
    glm::glm
    opengl32
)
