#ifndef AST_TO_BYTECODE_VISITOR_H__
#define AST_TO_BYTECODE_VISITOR_H__

#include <map>
#include <utility>
#include <memory>
#include <stack>
#include <vector>

#include "ast.h"

#include "result_bytecode.h"
#include "variable_context.h"
#include "translation_exception.h"

using namespace mathvm;

class AstToBytecodeVisitor: public AstVisitor {
  public:
    typedef std::stack<BytecodeFunction*> FunctionStack;
    typedef std::vector<VarType> VariableTypeStack;
    typedef std::vector<VariableContext> VariableContextStack;

    AstToBytecodeVisitor(): mCode(new ResultBytecode) {}

    ~AstToBytecodeVisitor() {delete mCode;}

    void visitTop(AstFunction* top) {
      mCode->addFunction(new BytecodeFunction(top));
      visitAstFunction(top);
      std::string topFunctionName = "<top>";
      ((BytecodeFunction*) mCode->functionByName(topFunctionName))->bytecode()->addInsn(BC_RETURN);
    }

    void visitAstFunction(AstFunction* astFunction) {
      mFunctionStack.push(
          static_cast<BytecodeFunction*>(
            mCode->functionByName(astFunction->name())));
      newContext();
      astFunction->node()->visit(this);
      mFunctionStack.pop();
    }

    void visitBlockNode(BlockNode* node) {
      // The context is already initialised
      addVariables(node);
      addFunctions(node);
      visitFunctions(node);
      node->visitChildren(this);
    }

    void visitCallNode(CallNode* node) {
      auto function = mCode->functionByName(mangledFunctionName(node->name()));
      auto functionId = function->id();
      for (size_t i = 0; i != node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        bool typecastSuccess = typecastStackValueTo(function->parameterType(i));
        if (!typecastSuccess) {
          typeMismatch(node->parameterAt(i));
        }
      }
      insn(BC_CALL);
      int16(functionId);
      if (function->returnType() != VT_VOID) {
        pushTypeToStack(function->returnType());
      }
    }

    void visitFunctionNode(FunctionNode* node) {
      // Context is already initialised
      getVariablesFromStackReverseOrder(node);
      node->body()->visit(this);
      if (!isTopFunction(node) && node->returnType() != VT_VOID) {
        bool typecastSuccess = typecastStackValueTo(node->returnType());
        if (!typecastSuccess) {
          typeMismatch(node);
        }
      }
      outContext();
    }

    void typeMismatch(AstNode* node) {
      throw TranslationException(Status::Error("Type mismatch", 
            node->position()));
    }

    void getVariablesFromStackReverseOrder(FunctionNode* node) {
      for (int32_t i = node->parametersNumber() - 1; i != -1; --i) {
        auto variableId = newVariable(node->parameterName(i));
        switch (node->parameterType(i)) {
          case VT_INT: insn(BC_STOREIVAR); break;
          case VT_DOUBLE: insn(BC_STOREDVAR); break;
          case VT_STRING: insn(BC_STORESVAR); break;
          default: invalid(); break;
        }
        int16(variableId);
      }
    }

    void visitReturnNode(ReturnNode* node) {
      if (node->returnExpr()) {
        if (currentFunction()->returnType() == VT_VOID) {
          throw TranslationException(
              Status::Error(("Function " + currentFunction()->name() +
              " returns something instead of declared void").c_str(), node->position()));
        }
        node->returnExpr()->visit(this);
        typecastStackValueTo(currentFunction()->returnType());
      }
      insn(BC_RETURN);
    }

    void visitStringLiteralNode(StringLiteralNode* node) {
      uint16_t constantId = mCode->makeStringConstant(node->literal());
      insn(BC_SLOAD);
      int16(constantId);
      stringTypeToStack();
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
      insn(BC_ILOAD);
      currentBytecode()->addInt64(node->literal());
      intTypeToStack();
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      insn(BC_DLOAD);
      currentBytecode()->addDouble(node->literal());
      doubleTypeToStack();
    }

    void visitPrintNode(PrintNode* node) {
      for (uint32_t i = 0; i < node->operands(); ++i) {
        printOneOperand(node->operandAt(i));
      }
    }

