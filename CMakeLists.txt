cmake_minimum_required(VERSION 3.14)
project(image-processing-pipeline)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(atta 0.3.10 REQUIRED)

# Project script
atta_add_target(projectScript "src/projectScript.cpp")
target_link_libraries(projectScript PRIVATE atta::imgui atta::implot)
