cmake_minimum_required(VERSION 3.16)

# Project metadata
project(AloEngine CXX)

# Language standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Release if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Collect sources
file(GLOB SRC_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Main engine executable (UCI)
set(ENGINE_SOURCES ${SRC_FILES})
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/perft_main\\.cpp$")
add_executable(AloEngine ${ENGINE_SOURCES})

target_include_directories(AloEngine PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

# Optimize in Release builds similar to the Makefile
if(MSVC)
  target_compile_options(AloEngine PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(AloEngine PRIVATE $<$<CONFIG:Release>:-O3 -ffast-math -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(AloEngine PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

# Convenience target to run the engine from the build directory
add_custom_target(run
  COMMAND AloEngine
  DEPENDS AloEngine
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running AloEngine"
)

# Perft target: build a separate binary without main.cpp
set(PERFT_SOURCES ${SRC_FILES})
list(FILTER PERFT_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER PERFT_SOURCES EXCLUDE REGEX ".*/perft_main\\.cpp$")
add_executable(perft ${PERFT_SOURCES} ${CMAKE_SOURCE_DIR}/src/perft_main.cpp)
target_include_directories(perft PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(perft PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(perft PRIVATE $<$<CONFIG:Release>:-O3 -ffast-math -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(perft PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

add_custom_target(perft-run
  COMMAND perft 5
  DEPENDS perft
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running perft at depth 5"
)

# Export compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Perft suite runner: read (depth,FEN) lines from a file and write timing/NPS results.
add_executable(perft_suite
  ${PERFT_SOURCES}
  ${CMAKE_SOURCE_DIR}/tools/perft_suite.cpp
)
target_include_directories(perft_suite PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(perft_suite PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(perft_suite PRIVATE $<$<CONFIG:Release>:-O3 -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(perft_suite PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

# -------------------------
# Self-play dataset generator
# -------------------------
# Reuse PERFT_SOURCES (no main.cpp, no perft_main.cpp) and add selfplay_main.cpp
add_executable(selfplay
  ${PERFT_SOURCES}
  ${CMAKE_SOURCE_DIR}/tools/selfplay_main.cpp
)

target_include_directories(selfplay PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(selfplay PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(selfplay PRIVATE $<$<CONFIG:Release>:-O3 -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(selfplay PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

add_custom_target(selfplay-run
  COMMAND selfplay 1000 nnue_dataset.jsonl
  DEPENDS selfplay
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running selfplay to generate NNUE dataset"
)
