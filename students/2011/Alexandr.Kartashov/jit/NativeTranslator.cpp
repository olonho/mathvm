#include <iostream>
#include <map>
#include <vector>

#include <string.h>
#include <assert.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "VarNum.h"
#include "VarCollector.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {

  class NativeGenerator : public AstVisitor {
    /**
     *  There's no way to use RSP and probably RBP...
     *  Nevertheless, we have 8 x87 registers and 8 128 bit XMM registers.
     *  In SSE2 the latter can store 2 64-bit integers and doubles, so we have
     *  32 integer registers and 24 floating-point registers.
     *  We shall consider using it...
     */

    #define REGS 12
    #define VAR_SIZE 8

  private:
    Runtime *_runtime;
    NativeCode* _code;
    char _retReg;

    std::map<std::string, uint16_t> _stringConst;
    std::map<AstFunction*, uint16_t> _functions;

    // --------------------------------------------------------------------------------

    static char reg(const AstVar* v) {
      return 0;
      //return boost::any_cast<char>(v->userData);     
    }

    static char remap(char reg) {
      static char rmaps[] = {3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

      return rmaps[(size_t)reg];
    }

    static bool isAtomic(const AstNode* node) {
      return 
        node->isIntLiteralNode() || 
        node->isDoubleLiteralNode() || 
        node->isStringLiteralNode() ||
        node->isLoadNode() ||
        node->isStoreNode();
    }

    static char other(char reg) {
      return reg == RAX ? RCX : RAX;
    }

  public:
    NativeGenerator(AstFunction* root) { 
      VarCollector vc;

      _runtime = new Runtime;
      vc.collect(root, _runtime);

      root->node()->visit(this);
    }

    Code *getCode() {
      return _runtime;
    }


    // Visitor interface implementation 
    /*
    VarType gen_num_conversions(BinaryOpNode* node) {
      VarType l = node_type[node->left()];
      VarType r = node_type[node->right()];

      if (l != r) {
        if (r == VT_INT) {
          code->add(BC_SWAP);
        }

        code->add(BC_I2D);

        return VT_DOUBLE;
      } else {
        return l;
      }
    }

    void gen_arith(BinaryOpNode* node) {
      VarType t = gen_num_conversions(node);

      switch (t) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tADD:
          code->add(BC_DADD);
          break;

        case tSUB:
          code->add(BC_DSUB);
          break;

        case tMUL:
          code->add(BC_DMUL);
          break;

        case tDIV:
          code->add(BC_DDIV);
          break;

        case tINCRSET:
          code->add(BC_DADD);
          code->add(BC_STOREDVAR);
          put(node->left()->asLoadNode()->var());
          break;

        case tDECRSET:
          code->add(BC_DSUB);
          code->add(BC_STOREDVAR);
          put(node->left()->asLoadNode()->var());
          break;

        default:
          break;
        }
        break;

      case VT_INT:
        switch (node->kind()) {
          // Arithmetics 
        case tADD:
          code->add(BC_IADD);
          break;

        case tSUB:
          code->add(BC_ISUB);
          break;

        case tMUL:
          code->add(BC_IMUL);
          break;

        case tDIV:
          code->add(BC_IDIV);
          break;
          
        case tINCRSET:
          code->add(BC_IADD);
          code->add(BC_STOREIVAR);
          put(node->left()->asLoadNode()->var());
          break;

        case tDECRSET:
          code->add(BC_ISUB);
          code->add(BC_STOREIVAR);
          put(node->left()->asLoadNode()->var());
          break;

        default:
          break;
        }
        break;

      default:
        break;
      }

      node_type[node] = t;
    }

    void gen_comp(BinaryOpNode* node) {
      VarType t = gen_num_conversions(node);

      if (t == VT_DOUBLE) {
        code->add(BC_DCMP);

        code->add(BC_SWAP);    // Remove arguments
        code->add(BC_POP);
        code->add(BC_SWAP);
        code->add(BC_POP);

        code->add(BC_ILOAD0);  // convert to the integer case :)
        //code->add(BC_SWAP);
      }

      switch (node->kind()) {
      case tEQ:
        code->add(BC_IFICMPE);
        break;
          
      case tNEQ:
        code->add(BC_IFICMPNE);
        break;

      case tGT:
        code->add(BC_IFICMPG);
        break;

      case tGE:
        code->add(BC_IFICMPGE);
        break;
          
      case tLT:
        code->add(BC_IFICMPL);
        break;

      case tLE:
        code->add(BC_IFICMPLE);
        break;

      default:
        break;
      }
      code->addInt16(4);

      code->add(BC_ILOAD0); // If the condition doesn't hold
      
      code->add(BC_JA);
      code->addInt16(1);
      
      code->add(BC_ILOAD1); // If the condition holds
      
      node_type[node] = VT_INT;       
    }

    void gen_logic(BinaryOpNode* node) {
      switch(node->kind()) {
      case tOR:
        code->add(BC_IADD);
        code->add(BC_ILOAD0);

        code->add(BC_IFICMPG);
        code->addInt16(4);
        
        code->add(BC_ILOAD0);

        code->add(BC_JA);
        code->add(1);

        code->add(BC_ILOAD1);        
        break;

      case tAND:
        code->add(BC_IMUL);
        break;

      default:
        break;
      }

      node_type[node] = VT_INT;
    }
    */

    

    VISIT(BinaryOpNode) {
      char oldRet = _retReg;
      char op1 = _retReg;
      char op2 = other(_retReg);

      node->left()->visit(this);

      _retReg = other(_retReg);
      node->right()->visit(this);
             

      switch(node->kind()) {
      case tADD:
        _code->add_rr(op1, op2);
        break;
        

      case tSUB:
        _code->sub_rr(op1, op2);
        break;

      case tMUL:
        _code->mul_rr(op1, op2);
        break;


      case tDIV:
      case tEQ:
      case tNEQ:
      case tGT:
      case tLT:
      case tLE:          
      case tOR:
      case tAND:
      default:
        ABORT("The operation isn't supported yet");
      }

      _retReg = oldRet;

      //node->userData = node->left()->userData;
    }

    /*
    VISIT(UnaryOpNode) {
      node->operand()->visit(this); 

      switch (node_type[node->operand()]) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tSUB:
          code->add(BC_DNEG);
          break;

        default:
          break;
        }
        break;

      case VT_INT:
        switch (node->kind()) {
        case tSUB:
          code->add(BC_INEG);
          break;

        case tNOT:
          code->add(BC_ILOAD1);
          code->add(BC_SWAP);
          code->add(BC_ISUB);
          break;

        default:
          break;
        }
        break;

      default:
        break;
      }

      node_type[node] = node_type[node->operand()];
    }

    VISIT(DoubleLiteralNode) {
      code->add(BC_DLOAD);
      double val = node->literal();
      code->addDouble(val);

      node_type[node] = VT_DOUBLE;
    }
    */

    VISIT(IntLiteralNode) {
      _code->mov_r_imm(_retReg, node->literal());
    }


    /*
    VISIT(StringLiteralNode) {
      std::map<std::string, uint16_t>::const_iterator it;
      uint16_t id;

      it = string_const.find(node->literal());
      if (it == string_const.end()) {
        strings->push_back(node->literal());
        id = string_const[node->literal()] = strings->size() - 1;        
      } else {
        id = it->second;
      }
      
      code->add(BC_SLOAD);
      code->addUInt16(id);

      node_type[node] = VT_STRING;
    }
    */
 

    VISIT(LoadNode) {
      switch (node->var()->type()) {
      case VT_DOUBLE:
        ABORT("Doubles are not supported yet");
        break;

      case VT_INT:
        _code->mov_rm(_retReg, RBP, -VAR_SIZE*VAR_INFO(node->var())->fPos - 8);
        break;

      case VT_STRING:
        ABORT("Strings are not supported yet");
        break;

      default:
        // Should never happen
        break;
      }
    }
    

    VISIT(StoreNode) {
      _retReg = RAX;
      node->visitChildren(this);

      switch (node->var()->type()) {
      case VT_INT:
        if (node->op() == tASSIGN) {
          _code->mov_mr(RAX, RBP, -VAR_SIZE*VAR_INFO(node->var())->fPos - 8);
        } else {
          ABORT("Not supported");
        }
        break;

      case VT_DOUBLE:
      case VT_STRING:
      default:
        ABORT("Not supported");
      }
    }
    
    /*
    VISIT(ForNode) {
      uint32_t jmp_pos;

      _num.add(node->var());
      
      node->inExpr()->asBinaryOpNode()->left()->visit(this);

      code->add(BC_STOREIVAR);
      put(node->var());

      jmp_pos = code->current();
      
      node->body()->visit(this);

      code->add(BC_LOADIVAR);
      put(node->var());

      code->add(BC_ILOAD1);
      code->add(BC_IADD);
      code->add(BC_STOREIVAR);
      put(node->var());

      code->add(BC_LOADIVAR);
      put(node->var());
      node->inExpr()->asBinaryOpNode()->right()->visit(this);      

      code->add(BC_IFICMPLE);
      code->addInt16((int16_t)jmp_pos - code->current() - 2);
    }
  
    VISIT(IfNode) {
      uint32_t jmp_pos;

      node->ifExpr()->visit(this);

      code->add(BC_ILOAD1);
      code->add(BC_IFICMPNE);
      code->addInt16(0);      
      jmp_pos = code->current();

      node->thenBlock()->visit(this);
      
      if (node->elseBlock()) {
        code->add(BC_JA);
        code->addInt16(0);
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));

        jmp_pos = code->current();

        node->elseBlock()->visit(this);
        
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));
      } else {
        code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));
      }             
    }

    VISIT(WhileNode) {
      uint32_t jmp_pos, cond_pos;

      cond_pos = code->current();
      node->whileExpr()->visit(this);
      code->add(BC_ILOAD1);
      code->add(BC_IFICMPNE);
      code->addInt16(0);
      jmp_pos = code->current();

      node->loopBlock()->visit(this);
      code->add(BC_JA);
      code->addInt16((int16_t)cond_pos - code->current() - 2);

      code->setTyped(jmp_pos - 2, (int16_t)((int32_t)code->current() - jmp_pos));
    }
    */
    
    VISIT(BlockNode) {
      node->visitChildren(this);
    }


    /*
    VISIT(PrintNode) {
      for (size_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        
        code->add(BC_DUMP);
        code->add(BC_POP);
      }
    }

    VISIT(CallNode) {
      node->visitChildren(this);
      
      code->add(BC_CALL);
      AstFunction *af = _curBlock->scope()->lookupFunction(node->name());
      uint16_t fid = _functions[af] + 1;
      //assert(fid != 0 && af != NULL);
      code->addUInt16(fid);

      node_type[node] = af->returnType();
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
      code->add(BC_RETURN);
    }
    */

    VISIT(FunctionNode) {
      NativeFunction* f = (NativeFunction*)node->info();
      NativeCode* oldCode = _code;

      _code = f->code();

      _code->push_r(RBP);
      _code->mov_rr(RBP, RSP);
      _code->sub_rm_imm(RSP, f->localsNumber()*VAR_SIZE);

      node->visitChildren(this);

      _code->add_rm_imm(RSP, f->localsNumber()*VAR_SIZE);
      _code->pop_r(RBP);
      _code->add(RET);      

      _code->done();

      _code = oldCode;
    }

#undef VISIT

  };

  // --------------------------------------------------------------------------------  

  class NativeTranslator : public Translator {
  public:
    Status* translate(const string& program, Code** code) {
      Parser parser;

      Status *status = parser.parseProgram(program);
      if (!status) {
        NativeGenerator ng(parser.top());

        *code = ng.getCode();
      }

      return status;
    }
  };

  // --------------------------------------------------------------------------------

  Translator* Translator::create(const string& impl) {
    return new NativeTranslator;
  }  
}
