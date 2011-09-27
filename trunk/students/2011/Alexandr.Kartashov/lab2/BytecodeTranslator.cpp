#include <stdio.h>
#include <iostream>
#include <map>
#include <vector>

#include <string.h>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

  class BytecodeGenerator : public AstVisitor {
    class VarNum {
    public:
      void add(AstVar* var) {
        vars.push_back(var);
        num_map.insert(make_pair(var, vars.size() - 1));
      }
      
      unsigned int getId(const AstVar* var) {
        return num_map.find(var)->second;
      }

    private:
      std::vector<const AstVar*> vars;
      std::map<const AstVar*, unsigned int> num_map;
    };

  private:
    Bytecode code;
    AstNode* root;
    VarNum num;
    std::map<AstNode*, VarType> node_type;

    void put(const AstVar *v) {
      code.add((uint8_t)num.getId(v));
    }

    void put(unsigned char *buf, size_t size) {
      for(size_t i = 0; i < size; ++i) {
        code.add(buf[i]);
      }
    }


  public:
    BytecodeGenerator(AstNode* _root) { 
      root = _root;
      root->visit(this);

      for (size_t i = 0; i < code.length(); ++i) {
        printf("%02X ", code.get(i));
      }
      printf("\n");
    }


    // Visitor interface implementation 

    #define VISIT(type)                      \
      void visit##type(type* node)


    VISIT(BinaryOpNode) {
      node->visitChildren(this);

      switch (node_type[node->left()]) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tADD:
          code.add(BC_DADD);
          break;

        case tSUB:
          code.add(BC_DSUB);
          break;

        case tMUL:
          code.add(BC_DMUL);
          break;

        case tDIV:
          code.add(BC_DDIV);

          // Conditions?..

        default:
          // ...
          break;
        }
        break;
        

      case VT_INT:  
        switch (node->kind()) {
        case tADD:
          code.add(BC_IADD);
          break;

        case tSUB:
          code.add(BC_ISUB);
          break;

        case tMUL:
          code.add(BC_IMUL);
          break;

        case tDIV:
          code.add(BC_IDIV);

        default:
          // ...
          break;
        }
        break;
        
      default:
        break;
      }

      node_type[node] = node_type[node->left()];
    }

    VISIT(UnaryOpNode) {
      node->operand()->visit(this); 

      switch (node_type[node->operand()]) {
      case VT_DOUBLE:
        switch (node->kind()) {
        case tSUB:
          code.add(BC_DNEG);
          break;

        default:
          break;
        }
        break;

      case VT_INT:
        switch (node->kind()) {
        case tSUB:
          code.add(BC_DNEG);
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
      code.add(BC_DLOAD);
      double val = node->literal();
      code.addDouble(val);

      node_type[node] = VT_DOUBLE;
    }

    VISIT(IntLiteralNode) {
      code.add(BC_ILOAD);
      int64_t val = node->literal();
      code.addInt64(val);

      node_type[node] = VT_INT;
    }


    VISIT(StringLiteralNode) {
      code.add(BC_SLOAD);
      const char* s = node->literal().c_str();
      put((unsigned char*)&s, sizeof(char*));

      node_type[node] = VT_STRING;
    }
 

    VISIT(LoadNode) {
      switch (node->var()->type()) {
      case VT_DOUBLE:
        code.add(BC_LOADDVAR);
        break;

      case VT_INT:
        code.add(BC_LOADIVAR);
        break;

      case VT_STRING:
        code.add(BC_LOADSVAR);
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
        code.add(BC_STOREDVAR);
        break;

      case VT_INT:
        code.add(BC_STOREIVAR);
        break;

      case VT_STRING:
        code.add(BC_STORESVAR);
        break;

      default:
        break;
      }

      node_type[node] = VT_INVALID;
      put(node->var());
    }
    
    VISIT(ForNode) {
      // TODO
    }
  
    VISIT(IfNode) {
      // TODO
    }

    VISIT(WhileNode) {
      // TODO
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
      // TODO
    }

#undef VISIT

  };

  // --------------------------------------------------------------------------------  

  class BytecodeTranslator : public  Translator {
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

        *code = new DummyCode;
      }

      return status;
    }
  };

  // --------------------------------------------------------------------------------

  Translator* Translator::create(const string& impl) {
    return new BytecodeTranslator;
  }  
}
