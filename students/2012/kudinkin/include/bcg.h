
#ifndef _MATHVM_TRANSLATOR_H_181012_
#define _MATHVM_TRANSLATOR_H_181012_

#include <algorithm>
#include <functional>
#include <memory>
#include <stack>
#include <unordered_map>

#include "ast.h"
#include "be.h"
#include "mathvm.h"


namespace mathvm
{

  class BytecodeTranslator : public Translator {

   protected:
    
  
    class _TypeInferencingVisitorAdapter : protected AstVisitor {
     public:
#ifdef VISITOR_FUNCTION
# undef VISITOR_FUNCTION
#endif
#define VISITOR_FUNCTION(type, name) void visit##type(type* node) { most_common_type_ = VT_VOID; };
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
     protected:
      VarType most_common_type_;
    };


    /**
     * Inferences most-common type for given AST sub-tree
     */
    class TypeInferencingVisitor : protected _TypeInferencingVisitorAdapter {
      
      friend class BytecodeTranslator;
      
     public:
      
      TypeInferencingVisitor(const Code *code_seg);
      
      void visitBinaryOpNode(BinaryOpNode *);
      void visitUnaryOpNode(UnaryOpNode *);
      void visitCallNode(CallNode *);
      void visitDoubleLiteralNode(DoubleLiteralNode *);
      void visitIntLiteralNode(IntLiteralNode *);
      void visitStringLiteralNode(StringLiteralNode *);
      void visitLoadNode(LoadNode *);
      void visitStoreNode(StoreNode *);

     private:

      const Code *bce_;
      
    };
  
  
    /**
      * Translates given AST sub-tree into byte-code
      */
    class BytecodeTranslatingVisitor : protected AstVisitor {

      friend class BytecodeTranslator;
      
      
      /**
        * Represents scopes (as local-contexts) _inhabited_ with variables
        */
      class LocalContext {
        
       public:
        
        static const uint16_t VARIABLE_ID_SEED = 0x03;
        
        typedef Scope           scope_t;
        typedef uint16_t        var_id_t;
        typedef uint16_t        ctx_id_t;

        typedef vector<scope_t*>::difference_type  off_t;

        typedef Signature signature_t;

        
        //LocalContext(Scope *s, var_id_t seed = VARIABLE_ID_SEED) : scope_(s), vid_(seed) {};
        LocalContext(var_id_t seed = VARIABLE_ID_SEED) : vid_(seed) {};
        
        
        //Scope*    scope() const { return scope_; }

        off_t     get_scp_id(scope_t *scp) const { 
                    return scp_stack_.cend() - find(scp_stack_.cbegin(), scp_stack_.cend(), scp);
                  }

        void      push_scp(scope_t *scp) { scp_stack_.push_back(scp); }

        void      pop_scp() { scp_stack_.pop_back(); }

        bool      contains(scope_t *scp) const { 
                    return find(scp_stack_.cbegin(), scp_stack_.cend(), scp) != scp_stack_.cend();
                  }

        Scope*    get_im_scp() const { return scp_stack_.back(); }
        Scope*    get_om_scp() const { return scp_stack_.front(); }
        
        var_id_t  get_vid(const string &vname) { return ctx_.find(vname) != ctx_.end() ? ctx_.at(vname) : ctx_[vname] = vid_++; }


       private:
        
        /**
          * Scopes corresponding to the current context
          */
        vector<scope_t *> scp_stack_;
        
        /**
          * Pointer to the next non-bound (yet) variable-id
          */
        var_id_t  vid_;
        
        /**
          * Context: assigns each variable pre-defined id
          */
        unordered_map<string, var_id_t> ctx_;
        
      };  // class LocalContext
      
      
     public:
      
      // CONSTRUCTORS
      
      BytecodeTranslatingVisitor(Scope *top);

#ifdef VISITOR_FUNCTION
# undef VISITOR_FUNCTION
#endif
#define VISITOR_FUNCTION(type, name) void visit##type(type* node);
      FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
      
     protected:

      typedef LocalContext        lctx_t;
      typedef Scope               scope_t;
      typedef Bytecode            bcode_t;
      typedef BytecodeExecutable  bcode_exec_t;

      typedef LocalContext::var_id_t  var_id_t;
      typedef LocalContext::ctx_id_t  ctx_id_t;
      
      
      bcode_exec_t* dump() { return bce_.release(); };

      void          push_scp(scope_t *scp) {
                      cur_ctx().push_scp(scp);
                      lct_[scp] = ctx_stack_.size() - 1;
                    }

      void          pop_scp() { 
                      lct_.erase(&cur_scp());
                      cur_ctx().pop_scp(); 
                    }

      lctx_t&       cur_ctx() { return ctx_stack_.back(); }
      
      bcode_t&      cur_fbcs() { return *bcs_stack_.top().first; }

      const string& cur_fname() { return bcs_stack_.top().second; }
      
      scope_t&      cur_scp() { /* return scp_stack_.top(); */ return *cur_ctx().get_im_scp(); }
      
      lctx_t&       map(scope_t *scope) { return ctx_stack_[lct_.at(scope)]; };
      
      ctx_id_t      get_ctx_id(const lctx_t &);
      
      
      VarType       infer_type_for(AstNode *node);
      
      void _insert_coercing_prologue(AstNode    *lop,
                                     AstNode    *rop,
                                     const Code *code_seg,
                                     Bytecode   *bcs,
                                     VarType    *most_common_type_rec);

     private:
      
      /**
        * Target code segment being populated with translated entities
        */
      unique_ptr<bcode_exec_t> bce_;

      /**
        * Outer contexts' stack
        *
        * NOTE: Since scopes are managed "outside" there is no need
        *       to strictly adhere to the RAII paradigm
        */
      vector<lctx_t> ctx_stack_;
    
      /**
        * BC-sections stack being currently translated
        */
      stack<pair<bcode_t *, string> > bcs_stack_;
      
      /**
        * Stack of nested scopes
        */
      stack<reference_wrapper<scope_t> > scp_stack_;
      
      /**
        * LC-table mapping _inhabitant_ scopes into corresponding local-contexts
        */
      unordered_map<scope_t *, size_t> lct_;
      
    };
  
    
    /**
      * ...
      */
    Status* translate(const string& source, Code **code);


    /**
      * Pseudo-function comprising whole outtermost scope
      */

    BytecodeFunction *top_pfunc_;

  };


}

#endif // #ifndef _MATHVM_TRANSLATOR_H_181012_
