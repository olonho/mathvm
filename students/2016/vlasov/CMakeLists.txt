cmake_minimum_required(VERSION 3.5)
project(mathvm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(VMDIR ../../..)

include_directories(${VMDIR}/include
    ${VMDIR}/vm
    ${VMDIR}/libs)

set(SOURCE_FILES
    ${VMDIR}/include/ast.h
    ${VMDIR}/include/mathvm.h
    ${VMDIR}/include/visitors.h
    ${VMDIR}/vm/ast.cpp
    ${VMDIR}/vm/mathvm.cpp
    ${VMDIR}/vm/parser.cpp
    ${VMDIR}/vm/parser.h
    ${VMDIR}/vm/scanner.cpp
    ${VMDIR}/vm/scanner.h
    ${VMDIR}/vm/utils.cpp
    ast_to_code.cpp
    ast_to_code.h)

set(TEST_FILES ${SOURCE_FILES} test.cpp)
set(MAIN_FILES ${SOURCE_FILES} main.cpp)

add_executable(hwtest ${TEST_FILES})
add_executable(hwmain ${MAIN_FILES})
