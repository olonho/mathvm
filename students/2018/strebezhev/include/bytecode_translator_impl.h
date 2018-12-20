#ifndef MAIN_BYTECODE_TRANSLATOR_IMPL_H
#define MAIN_BYTECODE_TRANSLATOR_IMPL_H

#include <queue>
#include <dlfcn.h>
#include <parser.h>
#include "interpreter_code_impl.h"


using namespace mathvm;
using namespace std;

struct BytecodeTranslatorImpl : public AstVisitor, Translator {
  unique_ptr <InterpreterCodeImpl> code;  // helpers to translate into bytecode
  Bytecode* bc = nullptr;

  stack <VarType> typestack;              // "lives" with bc stack of operated values
  bool preserveTop = true;                // if false, the top of type stack & bc stack will be popped
  void* processHandle;                    // used to load native functions

  queue<AstFunction*> funcsToTranslate;
  AstFunction* curFunc = nullptr;         // currently translated function

  map<Scope*, Scope*> nearestFuncScope;   // if { block.scope } -> function.scope that contains that if

  map<const AstVar*, uint16_t> _varToId;
  uint16_t lastVarId = 0;

  map<Scope*, uint16_t> _scopeToId;
  uint16_t lastScopeId = 0;

  BytecodeTranslatorImpl() :
    code(new InterpreterCodeImpl),
    processHandle(dlopen(nullptr, RTLD_LAZY)) {}


  // Functions

  void visitFunctionNode(FunctionNode* node) override {
    node->visitChildren(this);
  }

  void visitReturnNode(ReturnNode* node) override {
    node->visitChildren(this);
    auto returnTy = curFunc->returnType();

    if (returnTy != VT_VOID)
      castTo(returnTy);

    bc->addInsn(BC_RETURN);
    preserveTop = true;
  }

  void visitPrintNode(PrintNode* node) override {
    for (uint32_t i = 0; i < node->operands(); ++i) {
      node->operandAt(i)->visit(this);
      auto type = pop(typestack);
      bc->addInsn(type == VT_STRING ? BC_SPRINT : type == VT_DOUBLE ? BC_DPRINT : BC_IPRINT);
    }

    preserveTop = true;
  }


  // Literals

  void visitIntLiteralNode(IntLiteralNode* node) override {
    loadTypedLiteralAndDiscard(BC_ILOAD, VT_INT, node->literal());
  }

  void visitDoubleLiteralNode(DoubleLiteralNode* node) override {
    loadTypedLiteralAndDiscard(BC_DLOAD, VT_DOUBLE, node->literal());
  }

  void visitStringLiteralNode(StringLiteralNode* node) override {
    loadTypedLiteralAndDiscard(BC_SLOAD, VT_STRING, code->makeStringConstant(node->literal()));
  }


  // Arithmetic & logic operations

  void visitBinaryOpNode(BinaryOpNode* node) override {
    node->right()->visit(this);
    node->left()->visit(this);
    preserveTop = false;

    auto op = node->kind();

    if (op == tRANGE)
      return;

    if (op == tOR || op == tAND) {
      auto lTy = pop(typestack);
      if (lTy == VT_INT)
        castIntToBool();

      auto rTy = pop(typestack);
      if (rTy == VT_INT) {
        bc->addInsn(BC_SWAP);
        castIntToBool();
      }

      typestack.push(VT_BOOL);  // result type
      // todo: don't check the other arg, if left failed
      return bc->addInsn(op == tOR ? BC_IAOR : BC_IAAND);
    }

    if (tEQ <= op && op <= tLE)
      return compare(op);

    // arithmetic op
    if (castToSameType())
      bc->addInsn(BC_SWAP);

    bc->addInsn(isBoolOrInt(pop(typestack)) ? intMathInsn(op) : doubleMathInsn(op));
  }

  void visitUnaryOpNode(UnaryOpNode* node) override {
    node->visitChildren(this);
    node->kind() == tSUB ? applyMathNegation() : applyBooleanNot();
    preserveTop = false;
  }


  // Structural

  void visitIfNode(IfNode* node) override {
    node->ifExpr()->visit(this);
    typestack.pop();

    Label lEnd(bc), lElse(bc);
    bool hasElseBranch = node->elseBlock() != nullptr;
    jumpIfEqualsTo(BC_ILOAD0, hasElseBranch ? lElse : lEnd);

    node->thenBlock()->visit(this);

    if (hasElseBranch) {
      jumpTo(lEnd);
      bc->bind(lElse);
      node->elseBlock()->visit(this);
    }

    bc->bind(lEnd);
    preserveTop = true;
  }

