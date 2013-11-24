#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tr1/memory>

#include "parser.h"

#include "bytecode_translator.h"

using namespace mathvm;

class translator_exception : public std::logic_error {
   uint32_t _position;
public:
   translator_exception(std::string const& reason, uint32_t position):
      std::logic_error(reason), _position(position) { }
   uint32_t position() const { return _position; }
   static translator_exception no_such_variable(AstNode* node, std::string const& name) {
      return translator_exception("Variable is not in scope: " + name, node->position());
   }
   static translator_exception variable_count_overflow(AstNode* node) {
      return translator_exception("Too much variables. Couldn't hope to count",
            node->position());
   }
   static translator_exception function_not_found(AstNode* node, std::string const& name) {
      return translator_exception("Function is not in scope: " + name, node->position());
   }
   static translator_exception unknown_operator(AstNode* node, TokenKind op) {
      return translator_exception(std::string("Unknown operator: ") +
            tokenStr(op), node->position());
   }
   static translator_exception internal_error(AstNode* node, std::string const& desc) {
      return translator_exception("IMPOSSIBRU: " + desc, node->position());
   }
   static translator_exception syntax_error(AstNode* node, std::string const& desc) {
      return translator_exception("Syntax error: " + desc, node->position());
   }
   static translator_exception function_invalid_signature(AstNode* node,
         std::string const& name, int arg_count, int expected_arg_count) {
      // I would rather have used std::to_string
      std::stringstream text;
      text << "Call error for " << name << ": expected "
           << arg_count << " got " << expected_arg_count << " arguments";
      return translator_exception(text.str(), node->position());
   }
   // I would rather have done it using variadic templates
   static translator_exception type_error(AstNode* node, VarType type,
         VarType expected_type) {
      return translator_exception(std::string("Type error: got ") + typeToName(type) +
         " expected " + typeToName(expected_type), node->position());
   }
   static translator_exception type_error(AstNode* node, VarType type,
         VarType expected_type1, VarType expected_type2) {
      return translator_exception(std::string("Type error: got ") + typeToName(type) +
         " expected one of " + typeToName(expected_type1) + ", " +
         typeToName(expected_type2), node->position());
   }
   static translator_exception type_error(AstNode* node, VarType type,
         VarType expected_type1, VarType expected_type2, VarType expected_type3) {
      return translator_exception(std::string("Type error: got ") + typeToName(type) +
         " expected one of " + typeToName(expected_type1) + ", " +
         typeToName(expected_type2) + ", " + typeToName(expected_type3),
         node->position());
   }
   static void assert_type(AstNode* node, VarType type, VarType expected_type) {
      if(type != expected_type)
         throw translator_exception::type_error(node, type, expected_type);
   }
   static void assert_type(AstNode* node, VarType type, VarType expected_type1,
         VarType expected_type2) {
      if(type != expected_type1 && type != expected_type2)
         throw translator_exception::type_error(node, type, expected_type1,
               expected_type2);
   }
   static void assert_type(AstNode* node, VarType type, VarType expected_type1,
         VarType expected_type2, VarType expected_type3) {
      if(type != expected_type1 && type != expected_type2 && type != expected_type3)
         throw translator_exception::type_error(node, type, expected_type1,
               expected_type2, expected_type3);
   }
};

struct variable_t {
   VarType type;
   std::string name;
   uint16_t id;
   uint16_t context_id;
};

struct function_t {
   Signature signature;
   Bytecode* ip;
   uint16_t id;
   bool is_native;
};

struct varmap_t {
   typedef std::map<std::string, std::vector<variable_t> >::iterator iterator;
   typedef std::map<std::string, std::vector<variable_t> >::const_iterator const_iterator;
   varmap_t() : max_id(new uint16_t(0)) { }
   std::map<std::string, std::vector<variable_t> > map;
   std::tr1::shared_ptr<uint16_t> max_id;
};

typedef std::map<std::string, function_t> funmap_t;

