cmake_minimum_required(VERSION 3.23)

option(REVCL_HISTORICAL "Reproduce the original VCL binary with RH9 GCC 3.2.2" OFF)
option(REVCL_NATIVE_MACOS "Build a native macOS Mach-O executable" OFF)
project(REVCL LANGUAGES CXX)
if(REVCL_NATIVE_MACOS AND NOT APPLE)
  message(FATAL_ERROR "The macos preset requires macOS and Apple Clang")
endif()
include(CTest)
include(config/vcl-link-order.cmake)

list(LENGTH VCL_SOURCES VCL_SOURCE_COUNT)
if(NOT VCL_SOURCE_COUNT EQUAL 131)
  message(FATAL_ERROR "Expected 131 VCL translation units, got ${VCL_SOURCE_COUNT}")
endif()

add_executable(vcl ${VCL_SOURCES})
target_include_directories(vcl PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
target_link_libraries(vcl PRIVATE m)

if(REVCL_HISTORICAL)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "i386")
    message(FATAL_ERROR "Historical mode requires cmake/toolchains/rh9.cmake")
  endif()
  # Use the existing, independently validated build.json as the sole source
  # of historical compiler flags. CMake 3.23 supports JSON parsing natively.
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/config/build.json" _build_json)
  string(JSON _num_common LENGTH "${_build_json}" cxxflags)
  math(EXPR _last_common "${_num_common} - 1")
  set(_common_flags)
  foreach(_i RANGE 0 ${_last_common})
    string(JSON _flag GET "${_build_json}" cxxflags ${_i})
    list(APPEND _common_flags "${_flag}")
  endforeach()
  target_compile_options(vcl PRIVATE ${_common_flags})

  string(JSON _num_units LENGTH "${_build_json}" units)
  math(EXPR _last_unit "${_num_units} - 1")
  foreach(_i RANGE 0 ${_last_unit})
    string(JSON _source MEMBER "${_build_json}" units ${_i})
    string(JSON _num_unit_flags LENGTH "${_build_json}" units "${_source}" flags)
    set(_unit_flags)
    math(EXPR _last_flag "${_num_unit_flags} - 1")
    foreach(_j RANGE 0 ${_last_flag})
      string(JSON _flag GET "${_build_json}" units "${_source}" flags ${_j})
      list(APPEND _unit_flags "${_flag}")
    endforeach()
    list(SUBLIST _unit_flags 0 ${_num_common} _unit_prefix)
    if(NOT "${_unit_prefix}" STREQUAL "${_common_flags}")
      message(FATAL_ERROR "Historical flags for ${_source} do not extend global cxxflags")
    endif()
    list(SUBLIST _unit_flags ${_num_common} -1 _unit_extra)
    set_source_files_properties("${_source}" PROPERTIES COMPILE_OPTIONS "${_unit_extra}")
  endforeach()
  target_link_options(vcl PRIVATE "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/tools/rh9/vcl.ld")

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vcl.stripped"
    COMMAND "${CMAKE_STRIP}" -o "${CMAKE_CURRENT_BINARY_DIR}/vcl.stripped" "$<TARGET_FILE:vcl>"
    DEPENDS vcl
    VERBATIM)
  add_custom_target(vcl-stripped ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/vcl.stripped")
  if(BUILD_TESTING)
    add_test(NAME exact-sha256
      COMMAND "${CMAKE_COMMAND}"
      "-DREVCL_BINARY=${CMAKE_CURRENT_BINARY_DIR}/vcl.stripped"
      "-DREVCL_SHA_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tools/rh9/vcl.sha256"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifySha.cmake")
  endif()
else()
  # Linux reproduces the i386 data model; macOS builds a real native
  # arm64/x86_64 Mach-O executable, with pointer-sized compatibility shims.
  target_compile_options(vcl PRIVATE -std=gnu++98 -O2 -Wno-write-strings)
  if(NOT APPLE)
    target_compile_options(vcl PRIVATE -m32)
    target_link_options(vcl PRIVATE -m32)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vcl PRIVATE -fpermissive)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang ignores GCC 3.2's deferred inline pragma, so expose the actual
    # function definitions to main.cpp without altering the historic source.
    set_source_files_properties(src/main.cpp PROPERTIES COMPILE_OPTIONS
      "-include;${CMAKE_CURRENT_SOURCE_DIR}/src/RegWeb_inline.h")
  endif()

  if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME python-unit
      COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests/unit -q)
    set_tests_properties(python-unit PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    # Each standalone fixture becomes a distinct CTest case. The archived
    # macro-heavy input requires MASP and remains in the separate oracle suite.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/cases.json" _cases_json)
    string(JSON _case_count LENGTH "${_cases_json}")
    math(EXPR _last_case "${_case_count} - 1")
    foreach(_i RANGE 0 ${_last_case})
      string(JSON _case_name GET "${_cases_json}" ${_i} name)
      string(JSON _archived ERROR_VARIABLE _archive_error GET "${_cases_json}" ${_i} archive)
      if(NOT _archive_error)
        continue()
      endif()
      add_test(NAME "fixture-${_case_name}"
        COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/ci/test_modern_vcl.py"
        --executable "$<TARGET_FILE:vcl>" --case "${_case_name}")
    endforeach()
  endif()
endif()