  void visitWhileNode(WhileNode* node) override {
    Label lClause(bc), lEnd(bc);

    bc->bind(lClause);
    node->whileExpr()->visit(this);
    jumpIfEqualsTo(BC_ILOAD0, lEnd);

    node->loopBlock()->visit(this);
    jumpTo(lClause);
    bc->bind(lEnd);

    typestack.pop();
    preserveTop = true;
  }

  void visitForNode(ForNode* node) override {
    Label lClause(bc), lBody(bc), lEnd(bc);

    auto var = node->var();
    uint16_t varId = varIdOf(var);
    uint16_t scopeId = scopeIdOf(nearestFuncScope[var->owner()]);

    // for i in 3..5
    node->inExpr()->visit(this);                              // 3 5
    storeToIntVar(varId, scopeId);                            //   5

    bc->bind(lClause);
    bc->addInsns(BC_STOREIVAR0, BC_LOADIVAR0, BC_LOADIVAR0);  // 5 5
    loadFromIntVar(varId, scopeId);                           // 5 5 3
    bc->addBranch(BC_IFICMPG, lEnd);                          // 5

    bc->bind(lBody);
    node->body()->visit(this);
    loadFromIntVar(varId, scopeId);                           // 5 3
    bc->addInsns(BC_ILOAD1, BC_IADD);                         // 5 4
    storeToIntVar(varId, scopeId);                            // 5
    jumpTo(lClause);

    bc->bind(lEnd);
    bc->addInsn(BC_POP);
    preserveTop = true;
  }


  // Variables

  void visitLoadNode(LoadNode* node) override {
    loadVariable(node->var());
    typestack.push(node->var()->type());
    preserveTop = true;
  }

  void visitStoreNode(StoreNode* node) override {
    auto var = node->var();
    node->value()->visit(this);
    VarType type = castTo(var->type());

    uint16_t varId = varIdOf(var);
    uint16_t scopeId = scopeIdOf(nearestFuncScope[var->owner()]);

    // += <=> load, add, store
    if (node->op() != tASSIGN) {
      doWith(varId, scopeId, BC_LOADCTXDVAR);
      bc->addInsn((isBoolOrInt(type) ? node->op() == tINCRSET ? BC_IADD : BC_ISUB
                                     : node->op() == tINCRSET ? BC_DADD : BC_DSUB));
    }

    doWith(varId, scopeId, (type == VT_DOUBLE) ? BC_STORECTXDVAR :
                           (type == VT_STRING) ? BC_STORECTXSVAR : BC_STORECTXIVAR);

    preserveTop = true;
  }


  // Calls

  void visitCallNode(CallNode* node) override {
    TranslatedFunction* func = code->functionByName(node->name());

    for (auto idx = node->parametersNumber(); idx > 0; --idx) {
      node->parameterAt(idx - 1)->visit(this);
      castTo(func->parameterType(idx - 1));
    }

    bc->addInsn(BC_CALL);
    bc->addTyped(func->id());

    bool isVoid = func->returnType() == VT_VOID;
    if (!isVoid) typestack.push(func->returnType());
    preserveTop = isVoid; // then there's nothing to pop
  }

  void visitNativeCallNode(NativeCallNode* func) override {
    auto& signature = func->nativeSignature();
    auto runtimeAddr = dlsym(processHandle, func->nativeName().c_str());
    auto funcId = code->makeNativeFunction(func->nativeName(), signature, runtimeAddr);

    // idx=0 is return type
    for (size_t idx = 1; idx < signature.size(); ++idx) {
      auto& varName = signature[idx].second;
      loadVariable(curFunc->scope()->lookupVariable(varName));
    }

    bc->addInsn(BC_CALLNATIVE);
    bc->addTyped(funcId);

    // todo
    //bool isVoid = signature[0].first == VT_VOID;
    //if (!isVoid) typestack.push(signature[0].first);
    //preserveTop = isVoid; // then there's nothing to pop
  }


  // Blocks

  void visitBlockNode(BlockNode* node) override {
    auto scope = node->scope();
    nearestFuncScope[scope] = curFunc->scope();

    Scope::FunctionIterator funIt(scope);
    while (funIt.hasNext()) addFuncToQueue(funIt.next());

    for (uint32_t idx = 0; idx < node->nodes(); ++idx) {
      preserveTop = true;
      node->nodeAt(idx)->visit(this);
      if (preserveTop) continue;
      bc->addInsn(BC_POP);
      typestack.pop();
    }

    preserveTop = true;
  }


  // Translator interface

  Status* translate(const string& program, Code** pCode) override {
    Parser p;
    Status* result = p.parseProgram(program);

    if (!result->isOk())
      return result;

    this->translateProgram(p.top());
    *pCode = this->code.release();
    return result;
  }