    void visitLoadNode(LoadNode* node) {
      auto variableId = findVariableInContexts(node->var()->name());
      loadVariable(node->var()->type(), variableId);
    }

    void visitStoreNode(StoreNode* node) {
      node->value()->visit(this);
      auto variableId = findVariableInContexts(node->var()->name());
      if (node->var()->type() == VT_STRING) {
        // If var is string, can only store
        storeString(variableId);
      } else {
        // If not assign, load var value
        if (node->op() != tASSIGN) {
          loadVariable(node->var()->type(), variableId);

          bool typecastSuccess = typecastStackValuesToCommonType();
          if (!typecastSuccess) {
            typeMismatch(node);
          }
          // Perform operation on loaded value if not assign
          if (node->op() == tINCRSET) {
            add();
          } else if (node->op() == tDECRSET) {
            sub();
          }
        }

        // Perform casts or fail if types are different
        bool typecastSuccess = typecastStackValueTo(node->var()->type());
        if (!typecastSuccess) {
          typeMismatch(node);
        }

        // Store the result into var
        storeVariable(node->var()->type(), variableId);
      }
      popTypeFromStack();
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
      node->operand()->visit(this);
      if (ttos() == VT_INT) {
        switch (node->kind()) {
          case tADD: nop(); break;
          case tSUB: insn(BC_INEG); break;
          case tNOT: notop(); break;
          default: break;
        }
      } else if (ttos() == VT_DOUBLE) {
        switch (node->kind()) {
          case tADD: nop(); break;
          case tSUB: insn(BC_DNEG); break;
          case tNOT: typeMismatch(node); break;
          default: break;
        }
      } else {
        typeMismatch(node);
      }
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
      node->right()->visit(this);
      node->left()->visit(this);
      bool typecastSuccess = typecastStackValuesToCommonType();
      if (!typecastSuccess) {
        typeMismatch(node);
      }
      switch (node->kind()) {
        case tOR: lor(); break;
        case tAND: land(); break;
        case tAOR: iaor(); break;
        case tAAND: iaand(); break;
        case tAXOR: iaxor(); break;
        case tEQ: eq(); break;
        case tNEQ: neq(); break;
        case tGT: gt(); break;
        case tLT: lt(); break;
        case tLE: le(); break;
        case tGE: ge(); break;
        case tADD: add(); break;
        case tSUB: sub(); break;
        case tMUL: mul(); break;
        case tDIV: div(); break;
        case tMOD: mod(); break;
        // Basically do nothing, upper is value to be assigned
        // lower - bound value
        case tRANGE:
                   if (!isRangeAllowed()) {
                     throw TranslationException(Status::Error("Ranges are allowed only in for loops", node->position()));
                   }
                   disallowRange();
                   break;
        default: break;
      }
    }

    void visitIfNode(IfNode* node) {
      // Check condition, 0 if false
      node->ifExpr()->visit(this);

      // Init label and make a branch to bypass true block
      // jump if 0
      Label afterTrueBlockLabel(currentBytecode());
      izero();
      currentBytecode()->addBranch(BC_IFICMPE, afterTrueBlockLabel);

      // Translate true block
      node->thenBlock()->visit(this);

      // Init a label to bypass false block if true block is hit
      // jump unconditionally
      Label afterFalseBlockLabel(currentBytecode());
      currentBytecode()->addBranch(BC_JA, afterFalseBlockLabel);

      // Bind label for bypassing true block to current address
      currentBytecode()->bind(afterTrueBlockLabel);

      // Translate false block
      if (node->elseBlock()) {
        node->elseBlock()->visit(this);
      }

      // Bind after false block label to a current address
      currentBytecode()->bind(afterFalseBlockLabel);
    }

