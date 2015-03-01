#pragma once

#include <iostream>
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

        MvmRuntime() {
            _fConstPool.reserve(4096);
        }

        static const int64_t FNEG_MASK;

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
            if (_program) jitRuntime.release((void *) _program);
            if (_starter) jitRuntime.release((void *) _starter);
        }

        double *vivify(double c) {
            for (double &d : _fConstPool) if (d == c) return &d;
            _fConstPool.push_back(c);
            return &_fConstPool.back();
        }

        std::vector<double> const &floatConstants() const {
            return _fConstPool;
        }

    private :

        std::vector<double> _fConstPool;

        void pushRegs(X86Assembler &_);

        void popRegs(X86Assembler &_);

        Program _starter = NULL;
        Program _program = NULL;

    };

    inline std::ostream &operator<<(std::ostream &str, MvmRuntime const &rt) {
        str << " -- Runtime -- \n";
        size_t i = 0;
        str << "  strings:  \n";
        for (auto &s : rt.stringPool)
            str << i++ << " : " << (void*)s.c_str() <<  " : '" << s.c_str() << "'\n";

        i = 0;
        str << "  floats:  \n";
        for (auto &d : rt.floatConstants())
            str << i++ << " : " << &d << " : " << d << std::endl;


        str << "\n\n";

        return str;
    }


}