#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>

#include <string.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {

  class BytecodeGenerator : public AstVisitor {
    class VarNum {
    public:
      void add(const AstVar* var) {
        vars.push_back(var);
        num_map.insert(make_pair(var, vars.size() - 1));
      }
      
      unsigned int getId(const AstVar* var) {
        return num_map.find(var)->second;
      }

      unsigned int size() {
        return num_map.size();
      }

    private:
      std::vector<const AstVar*> vars;
      std::map<const AstVar*, unsigned int> num_map;
    };

  private:
    Bytecode* code;
    AstNode* root;
    StringPool *strings;

    VarNum num;
    std::map<AstNode*, VarType> node_type;
    BytecodeInterpreter *interpreter;



    void put(const AstVar* v) {
      code->add((uint8_t)num.getId(v));
    }

    void put(unsigned char* buf, size_t size) {
      for(size_t i = 0; i < size; ++i) {
        code->add(buf[i]);
      }
    }


  public:
    BytecodeGenerator(AstNode* _root) { 
      root = _root;
      interpreter = new BytecodeInterpreter;
      code = interpreter->bytecode();
      strings = interpreter->strings();

      root->visit(this);

      interpreter->setVarPoolSize(num.size());
      
      /*
      for (size_t i = 0; i < code->length(); ++i) {
        printf("%02X ", code->get(i));
      }
      printf("\n");
      */
    }

    Code *getCode() {
      return interpreter;
    }


    // Visitor interface implementation 

    #define VISIT(type)                      \
      void visit##type(type* node)

    
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
        code->add(BC_SWAP);
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
      code->addInt16(3);

      code->add(BC_ILOAD0); // If the condition doesn't hold
      
      code->add(BC_JA);
      code->addInt16(1);
      
      code->add(BC_ILOAD1); // If the condition holds

      code->add(BC_POP);
      code->add(BC_POP);    // Remove arguments
      
      node_type[node] = VT_INT;       
    }

    void gen_logic(BinaryOpNode* node) {
      switch(node->kind()) {
      case tOR:
        code->add(BC_IADD);
        code->add(BC_ILOAD0);
        code->add(BC_SWAP);

        code->add(BC_IFICMPG);
        code->addInt16(3);
        
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

    VISIT(BinaryOpNode) {
      node->visitChildren(this);

      switch(node->kind()) {
      case tADD:
      case tSUB:
      case tMUL:
      case tDIV:
        gen_arith(node);
        break;

      case tEQ:
      case tNEQ:
      case tGT:
      case tLT:
      case tLE:          
        gen_comp(node);
        break;

      case tOR:
      case tAND:
        gen_logic(node);
        break;
        
      default:
        break;
      }

      if (node_type.find(node) == node_type.end()) {
        node_type[node] = node_type[node->left()];
      }
    }

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

        default:
          break;
        }
        break;

      default:
        break;
      }
    }

    VISIT(DoubleLiteralNode) {
      code->add(BC_DLOAD);
      double val = node->literal();
      code->addDouble(val);

      node_type[node] = VT_DOUBLE;
    }

    VISIT(IntLiteralNode) {
      code->add(BC_ILOAD);
      int64_t val = node->literal();
      code->addInt64(val);

      node_type[node] = VT_INT;
    }


    VISIT(StringLiteralNode) {
      strings->push_back(node->literal());

      code->add(BC_SLOAD);
      const char* s = strings->back().c_str();
      put((unsigned char*)&s, sizeof(char*));

      node_type[node] = VT_STRING;
    }
 

    VISIT(LoadNode) {
      switch (node->var()->type()) {
      case VT_DOUBLE:
        code->add(BC_LOADDVAR);
        break;

      case VT_INT:
        code->add(BC_LOADIVAR);
        break;

      case VT_STRING:
        code->add(BC_LOADSVAR);
        break;

      default:
        // Should never happen
        break;
      }

      node_type[node] = node->var()->type();
      put(node->var());
    }

    VISIT(StoreNode) {
      node->visitChildren(this);

      switch (node->var()->type()) {
      case VT_DOUBLE:
        code->add(BC_STOREDVAR);
        break;

      case VT_INT:
        code->add(BC_STOREIVAR);
        break;

      case VT_STRING:
        code->add(BC_STORESVAR);
        break;

      default:
        break;
      }

      node_type[node] = VT_INVALID;
      put(node->var());
    }
    
    VISIT(ForNode) {
      uint32_t jmp_pos;

      num.add(node->var());
      
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

      node->inExpr()->asBinaryOpNode()->right()->visit(this);      

      code->add(BC_LOADIVAR);
      put(node->var());

      code->add(BC_IFICMPNE);
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
        node->elseBlock()->visit(this);
      }

      code->setTyped(jmp_pos - 2, (int16_t)(code->current() - jmp_pos));
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

      code->setTyped(jmp_pos, (int16_t)code->current() - jmp_pos);
    }
    
    VISIT(BlockNode) {
      Scope::VarIterator vi(node->scope());
      AstVar *v;
      
      while (vi.hasNext()) {
        v = vi.next();    
        num.add(v);
      }

      node->visitChildren(this);
    }

    VISIT(FunctionNode) {
      // TODO
    }
    
    VISIT(PrintNode) {
      for (size_t i = 0; i < node->operands() - 1; i++) {
        node->operandAt(i)->visit(this);
        
        code->add(BC_DUMP);
      }
    }

#undef VISIT

  };

  // --------------------------------------------------------------------------------  

  class BytecodeTranslator : public Translator {
    class DummyCode : public Code {
    public:
      Status* execute(vector<Var*> vars) { 
        return 0;
      }
    };

  public:
    Status* translate(const string& program, Code** code) {
      Parser parser;

      Status *status = parser.parseProgram(program);
      if (!status) {
        BytecodeGenerator bg(parser.top());

        *code = bg.getCode();
      }

      return status;
    }
  };

  // --------------------------------------------------------------------------------

  Translator* Translator::create(const string& impl) {
    return new BytecodeTranslator;
  }  
}
