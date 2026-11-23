# VUC 2004.1 is still a fragment-level reconstruction, not a linkable program.
# Build its independently compilable C++ source owners as a CMake OBJECT target.
# This must not be mistaken for a working executable or a whole-ELF byte match.
set(VUC_PORTABLE_SOURCES
  src/AST.cpp
  src/ASTNodeArena.cpp
  src/ASTNodeArenaCleanup.cpp
  src/ASTRttiNames.cpp
  src/ASTVtableProbes.cpp
  src/AlignConstant.cpp
  src/AlignString.cpp
  src/AsmOutput.cpp
  src/AsmOutputUnit.cpp
  src/AssembleModule.cpp
  src/BindLists.cpp
  src/CheckASTFunCall.cpp
  src/CheckExprList.cpp
  src/CheckLists.cpp
  src/ConvVal.cpp
  src/CurrFileInitialization.cpp
  src/EmptyHooks.cpp
  src/FlexLexer.cpp
  src/FlexLexerConstructors.cpp
  src/FlexLexerCreateBufferUnit.cpp
  src/FlexLexerDestructors.cpp
  src/FlexLexerInput.cpp
  src/FlexLexerRestart.cpp
  src/FlexLexerSwitchStreams.cpp
  src/GetOpType.cpp
  src/GlobalBuffers.cpp
  src/IR2AsmBinExprUnit.cpp
  src/IR2AsmBroadcastUnit.cpp
  src/IRAccumulatorableExact.cpp
  src/IRPredicatesEnvironments.cpp
  src/IRRTTIDummies.cpp
  src/IRRttiNames.cpp
  src/IRVtableProbes.cpp
  src/InlineAsmUnit.cpp
  src/Instruction.cpp
  src/InstructionArena.cpp
  src/InstructionArenaLifecycle.cpp
  src/InstructionList.cpp
  src/InstructionListLineCommentUnit.cpp
  src/IostreamGlobal_constantsTabStop.cpp
  src/IostreamInit.cpp
  src/ListDispatch.cpp
  src/ListFile.cpp
  src/ListFilePrintLine.cpp
  src/ListFileUnit.cpp
  src/MLGameString.cpp
  src/MLGameStringTableAdd.cpp
  src/MLGameStringTableLifecycle.cpp
  src/MLGameStringTableUnit.cpp
  src/MLHashTable.cpp
  src/MLHashTableFindCopy.cpp
  src/MLHashTableFindPtr.cpp
  src/MLTempStringData.cpp
  src/MLTempStringDefault.cpp
  src/MLTempStringFormatted.cpp
  src/MLTempVFormatExact.cpp
  src/Main.cpp
  src/PrintASTLists.cpp
  src/PrintASTVarList.cpp
  src/Printer.cpp
  src/ReduceBinOp.cpp
  src/RuntimeAbiConstants.cpp
  src/ScopeVectorLifecycle.cpp
  src/ScriptLexer.cpp
  src/ScriptParserUnit.cpp
  src/ScriptScannerUnit.cpp
  src/SemantCheck.cpp
  src/SemantUnit.cpp
  src/StdAfx.cpp
  src/StreambufPbackConstants.cpp
  src/SymbolTable.cpp
  src/SymbolTableBeginScope.cpp
  src/SymbolTableFind.cpp
  src/SymbolTableUnit.cpp
  src/TranslateFrag.cpp
  src/TranslateTypeUnit.cpp
  src/TypeAssignable.cpp
  src/Value.cpp
  src/ValueArithmeticUnit.cpp
  src/ValueBitwise.cpp
  src/ValueComparison.cpp
  src/ValueLogicalOr.cpp
  src/ValueMinus.cpp
  src/ValueTypeConvert.cpp
  src/VucCompilerUnit.cpp
  src/YyGetPreviousState.cpp
  src/YyInput.cpp
  src/YyTryNulTrans.cpp
  src/YyUnput.cpp
)

# These reconstructions reproduce internal GCC 3.3 libstdc++ templates that
# do not exist in modern libc++ or libstdc++. Preserve them for per-function
# historical compiler checks; do not silently substitute different STL code.
set(VUC_GCC33_ONLY_SOURCES
  src/AsmVariableUninitializedCopy.cpp
  src/ScopeAllocateAndCopy.cpp
  src/ScopeUninitializedCopy.cpp
  src/StringUninitializedCopy.cpp
  src/TypeUninitializedCopy.cpp
)

