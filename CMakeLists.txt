project(roketz)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Werror -Wsign-compare")

find_package(OpenGL)
find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_image REQUIRED)

message(GL = ${OPENGL_INCLUDE_DIR})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SDL_INCLUDE_DIR}
  ${SDLMIXER_INCLUDE_DIR}
#  ${OPENGL_INCLUDE_DIR}/Headers/
  "/Developer/GPU\ Computing/shared/inc/"
  /opt/local/include/
  )

link_directories("/opt/local/lib")

add_executable(roketz
  aabsp.cc
  billboard.cc
  binfile.cc
  camera.cc
  frustum.cc
  geometry.cc
  input.cc
  loadd3l.cc
  loadobj.cc
  main.cc
  object.cc
  particle.cc
  printer.cc
  rocket.cc
  room.cc
  texture.cc
  world.cc
  )

target_link_libraries(roketz
  ${SDL_LIBRARY}
  ${SDLMIXER_LIBRARY}
  ${SDLIMAGE_LIBRARY}
  ${OPENGL_LIBRARIES}
  plibsg
  plibul
  jpeg
  png
  )

