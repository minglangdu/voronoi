cmake_minimum_required(VERSION 4.3.0)

project(VoronoiProject)

find_package(OpenGL REQUIRED)

# compile executable

add_executable(Voronoi
    main.cpp 
    external/GLAD/src/glad.c
)

# GLFW

include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW ON
)

FetchContent_MakeAvailable(glfw)
target_link_libraries(Voronoi PRIVATE glfw OpenGL::GL)

# GLM

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3 
  GIT_SHALLOW ON
)

FetchContent_MakeAvailable(glm)
target_link_libraries(Voronoi PRIVATE glm::glm)

# GLAD

target_include_directories(Voronoi PRIVATE external/GLAD/include)