    void visitForNode(ForNode* node) {
      if (!node->inExpr()->isBinaryOpNode() || 
          node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw TranslationException(
            Status::Error("Malformed 'in' expression",
              node->inExpr()->position()));
      }
      allowRange();
      node->inExpr()->visit(this);

      // Store loop counter and its type
      auto counterVariableId = findVariableInContexts(node->var()->name());
      auto counterVariableType = node->var()->type();
      if (counterVariableType != VT_INT) {
        throw TranslationException(Status::Error(
              ("For counter variable is " + std::string(typeToName(counterVariableType)) +
              " instead of int").c_str(), node->position()));
      }
      storeVariable(ttos(), counterVariableId);
      popTypeFromStack();

      // We don't pop the upper border from stack,
      // use it for checking for end later

      // Init label for back jump at the end of the loop and bind
      // it to the current address
      Label beforeConditionLabel(currentBytecode());
      currentBytecode()->bind(beforeConditionLabel);

      // Init label for bypassing the loop if the condition is false
      Label afterForLabel(currentBytecode());

      // We compare the counter variable to the upper bound using <
      loadVariable(counterVariableType, counterVariableId);

      // Add branch for a bypass, jump if 0 (false)
      currentBytecode()->addBranch(BC_IFICMPG, afterForLabel);

      // Translate body
      node->body()->visit(this);

      iinc(counterVariableId);

      // Add branch for a new iteration
      // jump unconditionally
      currentBytecode()->addBranch(BC_JA, beforeConditionLabel);

      // Bind the bypass label to the current address
      currentBytecode()->bind(afterForLabel);

      // Pop upper bound, since we're done
      pop();
    }

    void iinc(ContextVariableId& id) {
      loadInt(id);
      insn(BC_ILOAD1);
      add();
      storeInt(id);
    }

    void visitWhileNode(WhileNode* node) {
      // Init label for back jump at the end of the loop and bind
      // it to the current address
      Label beforeConditionLabel(currentBytecode());
      currentBytecode()->bind(beforeConditionLabel);

      // Init label for bypassing the loop if the condition is false
      Label afterWhileLabel(currentBytecode());

      // Check condition, 0 if false
      // and add branch for a bypass, jump if 0 (false)
      node->whileExpr()->visit(this);
      izero();
      currentBytecode()->addBranch(BC_IFICMPE, afterWhileLabel);

      // Translate body
      node->loopBlock()->visit(this);

      // Add branch for a new iteration
      // jump unconditionally
      currentBytecode()->addBranch(BC_JA, beforeConditionLabel);

      // Bind the bypass label to the current address
      currentBytecode()->bind(afterWhileLabel);
    }

    Code* code() {
      Code* result = mCode;
      mCode = nullptr;
      return result;
    }

  private:
    Bytecode* currentBytecode() {return currentFunction()->bytecode();}

    BytecodeFunction* currentFunction() {return mFunctionStack.top();}

    void insn(Instruction instruction) {currentBytecode()->addInsn(instruction);}

    void int16(int16_t value) {currentBytecode()->addInt16(value);}

    void nop() {}

    void lor() {
      if (ttos() == VT_INT) {
        iaor();
        izero();
        icmp();
      } else {
        dzero();
        dcmp();
        swap();
        dzero();
        dcmp();
        lor();
      }
    }

    void land() {
      if (ttos() == VT_INT) {
        izero();
        eq();
        swap();
        izero();
        eq();
      } else {
        dzero();
        eq();
        swap();
        dzero();
        eq();
      }
      lor();
      notop();
    }

    void eq() {
      neq();
      notop();
    }

    void neq() {
      if (ttos() == VT_INT) {
        icmp();
      } else {
        dcmp();
      }
    }

    void notop() {
      izero();
      icmp();
      iload1();
      iaand();
      iload1();
      iaxor();
    }

    void gt() {
      if (ttos() == VT_INT) {
        icmp();
      } else {
        dcmp();
      }
      iload1();
      eq();
    }

    void iloadm1() {
      insn(BC_ILOADM1);
      intTypeToStack();
    }

    void iload1() {
      insn(BC_ILOAD1);
      intTypeToStack();
    }

    void lt() {
      if (ttos() == VT_INT) {
        icmp();
      } else {
        dcmp();
      }
      iloadm1();
      eq();
    }

    void ge() {
      lt();
      notop();
    }

    void le() {
      gt();
      notop();
    }

    void add() {
      if (ttos() == VT_INT) {
        insn(BC_IADD);
      } else {
        insn(BC_DADD);
      }
      // Both operands should be the same type now
      popTypeFromStack();
    }

    void sub() {
      if (ttos() == VT_INT) {
        insn(BC_ISUB);
      } else {
        insn(BC_DSUB);
      }
      // Both operands should be the same type now
      popTypeFromStack();
    }

