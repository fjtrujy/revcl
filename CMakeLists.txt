cmake_minimum_required(VERSION 3.23)

option(REVCL_HISTORICAL "Reproduce the original VCL binary with RH9 GCC 3.2.2" OFF)
project(REVCL LANGUAGES CXX)
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
    string(JSON _num_unit_flags LENGTH "${_build_json}" units "${_source}")
    set(_unit_extra)
    math(EXPR _last_flag "${_num_unit_flags} - 1")
    foreach(_j RANGE 0 ${_last_flag})
      string(JSON _flag GET "${_build_json}" units "${_source}" ${_j})
      list(APPEND _unit_extra "${_flag}")
    endforeach()
    set_source_files_properties("${_source}" PROPERTIES COMPILE_OPTIONS "${_unit_extra}")
  endforeach()
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
    # Historical per-file exceptions should disappear when equivalent C++
    # reconstruction makes them unnecessary. Verify their necessity by
    # substituting one plain-flag object at the same link position each time.
    add_test(NAME historical-compiler-override-necessity
      COMMAND "${CMAKE_COMMAND}"
      "-DREVCL_HISTORICAL_BUILD=${CMAKE_CURRENT_BINARY_DIR}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AuditHistoricalCompilerOverrides.cmake")
    set_tests_properties(historical-compiler-override-necessity
      PROPERTIES TIMEOUT 180)
  endif()
else()
  # Modern builds follow the native 64-bit host ABI. The sole 32-bit
  # configuration is the historical GCC 3.2.2 Docker reproduction above.
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
      "Modern VCL requires a 64-bit target. Use the historical Docker "
      "toolchain to reproduce the original 32-bit Linux executable.")
  endif()
  target_compile_options(vcl PRIVATE -std=gnu++98 -O2 -Wno-write-strings)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vcl PRIVATE -fpermissive)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang ignores GCC 3.2's deferred inline pragma, so expose the actual
    # function definitions to main.cpp without altering the historic source.
    set_source_files_properties(src/main.cpp PROPERTIES COMPILE_OPTIONS
      "-include;${CMAKE_CURRENT_SOURCE_DIR}/src/RegWeb_inline.h")
  endif()

  if(BUILD_TESTING)
    # Native CMake/CTest validation does not require a Python interpreter.
    # The Python tests under tests/unit exercise optional decompilation tools,
    # not the VCL executable, and remain available to developers separately.
    add_test(NAME project-configuration
      COMMAND "${CMAKE_COMMAND}"
        "-DREVCL_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyProject.cmake")
    add_test(NAME cli-help
      COMMAND "${CMAKE_COMMAND}"
        "-DREVCL_BINARY=$<TARGET_FILE:vcl>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyCLI.cmake")

    # Individual fixture tests check the complete generated stdout SHA,
    # correct status, and errors using CMake's execute_process/file(SHA256).
    # The archived macro-heavy input needs MASP and is tested separately.
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
        COMMAND "${CMAKE_COMMAND}"
          "-DREVCL_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
          "-DREVCL_OUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/fixture-results"
          "-DREVCL_BINARY=$<TARGET_FILE:vcl>"
          "-DREVCL_CASE=${_case_name}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyFixture.cmake")
      set_tests_properties("fixture-${_case_name}" PROPERTIES TIMEOUT 20)
    endforeach()
  endif()
endif()