variable_t introduce_var(AstNode* node, varmap_t& varmap, uint16_t context,
      std::string const& name, VarType type, Bytecode* ip) {
   if(uint16_t(*varmap.max_id + 1) < *varmap.max_id)
      throw translator_exception::variable_count_overflow(node);
   variable_t val = { type, name, (*varmap.max_id)++ , context };
   varmap.map[name].push_back(val);
   loggerstr << "Introducing var " << name
             << " of type " << typeToName(type)
             << " with id " << val.id
             << std::endl;
   //ip->addInsn(BC_ILOAD0);
   //ip->addInsn(BC_STORECTXIVAR);
   //ip->addUInt16(context);
   //ip->addUInt16(val.id);
   return val;
}

variable_t introduce_context(AstNode* node, varmap_t& varmap, Bytecode* ip)  {
   if(uint16_t(*varmap.max_id + 1) < *varmap.max_id)
      throw translator_exception::variable_count_overflow(node);
   variable_t context = { VT_INT, "@context", (*varmap.max_id)++ , 0 };
   varmap.map["@context"].push_back(context);
   loggerstr << "Introducing @context with id " << context.id << std::endl;
   ip->addInsn(BC_ILOAD0);
   ip->addInsn(BC_STOREIVAR);
   ip->addUInt16(context.id);
   return context;
}

variable_t introduce_var(AstNode* node, varmap_t& varmap, uint16_t context, AstVar* var,
      Bytecode* ip) {
   return introduce_var(node, varmap, context, var->name(), var->type(), ip);
}

void pop_var(AstNode* node, varmap_t& varmap, variable_t const& v) {
   loggerstr << "Popping " << v.name << std::endl;;
   varmap_t::iterator mit = varmap.map.find(v.name);
   if(mit == varmap.map.end())
      throw translator_exception::internal_error(node, "Can't find variable to pop");
   loggerstr << "Found variable to pop" << std::endl;
   for(std::vector<variable_t>::iterator vit = mit->second.begin();
         vit != mit->second.end(); ++vit) {
      if(vit->context_id != v.context_id) continue;
      mit->second.erase(vit);
      return;
   }
   loggerstr << "Haven't found contexted variable to pop" << std::endl;
   throw translator_exception::internal_error(node, "Can't find variable to pop");
}

variable_t find_var(AstNode* node, varmap_t const& varmap, std::string const& name) {
   varmap_t::const_iterator mit = varmap.map.find(name);
   if(mit == varmap.map.end())
      throw translator_exception::no_such_variable(node, name);
   for(std::vector<variable_t>::const_reverse_iterator vit = mit->second.rbegin();
         vit != mit->second.rend(); ++vit) {
      return *vit;
   }
   throw translator_exception::no_such_variable(node, name);
}

function_t find_function(AstNode* node, funmap_t const& funmap, std::string const& name) {
   funmap_t::const_iterator it = funmap.find(name);
   if(it == funmap.end())
      throw translator_exception::function_not_found(node, name);
   return it->second;
}

void increment_context(Bytecode* ip, uint16_t context) {
   ip->addInsn(BC_LOADIVAR);
   ip->addUInt16(context);
   ip->addInsn(BC_ILOAD1);
   ip->addInsn(BC_IADD);
   ip->addInsn(BC_STOREIVAR);
   ip->addUInt16(context);
}

void decrement_context(Bytecode* ip, uint16_t context) {
   ip->addInsn(BC_LOADIVAR);
   ip->addUInt16(context);
   ip->addInsn(BC_ILOADM1);
   ip->addInsn(BC_IADD);
   ip->addInsn(BC_STOREIVAR);
   ip->addUInt16(context);
}

void store_var(AstNode* node, Bytecode* ip, variable_t v) {
   loggerstr << "Storing var " 
             << " of type " << typeToName(v.type)
             << " with id " << v.id
             << std::endl;
   switch(v.type) {
      case VT_DOUBLE: ip->addInsn(BC_STORECTXDVAR); break;
      case VT_INT: ip->addInsn(BC_STORECTXIVAR); break;
      case VT_STRING: ip->addInsn(BC_STORECTXSVAR); break;
      default: throw translator_exception::type_error(node, v.type,
                     VT_DOUBLE, VT_INT, VT_STRING);
   }
   ip->addUInt16(v.context_id);
   ip->addUInt16(v.id);
}