  void translateProgram(AstFunction* top) {
    addFuncToQueue(top);

    do {
      curFunc = funcsToTranslate.front(), funcsToTranslate.pop();
      translateCurrentFunction();
    } while (!funcsToTranslate.empty());

    // global variables
    registerGlobalVars(top);

    this->code->scopeIdToVarsStack.resize(lastScopeId);
  }

  void addFuncToQueue(AstFunction* func) {
    funcsToTranslate.push(func);
    auto translated = new BytecodeFunction(func);
    translated->setScopeId(scopeIdOf(func->scope()));
    code->addFunction(translated);
  }

  void translateCurrentFunction() {
    Scope* scope = curFunc->scope();
    nearestFuncScope[scope] = scope;

    // associate visitor with function's bytecode storage
    bc = ((BytecodeFunction*) code->functionByName(curFunc->name()))->bytecode();

    for (uint32_t i = 0; i < curFunc->parametersNumber(); ++i) {
      auto var = scope->lookupVariable(curFunc->parameterName(i));
      doWith(varIdOf(var), scopeIdOf(scope), storeCtxInsn(curFunc->parameterType(i)));
    }

    typestack.push(curFunc->returnType());
    curFunc->node()->visit(this);
  }

  void registerGlobalVars(const AstFunction* top) {
    Scope::VarIterator varIt(top->node()->body()->scope());

    while (varIt.hasNext()) {
      auto var = varIt.next();
      code->varIdByName[var->name()] = varIdOf(var);
    }
  }


private:

  uint16_t varIdOf(const AstVar* var) {
    auto it = _varToId.find(var);
    return it == _varToId.end() ? (_varToId[var] = lastVarId++) : it->second;
  }

  uint16_t scopeIdOf(Scope* scope) {
    auto it = _scopeToId.find(scope);
    return it == _scopeToId.end() ? (_scopeToId[scope] = lastScopeId++) : it->second;
  }

  inline static bool isBoolOrInt(int type) {
    return type == VT_BOOL || type == VT_INT;
  }

  /**
   * Do comparison of two args on TOP.
   * The result type is VT_BOOL.
   * @param opKind one of EQ, LE, GT, ...
   */
  void compare(const TokenKind& opKind) {
    auto lTy = pop(typestack);
    auto rTy = typestack.top();
    typestack.push(lTy);

    bool argsAreSwapped = false;
    if (!(isBoolOrInt(lTy) && isBoolOrInt(rTy))) {
      // both are double or one is int-bool
      argsAreSwapped = castToSameType();
      bc->addInsns(isBoolOrInt(pop(typestack)) ? BC_ICMP : BC_DCMP, BC_ILOAD0);
      argsAreSwapped = !argsAreSwapped;
    }

    Label lEnd(bc), lTrue(bc);
    bc->addBranch(intCompareInsn(opKind, argsAreSwapped), lTrue);

    // lFalse
    bc->addInsn(BC_ILOAD0);
    jumpTo(lEnd);

    bc->bind(lTrue);
    bc->addInsn(BC_ILOAD1);

    bc->bind(lEnd);
    typestack.top() = VT_BOOL;
  }

  inline void applyMathNegation() const {
    bc->addInsn(typestack.top() == VT_DOUBLE ? BC_DNEG : BC_INEG);
  }

  void applyBooleanNot() {
    if (typestack.top() == VT_BOOL)
      return bc->addInsns(BC_ILOAD1, BC_IAXOR);

    // should fix plot.mvm
    if (typestack.top() == VT_STRING)
      bc->addInsn(BC_S2I);

    ifEqualsToThenPut(BC_ILOAD0, BC_ILOAD1, BC_ILOAD0);
    typestack.top() = VT_BOOL;
  }

  /**
   * Cast the remaining arg on the TOP to same type.
   * If one of arguments is double, cast to double.
   * @return true, if args were swapped while casting
   */
  bool castToSameType() {
    auto lTy = pop(typestack);
    auto rTy = pop(typestack);
    bool argsSwapped = false;

    if (lTy != rTy) {
      if (isBoolOrInt(rTy)) {
        bc->addInsn(BC_SWAP);
        argsSwapped = true;
      }

      bc->addInsn(BC_I2D);
      lTy = VT_DOUBLE;
    }

    typestack.push(lTy);
    typestack.push(lTy);
    return argsSwapped;
  }

  /**
   * Cast TOP to VT_BOOL (0 or 1).
   * Modifies typestack
   */
  inline void castIntToBool() {
    ifEqualsToThenPut(BC_ILOAD0, BC_ILOAD0, BC_ILOAD1);
  }

