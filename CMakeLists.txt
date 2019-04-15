cmake_minimum_required(VERSION 3.9.2)
project (randocha VERSION 0.0.1 DESCRIPTION "Fast x86 intrinsic based Random Number Generator ")

if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
endif()

# Dependencies
add_subdirectory(third_party/fmt-5.3.0 EXCLUDE_FROM_ALL)

# This Library (header-only)
add_library(randocha INTERFACE)
target_include_directories(randocha INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Benchmark Executable
add_executable(randocha-bench source/bench.cpp)
target_link_libraries(randocha-bench
    PRIVATE randocha fmt-header-only)
set_target_properties(randocha-bench PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# CSV Exporter Executable (for distribution test)
add_executable(randocha-csv source/csv_exporter.cpp)
target_link_libraries(randocha-csv
    PRIVATE randocha fmt-header-only)
set_target_properties(randocha-csv PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

  # Distribution visualiser
add_executable(distribution_viz source/distribution_viz.cpp)
target_link_libraries(distribution_viz
    PRIVATE randocha fmt-header-only)
set_target_properties(distribution_viz PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
