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

using namespace mathvm;

class AstToBytecodeVisitor: public AstVisitor {
  public:
    typedef std::stack<BytecodeFunction*> FunctionStack;
    typedef std::vector<VarType> VariableTypeStack;
    typedef std::vector<VariableContext> VariableContextStack;

    AstToBytecodeVisitor(): mCode(new ResultBytecode) {
      mVariableContextStack.push_back(VariableContext());
    }

    ~AstToBytecodeVisitor() {mCode = nullptr;}

    void visitTop(AstFunction* top) {
      visitAstFunction(top);
      std::string topFunctionName = "<top>";
      ((BytecodeFunction*) mCode->functionByName(topFunctionName))->bytecode()->addInsn(BC_RETURN);
    }

    void visitAstFunction(AstFunction* astFunction) {
      mFunctionStack.push(new BytecodeFunction(astFunction));
      // Add ast function before visiting for recursion
      code()->addFunction(mFunctionStack.top());
      newContext();
      astFunction->node()->visit(this);
      mFunctionStack.pop();
    }

    void visitBlockNode(BlockNode* node) {
      // The context is already initialised
      addVariables(node);
      addFunctions(node);
      node->visitChildren(this);
    }

    void visitCallNode(CallNode* node) {
      node->visitChildren(this);
      insn(BC_CALL);
      auto functionId =
        code()->functionByName(mangledFunctionName(node->name()))->id();
      int16(functionId);
    }

    void visitFunctionNode(FunctionNode* node) {
      // Context is already initialised
      getVariablesFromStackReverseOrder(node);
      node->body()->visit(this);
      if (!isTopFunction(node)) {
        typecastStackValueTo(node->returnType());
      }
      outContext();
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
        node->returnExpr()->visit(this);
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

          typecastStackValuesToCommonType();
          // Perform operation on loaded value if not assign
          if (node->op() == tINCRSET) {
            add();
          } else if (node->op() == tDECRSET) {
            sub();
          }
        }

        // Perform casts or fail if types are different
        typecastStackValueTo(node->var()->type());

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
          case tSUB: insn(BC_ILOADM1);
                     insn(BC_IMUL);
                     break;
          case tNOT: insn(BC_INEG); break;
          default: break;
        }
        intTypeToStack();
      } else if (ttos() == VT_DOUBLE) {
        switch (node->kind()) {
          case tADD: nop(); break;
          case tSUB: insn(BC_DLOADM1);
                     insn(BC_DMUL);
                     break;
          case tNOT: insn(BC_DNEG); break;
          default: break;
        }
        if (node->kind() == tNOT) {
          intTypeToStack();
        } else {
          doubleTypeToStack();
        }
      } else {
        invalid();
      }
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
      node->right()->visit(this);
      node->left()->visit(this);
      typecastStackValuesToCommonType();
      switch (node->kind()) {
        case tOR: lor(); break;
        case tAND: land(); break;
        case tAOR: insn(BC_IAOR); break;
        case tAAND: insn(BC_IAAND); break;
        case tAXOR: insn(BC_IAXOR); break;
        case tEQ: eq(); break;
        case tNEQ: neq(); break;
        // Operations lower are reversed due to
        // first traversal of right argument
        case tGT: lt(); break;
        case tLT: gt(); break;
        case tLE: ge(); break;
        case tGE: le(); break;
        case tADD: add(); break;
        case tSUB: sub(); break;
        case tMUL: mul(); break;
        case tDIV: div(); break;
        case tMOD: mod(); break;
        // Basically do nothing, upper is value to be assigned
        // lower - bound value
        case tRANGE: break;
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
      // TODO: check correctness
      node->inExpr()->visit(this);

      // Store loop counter and its type
      auto counterVariableId = findVariableInContexts(node->var()->name());
      auto counterVariableType = ttos();
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

    Code* code() {return mCode;}

  private:
    Bytecode* currentBytecode() {return mFunctionStack.top()->bytecode();}

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
      neg();
    }

    void eq() {
      neq();
      neg();
    }

    void neq() {
      if (ttos() == VT_INT) {
        icmp();
      } else {
        dcmp();
      }
    }

    void neg() {
      insn(BC_INEG);
      popTypeFromStack();
      intTypeToStack();
    }

    void gt() {
      if (ttos() == VT_INT) {
        icmp();
      } else {
        dcmp();
      }
      iloadm1();
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
        insn(BC_ICMP);
      } else {
        insn(BC_DCMP);
      }
      iload1();
      eq();
    }

    void ge() {
      lt();
      neg();
    }

    void le() {
      gt();
      neg();
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

    uint16_t newContext() {
      VariableContext context(currentContext().contextId() + 1);
      mVariableContextStack.push_back(context);
      return context.contextId();
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
      return currentContext().newVariable(name);
    }

    void typecastStackValueTo(VarType desiredType) {
      if (desiredType == ttos()) {
        return;
      }
      // Cannot typecast to/from strings
      if (ttos() == VT_STRING || desiredType == VT_STRING) {
        pop();
        invalid();
      // If type of top of stack is VT_INT than we are casting int to double
      } else if (ttos() == VT_INT) {
        int2double();
      // Otherwise, it's double to int
      } else {
        double2int();
      }
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

    void typecastStackValuesToCommonType() {
      auto last = mVariableTypeStack.back();
      auto prevLast = mVariableTypeStack[mVariableTypeStack.size() - 2];
      // If any of values are invalid, then we're done here
      if (last == VT_INVALID || prevLast == VT_INVALID) {
        pop();
        pop();
        invalid();
        invalid();
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

    Code* mCode;
    FunctionStack mFunctionStack;
    VariableTypeStack mVariableTypeStack;
    VariableContextStack mVariableContextStack;
};

#endif
