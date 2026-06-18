cmake_minimum_required(VERSION 3.24)

project(hayo_haya LANGUAGES CXX)

option(HYH_REGENERATE_GRAMMAR "Regenerate the Bison/re2c sources at build time" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

set(PARSER_YY "${CMAKE_CURRENT_SOURCE_DIR}/src/parser.yy")
set(LEXER_RE "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.re")
set(PARSER_SRC "${GENERATED_DIR}/hyh_grammar_parser.cpp")
set(PARSER_HDR "${GENERATED_DIR}/hyh_grammar_parser.hpp")
set(LEXER_SRC "${GENERATED_DIR}/hyh_lexer.cpp")

if(HYH_REGENERATE_GRAMMAR)
    find_package(BISON QUIET)
    find_program(RE2C_EXECUTABLE re2c)

    if(NOT BISON_FOUND)
        message(FATAL_ERROR "bison is required to regenerate src/parser.yy. Use the devcontainer/Dockerfile or install bison locally.")
    endif()

    if(NOT RE2C_EXECUTABLE)
        message(FATAL_ERROR "re2c is required to regenerate src/lexer.re. Use the devcontainer/Dockerfile or install re2c locally.")
    endif()

    add_custom_command(
  OUTPUT "${PARSER_SRC}" "${PARSER_HDR}"
  COMMAND "${BISON_EXECUTABLE}"
   "--defines=${PARSER_HDR}"
   "--output=${PARSER_SRC}"
   "${PARSER_YY}"
  DEPENDS "${PARSER_YY}"
  COMMENT "Generating Bison parser"
  VERBATIM
 )

    add_custom_command(
  OUTPUT "${LEXER_SRC}"
  COMMAND "${RE2C_EXECUTABLE}"
   --utf8
   --input-encoding utf8
   -o "${LEXER_SRC}"
   "${LEXER_RE}"
  DEPENDS "${LEXER_RE}" "${PARSER_HDR}"
  COMMENT "Generating re2c lexer"
  VERBATIM
 )
else()
    if(NOT EXISTS "${PARSER_SRC}" OR NOT EXISTS "${PARSER_HDR}" OR NOT EXISTS "${LEXER_SRC}")
        message(FATAL_ERROR "HYH_REGENERATE_GRAMMAR=OFF requires pre-generated sources in ${GENERATED_DIR}.")
    endif()
endif()

add_executable(hyh
 src/main.cpp
 src/driver.cpp
 src/codegen/cpp_codegen.cpp
 src/hebrew/unicode.cpp
 src/hebrew/orthography.cpp
 src/hebrew/morphology.cpp
 src/hebrew/selection.cpp
 src/hebrew/sentence_analyzer.cpp
 src/fairy/discourse_resolver.cpp
 src/fairy/mirror_questions.cpp
 src/fairy/mirror_type_resolver.cpp
 src/fairy/semantic_dump.cpp
 src/fairy/semantic_analyzer.cpp
 "${PARSER_SRC}"
 "${LEXER_SRC}"
)

set_target_properties(hyh PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
 RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
 RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
 RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

target_include_directories(hyh PRIVATE
 "${CMAKE_CURRENT_SOURCE_DIR}/src"
 "${CMAKE_CURRENT_SOURCE_DIR}/third_party/args"
 "${GENERATED_DIR}"
)

if(MSVC)
    target_compile_options(hyh PRIVATE /W4 /permissive- /wd4065)
    target_compile_definitions(hyh PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(hyh PRIVATE -Wall -Wextra -Wpedantic)
endif()
