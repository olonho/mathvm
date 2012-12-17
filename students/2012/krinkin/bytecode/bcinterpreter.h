#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <stack>

#include "bccode.h"

#define MAX_FRAME_SIZE 1024

using namespace mathvm;

class BCInterpreter
{
public:
    void run(BCCode *code)
    {
        m_code = code;
	    m_local_memory.resize(m_code->function_count());
	    m_int_vars.resize(4, (int64_t)0);
	    m_double_vars.resize(4, 0.0);
	    m_string_vars.resize(4, (char *)0);
	    make_call((BytecodeFunction *)m_code->functionByName(AstFunction::top_name));
    }
    
    BCInterpreter() {}
    virtual ~BCInterpreter() {}

private:
    typedef void (*void_call)(char *frame);
    typedef int64_t (*int64_t_call)(char *frame);
    typedef double (*double_call)(char *frame);
    typedef char const *(*string_call)(char *frame);
    typedef union _value
    {
		int64_t _int;
		double _double;
		char const *_string;
		
		_value(int64_t val) : _int(val) {}
		_value(double val) : _double(val) {}
		_value(char const *val) : _string(val) {}
		
		operator int64_t() { return _int; }
		operator double() { return _double; }
		operator char const *() { return _string; }
    } value;

    std::vector<std::stack<char *> > m_local_memory;
    std::stack<value> m_eval_stack;

	std::vector<int64_t> m_int_vars;
	std::vector<double> m_double_vars;
	std::vector<char const *> m_string_vars;
	
	BCCode *m_code;
	
	char m_frame[MAX_FRAME_SIZE];
	std::string m_empty;

    void call(BytecodeFunction *function);

    void make_call(BytecodeFunction *function)
    {
        size_t sp = m_eval_stack.size();
        push_scope(function);
        call(function);
        pop_scope(function);
        while (sp < m_eval_stack.size()) m_eval_stack.pop();
    }
    
    bool no_arg(Instruction insn) const
    {
        switch (insn)
        {
		case BC_DLOAD: case BC_ILOAD: case BC_SLOAD:
		case BC_DLOAD0: case BC_ILOAD0: case BC_SLOAD0:
		case BC_DLOAD1: case BC_ILOAD1: case BC_DLOADM1: case BC_ILOADM1:
		case BC_LOADDVAR0: case BC_LOADDVAR1: case BC_LOADDVAR2: case BC_LOADDVAR3:
		case BC_LOADIVAR0: case BC_LOADIVAR1: case BC_LOADIVAR2: case BC_LOADIVAR3:
		case BC_LOADSVAR0: case BC_LOADSVAR1: case BC_LOADSVAR2: case BC_LOADSVAR3:
		case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
		case BC_LOADCTXDVAR: case BC_LOADCTXIVAR: case BC_LOADCTXSVAR:
		case BC_CALL: case BC_CALLNATIVE: case BC_RETURN:
		    return true;
        default:
            return jump(insn);
		}
    }
    
    bool jump(Instruction insn) const
    {
	    switch (insn)
	    {
		    case BC_IFICMPNE: case BC_IFICMPE:
		    case BC_IFICMPG: case BC_IFICMPGE:
		    case BC_IFICMPL: case BC_IFICMPLE:
		    case BC_JA:	
			    return true;
		    default:
			    return false;
	    }
    }
    
    bool one_arg(Instruction insn) const
    {
	    switch (insn)
	    {
	    case BC_DNEG: case BC_INEG:
	    case BC_IPRINT: case BC_DPRINT: case BC_SPRINT:
	    case BC_I2D: case BC_D2I: case BC_S2I:
	    case BC_POP:
	    case BC_STOREDVAR0: case BC_STOREDVAR1: case BC_STOREDVAR2: case BC_STOREDVAR3:
	    case BC_STOREIVAR0: case BC_STOREIVAR1: case BC_STOREIVAR2: case BC_STOREIVAR3:
	    case BC_STORESVAR0: case BC_STORESVAR1: case BC_STORESVAR2: case BC_STORESVAR3:
	    case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:
	    case BC_STORECTXDVAR: case BC_STORECTXIVAR: case BC_STORECTXSVAR:
		    return true;
	    default:
		    return false;
	    }
    }
    
    bool two_arg(Instruction insn) const
    {
	    switch (insn)
	    {
	    case BC_DADD: case BC_IADD:
	    case BC_DSUB: case BC_ISUB:
	    case BC_DMUL: case BC_IMUL:
	    case BC_DDIV: case BC_IDIV:
	    case BC_IMOD:
	    case BC_SWAP:
	    case BC_DCMP: case BC_ICMP:
		    return true;
	    default:
		    return false;
	    }
    }
    
    size_t insn_len(Instruction insn) const
    {
	    static const struct
	    {
		    const char* name;
		    Instruction insn;
		    size_t length;
	    } names[] = {
		    #define BC_NAME(b, d, l) {#b, BC_##b, l},
		    FOR_BYTECODES(BC_NAME)
		    #undef BC_NAME
	    };
	    if (insn >= BC_INVALID && insn < BC_LAST) return names[insn].length;
	    assert(0);
	    return 0;
    }
    
    void push_scope(BytecodeFunction const * const function)
    {
        char *local = new char[function->localsNumber()];
        m_local_memory[function->id()].push(local);
    }
    
    void pop_scope(BytecodeFunction const * const function)
    {
        char *local = m_local_memory[function->id()].top();
        m_local_memory[function->id()].pop();
        delete [] local;
    }
	
	value pop()
	{
	    value v = m_eval_stack.top();
	    m_eval_stack.pop();
	    return v;
	}
	
	void push(value v) { m_eval_stack.push(v); }
	
	int64_t pop_int() { return (int64_t) pop(); }
    void push_int(int64_t val) { push(value(val)); }

	double pop_double() { return (double) pop(); }
    void push_double(double val) { push(value(val)); }

	char const *pop_string() { return (char const *) pop(); }
    void push_string(char const *val) { push(value(val)); }

    int64_t icmp(int64_t upper, int64_t lower) const { return upper - lower; }
    int64_t dcmp(double upper, double lower) const
    {
        if (upper < lower) return (int64_t)-1;
        else if (upper == lower) return (int64_t)0;
        else return (int64_t)1;
    }

    int64_t load_int(uint16_t scope, uint16_t local) const
    {
        return *((int64_t *)(m_local_memory[scope].top() + local));
    }
    
    double load_double(uint16_t scope, uint16_t local) const
    {
        return *((double *)(m_local_memory[scope].top() + local));
    }
    
    char const *load_string(uint16_t scope, uint16_t local) const
    {
        return *((char const **)(m_local_memory[scope].top() + local));
    }

    void store_int(uint16_t scope, uint16_t local, int64_t value)
    {
        *((int64_t *)(m_local_memory[scope].top() + local)) = value;
    }
    
    void store_double(uint16_t scope, uint16_t local, double value)
    {
        *((double *)(m_local_memory[scope].top() + local)) = value;
    }
    
    void store_string(uint16_t scope, uint16_t local, char const *value)
    {
        *((char const **)(m_local_memory[scope].top() + local)) = value;
    }
    
    void print_int(int64_t value) const { std::cout << value; }
    void print_double(double value) const { std::cout << value; }
    void print_string(char const *value) const { std::cout << value; }
    
    void fill_frame(Signature const &signature);
    VarType return_type(Signature const &signature) { return signature[0].first; }
};

#endif /* __INTERPRETER_H__ */
