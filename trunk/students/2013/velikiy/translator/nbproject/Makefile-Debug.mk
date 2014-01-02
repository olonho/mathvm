#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/libs/AsmJit/Assembler.o \
	${OBJECTDIR}/libs/AsmJit/AssemblerX86X64.o \
	${OBJECTDIR}/libs/AsmJit/CodeGenerator.o \
	${OBJECTDIR}/libs/AsmJit/Compiler.o \
	${OBJECTDIR}/libs/AsmJit/CompilerX86X64.o \
	${OBJECTDIR}/libs/AsmJit/CpuInfo.o \
	${OBJECTDIR}/libs/AsmJit/Defs.o \
	${OBJECTDIR}/libs/AsmJit/DefsX86X64.o \
	${OBJECTDIR}/libs/AsmJit/Logger.o \
	${OBJECTDIR}/libs/AsmJit/MemoryManager.o \
	${OBJECTDIR}/libs/AsmJit/Operand.o \
	${OBJECTDIR}/libs/AsmJit/OperandX86X64.o \
	${OBJECTDIR}/libs/AsmJit/Platform.o \
	${OBJECTDIR}/libs/AsmJit/Util.o \
	${OBJECTDIR}/src/ast.o \
	${OBJECTDIR}/src/bytecode.o \
	${OBJECTDIR}/src/bytecodeCode.o \
	${OBJECTDIR}/src/bytecodeTranslator.o \
	${OBJECTDIR}/src/interpreter.o \
	${OBJECTDIR}/src/jit.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/mathvm.o \
	${OBJECTDIR}/src/parser.o \
	${OBJECTDIR}/src/scanner.o \
	${OBJECTDIR}/src/translator.o \
	${OBJECTDIR}/src/utils.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mymathvm

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mymathvm: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mymathvm ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/libs/AsmJit/Assembler.o: libs/AsmJit/Assembler.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Assembler.o libs/AsmJit/Assembler.cpp

${OBJECTDIR}/libs/AsmJit/AssemblerX86X64.o: libs/AsmJit/AssemblerX86X64.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/AssemblerX86X64.o libs/AsmJit/AssemblerX86X64.cpp

${OBJECTDIR}/libs/AsmJit/CodeGenerator.o: libs/AsmJit/CodeGenerator.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/CodeGenerator.o libs/AsmJit/CodeGenerator.cpp

${OBJECTDIR}/libs/AsmJit/Compiler.o: libs/AsmJit/Compiler.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Compiler.o libs/AsmJit/Compiler.cpp

${OBJECTDIR}/libs/AsmJit/CompilerX86X64.o: libs/AsmJit/CompilerX86X64.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/CompilerX86X64.o libs/AsmJit/CompilerX86X64.cpp

${OBJECTDIR}/libs/AsmJit/CpuInfo.o: libs/AsmJit/CpuInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/CpuInfo.o libs/AsmJit/CpuInfo.cpp

${OBJECTDIR}/libs/AsmJit/Defs.o: libs/AsmJit/Defs.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Defs.o libs/AsmJit/Defs.cpp

${OBJECTDIR}/libs/AsmJit/DefsX86X64.o: libs/AsmJit/DefsX86X64.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/DefsX86X64.o libs/AsmJit/DefsX86X64.cpp

${OBJECTDIR}/libs/AsmJit/Logger.o: libs/AsmJit/Logger.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Logger.o libs/AsmJit/Logger.cpp

${OBJECTDIR}/libs/AsmJit/MemoryManager.o: libs/AsmJit/MemoryManager.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/MemoryManager.o libs/AsmJit/MemoryManager.cpp

${OBJECTDIR}/libs/AsmJit/Operand.o: libs/AsmJit/Operand.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Operand.o libs/AsmJit/Operand.cpp

${OBJECTDIR}/libs/AsmJit/OperandX86X64.o: libs/AsmJit/OperandX86X64.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/OperandX86X64.o libs/AsmJit/OperandX86X64.cpp

${OBJECTDIR}/libs/AsmJit/Platform.o: libs/AsmJit/Platform.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Platform.o libs/AsmJit/Platform.cpp

${OBJECTDIR}/libs/AsmJit/Util.o: libs/AsmJit/Util.cpp 
	${MKDIR} -p ${OBJECTDIR}/libs/AsmJit
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/libs/AsmJit/Util.o libs/AsmJit/Util.cpp

${OBJECTDIR}/src/ast.o: src/ast.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/ast.o src/ast.cpp

${OBJECTDIR}/src/bytecode.o: src/bytecode.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/bytecode.o src/bytecode.cpp

${OBJECTDIR}/src/bytecodeCode.o: src/bytecodeCode.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/bytecodeCode.o src/bytecodeCode.cpp

${OBJECTDIR}/src/bytecodeTranslator.o: src/bytecodeTranslator.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/bytecodeTranslator.o src/bytecodeTranslator.cpp

${OBJECTDIR}/src/interpreter.o: src/interpreter.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/interpreter.o src/interpreter.cpp

${OBJECTDIR}/src/jit.o: src/jit.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/jit.o src/jit.cpp

${OBJECTDIR}/src/main.o: src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/main.o src/main.cpp

${OBJECTDIR}/src/mathvm.o: src/mathvm.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mathvm.o src/mathvm.cpp

${OBJECTDIR}/src/parser.o: src/parser.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/parser.o src/parser.cpp

${OBJECTDIR}/src/scanner.o: src/scanner.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/scanner.o src/scanner.cpp

${OBJECTDIR}/src/translator.o: src/translator.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/translator.o src/translator.cpp

${OBJECTDIR}/src/utils.o: src/utils.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -g -Iinclude -Ilibs -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/utils.o src/utils.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/mymathvm

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
