/* 
 * File:   bytecodeInterpretator.h
 * Author: alex
 *
 * Created on January 2, 2014, 9:22 PM
 */

#ifndef BYTECODEINTERPRETATOR_H
#define	BYTECODEINTERPRETATOR_H

#include "mathvm.h"
#include "bytecodeCode.h"
#include <map>
#include <typeinfo>

namespace mathvm {

    using namespace std;

    class DataBytecode : public Bytecode {
    public:

        DataBytecode() {
        }

        DataBytecode(size_t size) {
            _data.resize(size);
        }

        int64_t popi() {
            return pop<int64_t>();
        }

        void pushi(int64_t v) {
            push<int64_t>(v);
        }

        int64_t getTopI() {
            return getTyped<int64_t>(length() - (uint32_t)sizeof (int64_t));
        }

        int64_t getTopI2() {
            return getTyped<int64_t>(length() -
                    2 * ((uint32_t)sizeof (int64_t)));
        }

        double popd() {
            return pop<double>();
        }

        void pushd(double v) {
            push<double>(v);
        }

        uint16_t popid() {
            return pop<uint16_t>();
        }

        void pushid(uint16_t v) {
            push<uint16_t>(v);
        }

        template<class T> T pop() {
            T value = getTyped<T>(length() - (uint32_t)sizeof (T));
            _data.resize(length() - (uint32_t)sizeof (T));
            return value;
        }

        template<class T> void push(T value) {
            addTyped(value);
        }

        inline void dropToSize(size_t to) {
            _data.resize(to);
        }

    };

    class FunctionContex {
        double* ddata;
        int64_t* idata;
        uint16_t* sdata;

    public:

        inline FunctionContex(const BytecodeFunction* fun)  {
            ddata = new double[fun->sizeDoubles];
            idata = new int64_t[fun->sizeInts];
            sdata = new uint16_t[fun->sizeStrings];
        }

        inline void setd(uint32_t id, double v) {
            ddata[id] = v;
        }

        inline void seti(uint32_t id, int64_t v) {
            idata[id] = v;
        }

        inline void sets(uint32_t id, uint16_t v) {
            sdata[id] = v;
        }

        inline double getd(uint32_t id) {
            return ddata[id];
        }

        inline int64_t geti(uint32_t id) {
            return idata[id];
        }

        inline uint16_t gets(uint32_t id) {
            return sdata[id];
        }
        
        inline ~FunctionContex(){
            delete[] ddata;
            delete[] idata;
            delete[] sdata;
        }

    };

    typedef map<uint16_t, FunctionContex*> OuterContexts;

    class BytecodeInterpretator {
        DataBytecode dstack;
        vector<const BytecodeFunction*> functions;
        vector<const string*> constants;

        void execFunction(const BytecodeFunction* fun, OuterContexts contexts);
        void setRootVars(const BytecodeCode& code, vector<Var*>& vars);

        Status* execStatus;

    public:
        Status* interpretate(const BytecodeCode& code, vector<Var*>& vars);
        ~BytecodeInterpretator();
        
        size_t callDepth;
        
    };
}


#endif	/* BYTECODEINTERPRETATOR_H */

