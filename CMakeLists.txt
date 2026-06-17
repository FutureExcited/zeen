cmake_minimum_required(VERSION 3.25)
project(zeen CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ── be-engine (consumed as a library; skip its bundled example apps) ───────────
# The engine lives as a git submodule under external/be-engine. We pull in its
# vendor/core/toolkit libraries + the be_* CMake helper functions, but NOT its
# own example executables (zeen builds its own app).
set(BE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/be-engine)

# ── zeen app ──────────────────────────────────────────────────────────────────
add_subdirectory(src)
