#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <AsmJit/Compiler.h>
#include <AsmJit/Logger.h>
#include <AsmJit/MemoryManager.h>

#include <vector>
#include <stack>
#include <string>

#include "bccode.h"

using namespace mathvm;
using namespace AsmJit;

#define MAX_FRAME_SIZE 1024

struct var_t
{
    bool kind;
    bool locked;
    
    GPVar gp;
    XMMVar xmm;
    
    var_t(GPVar const &gp_) : kind(true), locked(false), gp(gp_) {}
    var_t(XMMVar const &xmm_) : kind(false), locked(false), xmm(xmm_) {}
};

typedef void (*void_call)(char *frame);
typedef int64_t (*int64_t_call)(char *frame);
typedef double (*double_call)(char *frame);
typedef char const *(*string_call)(char *frame);

class BCCompiler
{
public:
    BCCompiler(BCCode *code);
    ~BCCompiler();
    
    void execute();
    
    void generate(BytecodeFunction *f);

    void push_locals(size_t id, size_t size);
    void pop_locals(size_t id);
    void * locals(size_t id);
    
    void fill_frame(Signature const *signature, Compiler *cc, std::stack<var_t> *vars);

    BCCode *m_code;
    
    std::vector<std::stack<char *> > m_locals;
    std::vector<void (*)()> m_fptrs;
    
    double m_dvars[4];
    int64_t m_ivars[4];
    char const * m_svars[4];
    
    std::string m_empty;

    char m_frame[MAX_FRAME_SIZE];
};

#endif /* __COMPILER_H__ */