    void mul() {
      if (ttos() == VT_INT) {
        imul();
      } else {
        dmul();
      }
    }

    void div() {
      if (ttos() == VT_INT) {
        insn(BC_IDIV);
      } else {
        insn(BC_DDIV);
      }
      // Both operands should be the same type now
      popTypeFromStack();
    }

    void mod() {
      insn(BC_IMOD);
      two2int();
    }

    void pop() {
      insn(BC_POP);
      popTypeFromStack();
    }

    void invalid() {
      insn(BC_INVALID);
      pushTypeToStack(VT_INVALID);
    }

    void icmp() {
      insn(BC_ICMP);
      two2int();
    }

    void dcmp() {
      insn(BC_DCMP);
      two2int();
    }


    void izero() {
      insn(BC_ILOAD0);
      intTypeToStack();
    }

    void dzero() {
      insn(BC_DLOAD0);
      doubleTypeToStack();
    }

    void imul() {
      insn(BC_IMUL);
      two2int();
    }

    void dmul() {
      insn(BC_DMUL);
      two2double();
    }

    void iaor() {
      insn(BC_IAOR);
      two2int();
    }

    void iaand() {
      insn(BC_IAAND);
    }

    void iaxor() {
      insn(BC_IAXOR);
    }

    void two2int() {
      pop2TypesFromStack();
      intTypeToStack();
    }

    void two2double() {
      pop2TypesFromStack();
      doubleTypeToStack();
    }

    void swap() {
      auto topType = mVariableTypeStack.back();
      auto secondFromTop = mVariableTypeStack[mVariableTypeStack.size() - 2];
      mVariableTypeStack[mVariableTypeStack.size() - 1] = secondFromTop;
      mVariableTypeStack[mVariableTypeStack.size() - 2] = topType;
      insn(BC_SWAP);
    }

    ContextVariableId internalVariableId(int16_t internalId) {
      return std::make_pair(0, internalId);
    }


    VarType ttos() {return mVariableTypeStack.back();}

    void pushTypeToStack(VarType type) {mVariableTypeStack.push_back(type);}

    void intTypeToStack() {pushTypeToStack(VT_INT);}

    void doubleTypeToStack() {pushTypeToStack(VT_DOUBLE);}

    void stringTypeToStack() {pushTypeToStack(VT_STRING);}

    void popTypeFromStack() {mVariableTypeStack.pop_back();}

    void pop2TypesFromStack() {
      popTypeFromStack();
      popTypeFromStack();
    }

    void printOneOperand(AstNode* node) {
      node->visit(this);
      Instruction printInstruction = BC_INVALID;
      switch (ttos()) {
        case VT_STRING: printInstruction = BC_SPRINT; break;
        case VT_INT: printInstruction = BC_IPRINT; break;
        case VT_DOUBLE: printInstruction = BC_DPRINT; break;
        default: break;
      }
      insn(printInstruction);
      popTypeFromStack();
    }

    void varId(ContextVariableId& id) {
      if (!isInLocalContext(id)) {
        int16(id.first);
      }
      int16(id.second);
    }

    void storeVariable(VarType type, ContextVariableId& id) {
      switch (type) {
        case VT_INT: storeInt(id); break;
        case VT_DOUBLE: storeDouble(id); break;
        case VT_STRING: storeString(id); break;
        default: invalid(); break;
      }
    }

