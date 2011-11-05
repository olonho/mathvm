#include <iostream>
#include <map>
#include <vector>

#include <cstdio>
#include <cstring>
#include <assert.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "VarNum.h"
#include "VarCollector.h"
#include "FunctionCollector.h"
#include "Flow.h"
#include "Runtime.h"
#include "NativeGenerator2.h"

// ================================================================================

namespace mathvm {

#define INT_REG_POOL    0
#define DOUBLE_REG_POOL 1
#define POOLS 2

  static const size_t poolSize[] = { INT_REGS, DOUBLE_REGS };
  static const char poolTypes[] = { -1, -1, DOUBLE_REG_POOL, INT_REG_POOL, -1};

  class NativeGenerator : public AstVisitor {
    /**
     *  There's no way to use RSP and probably RBP...
     *  Nevertheless, we have 8 x87 registers and 8 128 bit XMM registers.
     *  In SSE2 the latter can store 2 64-bit integers and doubles, so we have
     *  32 integer registers and 24 floating-point registers.
     *  We shall consider using it...
     */

    #define REGS 16
    #define VAR_SIZE 8

  private:
    Runtime* _runtime;
    NativeCode* _code;
    char _retReg;

    std::map<std::string, uint16_t> _stringConst;
    std::map<AstFunction*, uint16_t> _functions;

    size_t _reallocs[POOLS];
    char _lastReg[POOLS];

    // --------------------------------------------------------------------------------

    //static char rmaps[] = {RAX, RCX, RDX, RBX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15};

    static char reg(const AstVar* v) {
      return 0;
      //return boost::any_cast<char>(v->userData);     
    }

    static char remap(char reg) {
      //static char rmaps[] = {3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
      
      //return rmaps[(size_t)reg];
      ABORT("Deprecated");
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

    char allocReg(int pool = INT_REG_POOL) {
      char prevReg = _lastReg[pool];

      _lastReg[pool] = (_lastReg[pool] + 1) % poolSize[pool];
      if (_lastReg[pool] == 0) {
        _reallocs[pool]++;
      }

      if (_reallocs[pool] > 0) {
        switch (pool) {
        case INT_REG_POOL:
          _code->push_r(prevReg);
          break;

        case DOUBLE_REG_POOL:
          _code->sub_rm_imm(RSP, sizeof(double));
          _code->movq_m_xmm(prevReg, RSP, sizeof(double));
          break;

        default:
          ABORT("An infernal thing happened. It's better to turn off your computer and do exorcism...");
          break;
        }
      }

      return _lastReg[pool];
    }

    void deallocReg(int pool = INT_REG_POOL) {
      if (_reallocs[pool]) {
        switch (pool) {
        case INT_REG_POOL:
          _code->pop_r(_lastReg[pool]);
          break;

        case DOUBLE_REG_POOL:
          _code->movq_xmm_m(_lastReg[pool], RSP, sizeof(double));
          _code->add_rm_imm(RSP, sizeof(double));
        }
      }

      _lastReg[pool]--;

      if (_lastReg[pool] < 0) {
        _lastReg[pool] = 0;
      }
    }

  public:
    NativeGenerator(AstFunction* root) { 
      CompilerPool pool;
      
      /*
      for (int i = 0; i < POOLS; ++i) {
        _lastReg[i] = 0;
        _reallocs[i] = 0;
      }
      */

      _runtime = new Runtime;
      
      FunctionCollector fc(root, _runtime, &pool);
      VarCollector vc(root, _runtime, &pool);
      Flow flow(root, &pool);
      NativeGenerator2 ng1(root);

      info(root->node())->funRef->code()->link();

      //root->node()->visit(this);
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
    

    void doubleArith(char op1, char op2, TokenKind kind) {
      switch (kind) {
      case tADD:
        _code->add_xmm_xmm(op1, op2);
        break;

      case tSUB:
        _code->sub_xmm_xmm(op1, op2);
        break;

      case tMUL:
        _code->mul_xmm_xmm(op1, op2);
        break;

      case tDIV:
        _code->div_xmm_xmm(op1, op2);
        break;
        
      default:
        ABORT("This shouldn't have happened...");
      }
    }

    void intArith(char op1, char op2, TokenKind kind) {
      switch (kind) {
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
        _code->push_r(RAX);
        _code->push_r(RDX);
        _code->mov_rr(RAX, op1);
        if (op2 != RDX) {
          _code->mov_r_imm(RDX, 0);            
          _code->div_r(op2);
        } else {
          char r = allocReg(INT_REG_POOL);
          _code->mov_rr(r, RDX);
          _code->mov_r_imm(RDX, 0);
          _code->div_r(r);
          deallocReg(INT_REG_POOL);
        }
        _code->mov_rr(_retReg, RAX);
        _code->pop_r(RDX);
        _code->pop_r(RAX);
        break;
        
      default:
        ABORT("This shouldn't have happened...");
      }
    }

    void intComp(char op1, char op2, TokenKind kind) {
      _code->cmp_rr(op1, op2);

      switch (kind) {
      case tEQ:
        _code->setcc_r(_retReg, CC_E);
        break;

      case tNEQ:  
        _code->setcc_r(_retReg, CC_NE);
        break;

      case tLT:
        _code->setcc_r(_retReg, CC_L);
        break;

      case tLE:
        _code->setcc_r(_retReg, CC_LE);
        break;

      case tGT:
        _code->setcc_r(_retReg, CC_G);
        break;

      case tGE:
        _code->setcc_r(_retReg, CC_GE);
        break;

      default:
        ABORT("This shouldn't have happened...");
      }

      _code->and_r_imm8(_retReg, 0x7F);
    }

    void doubleComp(char op1, char op2, TokenKind kind) {
      switch (kind) {
      case tEQ:
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_EQ);
        
        break;

      case tNEQ:  
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_NEQ);
        break;

      case tLT:
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_LT);
        break;

      case tLE:
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_LE);
        break;

