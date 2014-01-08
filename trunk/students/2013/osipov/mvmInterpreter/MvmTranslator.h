/* 
 * File:   MvmTranslator.h
 * Author: stasstels
 *
 * Created on January 3, 2014, 4:56 PM
 */

#ifndef MVMTRANSLATOR_H
#define	MVMTRANSLATOR_H

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <tr1/memory>


#include "mathvm.h"
#include "ast.h"

namespace mathvm {
    using namespace std;

    struct Val {
        u_int16_t id;
        VarType type;
        string name;
        u_int16_t scopeId;
    };

    struct Fun {
        u_int16_t id;
        Signature sign;
        Bytecode* body;
        bool isNative;
    };

    struct VarMap {
        typedef map<string, vector<Val> >::iterator iterator;
        typedef map<string, vector<Val> >::const_iterator c_iterator;

        VarMap() : nextId(new uint16_t(0)) {
        }
                        
        map<string, vector<Val> > varMap;
        std::tr1::shared_ptr<uint16_t> nextId;
    };

    typedef map<string, Fun> FunMap;

    class MvmBytecode : public Code {
        Bytecode* bytecode;

    public:

        MvmBytecode() : bytecode(new Bytecode()) {
        }

        ~MvmBytecode() {
            delete bytecode;
        }

        virtual Status* execute(vector<Var*>& vars) {
            return 0;
        }

        virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0) {
            Code::disassemble(out, filter);
            bytecode -> dump(out);
        }

        Bytecode* getBytecode() {
            return bytecode;
        }

    };

    class MvmTranslator : public Translator {
    public:

        MvmTranslator();

        virtual ~MvmTranslator() {
        }

        virtual Status* translate(const string& program, Code**code);
    };






}

#endif	/* MVMTRANSLATOR_H */

