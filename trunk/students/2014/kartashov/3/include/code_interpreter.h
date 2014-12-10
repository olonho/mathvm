#ifndef CODE_INTERPRETER_H__
#define CODE_INTERPRETER_H__

#include "mathvm.h"
#include "execution_exception.h"

#include <vector>
#include <stack>
#include <map>
#include "data_holder.h"
#include "context.h"

using namespace mathvm;

class CodeInterpreter {
  public:
    typedef std::vector<DataHolder> DataStack;
    typedef std::pair<DataHolder, DataHolder> HolderPair;
    typedef std::vector<int32_t> ProgramCounterStack;
    typedef std::vector<TranslatedFunction*> FunctionStack;
    typedef std::vector<Context> ContextStack;

    const size_t preallocatedMemorySize = 1024 * 1024;

    CodeInterpreter(Code* code): mCode(code),
    mDefaultStringId(mCode->makeStringConstant("")) {
      mContextStack.reserve(preallocatedMemorySize);
      mFunctionStack.reserve(preallocatedMemorySize);
      mProgramCounterStack.reserve(preallocatedMemorySize);
      auto topFunction = mCode->functionByName(topFunctionName);
      mFunctionStack.push_back(topFunction);
      mContextStack.push_back(Context());
      newCounter();
    }

    void execute() {
      // mCode->disassemble(std::cout);
      while(!mFunctionStack.empty()) {
        executeInstruction();
      }
    }

