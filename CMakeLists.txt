cmake_minimum_required(VERSION 3.16)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "")

project(diagen)

set(CMAKE_CXX_STANDARD 20)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.14/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

FOREACH(subdir ${SUBDIRS})
  ADD_SUBDIRECTORY(${subdir})
ENDFOREACH()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(CONANFILE conanfile.txt
                BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

set(SUBDIRS 
      tetris)
FOREACH(subdir ${SUBDIRS})
   add_subdirectory(${subdir})

#    add_custom_command(
#       TARGET ${subdir} POST_BUILD
#       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR} $<TARGET_FILE_DIR:${SUBDIR}>
#       COMMENT "Copying shaders ${CMAKE_SOURCE_DIR} to $<TARGET_FILE_DIR:${SUBDIR}>" 
#   )
ENDFOREACH()
