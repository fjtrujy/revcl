# CMake runs on the host; compiler and strip run in the original RH9 image.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)
get_filename_component(REVCL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(CMAKE_CXX_COMPILER "${REVCL_ROOT}/scripts/cmake/rh9-g++.sh")
set(CMAKE_STRIP "${REVCL_ROOT}/scripts/cmake/rh9-strip.sh" CACHE FILEPATH "Historical strip" FORCE)
# No run tests for a 32-bit 2003 ELF on a modern 64-bit / macOS host.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
