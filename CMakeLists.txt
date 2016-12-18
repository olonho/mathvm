cmake_minimum_required(VERSION 2.8)

set(OUT ${ROOT}/build)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(CMAKE_BUILD_TYPE MATCHES "debug")
    set(OBJ "${OUT}/debug")
    set(BIN "${OUT}/debug")
else()
    set(OBJ "${OUT}/opt")
    set(BIN "${OUT}/opt")
endif()

set(MATHVM "${BIN}/mvm")
set(MATHVMTGZ ../MathVM.tgz)

set(CFLAGS "-Wall -Werror -D_REENTRANT -fPIC ${USER_CFLAGS}")
set(LIBS_ROOT "${VM_ROOT}/libs")
set(ASMJIT_CFLAGS -Wno-error)
set(INCLUDE_DIRS
        ${VM_ROOT}/include
        ${LIBS_ROOT})
set(VM_INCLUDE ${VM_ROOT}/vm)
set(ASMJIT_INCLUDE ${LIBS_ROOT}/asmjit)
set(USER_INCLUDE ${ROOT})
set(LIBS ${USER_LIBS} pthread)
add_definitions("${USER_DEFS} -D_POSIX_SOURCE")

if(MINGW)
    add_definitions("-DMATHVM_WIN")
else(MINGW)
    find_package(LibDL REQUIRED)
    add_definitions("-DMATHVM_POSIX")
    set(LIBS ${LIBS} ${LIBDL_LIBRARIES})
endif(MINGW)

if(LINUX)
    set(LIBS "${LIBS} -rdynamic")
endif(LINUX)

if(${WITH_SDL})
    find_package(SDL)
    set(SDL_BUILDING_LIBRARY)
    add_definitions("-DMATHVM_WITH_SDL")
    set(LIBS "${LIBS} ${SDL_LIBRARY}")
endif()

set(ASMJIT_OBJ_DIR ${OBJ}/asmjit)

AUX_SOURCE_DIRECTORY(${VM_ROOT}/vm VM_SOURCES)
AUX_SOURCE_DIRECTORY(${LIBS_ROOT} ASM_JIT_SOURCES)
set(COMMON_SRC
        ${VM_ROOT}/include/ast.h
        ${VM_ROOT}/include/mathvm.h
        ${VM_ROOT}/include/visitors.h
        ${VM_ROOT}/vm/scanner.h
        ${VM_ROOT}/vm/parser.h)

set(STUDENT_SOURCES
        ${STUDENT_SOURCES}
        ${COMMON_SRC})

set(VM_SOURCES
        ${VM_ROOT}/vm/ast.cpp
        ${VM_ROOT}/vm/interpreter.cpp
        ${VM_ROOT}/vm/mathvm.cpp
        ${VM_ROOT}/vm/parser.cpp
        ${VM_ROOT}/vm/scanner.cpp
        ${VM_ROOT}/vm/utils.cpp
        ${COMMON_SRC})

set(EXECUTABLE_OUTPUT_PATH ${BIN})

#SET_SOURCE_FILES_PROPERTIES(${VM_SOURCES}
#    PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRS} ${VM_INCLUDE}")
#SET_SOURCE_FILES_PROPERTIES(${STUDENT_SOURCES}
#    PROPERTIES INCLUDE_DIRECTORIES "${INCLUDE_DIRS} ${USER_INCLUDE} ${VM_INCLUDE}")
#SET_SOURCE_FILES_PROPERTIES(${ASM_JIT_SOURCES}
#    PROPERTIES INCLUDE_DIRECTORIES "${ASMJIT_INCLUDE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")
#message(${LIBS_ROOT})
#set_property(DIRECTORY ${LIBS_ROOT} APPEND PROPERTY CMAKE_CXX_FLAGS "${ASMJIT_CFLAGS}")
