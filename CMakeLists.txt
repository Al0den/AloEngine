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

target_include_directories(AloEngine PRIVATE ${CMAKE_SOURCE_DIR}/src)

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
add_executable(perft ${PERFT_SOURCES} ${CMAKE_SOURCE_DIR}/src/perft_main.cpp)
target_include_directories(perft PRIVATE ${CMAKE_SOURCE_DIR}/src)
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

# Eval: Elo estimator utility
add_executable(evaluate_elo ${CMAKE_SOURCE_DIR}/eval/evaluate_elo.cpp)
if(MSVC)
  target_compile_options(evaluate_elo PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(evaluate_elo PRIVATE $<$<CONFIG:Release>:-O3 -ffast-math -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(evaluate_elo PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

# UCI match runner: link core sources except engine mains
set(MATCH_SOURCES ${SRC_FILES})
list(FILTER MATCH_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER MATCH_SOURCES EXCLUDE REGEX ".*/perft_main\\.cpp$")
add_executable(uci_match ${MATCH_SOURCES} ${CMAKE_SOURCE_DIR}/tools/uci_match.cpp)
target_include_directories(uci_match PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(MSVC)
  target_compile_options(uci_match PRIVATE $<$<CONFIG:Release>:/O2>)
else()
  target_compile_options(uci_match PRIVATE $<$<CONFIG:Release>:-O3 -ffast-math -march=native -flto -fno-exceptions -fno-rtti>)
  target_link_options(uci_match PRIVATE $<$<CONFIG:Release>:-flto -Wl,-dead_strip>)
endif()

# Export compile_commands.json for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