    void executeInstruction() {
      Instruction instruction = currentBytecode()->getInsn(programCounter());
      next();

      // std::cout << bytecodeName(instruction) << std::endl;

      switch (instruction) {
        case BC_INVALID: throw ExecutionException("Invalid instruction"); break;
        case BC_DLOAD: dload(); break;
        case BC_ILOAD: iload(); break;
        case BC_SLOAD: sload(); break;
        case BC_DLOAD0: dload0(); break;
        case BC_ILOAD0: iload0(); break;
        case BC_SLOAD0: sload0(); break;
        case BC_ILOAD1: iload1(); break;
        case BC_DLOAD1: dload1(); break;
        case BC_ILOADM1: iloadm1(); break;
        case BC_DLOADM1: dloadm1(); break;
        case BC_IADD: iadd(); break;
        case BC_DADD: dadd(); break;
        case BC_ISUB: isub(); break;
        case BC_DSUB: dsub(); break;
        case BC_IMUL: imul(); break;
        case BC_DMUL: dmul(); break;
        case BC_IDIV: idiv(); break;
        case BC_DDIV: ddiv(); break;
        case BC_IMOD: imod(); break;
        case BC_INEG: ineg(); break;
        case BC_DNEG: dneg(); break;
        case BC_IAOR: iaor(); break;
        case BC_IAAND: iaand(); break;
        case BC_IAXOR: iaxor(); break;
        case BC_IPRINT: iprint(); break;
        case BC_DPRINT: dprint(); break;
        case BC_SPRINT: sprint(); break;
        case BC_I2D: i2d(); break;
        case BC_D2I: d2i(); break;
        case BC_SWAP: swap(); break;
        case BC_POP: pop(); break;
/*        DO(LOADDVAR0, "Load double from variable 0, push on TOS.", 1)   \
        DO(LOADDVAR1, "Load double from variable 1, push on TOS.", 1)   \
        DO(LOADDVAR2, "Load double from variable 2, push on TOS.", 1)   \
        DO(LOADDVAR3, "Load double from variable 3, push on TOS.", 1)   \
        DO(LOADIVAR0, "Load int from variable 0, push on TOS.", 1)      \
        DO(LOADIVAR1, "Load int from variable 1, push on TOS.", 1)      \
        DO(LOADIVAR2, "Load int from variable 2, push on TOS.", 1)      \
        DO(LOADIVAR3, "Load int from variable 3, push on TOS.", 1)      \
        DO(LOADSVAR0, "Load string from variable 0, push on TOS.", 1)   \
        DO(LOADSVAR1, "Load string from variable 1, push on TOS.", 1)   \
        DO(LOADSVAR2, "Load string from variable 2, push on TOS.", 1)   \
        DO(LOADSVAR3, "Load string from variable 3, push on TOS.", 1)   \
        DO(STOREDVAR0, "Pop TOS and store to double variable 0.", 1)    \
        DO(STOREDVAR1, "Pop TOS and store to double variable 1.", 1)    \
        DO(STOREDVAR2, "Pop TOS and store to double variable 0.", 1)    \
        DO(STOREDVAR3, "Pop TOS and store to double variable 3.", 1)    \
        DO(STOREIVAR0, "Pop TOS and store to int variable 0.", 1)       \
        DO(STOREIVAR1, "Pop TOS and store to int variable 1.", 1)       \
        DO(STOREIVAR2, "Pop TOS and store to int variable 0.", 1)       \
        DO(STOREIVAR3, "Pop TOS and store to int variable 3.", 1)       \
        DO(STORESVAR0, "Pop TOS and store to string variable 0.", 1)    \
        DO(STORESVAR1, "Pop TOS and store to string variable 1.", 1)    \
        DO(STORESVAR2, "Pop TOS and store to string variable 0.", 1)    \
        DO(STORESVAR3, "Pop TOS and store to string variable 3.", 1)    \*/
        case BC_LOADDVAR: loaddvar(); break;
        case BC_LOADIVAR: loadivar(); break;
        case BC_LOADSVAR: loadsvar(); break;
        case BC_STOREDVAR: storedvar(); break;
        case BC_STOREIVAR: storeivar(); break;
        case BC_STORESVAR: storesvar(); break;
        case BC_LOADCTXDVAR: loadctxdvar(); break;
        case BC_LOADCTXIVAR: loadctxivar(); break;
        case BC_LOADCTXSVAR: loadctxsvar(); break;
        case BC_STORECTXDVAR: storectxdvar(); break;
        case BC_STORECTXIVAR: storectxivar(); break;
        case BC_STORECTXSVAR: storectxsvar(); break;
        case BC_DCMP: dcmp(); break;
        case BC_ICMP: icmp(); break;
        case BC_JA: ja(); break;
        case BC_IFICMPNE:  ificmpne(); break;
        case BC_IFICMPE: ificmpe(); break;
        case BC_IFICMPG: ificmpg(); break;
        case BC_IFICMPGE: ificmpge(); break;
        case BC_IFICMPL: ificmpl(); break;
        case BC_IFICMPLE: ificmple(); break;
        case BC_DUMP: dump(); break;
        case BC_STOP: throw ExecutionException("Execution stopped");
        case BC_CALL: call(); break;
        case BC_CALLNATIVE: throw ExecutionException("Natives are not supported"); break;
        case BC_RETURN: bc_return(); break;
        case BC_BREAK: next(); break;
        default: break;
      }
    }

    void call() {
      int16_t id = int16();
      mFunctionStack.push_back(mCode->functionById(id));
      // currentFunction()->disassemble(std::cout);
      newCounter();
      newContext();
    }

    void ja() {
      int16_t relativeOffset = currentBytecode()->getInt16(programCounter());
      addRelativeOffset(relativeOffset);
    }

