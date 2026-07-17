cmake_minimum_required(VERSION 3.16)
project(XLZD_LXe)

find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(XLZD_LXe main.cc ${sources} ${headers})
target_link_libraries(XLZD_LXe ${Geant4_LIBRARIES})