  /**
   * Cast TOP to type. Pops typestack
   * @param type VT_INT or VT_DOUBLE
   * @return casted type
   */
  VarType castTo(VarType type) {
    auto prev = pop(typestack);
    if (prev != type) bc->addInsn(isBoolOrInt(prev) ? BC_I2D : BC_D2I);
    return type;
  }

  /**
   * Used to add instructions to load a literal.
   * @param insn one of BC_?LOAD
   * @param type type to put on typestack
   * @param value literal's value or id (in case of strings)
   */
  template<class T>
  inline void loadTypedLiteralAndDiscard(Instruction insn, VarType type, T value) {
    bc->addInsn(insn);
    bc->addTyped(value);
    typestack.push(type);
    preserveTop = false;  // { 5; } means nothing
  }

  inline void loadVariable(const AstVar* var) {
    VarType type = var->type();
    Instruction load = type == VT_DOUBLE ? BC_LOADCTXDVAR : type == VT_STRING ? BC_LOADCTXSVAR : BC_LOADCTXIVAR;
    doWith(varIdOf(var), scopeIdOf(nearestFuncScope[var->owner()]), load);
  }

  inline void loadFromIntVar(uint16_t varId, uint16_t inScopeId) {
    doWith(varId, inScopeId, BC_LOADCTXIVAR);
  }

  inline void storeToIntVar(uint16_t varId, uint16_t inScopeId) {
    doWith(varId, inScopeId, BC_STORECTXIVAR);
  }

  inline void doWith(uint16_t varId, uint16_t inScopeId, Instruction operation) {
    bc->addInsn(operation);
    bc->addTyped(inScopeId);
    bc->addTyped(varId);
  }

  inline void jumpIfEqualsTo(Instruction iloadInsn, Label& label) const {
    bc->addInsn(iloadInsn);
    bc->addBranch(BC_IFICMPE, label);
  }

  inline void jumpTo(Label& lEnd) const {
    bc->addBranch(BC_JA, lEnd);
  }

  // (TOP == value) ? thenVal : elseVal
  inline void ifEqualsToThenPut(Instruction value, Instruction thenVal, Instruction elseVal) {
    Label lEnd(bc), lThen(bc);
    jumpIfEqualsTo(value, lThen);

    bc->addInsn(elseVal);
    jumpTo(lEnd);

    bc->bind(lThen);
    bc->addInsn(thenVal);
    bc->bind(lEnd);
  }

  /**
   * @param argsAreSwapped swap could happen while casting arguments to same type.
   *                       if true, reversed operation will be used (LT -> GE)
   * @return bytecode instruction converted from AST binary operation
   */
  inline static Instruction intCompareInsn(const TokenKind& op, bool argsAreSwapped) {
    switch (op) {
      case tEQ:
        return BC_IFICMPE;
      case tNEQ:
        return BC_IFICMPNE;
      case tLT:
        return argsAreSwapped ? BC_IFICMPGE : BC_IFICMPL;
      case tLE:
        return argsAreSwapped ? BC_IFICMPG : BC_IFICMPLE;
      case tGT:
        return argsAreSwapped ? BC_IFICMPLE : BC_IFICMPG;
      case tGE:
        return argsAreSwapped ? BC_IFICMPL : BC_IFICMPGE;
      default:
        assert(false);
        return BC_INVALID;
    }
  }

  inline static Instruction intMathInsn(const TokenKind& op) {
    switch (op) {
      case tADD:
        return BC_IADD;
      case tSUB:
        return BC_ISUB;
      case tMUL:
        return BC_IMUL;
      case tDIV:
        return BC_IDIV;
      case tMOD:
        return BC_IMOD;
      case tAAND:
        return BC_IAAND;
      case tAOR:
        return BC_IAOR;
      case tAXOR:
        return BC_IAXOR;
      default:
        assert(false);
        return BC_INVALID;
    }
  }

  inline static Instruction doubleMathInsn(const TokenKind& op) {
    switch (op) {
      case tADD:
        return BC_DADD;
      case tSUB:
        return BC_DSUB;
      case tMUL:
        return BC_DMUL;
      case tDIV:
        return BC_DDIV;
      default:
        assert(false);
        return BC_INVALID;
    }
  }

  inline static Instruction storeCtxInsn(VarType type) {
    return type == VT_DOUBLE ? BC_STORECTXDVAR : type == VT_STRING ? BC_STORECTXSVAR : BC_STORECTXIVAR;
  }

  template<typename T>
  static T pop(std::stack<T>& container) {
    assert(!container.empty());
    T res = container.top();
    container.pop();
    return res;
  }
};

#endif //MAIN_BYTECODE_TRANSLATOR_IMPL_H
