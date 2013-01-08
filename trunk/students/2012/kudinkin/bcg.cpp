
#include "bcg.h"

#include "parser.h"

namespace mathvm {

  
  /**
    * ...
    */

  Status* BytecodeTranslator::translate(const string& source, Code **code) {

    // Build AST IR

    Parser parser;

    Status *status = parser.parseProgram(source);
    
    if (status != 0) {

      if (!status->isOk()) {
        return status;
      }

      // Free memory occupied by the status-descriptor
      delete status;

    }
    
    //
    // TRANSLATE TOP-SCOPE BLOCK
    // 

    // Retrieve top-scope block

    AstFunction *top = parser.top();

    // Run ``translating'' visitor upon retrieved AST
    
    BytecodeTranslatingVisitor btv(top->scope());
    
    top->node()->visit(&btv);
    
    // Translation finished, return

    *code = btv.dump();

    return new Status();

  }

  
  //
  // TRANSLATING VISITOR
  //
  
  BytecodeTranslator::BytecodeTranslatingVisitor::BytecodeTranslatingVisitor(Scope *outtermost_scope)
    : bce_(new bcode_exec_t)
  {

    BytecodeFunction *top_func_p = new BytecodeFunction(outtermost_scope->lookupFunction("<top>"));

    // Store ``<top>'' function in code-segment
    bce_->addFunction(top_func_p);
    
  }

  
  /**
    * ...
    */

  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitBinaryOpNode(BinaryOpNode *node)
  {

      // Traverse LOP
      node->left()->visit(this);
      
      // Traverse ROP
      node->right()->visit(this);
      
      // Swap results
      cur_fbcs().addInsn(BC_SWAP);
      
      // Up to this point both operands would be pushed on stack, and
      // LOP would be on TOS, i.e.:
      //
      //    LOP   |
      //    ROP   | STACK
      //    ...   |
      
      // Branch upon particular kind of operation ought to be executed
      
      // NOTE:
      //  Since MVM-language doesn't contain boolean type all boolean-like
      //  expressions would be converted to integer implicitly
      
          
      switch (node->kind()) {

        case tOR: {
          
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          cur_fbcs().addInsn(BC_ILOAD0);     // if FALSE < LOP jump to true-path label
          cur_fbcs().addBranch(BC_IFICMPL, tp_l);

          cur_fbcs().addInsn(BC_ILOAD0);     // if FALSE < ROP jump to true-path label
          cur_fbcs().addBranch(BC_IFICMPL, tp_l);

          cur_fbcs().addInsn(BC_ILOAD0);     // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l); // Jump to false-path label
          

          cur_fbcs().bind(tp_l);             // Bind true-path label
          
          cur_fbcs().addInsn(BC_ILOAD1);     // TRUE
          
          cur_fbcs().bind(fp_l);             // Bind false-path label

          break;
          
        }
          
        case tAND: {

          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          cur_fbcs().addInsn(BC_ILOAD1);     // if TRUE > LOP jump to false-path label
          cur_fbcs().addBranch(BC_IFICMPG, fp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);     // if TRUE > ROP jump to false-path label
          cur_fbcs().addBranch(BC_IFICMPG, fp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);     // TRUE
          cur_fbcs().addBranch(BC_JA, tp_l); // Jump to true-path label
          
          
          cur_fbcs().bind(fp_l);             // Bind false-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);     // FALSE
          
          cur_fbcs().bind(tp_l);             // Bind true-path label
          
          break;
          
        }
        
        case tEQ: {
          
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPE, tp_l);  // if LOP == ROP jump to true-path label

          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);

          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
        
        case tNEQ: {
         
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPNE, tp_l); // if LOP != ROP jump to true-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
          
        case tGT: {
          
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPL, tp_l);  // if LOP > ROP jump to true-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
        
        case tGE: {
          
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPLE, tp_l); // if LOP >= ROP jump to true-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
          
        case tLT: {
          
          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPG, tp_l);  // if LOP < ROP jump to true-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
          
        case tLE: {

          Label tp_l(&cur_fbcs()), fp_l(&cur_fbcs());
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of comparands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DCMP);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ICMP);
              break;
            default:
              assert(false);  // FIXME
          }
          
          //
          // Up to this point libc-style comparison result (integer) would be pushed on TOS
          //
          
          cur_fbcs().addInsn(BC_ILOAD0);           // Load 0 on TOS
          cur_fbcs().addBranch(BC_IFICMPGE, tp_l);  // if LOP <= ROP jump to true-path label
          
          cur_fbcs().addInsn(BC_ILOAD0);           // FALSE
          cur_fbcs().addBranch(BC_JA, fp_l);
          
          cur_fbcs().bind(tp_l);
          
          cur_fbcs().addInsn(BC_ILOAD1);           // TRUE
          
          cur_fbcs().bind(fp_l);
          
          break;
          
        }
          
        case tRANGE: {

          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          break;
          
        }
          
        case tADD: {
          
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of operands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DADD);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_IADD);
              break;
            default:
              assert(false);  // FIXME
          }

          break;

        }
          
        case tSUB: {
        
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of operands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DSUB);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ISUB);
              break;
            default:
              assert(false);  // FIXME
          }
          
          break;
          
        }
          
        case tMUL: {
        
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of operands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DMUL);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_IMUL);
              break;
            default:
              assert(false);  // FIXME
          }
          
          break;
          
        }
          
        case tDIV: {
        
          VarType most_common_type;
          
          // Place type-coercing prologue, if any required
          
          _insert_coercing_prologue(node->left(), node->right(), bce_.get(), &cur_fbcs(), &most_common_type);
          
          
          // Branch upon particular type of operands
          
          switch (most_common_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DDIV);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_IDIV);
              break;
            default:
              assert(false);  // FIXME
          }
          
          break;
          
        }
          
        case tMOD: {
          cur_fbcs().addInsn(BC_IMOD);
          break;
        }
       
        default: {
          cur_fbcs().addInsn(BC_INVALID);   // OR, POSSIBLY, BREAK CG?
          break;
        }
          
      }

  }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitUnaryOpNode(UnaryOpNode *node)
  {

      // Infer operand type
      VarType op_type = infer_type_for(node);
    
      assert(op_type == VT_DOUBLE || op_type == VT_INT);
    
      node->operand()->visit(this);
      
      switch (node->kind()) {
        
        case tNOT:                      // NOTE:  Only expressions of integer-typ
                                        //        could be logically NEGATED

          cur_fbcs().addInsn(BC_ILOAD1);
          cur_fbcs().addInsn(BC_IADD);
          cur_fbcs().addInsn(BC_ILOAD);
          cur_fbcs().addInt16(2);
          cur_fbcs().addInsn(BC_SWAP);
          cur_fbcs().addInsn(BC_IMOD);
          break;

        case tSUB:
          switch (op_type) {
            case VT_DOUBLE:
              cur_fbcs().addInsn(BC_DLOADM1);
              cur_fbcs().addInsn(BC_DMUL);
              break;
            case VT_INT:
              cur_fbcs().addInsn(BC_ILOADM1);
              cur_fbcs().addInsn(BC_IMUL);
              break;
          }
          break;
          
        default:
          cur_fbcs().addInsn(BC_INVALID);
          break;
          
      }

  }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitStringLiteralNode(StringLiteralNode *node) {

      uint16_t id = bce_->makeStringConstant(node->literal());

      cur_fbcs().addInsn(BC_SLOAD);
      cur_fbcs().addUInt16(id);
      
    }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitDoubleLiteralNode(DoubleLiteralNode *node) {
    
      cur_fbcs().addInsn(BC_DLOAD);
      cur_fbcs().addDouble(node->literal());
      
    }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitIntLiteralNode(IntLiteralNode *node) {
      
      cur_fbcs().addInsn(BC_ILOAD);
      cur_fbcs().addInt64(node->literal());
      
    }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitLoadNode(LoadNode *node) { 

      //
      // ...
      //
      
      Instruction load_i;
      
      scope_t *own_scp = node->var()->owner();
      
      if (cur_ctx().contains(own_scp))    // Check whether referenced variable is local to the current scope
      {
         switch(node->var()->type()) {
            case VT_DOUBLE: load_i = BC_LOADDVAR; break;
            case VT_INT:    load_i = BC_LOADIVAR; break;
            case VT_STRING: load_i = BC_LOADSVAR; break;
            default:        load_i = BC_INVALID;  /* OR, POSSIBLY, BREAK CODE GENERATION */
          }

        // Derive variable id
        var_id_t vid;

        if (own_scp == cur_ctx().get_om_scp())
        {
          vid = cur_ctx().get_vid(node->var()->name());
        } else 
        {
          vid = cur_ctx().get_vid(node->var()->name() + "_" + to_string(cur_ctx().get_scp_id(own_scp)));
        }

        cur_fbcs().addInsn(load_i);
        cur_fbcs().addUInt16(vid);
      }
      else
      {

        lctx_t &own_ctx = map(own_scp);

        // Derive variable id
        ctx_id_t ctxid  = get_ctx_id(own_ctx);
        var_id_t vid    = own_ctx.get_vid(node->var()->name());
        
        assert(ctxid  != 0xDEADBEEF);
        assert(vid    != 0xDEADBEEF);

        switch(node->var()->type()) {
          case VT_DOUBLE: load_i = BC_LOADCTXDVAR; break;
          case VT_INT:    load_i = BC_LOADCTXIVAR; break;
          case VT_STRING: load_i = BC_LOADCTXSVAR; break;
          default:        load_i = BC_INVALID;  /* OR, POSSIBLY, BREAK CODE GENERATION */
        }
        
        cur_fbcs().addInsn(load_i);
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }
      
    }

  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitStoreNode(StoreNode *node) {

      //
      // ...
      //
      
      node->value()->visit(this);


      Instruction load_i, store_i, sum_i, sub_i;
      
      scope_t *own_scp = node->var()->owner();
      lctx_t  &own_ctx = map(own_scp);
      
      bool local = own_ctx.get_om_scp() == cur_ctx().get_om_scp();
      

      switch(node->var()->type()) {

        case VT_DOUBLE:
          load_i  = local ? BC_LOADDVAR : BC_LOADCTXDVAR;
          store_i = local ? BC_STOREDVAR : BC_STORECTXDVAR;
          sum_i   = BC_DADD;
          sub_i   = BC_DSUB;
          break;

        case VT_INT:
          load_i  = local ? BC_LOADIVAR : BC_LOADCTXIVAR;
          store_i = local ? BC_STOREIVAR : BC_STORECTXIVAR;
          sum_i   = BC_IADD;
          sub_i   = BC_ISUB;
          break;

        case VT_STRING:
          load_i  = local ? BC_LOADSVAR : BC_LOADCTXSVAR;
          store_i = local ? BC_STORESVAR : BC_STORECTXSVAR;
          sum_i   = BC_INVALID;
          sub_i   = BC_INVALID;
          break;

        default:
          load_i  = BC_INVALID;
          store_i = BC_INVALID;
          sum_i   = BC_INVALID;
          sub_i   = BC_INVALID;

      }

      // Bind variable name with id 
      var_id_t vid = own_ctx.get_vid(node->var()->name());
      ctx_id_t ctxid  = get_ctx_id(own_ctx);
      
      if (node->op() != tASSIGN)
      {
        
        cur_fbcs().addInsn(load_i);

        if (local)
        {
          cur_fbcs().addUInt16(vid);
        }
        else
        {
          cur_fbcs().addUInt16(ctxid);
          cur_fbcs().addUInt16(vid);
        }
        
        // Branch upon particular type of the operation requested
        // Execution result of the expression to be stored should be already loaded on TOS
        
        if (node->op() == tINCRSET)
        {
          cur_fbcs().addInsn(sum_i);
        }
        else if (node->op() == tDECRSET)
        {
          cur_fbcs().addInsn(sub_i);
        }
        else
        {
          assert(false);  // FIXME
        }
        
      }
      
      // Up to this point, TOS should contain taget expression evaluated
      
      cur_fbcs().addInsn(store_i);
      
      if (local)
      {
        cur_fbcs().addUInt16(vid);
      }
      else
      {
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }
        
    }


  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitForNode(ForNode *node) {

      // Ensure that ``in-expression'' expression is actually made up by sole
      // binary operator ``..'' expression

      BinaryOpNode* in_expr = static_cast<BinaryOpNode*>(node->inExpr());
      
      assert(in_expr->kind() == tRANGE);
      
      //
      // Evalute ``in-expression''
      //
      // NOTE: Proper type coecion would be performed implicitly
      
      in_expr->visit(this);
      

      scope_t *own_scp = node->var()->owner();
      lctx_t  &own_ctx = map(own_scp);
      
      bool local = own_ctx.get_om_scp() == cur_ctx().get_om_scp();
 

      Instruction load_var_i,
                  load_var1_i,
                  store_var_i,
                  store_var1_i,
                  add_i,
                  load_unit_i;
      
      switch (node->var()->type()) {

        case VT_DOUBLE:
          store_var_i   = local ? BC_STOREDVAR : BC_STORECTXDVAR;
          store_var1_i  = BC_STOREDVAR1;
          load_var_i    = local ? BC_LOADDVAR : BC_LOADCTXDVAR;
          load_var1_i   = BC_LOADDVAR1;
          add_i         = BC_DADD;
          load_unit_i   = BC_DLOAD1;
          break;
          
        case VT_INT:
          store_var_i   = local ? BC_STOREIVAR : BC_STORECTXIVAR;
          store_var1_i  = BC_STOREIVAR1;
          load_var_i    = local ? BC_LOADIVAR : BC_LOADCTXIVAR;
          load_var1_i   = BC_LOADIVAR1;
          add_i         = BC_IADD;
          load_unit_i   = BC_ILOAD1;
          break;
          
        default:
          store_var_i   = BC_INVALID;
          store_var1_i  = BC_INVALID;
          load_var_i    = BC_INVALID;
          load_var1_i   = BC_INVALID;
          add_i         = BC_INVALID;
          load_unit_i   = BC_INVALID;
          break;
          
      }

      // Bind variable name with id 
      var_id_t vid    = own_ctx.get_vid(node->var()->name());
      ctx_id_t ctxid  = get_ctx_id(own_ctx);

      
      Label for_bs_l(&cur_fbcs()), for_be_l(&cur_fbcs());

      cur_fbcs().addInsn(store_var_i);           // Store LB in VAR1

      if (local)
      {
        cur_fbcs().addUInt16(vid);
      }
      else
      {
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }

      cur_fbcs().addInsn(store_var1_i);           // Store/Shift UB in/as VAR2


      cur_fbcs().bind(for_bs_l);
      
      cur_fbcs().addInsn(load_var1_i);            // Load UB on TOS

      cur_fbcs().addInsn(load_var_i);            // Load LB on TOS

      if (local)
      {
        cur_fbcs().addUInt16(vid);
      }
      else
      {
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }
      

      cur_fbcs().addBranch(BC_IFICMPG, for_be_l);    // if LB > UB jumpt to for-block-end label

      // Traverse inner block
      
      node->body()->visit(this);
      
      cur_fbcs().addInsn(load_unit_i);            // Increment LB and continue

      cur_fbcs().addInsn(load_var_i);

      if (local)
      {
        cur_fbcs().addUInt16(vid);
      }
      else
      {
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }

 
      cur_fbcs().addInsn(add_i);

      cur_fbcs().addInsn(store_var_i);

      if (local)
      {
        cur_fbcs().addUInt16(vid);
      }
      else
      {
        cur_fbcs().addUInt16(ctxid);
        cur_fbcs().addUInt16(vid);
      }
      
      cur_fbcs().addBranch(BC_JA, for_bs_l);
      
      cur_fbcs().bind(for_be_l);                     // Bind for-block-end label
      
  }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitWhileNode(WhileNode *node) {
      
      Label while_bs_l(&cur_fbcs()), while_be_l(&cur_fbcs());
      
      // Up to this point, TOS should contain ``while-expression'' evaluated
      //
      // NOTE:
      //  Since MVM-language doesn't contain boolean type all boolean-like
      //  expressions would be converted to integer implicitly
      
      // Evaluate ``while-expression''
      
      node->whileExpr()->visit(this);
      
      cur_fbcs().addInsn(BC_ILOAD1);
      
      cur_fbcs().bind(while_bs_l);
      
      cur_fbcs().addBranch(BC_IFICMPNE, while_be_l); // if WE != TRUE jump to while-block-end label
      
      node->loopBlock()->visit(this);
      
      cur_fbcs().addBranch(BC_JA, while_bs_l);
      
      cur_fbcs().bind(while_be_l);                   // Bind while-block-end label

  }
  
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitIfNode(IfNode *node) {
    
      // Evaluate ``if-expression''
      node->ifExpr()->visit(this);
      
      // Up to this point, TOS should contain ``if-expression'' evaluated
      //
      // NOTE:
      //  Since MVM-language doesn't contain boolean type all boolean-like
      //  expressions would be converted to integer implicitly
      
      Label else_bs_l(&cur_fbcs());
      
      cur_fbcs().addInsn(BC_ILOAD1);
      cur_fbcs().addBranch(BC_IFICMPNE, else_bs_l);  // if IFE != TRUE jump to else-block-start label
      
      node->thenBlock()->visit(this);

      if (node->elseBlock() != nullptr)
      {
        Label else_be_l(&cur_fbcs());
        
        cur_fbcs().addBranch(BC_JA, else_be_l);
        cur_fbcs().bind(else_bs_l);                    // Bind else-block-start label
        
        node->elseBlock()->visit(this);
        
        cur_fbcs().bind(else_be_l);                    // Bind else-block-end label
      }
      else
      {
        cur_fbcs().bind(else_bs_l);                    // Bind else-block-start label
      }
          
  }
  
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitBlockNode(BlockNode *node) {

      // Fold outer context and (possibly) create new one

      bool inhabitant = node->scope()->variablesCount() > 0;
      
      // Enter innermost scope
      push_scp(node->scope());

      // Enlist all functions declared in scope
      for (Scope::FunctionIterator f(node->scope()); f.hasNext();)
      {
        AstFunction *func = f.next();
        std::cerr << func->node()->name() << std::endl;
        bce_->addFunction(new BytecodeFunction(func));
      }

      // Translate all functions declared in scope
      for (Scope::FunctionIterator f(node->scope()); f.hasNext();)
      {
        f.next()->node()->visit(this);
      }

      // Traverse inner block

      node->visitChildren(this);
      
      // Exit innermost scope
      pop_scp();      
   
  }

  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitFunctionNode(FunctionNode *node) {
    
      // REENTRANCE POINT

      BytecodeFunction *func = 
        static_cast<BytecodeFunction *>(bce_->functionByName(node->name()));      

      // FIXME
//      if (node->name() == "<top>") 
//      {
//        new_func_p = static_cast<BytecodeFunction *>(bce_->functionByName("<top>"));
//      }
//      else 
//      {
//        new_func_p = new BytecodeFunction(cur_scp().lookupFunction(node->name()));
//        bce_->addFunction(new_func_p);
//      }
    
      // Create new BC-section and push previous one 
      bcs_stack_.push(make_pair(func->bytecode(), func->name()));

      //
      // EXPERIMENTAL
      //

      // Push new local-context on top of stack thereof
      ctx_stack_.emplace_back();

      // Bind formal parameters with corresponding values pushed on stack

      for (size_t i=0; i < node->parametersNumber(); ++i) 
      {
        Instruction store_i;
        
        switch(node->parameterType(i)) {
          case VT_DOUBLE: store_i = BC_STOREDVAR; break;
          case VT_INT:    store_i = BC_STOREIVAR; break;
          case VT_STRING: store_i = BC_STORESVAR; break;
          default:        store_i = BC_INVALID;  /* OR, POSSIBLY, BREAK CODE GENERATION */
        }

        // Derive variable id
        var_id_t vid;

        vid = cur_ctx().get_vid(node->parameterName(i));

        cur_fbcs().addInsn(store_i);
        cur_fbcs().addUInt16(vid);
      }

      // Traverse inner function-block
      
      node->body()->visit(this);
      
      // Pop context from the stack
      ctx_stack_.pop_back();

      // Restore previously active BC-section
      bcs_stack_.pop();
      
  }
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitReturnNode(ReturnNode *node) {

      AstNode *ret_expr = node->returnExpr();
 
      if (ret_expr) 
      {
        VarType act_ret_expr_type = infer_type_for(ret_expr);
        VarType exp_ret_expr_type = bce_->functionByName(cur_fname())->signature()[0].first;

        assert(act_ret_expr_type == VT_INT || act_ret_expr_type == VT_DOUBLE);
        assert(exp_ret_expr_type == VT_INT || exp_ret_expr_type == VT_DOUBLE);

        node->returnExpr()->visit(this);

        // Check whether any type-coercion step required
        if (exp_ret_expr_type != act_ret_expr_type) 
        {
          if (act_ret_expr_type == VT_INT)
            cur_fbcs().addInsn(BC_I2D);
          else
            cur_fbcs().addInsn(BC_D2I);
        }

        // Deliver invokation through register-like facility
        switch(exp_ret_expr_type) 
        {
          case VT_DOUBLE:
            cur_fbcs().addInsn(BC_STORECTXDVAR);
            break;
          case VT_INT:
            cur_fbcs().addInsn(BC_STORECTXIVAR);
            break;
          default:
            assert(false);  // FIXME
        } 
        cur_fbcs().addUInt16(1); // Deliver it to VAR0 of the EMBRACING context (CTXVAR @1,0)
        cur_fbcs().addUInt16(0);
      }
      
      cur_fbcs().addInsn(BC_RETURN);
    
  }
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitCallNode(CallNode *node) {
    
      // Evaluate supplied arguments

      node->visitChildren(this);

      // Check whether target function definition could be reached by callee
      
      if (cur_scp().lookupFunction(node->name()) != nullptr)
      {
        cur_fbcs().addInsn(BC_CALL);
        cur_fbcs().addUInt16(bce_->functionByName(node->name())->id());

        // Check out invokation outcome at the VAR0 and push it to the stack
        switch(bce_->functionByName(node->name())->signature()[0].first) {
          case VT_DOUBLE:
            cur_fbcs().addInsn(BC_LOADDVAR0);
            break;
          case VT_INT:
            cur_fbcs().addInsn(BC_LOADIVAR0);
            break;
          case VT_VOID:
            break;
          default:
            assert(false);
        }
      }
      else
      {
        cur_fbcs().addInsn(BC_INVALID);
      }
    
  }
  
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitNativeCallNode(NativeCallNode *node) {
    
      // Evaluate supplied parameters
    
      node->visitChildren(this);
    
      cur_fbcs().addInsn(BC_CALLNATIVE);
      cur_fbcs().addUInt16(0x00);           // Provide FUNCTION ID in next 2-bytes, STUBBED
  
  }
 
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    visitPrintNode(PrintNode *node) {
    
      typedef Instruction insn_type_t;
      
      vector<insn_type_t> acc;
      
      for (int i=0, oc = node->operands(); i < oc; ++i)
      {
        VarType op_type = infer_type_for(node->operandAt(i));
        
        switch (op_type)
        {
          case VT_DOUBLE: acc.push_back(BC_DPRINT); break;
          case VT_INT:    acc.push_back(BC_IPRINT); break;
          case VT_STRING: acc.push_back(BC_SPRINT); break;
          default:        assert(false);  // FIXME
        }
        
        // Translate this particular operand
        
        node->operandAt(oc - i - 1)->visit(this);
        
      }
      
      // Unfold accumulated instruction list
      
      for (vector<insn_type_t>::iterator i=acc.begin(); i != acc.end(); ++i)
      {
        cur_fbcs().addInsn(*i);
      }
    
  }
  
  
  BytecodeTranslator::BytecodeTranslatingVisitor::ctx_id_t
    BytecodeTranslator::BytecodeTranslatingVisitor::
      get_ctx_id(const lctx_t &ctx)
  {
    for (vector<lctx_t>::const_reverse_iterator i = ctx_stack_.rbegin(); i != ctx_stack_.rend(); ++i)
    {
      if (ctx.get_om_scp() == i->get_om_scp())
        return static_cast<ctx_id_t>(i - ctx_stack_.rbegin());
    }
    return 0xDEADBEEF;
  }
  
  void BytecodeTranslator::BytecodeTranslatingVisitor::
    _insert_coercing_prologue(AstNode  *lop,
                                       AstNode  *rop,
                                       const Code *code_seg,
                                       Bytecode *bcs,
                                       VarType  *most_common_type_rec)
  {
    
    VarType lop_type = infer_type_for(lop);
    VarType rop_type = infer_type_for(rop);
    
    assert(lop_type == VT_INT || lop_type == VT_DOUBLE);
    assert(rop_type == VT_INT || rop_type == VT_DOUBLE);
    
    VarType most_common_type;
    
    if (lop_type != rop_type) {
      most_common_type = VT_DOUBLE;     // Binary operands could be of either ``VT_INTEGER'' or ``VT_DOUBLE'' type only
    }
    else
    {
      most_common_type = lop_type;
    }
    
    
    // Perform coercion, if any
    
    if (lop_type != most_common_type)
    {
      switch (lop_type) {
        case VT_DOUBLE:
          cur_fbcs().addInsn(BC_D2I);
          break;
        case VT_INT:
          cur_fbcs().addInsn(BC_I2D);
          break;
        default:
          assert(false);  // FIXME
      }
    }
    
    if (rop_type != most_common_type)
    {
      cur_fbcs().addInsn(BC_SWAP);
      
      switch (rop_type) {
        case VT_DOUBLE:
          cur_fbcs().addInsn(BC_D2I);
          break;
        case VT_INT:
          cur_fbcs().addInsn(BC_I2D);
          break;
        default:
          assert(false);  // FIXME
      }
      
      cur_fbcs().addInsn(BC_SWAP);
    }
    
    *most_common_type_rec = most_common_type;

  }
  
  
  //
  // TYPE INFERENCING VISITOR
  //
  
  BytecodeTranslator::TypeInferencingVisitor::TypeInferencingVisitor(const Code *code_seg)
    : bce_(code_seg)
  {}
  
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitBinaryOpNode(BinaryOpNode *node)
  {

    node->left()->visit(this);
    
    VarType mc_lop_type = most_common_type_;
    
    node->right()->visit(this);
    
    VarType mc_rop_type = most_common_type_;
    
    assert(mc_lop_type != VT_INVALID && mc_lop_type != VT_VOID && mc_lop_type != VT_STRING);
    assert(mc_rop_type != VT_INVALID && mc_rop_type != VT_VOID && mc_rop_type != VT_STRING);
    
    if (mc_lop_type != mc_rop_type)
    {
      most_common_type_ = VT_DOUBLE;
    }
    else
    {
      most_common_type_ = mc_lop_type;
    }
    
  }
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitUnaryOpNode(UnaryOpNode *node)
  {
    node->operand()->visit(this);
    assert(most_common_type_ != VT_INVALID);
  }
  

  void BytecodeTranslator::TypeInferencingVisitor::
    visitCallNode(CallNode *node)
  {
    most_common_type_ = bce_->functionByName(node->name())->signature()[0].first;
    assert(most_common_type_ != VT_INVALID);
  }
  
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitDoubleLiteralNode(DoubleLiteralNode *)
  {
    most_common_type_ = VT_DOUBLE;
  }
  

  void BytecodeTranslator::TypeInferencingVisitor::
    visitIntLiteralNode(IntLiteralNode *)
  {
    most_common_type_ = VT_INT;
  }
  
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitStringLiteralNode(StringLiteralNode *)
  {
    most_common_type_ = VT_STRING;
  }
  
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitLoadNode(LoadNode *node)
  {
    most_common_type_ = node->var()->type();
    assert(most_common_type_ != VT_INVALID);
  }
  
  void BytecodeTranslator::TypeInferencingVisitor::
    visitStoreNode(StoreNode *node)
  {
    most_common_type_ = node->var()->type();
    assert(most_common_type_ != VT_INVALID);
  }
  
  
  VarType BytecodeTranslator::BytecodeTranslatingVisitor::infer_type_for(AstNode *node)
  {
    TypeInferencingVisitor tiv(bce_.get());
    
    node->visit(&tiv);
    
    return tiv.most_common_type_;
  }

} // namespace mathvm
