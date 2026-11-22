if(NOT EXISTS "${REVCL_BINARY}")
  message(FATAL_ERROR "Missing VCL executable: ${REVCL_BINARY}")
endif()
execute_process(
  COMMAND "${REVCL_BINARY}" -h
  RESULT_VARIABLE exit_status
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
  TIMEOUT 15)
if(NOT "${exit_status}" STREQUAL "0")
  message(FATAL_ERROR "VCL -h exited with ${exit_status}: ${stderr}")
endif()
string(FIND "${stdout}${stderr}" "vcl 1.4beta7" version_at)
if(version_at EQUAL -1)
  message(FATAL_ERROR "VCL -h did not print its expected version")
endif()
message(STATUS "VCL -h reports vcl 1.4beta7")
