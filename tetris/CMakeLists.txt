cmake_minimum_required(VERSION 3.16)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

file(GLOB TETRIS_SRCS CONFIGURE_DEPENDS *.cpp *.h)

add_executable(tetris ${TETRIS_SRCS})
conan_target_link_libraries(tetris)

add_dependencies(tetris copy_resources)