cmake_minimum_required(VERSION 3.24)

project(haya_hayta_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HYH_REGENERATE_GRAMMAR "Regenerate parser and lexer using Bison and re2c" ON)

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(PARSER_SRC ${GENERATED_DIR}/parser.cpp)
set(PARSER_HDR ${GENERATED_DIR}/parser.hpp)
set(LEXER_SRC  ${GENERATED_DIR}/lexer.cpp)

if(HYH_REGENERATE_GRAMMAR)
    find_program(BISON_EXECUTABLE bison REQUIRED)
    find_program(RE2C_EXECUTABLE re2c REQUIRED)

    add_custom_command(
        OUTPUT ${PARSER_SRC} ${PARSER_HDR}
        COMMAND ${BISON_EXECUTABLE}
            --defines=${PARSER_HDR}
            --output=${PARSER_SRC}
            ${CMAKE_SOURCE_DIR}/src/parser.yy
        DEPENDS ${CMAKE_SOURCE_DIR}/src/parser.yy ${CMAKE_SOURCE_DIR}/src/ast.hpp
        COMMENT "Generating parser with Bison"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${LEXER_SRC}
        COMMAND ${RE2C_EXECUTABLE}
            --utf8
            --input-encoding utf8
            --output ${LEXER_SRC}
            ${CMAKE_SOURCE_DIR}/src/lexer.re
        DEPENDS ${CMAKE_SOURCE_DIR}/src/lexer.re ${PARSER_HDR}
        COMMENT "Generating lexer with re2c"
        VERBATIM
    )
else()
    if(NOT EXISTS ${PARSER_SRC} OR NOT EXISTS ${PARSER_HDR} OR NOT EXISTS ${LEXER_SRC})
        message(FATAL_ERROR
            "Generated parser/lexer files are missing. "
            "Configure with -DHYH_REGENERATE_GRAMMAR=ON inside the devcontainer."
        )
    endif()
endif()

add_executable(hyh
    src/main.cpp
    src/driver.cpp
    src/codegen.cpp
    ${PARSER_SRC}
    ${LEXER_SRC}
)

target_include_directories(hyh PRIVATE
    src
    ${GENERATED_DIR}
)

target_compile_definitions(hyh PRIVATE
    HYH_DEFAULT_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
)

if(MSVC)
    target_compile_options(hyh PRIVATE /W4)
else()
    target_compile_options(hyh PRIVATE -Wall -Wextra -Wpedantic)
endif()
