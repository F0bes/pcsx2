cmake_minimum_required(VERSION 3.14)
project(sljit_subproject NONE)

# Minimal in-tree sljit target for testing:
# - expose headers
# - build the core source used by consumers
set(SLJIT_SRCS
    sljit_src/sljitLir.c
    sljit_src/sljitUtils.c
)

add_library(sljit STATIC ${SLJIT_SRCS})
target_include_directories(sljit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sljit_src")
set_target_properties(sljit PROPERTIES POSITION_INDEPENDENT_CODE ON)