      case tGT:
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_NLE);
        break;

      case tGE:
        _code->cmp_xmm_xmm(op1, op2, SSE_CMP_NLT);
        break;
        
      default:
        ABORT("This shouldn't have happened...");
      }

      _code->movq_r_xmm(_retReg, op1);
      _code->test_rr(_retReg, _retReg);
      _code->setcc_r(_retReg, CC_NE);
      _code->and_r_imm8(_retReg, 0x7F);
    }


    VISIT(BinaryOpNode) {
      ValType vtype = NODE_INFO(node)->type;
      TokenKind kind = node->kind();
      char ptype, oldRet, op1, op2; 
      NativeLabel<int32_t> shortCut;
      bool doShortcut = false;

      oldRet = _retReg;
      if (!isComp(node->kind())) {
        ptype = poolTypes[NODE_INFO(node)->type];        
        op1 = _retReg;
      } else {
        ptype = INT_REG_POOL;
        op1 = allocReg(INT_REG_POOL);
        _retReg = op1;
      }

      node->left()->visit(this);

      if (isLogic(kind)) {
        doShortcut = true;

        _code->test_rr(op1, op1);
        
        switch (kind) {
        case tAND:
          shortCut = _code->jcc_rel32(CC_Z);
          break;

        case tOR:
          shortCut = _code->jcc_rel32(CC_NZ);
          break;

        default:
          ABORT("Your proccessor is tainted...");
        }
      }

      op2 = allocReg(ptype);
      _retReg = op2;
      node->right()->visit(this);

      _retReg = oldRet;

      if (isArith(kind)) {
        switch (vtype) {
        case VAL_INT:
          intArith(op1, op2, kind);
          break;

        case VAL_DOUBLE:
          doubleArith(op1, op2, kind);
          break;

        default:
          ABORT("This shouldn't have taken place...");
        }
      } else if (isComp(kind)) {
        switch (vtype) {
        case VAL_INT:
          intComp(op1, op2, kind);
          break;

        case VAL_DOUBLE:
          doubleComp(op1, op2, kind);
          break;

        default:
          ABORT("This shouldn't have taken place...");
        }      
      } else if (isLogic(kind)) {
        _code->test_rr(op2, op2);
        _code->setcc_r(_retReg, CC_NZ);
        shortCut.bind(_code);
      } else {
        ABORT("Not supported yet");      
      }
             
      deallocReg(ptype);
      if (isComp(node->kind())) {
        deallocReg(ptype);
      }
    }

    VISIT(UnaryOpNode) {
      node->operand()->visit(this); 

      switch (NODE_INFO(node->operand())->type) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tSUB:
          char r;

          r = allocReg(DOUBLE_REG_POOL);
          _code->push_r(RAX);
          _code->movq_r_xmm(RAX, _retReg);
          _code->movq_xmm_r(r, RAX);

          _code->mov_r_imm(RAX, 0);          
          _code->movq_xmm_r(_retReg, RAX);
          _code->sub_xmm_xmm(_retReg, r);
          _code->pop_r(RAX);
          deallocReg(DOUBLE_REG_POOL);
          break;

        default:
          ABORT("Not supported");
        }
        break;

      case VT_INT:
        switch (node->kind()) {
        case tSUB:
          _code->neg_r(_retReg);
          break;

        case tNOT:
          _code->test_rr(_retReg, _retReg);
          _code->setcc_r(_retReg, CC_E);
          break;

        default:
          ABORT("Not supported");
        }
        break;

      default:
        break;
      }
    }

    #define UINT(d) (*(int64_t*)&d)

    VISIT(DoubleLiteralNode) {
      double d = node->literal();

      _code->push_r(RAX);
      _code->mov_r_imm(RAX, UINT(d));
      _code->movq_xmm_r(_retReg, RAX);
      _code->pop_r(RAX);
    }

    VISIT(IntLiteralNode) {
      _code->mov_r_imm(_retReg, node->literal());
    }


    VISIT(StringLiteralNode) {
      _code->mov_r_imm(_retReg, (uint64_t)NODE_INFO(node)->string);
    }
     

    VISIT(LoadNode) {
      switch (node->var()->type()) {
      case VT_DOUBLE:
        _code->movq_xmm_m(_retReg, RBP, -VAR_SIZE*VAR_INFO(node->var())->fPos - 8);
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
      VarType t = node->var()->type();

      _retReg = allocReg(poolTypes[t]);
      node->visitChildren(this);

      switch (t) {
      case VT_INT:
        if (node->op() == tASSIGN) {
          _code->mov_mr(_retReg, RBP, -VAR_SIZE*VAR_INFO(node->var())->fPos - 8);
        } else {
          ABORT("Not supported");
        }
        break;

      case VT_DOUBLE:
        if (node->op() == tASSIGN) {
          _code->movq_m_xmm(_retReg, RBP, -VAR_SIZE*VAR_INFO(node->var())->fPos - 8);
        } else {
          ABORT("Not supported");
        }
        break;
        break;

      case VT_STRING:
      default:
        ABORT("Not supported");
      }

      deallocReg(poolTypes[t]);
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
    */
  
    VISIT(IfNode) {
      NativeLabel<int32_t> branchNotTaken;

      node->ifExpr()->visit(this);
      _code->test_rr(_retReg, _retReg);
      branchNotTaken = _code->jcc_rel32(CC_E);

      node->thenBlock()->visit(this);
      
      if (node->elseBlock()) {
        NativeLabel<int32_t> elseCut = _code->jmp_rel32();

        branchNotTaken.bind();
        node->elseBlock()->visit(this);
        elseCut.bind();
      } else {
        branchNotTaken.bind();
      }
    }

    /*
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


    VISIT(PrintNode) {
      static const char iamap[] = { RDI, RSI, RDX, RCX, R8, R9 };      
      //static const char damap[] = { XMM2, XMM3, XMM4, XMM5, XMM6, XMM7 };
      char iReg, dReg;
      VarType atype[INT_REGS + DOUBLE_REGS];

      iReg = allocReg(INT_REG_POOL);
      dReg = allocReg(DOUBLE_REG_POOL);

      size_t iargs = 1, dargs = 0, nargs = 0;

      for (size_t i = 0; i < node->operands() && iargs < 6 && dargs < 6; ++i) {
        switch (NODE_INFO(node->operandAt(i))->type) {
        case VAL_INT:
          _retReg = iReg;
          node->operandAt(i)->visit(this);
          _code->push_r(iReg);

          iargs++;
          atype[nargs++] = VT_INT;
          break;

        case VAL_DOUBLE:
          _retReg = dReg;
          node->operandAt(i)->visit(this);
          _code->movq_r_xmm(iReg, dReg);
          _code->push_r(iReg);

          dargs++;
          atype[nargs++] = VT_DOUBLE;
          break;

        default:
          break;
        }
      }

      if (iargs > 6 || dargs > 6) {
        ABORT("Too many arguments are passed");
      }

      _code->mov_r_imm(iamap[0], (uint64_t)NODE_INFO(node)->string);
      _code->mov_r_imm(RAX, dargs);
      for (int i = nargs - 1; i >= 0; i--) {
        switch (atype[i]) {
        case VT_INT:
          _code->pop_r(iamap[--iargs]);
          break;

        case VT_DOUBLE:
          _code->movq_xmm_m(--dargs, RSP, sizeof(double));
          _code->pop_r(iReg);
          break;

        default:
          ABORT("Not suppoted");
          break;
        }
      }

      _code->mov_r_imm(iReg, (uint64_t)&printf);
      _code->call_r(iReg);

      deallocReg(DOUBLE_REG_POOL);
      deallocReg(INT_REG_POOL);

      //_code->add_rm_imm(RSP, (argn + 1)*VAR_SIZE);
    }

    /*
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
      NativeFunction* f = NODE_INFO(node)->funRef;
      NativeCode* oldCode = _code;

      _code = f->code();

      _code->push_r(RBP);
      _code->mov_rr(RBP, RSP);
      _code->sub_rm_imm(RSP, f->localsNumber()*VAR_SIZE);

      node->visitChildren(this);

      _code->add_rm_imm(RSP, f->localsNumber()*VAR_SIZE);
      _code->pop_r(RBP);
      _code->add(RET);

      _code->link();

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
