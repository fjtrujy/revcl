# Audit each exceptional historical GCC flag against the reference stripped ELF.
# Usage: cmake -P cmake/AuditHistoricalCompilerOverrides.cmake
# Run after building the historical preset; no tracked file or build object
# is modified. Any newly redundant override fails this audit.
cmake_minimum_required(VERSION 3.23)
get_filename_component(_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(NOT DEFINED REVCL_HISTORICAL_BUILD)
  set(REVCL_HISTORICAL_BUILD "${_root}/tmp/cmake/historical")
elseif(NOT IS_ABSOLUTE "${REVCL_HISTORICAL_BUILD}")
  get_filename_component(REVCL_HISTORICAL_BUILD "${_root}/${REVCL_HISTORICAL_BUILD}" ABSOLUTE)
endif()
set(_build "${REVCL_HISTORICAL_BUILD}")
set(_audit "${_build}/flag-audit")
set(_elf "${_build}/vcl.stripped")
set(_link_script "${_build}/CMakeFiles/vcl.dir/link.txt")
if(NOT EXISTS "${_elf}" OR NOT EXISTS "${_link_script}")
  message(FATAL_ERROR "Build the historical preset before auditing per-file flags")
endif()
file(READ "${_root}/tools/rh9/vcl.sha256" _checksum_line)
string(REGEX MATCH "^[0-9a-f]+" _expected "${_checksum_line}")
file(SHA256 "${_elf}" _baseline)
if(NOT _baseline STREQUAL _expected)
  message(FATAL_ERROR "Historical baseline is not byte-exact: ${_baseline}")
endif()

file(READ "${_root}/config/build.json" _configuration)
string(JSON _common_count LENGTH "${_configuration}" cxxflags)
math(EXPR _last_common "${_common_count} - 1")
set(_common_flags)
foreach(_i RANGE 0 ${_last_common})
  string(JSON _flag GET "${_configuration}" cxxflags ${_i})
  list(APPEND _common_flags "${_flag}")
endforeach()
string(JSON _unit_count LENGTH "${_configuration}" units)
if(_unit_count EQUAL 0)
  message(STATUS "No historical per-source compiler overrides remain")
  return()
endif()
math(EXPR _last_unit "${_unit_count} - 1")

file(READ "${_link_script}" _link_text)
separate_arguments(_link_arguments UNIX_COMMAND "${_link_text}")
list(FIND _link_arguments "-o" _output_option)
if(_output_option LESS 0)
  message(FATAL_ERROR "Historical linker command has no -o output")
endif()
math(EXPR _output_argument "${_output_option} + 1")
file(MAKE_DIRECTORY "${_audit}")

set(_required 0)
set(_redundant)
foreach(_i RANGE 0 ${_last_unit})
  string(JSON _source MEMBER "${_configuration}" units ${_i})
  get_filename_component(_stem "${_source}" NAME_WE)
  set(_original_object "CMakeFiles/vcl.dir/${_source}.o")
  set(_alternate_object "${_audit}/${_stem}.without-extra.o")
  set(_alternate_elf "${_audit}/${_stem}.without-extra.elf")
  set(_alternate_stripped "${_audit}/${_stem}.without-extra.stripped")
  list(FIND _link_arguments "${_original_object}" _object_index)
  if(_object_index LESS 0 OR NOT EXISTS "${_build}/${_original_object}")
    message(FATAL_ERROR "Original object absent from link inputs: ${_original_object}")
  endif()
  string(JSON _extra_count LENGTH "${_configuration}" units "${_source}")
  if(_extra_count EQUAL 0)
    message(FATAL_ERROR "Empty per-source override for ${_source}")
  endif()
  math(EXPR _last_extra "${_extra_count} - 1")
  set(_extra_flags)
  foreach(_j RANGE 0 ${_last_extra})
    string(JSON _extra GET "${_configuration}" units "${_source}" ${_j})
    list(APPEND _extra_flags "${_extra}")
  endforeach()

  execute_process(COMMAND "${_root}/scripts/cmake/rh9-g++.sh"
      "-I${_root}/src" "-I${_root}/src/include" ${_common_flags}
      -c "${_root}/${_source}" -o "${_alternate_object}"
    WORKING_DIRECTORY "${_build}"
    RESULT_VARIABLE _compile_result
    OUTPUT_VARIABLE _compile_stdout ERROR_VARIABLE _compile_stderr)
  if(NOT _compile_result EQUAL 0)
    message(FATAL_ERROR
      "Recompiling ${_source} without ${_extra_flags} failed:\n${_compile_stdout}\n${_compile_stderr}")
  endif()

  set(_alternative_link ${_link_arguments})
  list(REMOVE_AT _alternative_link ${_object_index})
  list(INSERT _alternative_link ${_object_index} "${_alternate_object}")
  list(REMOVE_AT _alternative_link ${_output_argument})
  list(INSERT _alternative_link ${_output_argument} "${_alternate_elf}")
  execute_process(COMMAND ${_alternative_link}
    WORKING_DIRECTORY "${_build}"
    RESULT_VARIABLE _link_result
    OUTPUT_VARIABLE _link_stdout ERROR_VARIABLE _link_stderr)
  if(NOT _link_result EQUAL 0)
    message(FATAL_ERROR
      "Linking ${_source} without ${_extra_flags} failed:\n${_link_stdout}\n${_link_stderr}")
  endif()

  execute_process(COMMAND "${_root}/scripts/cmake/rh9-strip.sh"
      -o "${_alternate_stripped}" "${_alternate_elf}"
    WORKING_DIRECTORY "${_build}"
    RESULT_VARIABLE _strip_result
    OUTPUT_VARIABLE _strip_stdout ERROR_VARIABLE _strip_stderr)
  if(NOT _strip_result EQUAL 0)
    message(FATAL_ERROR
      "Stripping alternate ${_source} failed:\n${_strip_stdout}\n${_strip_stderr}")
  endif()

  file(SHA256 "${_alternate_stripped}" _candidate)
  if(_candidate STREQUAL _expected)
    list(APPEND _redundant "${_source} (${_extra_flags})")
    message(STATUS "REDUNDANT: ${_source} ${_extra_flags} (SHA unchanged)")
  else()
    math(EXPR _required "${_required} + 1")
    message(STATUS "REQUIRED: ${_source} ${_extra_flags} (SHA ${_candidate})")
  endif()
endforeach()
if(_redundant)
  string(JOIN "\n  " _names ${_redundant})
  message(FATAL_ERROR "The following per-source overrides can be removed:\n  ${_names}")
endif()
message(STATUS "All ${_required} historical per-source overrides remain necessary")
