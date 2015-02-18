#pragma once

#include <vector>
#include <string>

#include "../../../../../libs/asmjit/asmjit.h"
#include "machcode_generator.h"

namespace mathvm {

    using namespace asmjit;
    using namespace asmjit::x86;

    typedef void (*Program)(void);

    struct MvmRuntime {

        JitRuntime jitRuntime;

        std::vector<std::string> stringPool;

        __attribute_noinline__
        static void print_int(int64_t i) {
            printf("%ld", i);
        }

        __attribute_noinline__
        static void print_double(double d) {
            printf("%lf", d);
        }

        __attribute_noinline__
        static void print_str(char const *s) {
            printf("%s", s);
        }


        MvmRuntime() {}

        Program getStarter(Program program) {
            if (!_starter) {
                X86Assembler entry(&jitRuntime);
                pushRegs(entry);
                entry.call(Ptr(program));
                popRegs(entry);
                entry.ret();
                _starter = (Program) entry.make();
                _program = program;
            }
            return (void (*)()) _starter;
        }

        ~MvmRuntime() {
            if (_program ) jitRuntime.release((void*)_program);
            if (_starter) jitRuntime.release((void*)_starter);
        }

    private :

        void pushRegs(X86Assembler &_);

        void popRegs(X86Assembler &_);

        Program _starter = NULL;
        Program _program = NULL;

    };
}