void load_var(AstNode* node, Bytecode* ip, variable_t v) {
   loggerstr << "Loading var " 
             << " of type " << typeToName(v.type)
             << " with id " << v.id
             << std::endl;
   switch(v.type) {
      case VT_DOUBLE: ip->addInsn(BC_LOADCTXDVAR); break;
      case VT_INT: ip->addInsn(BC_LOADCTXIVAR); break;
      case VT_STRING: ip->addInsn(BC_LOADCTXSVAR); break;
      default: throw translator_exception::type_error(node, v.type,
                     VT_DOUBLE, VT_INT, VT_STRING);
   }
   ip->addUInt16(v.context_id);
   ip->addUInt16(v.id);
}

VarType convert_int_to_double(AstNode* node, Bytecode* ip,
      VarType left_type, VarType right_type) {
   translator_exception::assert_type(node, left_type, VT_INT, VT_DOUBLE);
   translator_exception::assert_type(node, right_type, VT_INT, VT_DOUBLE);
   if(left_type == VT_INT && right_type == VT_INT) return VT_INT;
   if(left_type == VT_DOUBLE && right_type == VT_DOUBLE) return VT_DOUBLE;
   if(left_type == VT_INT && right_type == VT_DOUBLE) {
      ip->addInsn(BC_SWAP);
      ip->addInsn(BC_I2D);
      ip->addInsn(BC_SWAP);
      return VT_DOUBLE;
   }
   if(left_type == VT_DOUBLE && right_type == VT_INT) {
      ip->addInsn(BC_I2D);
      return VT_DOUBLE;
   }
   throw translator_exception::internal_error(node, "subexpressions have wrong types");
}

class BytecodeBlockTranslator : public AstVisitor {
   BytecodeImpl* _code;
   Bytecode* _ip;
   // Special variables are starting with '@'
   varmap_t _varmap;
   funmap_t _funmap;
   VarType _expected_return;
   VarType _last_expr_type;
   uint16_t _context;
public:
   void run(BytecodeImpl* code, Bytecode* ip, varmap_t& varmap, funmap_t& funmap,
         VarType expected_return, uint16_t context, BlockNode* node) {
      loggerstr << "BlockTranslator run " << std::endl;
      _code = code;
      _ip = ip;
      _varmap = varmap;
      _funmap = funmap;
      _expected_return = expected_return;
      _last_expr_type = VT_INVALID;
      _context = context;
      std::vector<variable_t> locals;
      Scope::VarIterator v(node->scope());
      while(v.hasNext())
         locals.push_back(introduce_var(node, _varmap, _context, v.next(), ip));
      Scope::FunctionIterator f (node->scope());
      while(f.hasNext()) {
         AstFunction* fun = f.next();
         function_t val = { fun->node()->signature(), NULL, 0, false };
         if(fun->node()->body()->nodeAt(0)->isNativeCallNode()) {
            NativeCallNode* n = (NativeCallNode*)fun->node()->body()->nodeAt(0);
            val.id = _code->makeNativeFunction(n->nativeName(),
                  n->nativeSignature(), NULL);
            val.is_native = true;
         } else {
            BytecodeFunction* bfun = new BytecodeFunction(fun);
            val.id = _code->addFunction(bfun);
            val.ip = bfun->bytecode();
         }
         _funmap[fun->node()->name()] = val;
      }
      // Now we have all the context in scope, time to
      // generate code for our functions.
      f = Scope::FunctionIterator(node->scope());
      while(f.hasNext()) f.next()->node()->visit(this);
      for(uint32_t i = 0; i < node->nodes(); ++i) node->nodeAt(i)->visit(this);
      // And now to go to previous scope
      loggerstr << "run going to previous scope" << std::endl;
      for(std::vector<variable_t>::iterator it = locals.begin();
            it != locals.end(); ++it) pop_var(node, _varmap, *it);
   }

