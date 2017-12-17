#pragma once

#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "visitors.h"
#include "asmjit/asmjit.h"

#include <map>
#include <vector>
#include <stack>

typedef union {
    int64_t intVal;
    double doubleVal;
    const char *stringVal; // actually it is its id
} BcValue;

typedef void (*NativeFunWrapper)(void *, void *);
typedef std::map<std::string, uint16_t> GlobalVarMap;
// surprisingly, map works MUCH faster than unordered_map
typedef std::map<uint16_t, BcValue> VarMap;
typedef std::map<uint16_t, VarMap> ScopeMap;

class BytecodeInterpreter : public mathvm::Code {
private:
    GlobalVarMap globalVarIds;

    BcValue reg0;
    BcValue reg1;
    BcValue reg2;
    BcValue reg3;

    uint32_t pc; // current pc
    mathvm::BytecodeFunction *fun; // current fun

    std::stack<BcValue> stack;
    std::stack<uint32_t> frameSizes;
/*
 * we need a stack of frames for each function
 * so that closures and recursion works fine
 */
    std::map<uint16_t, std::stack<ScopeMap *>> frames;
    std::stack<std::pair<mathvm::BytecodeFunction *, uint32_t>> returnAddress;
    std::stack<uint16_t> callStack;

// asmjit-related

    /*
     * native function wrapper just calls native function, passing it
     * arguments as defined but ABI.
     *
     * Wrapper arguments:
     * pointer to arguements list
     * pointer to where to save result
     */
    std::map<uint16_t, NativeFunWrapper> nativeWrappers;

    asmjit::JitRuntime jrt;
public:
    BytecodeInterpreter()
    {
    }

    void addVarId(const std::string &name, uint16_t id) // only for globals
    {
        globalVarIds[name] = id;
    }

    mathvm::Status* execute(std::vector<mathvm::Var*>&);

    virtual ~BytecodeInterpreter() {}
private:
    mathvm::Instruction nextInsn();

    void pushDouble(double d);
    void pushInt(int64_t i);
    void pushString(const char *str);

    double popDouble();
    int64_t popInt();
    const char * popString();

    BcValue getVar(uint16_t scopeId, uint16_t varId);
    void setVar(uint16_t scopeId, uint16_t varId, BcValue val);

    void createNativeWrapper(uint16_t nativeFunId);
    NativeFunWrapper getNativeWrapper(uint16_t nativeFunId);

    void execFun();

    void do_INVALID();
    void do_DLOAD();
    void do_ILOAD();
    void do_SLOAD();
    void do_DLOAD0();
    void do_ILOAD0();
    void do_SLOAD0();
    void do_DLOAD1();
    void do_ILOAD1();
    void do_DLOADM1();
    void do_ILOADM1();
    void do_DADD();
    void do_IADD();
    void do_DSUB();
    void do_ISUB();
    void do_DMUL();
    void do_IMUL();
    void do_DDIV();
    void do_IDIV();
    void do_IMOD();
    void do_DNEG();
    void do_INEG();
    void do_IAOR();
    void do_IAAND();
    void do_IAXOR();
    void do_IPRINT();
    void do_DPRINT();
    void do_SPRINT();
    void do_I2D();
    void do_D2I();
    void do_S2I();
    void do_SWAP();
    void do_POP();
    void do_LOADDVAR0();
    void do_LOADDVAR1();
    void do_LOADDVAR2();
    void do_LOADDVAR3();
    void do_LOADIVAR0();
    void do_LOADIVAR1();
    void do_LOADIVAR2();
    void do_LOADIVAR3();
    void do_LOADSVAR0();
    void do_LOADSVAR1();
    void do_LOADSVAR2();
    void do_LOADSVAR3();
    void do_STOREDVAR0();
    void do_STOREDVAR1();
    void do_STOREDVAR2();
    void do_STOREDVAR3();
    void do_STOREIVAR0();
    void do_STOREIVAR1();
    void do_STOREIVAR2();
    void do_STOREIVAR3();
    void do_STORESVAR0();
    void do_STORESVAR1();
    void do_STORESVAR2();
    void do_STORESVAR3();
    void do_LOADDVAR();
    void do_LOADIVAR();
    void do_LOADSVAR();
    void do_STOREDVAR();
    void do_STOREIVAR();
    void do_STORESVAR();
    void do_LOADCTXDVAR();
    void do_LOADCTXIVAR();
    void do_LOADCTXSVAR();
    void do_STORECTXDVAR();
    void do_STORECTXIVAR();
    void do_STORECTXSVAR();
    void do_DCMP();
    void do_ICMP();
    void do_JA();
    void do_IFICMPNE();
    void do_IFICMPE();
    void do_IFICMPG();
    void do_IFICMPGE();
    void do_IFICMPL();
    void do_IFICMPLE();
    void do_DUMP();
    void do_STOP();
    void do_CALL();
    void do_CALLNATIVE();
    void do_RETURN();
    void do_BREAK();
};