    void ificmpne() {
      auto args = topPair();
      if (args.first.intValue != args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void ificmpe() {
      auto args = topPair();
      if (args.first.intValue == args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void ificmpg() {
      auto args = topPair();
      push(args.second);
      if (args.first.intValue > args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void ificmpge() {
      auto args = topPair();
      push(args.second);
      if (args.first.intValue >= args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void ificmpl() {
      auto args = topPair();
      push(args.second);
      if (args.first.intValue < args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void ificmple() {
      auto args = topPair();
      push(args.second);
      if (args.first.intValue <= args.second.intValue) {
        ja();
      } else {
        int16();
      }
    }

    void loaddvar() {
      loaddvar(currentContext());
    }

    void loadivar() {
      loadivar(currentContext());
    }

    void loadsvar() {
      loadsvar(currentContext());
    }

    void storeivar() {
      storeivar(currentContext());
    }

    void storedvar() {
      storedvar(currentContext());
    }

    void storesvar() {
      storesvar(currentContext());
    }

    void loadctxivar() {
      int16_t id = int16();
      loadivar(contextWithId(id));
    }

    void loadctxdvar() {
      int16_t id = int16();
      loaddvar(contextWithId(id));
    }

    void loadctxsvar() {
      int16_t id = int16();
      loadsvar(contextWithId(id));
    }

    void storectxivar() {
      int16_t id = int16();
      storeivar(contextWithId(id));
    }

    void storectxdvar() {
      int16_t id = int16();
      storedvar(contextWithId(id));
    }

    void storectxsvar() {
      int16_t id = int16();
      storesvar(contextWithId(id));
    }

    void loaddvar(Context& context) {
      int16_t id = int16();
      auto result = context.load(id, DataHolder(0.0)).doubleValue;
      dpush(result);
    }

    void loadivar(Context& context) {
      int16_t id = int16();
      auto result = context.load(id, DataHolder((int64_t) 0)).intValue;
      ipush(result);
    }

    void loadsvar(Context& context) {
      int16_t id = int16();
      auto result = context.load(id, DataHolder((int16_t) mDefaultStringId)).stringId;
      spush(result);
    }

    void storeivar(Context& context) {
      auto args = tos();
      int16_t id = int16();
      context.store(id, args);
    }

    void storedvar(Context& context) {
      auto args = tos();
      int16_t id = int16();
      context.store(id, args);
    }

    void storesvar(Context& context) {
      auto args = tos();
      int16_t id = int16();
      context.store(id, args);
    }

    void dload() {
      dpush(currentBytecode()->getDouble(programCounter()));
      next8();
    }

    void iload() {
      ipush(currentBytecode()->getInt64(programCounter()));
      next8();
    }

    void sload() {
      int16_t id = int16();
      spush(id);
    }

    void dload0() {
      dpush((double) 0.0);
    }

    void iload0() {
      ipush((int64_t) 0);
    }

    void sload0() {
      int16_t id = mCode->makeStringConstant("");
      spush(id);
    }

    void iload1() {
      ipush((int64_t) 1);
    }

    void dload1() {
      dpush((double) 1.0);
    }

    void iloadm1() {
      ipush((int64_t) -1);
    }

    void dloadm1() {
      dpush((double) -1.0);
    }

    void iadd() {
      auto args = topPair();
      ipush(args.first.intValue + args.second.intValue);
    }

    void dadd() {
      auto args = topPair();
      dpush(args.first.doubleValue + args.second.doubleValue);
    }

    void isub() {
      auto args = topPair();
      ipush(args.first.intValue - args.second.intValue);
    }

    void dsub() {
      auto args = topPair();
      dpush(args.first.doubleValue - args.second.doubleValue);
    }

    void imul() {
      auto args = topPair();
      ipush(args.first.intValue * args.second.intValue);
    }

    void dmul() {
      auto args = topPair();
      dpush(args.first.doubleValue * args.second.doubleValue);
    }

    void idiv() {
      auto args = topPair();
      ipush(args.first.intValue / args.second.intValue);
    }

    void ddiv() {
      auto args = topPair();
      dpush(args.first.doubleValue / args.second.doubleValue);
    }

    void imod() {
      auto args = topPair();
      ipush(args.first.intValue % args.second.intValue);
    }

    void ineg() {
      auto args = tos();
      ipush((int64_t)-args.intValue);
    }

    void dneg() {
      auto args = tos();
      dpush(-args.doubleValue);
    }

    void iaor() {
      auto args = topPair();
      ipush(args.first.intValue | args.second.intValue);
    }

    void iaand() {
      auto args = topPair();
      ipush(args.first.intValue & args.second.intValue);
    }

    void iaxor() {
      auto args = topPair();
      ipush(args.first.intValue ^ args.second.intValue);
    }

    void iprint() {
      auto args = tos();
      printf("%lld", args.intValue);
    }

    void dprint() {
      auto args = tos();
      printf("%g", args.doubleValue);
    }

    void sprint() {
      auto args = tos();
      printf("%s", mCode->constantById(args.stringId).c_str());
    }

    void i2d() {
      auto args = tos();
      dpush((double) args.intValue);
    }

    void d2i() {
      auto args = tos();
      ipush((double) args.doubleValue);
    }

    void bc_return() {
      mFunctionStack.pop_back();
      mProgramCounterStack.pop_back();
      popContext();
    }

    void swap() {
      std::iter_swap(mDataStack.rbegin(), mDataStack.rbegin() + 1);
    }

    void icmp() {
      auto args = topPair();
      if (args.first.intValue == args.second.intValue) {
        ipush((int64_t) 0);
      } else if (args.first.intValue < args.second.intValue) {
        ipush((int64_t) -1);
      } else {
        ipush((int64_t) 1);
      }
    }

    void dcmp() {
      auto args = topPair();
      if (args.first.doubleValue == args.second.doubleValue) {
        ipush((int64_t) 0);
      } else if (args.first.doubleValue < args.second.doubleValue) {
        ipush((int64_t) -1);
      } else {
        ipush((int64_t) 1);
      }
    }

    void dump() {
      auto args = top();
      printf("%d\n", args.stringId);
      printf("%lld\n", args.intValue);
      printf("%f\n", args.doubleValue);
    }

    int32_t programCounter() {return mProgramCounterStack.back();}

    void push(DataHolder holder) {
      mDataStack.push_back(holder);
    }

    void spush(int16_t value) {
      mDataStack.push_back(DataHolder(value));
    }

    void ipush(int64_t value) {
      mDataStack.push_back(DataHolder(value));
    }

    void dpush(double value) {
      mDataStack.push_back(DataHolder(value));
    }

    void pop() {
      mDataStack.pop_back();
    }

    DataHolder top() {return mDataStack.back();}

    HolderPair topPair() {
      HolderPair result;
      result.first = tos();
      result.second = tos();
      return result;
    }

    DataHolder tos() {
      auto result = top();
      pop();
      return result;
    }

    Bytecode* currentBytecode() {
      return static_cast<BytecodeFunction*>(currentFunction())->bytecode();
    }

    TranslatedFunction* currentFunction() {return mFunctionStack.back();}

    void newContext() {
      mContextStack.push_back(currentContext().newContext());
    }

    void popContext() {
      mContextStack.pop_back();
    }

    Context& currentContext() {
      return mContextStack.back();
    }

    void addRelativeOffset(int16_t relativeOffset) {
      mProgramCounterStack.back() += relativeOffset;
    }

    int16_t int16() {
      int16_t result = currentBytecode()->getInt16(programCounter());
      next2();
      return result;
    }

    void next(size_t i = 1) {mProgramCounterStack.back() += i;
    }

    void next2() {next(2);}

    void next3() {next(3);}

    void next4() {next(4);}

    void next8() {next(8);}

    void newCounter() {mProgramCounterStack.push_back(0);}

    Context& contextWithId(int16_t id) {
      return mContextStack[id];
    }

    const std::string topFunctionName = "<top>";

  private:
    ProgramCounterStack mProgramCounterStack;
    FunctionStack mFunctionStack;
    Code* mCode;
    DataStack mDataStack;
    ContextStack mContextStack;
    const int16_t mDefaultStringId;
};

#endif
