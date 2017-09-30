TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    ../../../vm/ast.cpp \
    ../../../vm/interpreter.cpp \
    ../../../vm/mathvm.cpp \
    ../../../vm/parser.cpp \
    ../../../vm/scanner.cpp \
    ../../../vm/translator.cpp \
    ../../../vm/utils.cpp \
    translator_impl.cpp \
    ../../../vm/jit.cpp \
    ../../../libs/asmjit/base/assembler.cpp \
    ../../../libs/asmjit/base/compiler.cpp \
    ../../../libs/asmjit/base/compilercontext.cpp \
    ../../../libs/asmjit/base/constpool.cpp \
    ../../../libs/asmjit/base/containers.cpp \
    ../../../libs/asmjit/base/cpuinfo.cpp \
    ../../../libs/asmjit/base/globals.cpp \
    ../../../libs/asmjit/base/hlstream.cpp \
    ../../../libs/asmjit/base/logger.cpp \
    ../../../libs/asmjit/base/operand.cpp \
    ../../../libs/asmjit/base/podvector.cpp \
    ../../../libs/asmjit/base/runtime.cpp \
#    ../../../libs/asmjit/base/utils.cpp \
    ../../../libs/asmjit/base/vmem.cpp \
    ../../../libs/asmjit/base/zone.cpp \
    ../../../libs/asmjit/x86/x86assembler.cpp \
    ../../../libs/asmjit/x86/x86compiler.cpp \
    ../../../libs/asmjit/x86/x86compilercontext.cpp \
    ../../../libs/asmjit/x86/x86compilerfunc.cpp \
    ../../../libs/asmjit/x86/x86inst.cpp \
    ../../../libs/asmjit/x86/x86operand.cpp \
    ../../../libs/asmjit/x86/x86operand_regs.cpp

HEADERS += \
    ../../../include/ast.h \
    ../../../include/mathvm.h \
    ../../../include/visitors.h \
    ../../../vm/parser.h \
    ../../../vm/scanner.h \
    ../../../vm/jit.h \
    ../../../libs/asmjit/base/assembler.h \
    ../../../libs/asmjit/base/compiler.h \
    ../../../libs/asmjit/base/compilercontext_p.h \
    ../../../libs/asmjit/base/compilerfunc.h \
    ../../../libs/asmjit/base/constpool.h \
    ../../../libs/asmjit/base/containers.h \
    ../../../libs/asmjit/base/cpuinfo.h \
    ../../../libs/asmjit/base/globals.h \
    ../../../libs/asmjit/base/hlstream.h \
    ../../../libs/asmjit/base/logger.h \
    ../../../libs/asmjit/base/operand.h \
    ../../../libs/asmjit/base/podvector.h \
    ../../../libs/asmjit/base/runtime.h \
    ../../../libs/asmjit/base/utils.h \
    ../../../libs/asmjit/base/vectypes.h \
    ../../../libs/asmjit/base/vmem.h \
    ../../../libs/asmjit/base/zone.h \
    ../../../libs/asmjit/x86/x86assembler.h \
    ../../../libs/asmjit/x86/x86compiler.h \
    ../../../libs/asmjit/x86/x86compilercontext_p.h \
    ../../../libs/asmjit/x86/x86compilerfunc.h \
    ../../../libs/asmjit/x86/x86inst.h \
    ../../../libs/asmjit/x86/x86operand.h \
    ../../../libs/asmjit/apibegin.h \
    ../../../libs/asmjit/apiend.h \
    ../../../libs/asmjit/arm.h \
    ../../../libs/asmjit/asmjit.h \
    ../../../libs/asmjit/base.h \
    ../../../libs/asmjit/build.h \
    ../../../libs/asmjit/host.h \
    ../../../libs/asmjit/x86.h


INCLUDEPATH += ../../../include/ # VM_ROOT
INCLUDEPATH += ../../../libs # asmjit
INCLUDEPATH += ../../../vm

INCLUDEPATH += . # ROOT

QMAKE_CXXFLAGS += -Wno-unused-parameter