   virtual void visitBinaryOpNode(BinaryOpNode* node) {
      loggerstr << "BinaryOpNode " << tokenStr(node->kind()) << std::endl;
      node->left()->visit(this);
      VarType left_type = _last_expr_type;
      node->right()->visit(this);
      VarType right_type = _last_expr_type;
      // TODO: possible micro-optimization: add VT_BOOL phantom type and
      // remove all that jumps.
      switch(node->kind()) {
         case tOR:
         case tAND: {
            translator_exception::assert_type(node, left_type, VT_INT);
            translator_exception::assert_type(node, right_type, VT_INT);
            switch(node->kind()) {
               case tOR: _ip->addInsn(BC_IADD); break;
               case tAND: _ip->addInsn(BC_IMUL); break;
               default: throw translator_exception::internal_error(node,
                              "operator of wrong kind");
            }
            Label else_if(_ip);
            Label end_if(_ip);
            _ip->addInsn(BC_ILOAD0);
            _ip->addBranch(BC_IFICMPE, else_if);
            _ip->addInsn(BC_ILOAD1);
            _ip->addBranch(BC_JA, end_if);
            _ip->bind(else_if);
            _ip->addInsn(BC_ILOAD0);
            _ip->bind(end_if);
            _last_expr_type = VT_INT;
            break;
         }
         case tMOD:
         case tAOR:
         case tAXOR:
         case tAAND: {
            translator_exception::assert_type(node, left_type, VT_INT);
            translator_exception::assert_type(node, right_type, VT_INT);
            switch(node->kind()) {
               case tMOD: _ip->addInsn(BC_IMOD); break;
               case tAOR: _ip->addInsn(BC_IAOR); break;
               case tAXOR: _ip->addInsn(BC_IAXOR); break;
               case tAAND: _ip->addInsn(BC_IAAND); break;
               default: throw translator_exception::internal_error(node,
                              "operator of wrong kind");
            }
            _last_expr_type = VT_INT;
            break;
         }
         case tEQ:
         case tNEQ:
         case tGT:
         case tGE:
         case tLT:
         case tLE: {
            VarType type = convert_int_to_double(node, _ip, left_type, right_type);
            if(type == VT_DOUBLE) {
               _ip->addInsn(BC_DCMP);
               _ip->addInsn(BC_ILOAD0);
            }
            Label else_if(_ip);
            Label end_if(_ip);
            switch(node->kind()) {
               case tEQ: _ip->addBranch(BC_IFICMPE, else_if); break;
               case tNEQ: _ip->addBranch(BC_IFICMPNE, else_if); break;
               case tGT: _ip->addBranch(BC_IFICMPG, else_if); break;
               case tGE: _ip->addBranch(BC_IFICMPGE, else_if); break;
               case tLT: _ip->addBranch(BC_IFICMPL, else_if); break;
               case tLE: _ip->addBranch(BC_IFICMPLE, else_if); break;
               default: throw translator_exception::internal_error(node,
                              "operator of wrong kind");
            }
            _ip->addInsn(BC_ILOAD0);
            _ip->addBranch(BC_JA, end_if);
            _ip->bind(else_if);
            _ip->addInsn(BC_ILOAD1);
            _ip->bind(end_if);
            _last_expr_type = VT_INT;
            break;
         }
         case tADD:
         case tSUB:
         case tMUL:
         case tDIV: {
            VarType type = convert_int_to_double(node, _ip, left_type, right_type);
            switch(node->kind()) {
               case tADD: _ip->addInsn(type == VT_DOUBLE ? BC_DADD : BC_IADD); break;
               case tSUB: _ip->addInsn(type == VT_DOUBLE ? BC_DSUB : BC_ISUB); break;
               case tMUL: _ip->addInsn(type == VT_DOUBLE ? BC_DMUL : BC_IMUL); break;
               case tDIV: _ip->addInsn(type == VT_DOUBLE ? BC_DDIV : BC_IDIV); break;
               default: throw translator_exception::internal_error(node,
                              "operator of wrong kind");
            }
            _last_expr_type = type;
            break;
         }
         case tINCRSET:
         case tDECRSET: {
            if(!(node->left()->isLoadNode()))
               throw translator_exception::syntax_error(node,
                     "expecting variable on the left hand side of increment/decrement");
            LoadNode* var_node = (LoadNode*)node->left();
            variable_t v = find_var(node, _varmap, var_node->var()->name());
            translator_exception::assert_type(node, left_type, VT_INT, VT_DOUBLE);
            if(left_type == VT_INT && right_type != VT_INT)
               translator_exception::type_error(node, right_type, VT_INT);
            if(left_type == VT_DOUBLE) {
               if(right_type == VT_INT)
                  _ip->addInsn(BC_I2D);
               else if(right_type != VT_DOUBLE)
                  translator_exception::type_error(node, right_type, VT_INT, VT_DOUBLE);
            }
            switch(node->kind()) {
               case tINCRSET: _ip->addInsn(left_type == VT_DOUBLE ? BC_DADD : BC_IADD); break;
               case tDECRSET: _ip->addInsn(left_type == VT_DOUBLE ? BC_DSUB : BC_ISUB); break;
               default: throw translator_exception::internal_error(node,
                              "operator of wrong kind");
            }
            store_var(node, _ip, v);
            load_var(node, _ip, v);
            _last_expr_type = left_type;
            break;
         }
         default:
            throw translator_exception::unknown_operator(node, node->kind());
      }
   }