# Accepted historical C2 constructor owners require real preceding method
# definitions to reproduce GCC 3.3 register selection. They intentionally
# overlap existing C1/other provisional owner definitions; they must NOT be
# linked together or compiled as part of the portable object target until
# the authentic single-link translation-unit boundaries are reconstructed.
set(VUC_HISTORICAL_MATCH_SOURCES
  src/MLTempStringDefaultC2Owner.cpp
  src/MLTempStringFormattedC2Owner.cpp
  src/MLTempStringSetExactOwner.cpp
)

# Detect any new or missing VUC sources rather than implicitly compiling new
# experiments or silently leaving accepted owners out of the CMake project.
file(GLOB _vuc_actual_sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
set(_vuc_known_sources ${VUC_PORTABLE_SOURCES} ${VUC_GCC33_ONLY_SOURCES}
  ${VUC_HISTORICAL_MATCH_SOURCES})
list(SORT _vuc_actual_sources)
list(SORT _vuc_known_sources)
if(NOT _vuc_actual_sources STREQUAL _vuc_known_sources)
  message(FATAL_ERROR
    "VUC CMake source inventory differs from vuc/src/. Update both "
    "VUC_PORTABLE_SOURCES, VUC_GCC33_ONLY_SOURCES, and "
    "VUC_HISTORICAL_MATCH_SOURCES deliberately.")
endif()
list(LENGTH VUC_PORTABLE_SOURCES _vuc_portable_count)
list(LENGTH VUC_GCC33_ONLY_SOURCES _vuc_gcc_only_count)
list(LENGTH VUC_HISTORICAL_MATCH_SOURCES _vuc_owner_count)
message(STATUS
  "VUC partial reconstruction: ${_vuc_portable_count} native object units "
  "plus ${_vuc_gcc_only_count} GCC 3.3-only units and "
  "${_vuc_owner_count} historical match-only owners")

# Separate from VCL: source-level naming/types may overlap but they are
# unrelated projects with different historical compilers and link layouts.
add_library(vuc OBJECT ${VUC_PORTABLE_SOURCES})
target_include_directories(vuc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(vuc PRIVATE -std=gnu++98 -O2 -Wno-write-strings)
target_compile_definitions(vuc PRIVATE REVCL_VUC_NATIVE_ABI)

if(BUILD_TESTING)
  add_test(NAME vuc-candidate-manifest
    COMMAND "${CMAKE_COMMAND}"
      "-DREVCL_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
      -P "${PROJECT_SOURCE_DIR}/cmake/VerifyVucCandidateManifest.cmake")
endif()

# The strict VUC function-level byte matcher requires the historical GCC
# wrapper and a separately obtained, SHA-verified vendor reference archive.
# It is intentionally opt-in: native Clang/GCC builds are not historical
# proof, and Python is not required for the default VCL/VUC build.
set(REVCL_VUC_MATCH_COMPILER "" CACHE FILEPATH
  "Historical GCC 3.3.3 wrapper for VUC exact-function verification")
if(REVCL_VUC_MATCH_COMPILER)
  if(NOT EXISTS "${REVCL_VUC_MATCH_COMPILER}")
    message(FATAL_ERROR
      "REVCL_VUC_MATCH_COMPILER not found: ${REVCL_VUC_MATCH_COMPILER}")
  endif()
  set(_vuc_reference_archive
    "${PROJECT_SOURCE_DIR}/archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip")
  if(NOT EXISTS "${_vuc_reference_archive}")
    message(FATAL_ERROR
      "VUC reference archive must be obtained separately: ${_vuc_reference_archive}")
  endif()
  find_program(_vuc_python NAMES python3)
  if(NOT _vuc_python)
    message(FATAL_ERROR "VUC historical function matching requires Python 3")
  endif()
  set(_vuc_match_command
    "${_vuc_python}"
    "${PROJECT_SOURCE_DIR}/vuc/scripts/match_candidates.py"
    --compiler "${REVCL_VUC_MATCH_COMPILER}"
    --out "${CMAKE_CURRENT_BINARY_DIR}/exact-match-results")
  add_custom_target(vuc-exact-candidates COMMAND ${_vuc_match_command}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" VERBATIM)
  if(BUILD_TESTING)
    add_test(NAME vuc-exact-candidates COMMAND ${_vuc_match_command})
    set_tests_properties(vuc-exact-candidates PROPERTIES TIMEOUT 300)
  endif()
endif()