    void storeInt(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_STOREIVAR);
      } else {
        insn(BC_STORECTXIVAR);
      }
      varId(id);
    }

    void storeDouble(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_STOREDVAR);
      } else {
        insn(BC_STORECTXDVAR);
      }
      varId(id);
    }

    void storeString(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_STORESVAR);
      } else {
        insn(BC_STORECTXSVAR);
      }
      varId(id);
    }

    void loadVariable(VarType type, ContextVariableId& id) {
      switch (type) {
        case VT_INT: loadInt(id); break;
        case VT_DOUBLE: loadDouble(id); break;
        case VT_STRING: loadString(id); break;
        default: invalid(); break;
      }
    }

    void loadInt(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_LOADIVAR);
      } else {
        insn(BC_LOADCTXIVAR);
      }
      varId(id);
      intTypeToStack();
    }

    void loadDouble(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_LOADDVAR);
      } else {
        insn(BC_LOADCTXDVAR);
      }
      varId(id);
      doubleTypeToStack();
    }

    void loadString(ContextVariableId& id) {
      if (isInLocalContext(id)) {
        insn(BC_LOADSVAR);
      } else {
        insn(BC_LOADCTXSVAR);
      }
      varId(id);
      stringTypeToStack();
    }

    VariableContext& currentContext() {return mVariableContextStack.back();}

    void newContext() {
      if (mVariableContextStack.empty()) {
        mVariableContextStack.push_back(VariableContext());
      } else {
        mVariableContextStack.push_back(
            VariableContext(currentContext().contextId() + 1));
      }
    }

    void outContext() {
      mVariableContextStack.pop_back();
    }

    ContextVariableId findVariableInContexts(const std::string& name) {
      for (auto it = mVariableContextStack.rbegin(); it != mVariableContextStack.rend(); ++it) {
        if (it->variableExistsInContext(name)) {
          return it->findContextVariableId(name);
        }
      }
      return ContextVariableId();
    }

    uint16_t newVariable(const std::string name) {
      auto id = currentContext().newVariable(name);
      if (currentContext().hasOverflowed()) {
        throw TranslationException(
            Status::Error("Too many variables in a context"));
      }
      return id;
    }

    // True - successful cast
    bool typecastStackValueTo(VarType desiredType) {
      if (desiredType == ttos()) {
        return true;
      }
      // Cannot typecast to/from strings
      if (ttos() == VT_STRING || desiredType == VT_STRING) {
        return false;
      }
      // If type of top of stack is VT_INT than we are casting int to double
      if (ttos() == VT_INT) {
        int2double();
      // Otherwise, it's double to int
      } else {
        double2int();
      }
      return true;
    }

    void int2double() {
      insn(BC_I2D);
      popTypeFromStack();
      doubleTypeToStack();
    }

    void double2int() {
      insn(BC_D2I);
      popTypeFromStack();
      intTypeToStack();
    }

    // True - successful cast, false - not so much
    bool typecastStackValuesToCommonType() {
      auto last = mVariableTypeStack.back();
      auto prevLast = mVariableTypeStack[mVariableTypeStack.size() - 2];
      // If any of values are invalid or strings, then we're done here
      if (last == VT_INVALID || prevLast == VT_INVALID || 
          last == VT_STRING || prevLast == VT_STRING) {
        return false;
      }

      // We cannot typecast strings, so typecast int to double
      if (last != prevLast) {
        if (last == VT_INT) {
          int2double();
        } else {
          swap();
          int2double();
          swap();
        }
      }
      return true;
    }

    void addVariables(BlockNode* node) {
      Scope::VarIterator iter(node->scope());
      while(iter.hasNext()) {
        newVariable(iter.next()->name());
      }
    }

    void addFunctions(BlockNode* node) {
      Scope::FunctionIterator iter(node->scope());
      while (iter.hasNext()) {
        auto function = iter.next();
        if (!isFunctionKnown(function->name())) {
          mCode->addFunction(new BytecodeFunction(function));
        }
      }
    }

    void visitFunctions(BlockNode* node) {
      Scope::FunctionIterator iter(node->scope());
      while (iter.hasNext()) {
        visitAstFunction(iter.next());
      }
    }

    void pop2ndTop() {
      swap();
      insn(BC_POP);
    }

    void pop23thTop() {
      pop2ndTop();
      pop2ndTop();
    }

    void pop234thTop() {
      pop2ndTop();
      pop23thTop();
    }

    string mangledFunctionName(const string& functionName) {
      return functionName;
    }

    bool isInLocalContext(ContextVariableId& id) {
      return currentContext().contextId() == id.first;
    }

    bool isTopFunction(FunctionNode* node) {
      return node->name() == "<top>";
    }

    void allowRange() {
      mRangeAllowed = true;
    }

    void disallowRange() {
      mRangeAllowed = false;
    }

    bool isRangeAllowed() {return mRangeAllowed;}

    bool isFunctionKnown(const std::string& name) {
      return mCode->functionByName(name) != nullptr;
    }

    bool mRangeAllowed = false;
    Code* mCode;
    FunctionStack mFunctionStack;
    VariableTypeStack mVariableTypeStack;
    VariableContextStack mVariableContextStack;
};

#endif