   virtual void visitUnaryOpNode(UnaryOpNode* node) {
      loggerstr << "UnaryOpNode " << tokenStr(node->kind()) << std::endl;
      node->operand()->visit(this);
      VarType type = _last_expr_type;
      switch(node->kind()) {
         case tNOT: {
            translator_exception::assert_type(node, type, VT_INT);
            Label end_if(_ip);
            Label else_if(_ip);
            _ip->addInsn(BC_ILOAD0);
            _ip->addBranch(BC_IFICMPE, else_if);
            _ip->addInsn(BC_ILOAD0);
            _ip->addBranch(BC_JA, end_if);
            _ip->bind(else_if);
            _ip->addInsn(BC_ILOAD1);
            _ip->bind(end_if);
            break;
         }
         case tSUB: {
            translator_exception::assert_type(node, type, VT_INT, VT_DOUBLE);
            _ip->addInsn(type == VT_DOUBLE ? BC_DNEG : BC_INEG);
            break;
         }
         default:
            throw translator_exception::unknown_operator(node, node->kind());
      }
   }

   virtual void visitStringLiteralNode(StringLiteralNode* node) {
      loggerstr << "StringLiteralNode " << node->literal() << std::endl;
      uint16_t node_const = _code->makeStringConstant(node->literal());
      _ip->addInsn(BC_SLOAD);
      _ip->addUInt16(node_const);
      _last_expr_type = VT_STRING;
   }

   virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      loggerstr << "DoubleLiteralNode " << node->literal() << std::endl;
      _ip->addInsn(BC_DLOAD);
      _ip->addDouble(node->literal());
      _last_expr_type = VT_DOUBLE;
   }

   virtual void visitIntLiteralNode(IntLiteralNode* node) {
      loggerstr << "IntLiteralNode " << node->literal() << std::endl;
      _ip->addInsn(BC_ILOAD);
      _ip->addInt64(node->literal());
      _last_expr_type = VT_INT;
   
   }

   virtual void visitLoadNode(LoadNode* node) {
      loggerstr << "LoadNode " << node->var()->name()
                << " of type " << typeToName(node->var()->type())
                << std::endl;
      variable_t v = find_var(node, _varmap, node->var()->name());
      load_var(node, _ip, v);
      _last_expr_type = node->var()->type();
   }

   virtual void visitCallNode(CallNode* node) {
      loggerstr << "CallNode " << node->name() << std::endl;
      function_t f = find_function(node, _funmap, node->name());
      if(f.signature.size() - 1 != node->parametersNumber())
         throw translator_exception::function_invalid_signature(node, node->name(),
               node->parametersNumber(), f.signature.size() - 1);
      for(int i = node->parametersNumber() - 1; i >= 0; --i) {
         node->parameterAt(i)->visit(this);
         translator_exception::assert_type(node, _last_expr_type,
               f.signature[i + 1].first);
      }
      _ip->addInsn(f.is_native ? BC_CALLNATIVE : BC_CALL);
      _ip->addUInt16(f.id);
      // TODO: When in statement use it to keep stack balanced
      //if(f.signature[0].first != VT_VOID) _ip->addInsn(BC_POP); // Ignoring return value
      _last_expr_type = f.signature[0].first;
   }

   virtual void visitStoreNode(StoreNode* node) {
      loggerstr << "StoreNode " << node->var()->name() 
                << " of type " << typeToName(node->var()->type())
                << std::endl;
      variable_t v = find_var(node, _varmap, node->var()->name());
      if(node->op() == tINCRSET || node->op() == tDECRSET)
         load_var(node, _ip, v);
      translate_expression(node, node->value(), v.type);
      switch(node->op()) {
         case tASSIGN: break;
         case tINCRSET:
            _ip->addInsn(v.type == VT_DOUBLE ? BC_DADD : BC_IADD);
            break;
         case tDECRSET:
            _ip->addInsn(v.type == VT_DOUBLE ? BC_DSUB : BC_ISUB);
            break;
         default:
            throw translator_exception::unknown_operator(node, node->op());
      }
      store_var(node, _ip, v);
   }

   virtual void visitForNode(ForNode* node) {
      loggerstr << "ForNode " << std::endl;
      variable_t i_var = find_var(node, _varmap, node->var()->name());
      variable_t lo_var = introduce_var(node, _varmap, _context, "%lo", VT_INT, _ip);
      variable_t hi_var = introduce_var(node, _varmap, _context, "%hi", VT_INT, _ip);
      // Checking that expression is of form lo..hi
      if(!node->inExpr()->isBinaryOpNode())
         throw translator_exception::syntax_error(node, "expected range in for expression");
      BinaryOpNode* expr = (BinaryOpNode*)node->inExpr();
      if(expr->kind() != tRANGE)
         throw translator_exception::syntax_error(node, "expected range in for expression");
      AstNode* lo = expr->left();
      AstNode* hi = expr->right();
      // CODE: %lo = lo
      translate_expression(node, lo, VT_INT);
      store_var(node, _ip, lo_var);
      // CODE: %hi = hi
      translate_expression(node, hi, VT_INT);
      store_var(node, _ip, hi_var);
      // CODE: i = %lo
      load_var(node, _ip, lo_var);
      store_var(node, _ip, i_var);
      // CODE: loop_start:
      Label loop_start = _ip->currentLabel();
      Label loop_fin(_ip);
      // CODE: if (i > %hi) goto loop_fin 
      load_var(node, _ip, i_var);
      load_var(node, _ip, hi_var);
      _ip->addBranch(BC_IFICMPG, loop_fin);
      // CODE: body
      BytecodeBlockTranslator().run(_code, _ip, _varmap, _funmap, _expected_return,
            _context, node->body());
      // CODE: i = i + 1
      load_var(node, _ip, i_var);
      _ip->addInsn(BC_ILOAD1);
      _ip->addInsn(BC_IADD);
      store_var(node, _ip, i_var);
      // CODE: goto loop_start
      _ip->addBranch(BC_JA, loop_start);
      // CODE: loop_fin:
      _ip->bind(loop_fin);
      pop_var(node, _varmap, lo_var);
      pop_var(node, _varmap, hi_var);
   }

   virtual void visitWhileNode(WhileNode* node) {
      loggerstr << "WhileNode " << std::endl;
      // CODE: loop_start:
      Label loop_start = _ip->currentLabel();
      Label loop_fin(_ip);
      // CODE: if e = 0 goto loop_fin
      translate_expression(node, node->whileExpr(), VT_INT);
      _ip->addInsn(BC_ILOAD0);
      _ip->addBranch(BC_IFICMPE, loop_fin);
      // CODE: body
      node->loopBlock()->visit(this);
      // CODE: goto loop_start
      _ip->addBranch(BC_JA, loop_start);
      // CODE: loop_fin:
      _ip->bind(loop_fin);
   }

   virtual void visitIfNode(IfNode* node) {
      loggerstr << "IfNode " << std::endl;
      Label else_branch(_ip);
      Label if_fin(_ip);
      // CODE: if e = 0 goto else_branch
      translate_expression(node, node->ifExpr(), VT_INT);
      _ip->addInsn(BC_ILOAD0);
      _ip->addBranch(BC_IFICMPE, else_branch);
      // CODE: then_body
      node->thenBlock()->visit(this);
      // CODE: goto if_fin
      _ip->addBranch(BC_JA, if_fin);
      // CODE: else_branch:
      _ip->bind(else_branch);
      // CODE: else_body
      if(node->elseBlock())
         node->elseBlock()->visit(this);
      // CODE: if_fin:
      _ip->bind(if_fin);
   }

   virtual void visitBlockNode(BlockNode* node) {
      loggerstr << "BlockNode " << std::endl;
      BytecodeBlockTranslator().run(_code, _ip, _varmap, _funmap,
            _expected_return, _context, node);
   }

   virtual void visitFunctionNode(FunctionNode* node) {
      loggerstr << "FunctionNode " << std::endl;
      function_t& f = _funmap[node->name()];
      if(f.is_native) return;
      variable_t context = introduce_context(node, _varmap, _ip);
      increment_context(f.ip, context.id);
      for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
         variable_t v = introduce_var(node, _varmap, context.id, node->parameterName(i),
               node->parameterType(i), _ip);
         store_var(node, f.ip, v);
      }
      BytecodeBlockTranslator().run(_code, f.ip, _varmap, _funmap,
            node->returnType(), context.id, node->body());
   }

   virtual void visitReturnNode(ReturnNode* node) {
      loggerstr << "ReturnNode " << std::endl;
      if(node->returnExpr())
         translate_expression(node, node->returnExpr(), _expected_return);
      else if(_expected_return != VT_VOID)
         throw translator_exception::type_error(node, VT_VOID, _expected_return);
      decrement_context(_ip, _context);
      _ip->addInsn(BC_RETURN);
   }

   virtual void visitNativeCallNode(NativeCallNode* node) {
      // This case is unfortunately handled inside run.
      loggerstr << "NativeCallNode" << std::endl;
      throw translator_exception::internal_error(node,
            "NativeCallNode handling should have been already performed");
   }

   virtual void visitPrintNode(PrintNode* node) {
      loggerstr << "PrintCallNode" << std::endl;
      for(uint32_t i = 0; i < node->operands(); ++i) {
         node->operandAt(i)->visit(this);
         switch(_last_expr_type) {
            case VT_DOUBLE: _ip->addInsn(BC_DPRINT); break;
            case VT_INT: _ip->addInsn(BC_IPRINT); break;
            case VT_STRING: _ip->addInsn(BC_SPRINT); break;
            default: throw translator_exception::type_error(node, _last_expr_type,
                           VT_DOUBLE, VT_INT, VT_STRING);
         }
      }
   }
private:
   void translate_expression(AstNode* node, AstNode* expr, VarType expected_type) {
      expr->visit(this);
      translator_exception::assert_type(node, _last_expr_type, expected_type);
   }
};

Status* BytecodeTranslatorImpl::translate(std::string const& program, Code **code) {
   Parser parser;
   if(Status* s = parser.parseProgram(program)) return s;
   BytecodeImpl* bcode = new BytecodeImpl();
   *code = bcode;
   varmap_t varmap;
   variable_t context = introduce_context(parser.top()->node()->body(), varmap, bcode->bytecode());
   funmap_t funmap;
   try {
      BytecodeBlockTranslator().run(bcode, bcode->bytecode(), varmap, funmap,
            VT_VOID, context.id, parser.top()->node()->body());
      bcode->bytecode()->addInsn(BC_STOP);
   } catch(translator_exception e) {
      return new Status(e.what(), e.position());
   }
   return NULL;
}

void BytecodeImpl::disassemble(ostream& out, FunctionFilter* filter) {
   Code::disassemble(out, filter);
   _bytecode->dump(out);
}
