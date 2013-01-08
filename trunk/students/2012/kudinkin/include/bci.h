//
//  Bytecode Interpreter (BI) for MVM
//

#ifndef _MATHVM_INTERPRETER_H_301212_
#define _MATHVM_INTERPRETER_H_301212_

#include <memory>
#include <stack>

#include "mathvm.h"
#include "be.h"


namespace mathvm
{
  
  /**
    * ...
    */
  struct _Data {
    
    typedef double    double_t;
    typedef int64_t   int_t;
    typedef uint16_t  id_t;
    typedef uint16_t  off_t;
    
    /* volatile */ union {
      
      // FLOATING POINT VALUES
      
      double_t  d_;
      
      // INTEGRAL VALUES
      
      int_t     i_;
      id_t      id_;
      off_t     off_;
      
    } val_;
    
  };


  template<class T> struct _type_sym {};
  
  template<> struct _type_sym<_Data::double_t> {
    static const VarType value = VT_DOUBLE;
  };
  
  template<> struct _type_sym<_Data::int_t> {
    static const VarType value = VT_INT;
  };
  
  template<> struct _type_sym<_Data::id_t> {
    static const VarType value = VT_STRING;
  };  


  class BytecodeInterpreter {
    
    typedef _Data               data_t;
    typedef BytecodeExecutable  exec_t;
    typedef Bytecode            bcode_t;
    
    typedef vector<data_t>      ctx_t;



   protected:
    
    BytecodeInterpreter(BytecodeExecutable *bce, ostream &stdout, ostream &stderr)
      : stdout_(stdout),
        stderr_(stderr),
        bce_(bce),
        ctx_(1, ctx_t(3))     // CREATE TOP-LEVEL CONTEXT
    {
      sp_   = ds_.size();
      lctx_ = &ctx_.back();
    }
    
    
    /**
      * Execution entry-point
      */

    void exec();

    
   public:
    
    template<class T>
      void var(T **ref, data_t::id_t var_id, data_t::id_t ctx_id = 0);


    // PUSH

    template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type& val) 
    { ds_.emplace(); ++sp_; ds_.top().val_.d_ = val; }

   template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_INT, T>::type& val) 
    { ds_.emplace(); ++sp_; ds_.top().val_.i_ = val; } 
  
    template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_STRING, T>::type& val) 
    { ds_.emplace(); ++sp_; ds_.top().val_.id_ = val; } 
   
    void push(const BytecodeInterpreter::data_t& h) 
    {
      ds_.emplace(h); ++sp_;
    }


    // POP

    template<class T>
      typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type pop() 
    { data_t::double_t val = ds_.top().val_.d_; ds_.pop(); --sp_; return val; }

   template<class T>
      typename enable_if<_type_sym<T>::value == VT_INT, T>::type pop() 
    { data_t::int_t val = ds_.top().val_.i_; ds_.pop(); --sp_; return val; }
  
    template<class T>
      typename enable_if<_type_sym<T>::value == VT_STRING, T>::type pop() 
    { data_t::id_t val = ds_.top().val_.id_; ds_.pop(); --sp_; return val; }

    data_t pop() 
    {
      data_t tos = ds_.top(); ds_.pop(); --sp_;
      return tos;
    }


    // TOP

    template<class T>
      typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type& top()
    { return ds_.top().val_.d_; }

   template<class T>
      typename enable_if<_type_sym<T>::value == VT_INT, T>::type& top()
    { return ds_.top().val_.i_; }
  
    template<class T>
      typename enable_if<_type_sym<T>::value == VT_STRING, T>::type& top()
    { return ds_.top().val_.id_; }

    void unwind(size_t sp) 
    {
      assert(ds_.size() > sp);
      while(ds_.size() > sp + 1)
        ds_.pop();
    }


    // STATICS
    
    static void exec(BytecodeExecutable *bce);
    
    static const char * const top_func_name_;
    
   private:
    
    /**
      * Standard output streams
      */
    ostream &stdout_;
    ostream &stderr_;
    
    /**
      * ...
      */
    unique_ptr<BytecodeExecutable> bce_;

    /**
      * Data stack backing SSM
      */

    size_t sp_;

    stack<data_t> ds_;
    
    /**
      * Execution (control-flow) stack
      */
    stack<tuple<bcode_t *, data_t::off_t, size_t> > cfs_;
    
    
    /**
      * Execution contexts
      */
    
    ctx_t *lctx_;
    
    vector<ctx_t> ctx_;
    
  };
  
  
  template<class T>
    void BytecodeInterpreter::var(T **ref, data_t::id_t var_id, data_t::id_t ctx_id)
  {
    ctx_t &ctx = ctx_[ctx_.size() - 1 - ctx_id];
    
    if (var_id > ctx.size())
      ctx.resize(var_id);
    
    if (var_id < ctx.size())
    {
      switch (_type_sym<T>::value) {
        case VT_DOUBLE:
          *ref = reinterpret_cast<T *>(&ctx[var_id].val_.d_);
          break;
        case VT_INT:
          *ref = reinterpret_cast<T *>(&ctx[var_id].val_.i_);
          break;
        case VT_STRING:
          *ref = reinterpret_cast<T *>(&ctx[var_id].val_.id_);
          break;
        default:
          assert(false);
      }
    }
    else
    {
      ctx_t::iterator nvar = ctx.emplace(ctx.end());
      
      switch (_type_sym<T>::value) {
        case VT_DOUBLE:
          *ref = reinterpret_cast<T *>(&nvar->val_.d_);
          break;
        case VT_INT:
          *ref = reinterpret_cast<T *>(&nvar->val_.i_);
          break;
        case VT_STRING:
          *ref = reinterpret_cast<T *>(&nvar->val_.id_);
          break;
        default:
          assert(false);
      }
    }
  }

}

#endif // #ifndef _MATHVM_INTERPRETER_H_301212_
