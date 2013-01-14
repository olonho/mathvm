//
//  Bytecode Interpreter (BI) for MVM
//

#ifndef _MATHVM_INTERPRETER_H_301212_
#define _MATHVM_INTERPRETER_H_301212_

#include <algorithm>
#include <memory>
#include <vector>

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
    
    typedef vector<data_t>              ctx_t;
    typedef pair<data_t::id_t, size_t>  ctx_p_t;


   protected:
    
    BytecodeInterpreter(BytecodeExecutable *bce, ostream &stdout, ostream &stderr)
      : stdout_(stdout),
        stderr_(stderr),
        bce_(bce),
        ctx_(1, ctx_t(4)),            // Push top-level context
        ctx_p_(1, make_pair(0, 0))    // Establish pinter at the top-level context 
    {
      sp_   = ds_.size();
      lctx_ = &ctx_.back();
    }
    
    
    /**
      * Execution entry-point
      */

    void exec();

    
   public:
    
    // Local VAR referencing harness
    template<class T>
      T& var(data_t::id_t var_id);
      //void var(T **ref, data_t::id_t var_id);

    // Closure's VAR referencing harness
    template<class T>
      T& cvar(data_t::id_t var_id, data_t::id_t ctx_id);
      //void cvar(T **ref, data_t::id_t var_id, data_t::id_t ctx_id);


    // PUSH

    template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type& val) 
    { 
      ds_.emplace_back(); 
      ++sp_; 
      ds_.back().val_.d_ = val; 
    }

   template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_INT, T>::type& val) 
    { 
      ds_.emplace_back(); 
      ++sp_; 
      ds_.back().val_.i_ = val; 
    } 
  
    template<class T>
      void push(const typename enable_if<_type_sym<T>::value == VT_STRING, T>::type& val) 
    { 
      ds_.emplace_back(); 
      ++sp_; 
      ds_.back().val_.id_ = val; 
    } 
   
    void push(const BytecodeInterpreter::data_t& h) 
    {
      ds_.emplace_back(h); 
      ++sp_;
    }


    // POP

    template<class T>
      typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type pop() 
    { 
      assert(ds_.size() > 0);
      //assert(sp_ > 0);
      data_t::double_t val = ds_.back().val_.d_; 
      ds_.pop_back(); 
      --sp_; 
      return val; 
    }

   template<class T>
      typename enable_if<_type_sym<T>::value == VT_INT, T>::type pop() 
    { 
      assert(ds_.size() > 0);
      //assert(sp_ > 0);
      data_t::int_t val = ds_.back().val_.i_; 
      ds_.pop_back(); 
      --sp_; 
      return val; 
    }  

    template<class T>
      typename enable_if<_type_sym<T>::value == VT_STRING, T>::type pop() 
    { 
      assert(ds_.size() > 0);
      //assert(sp_ > 0);
      data_t::id_t val = ds_.back().val_.id_; 
      ds_.pop_back(); 
      --sp_; 
      return val; 
    }

    data_t pop() 
    {
      assert(ds_.size() > 0);
      //assert(sp_ > 0);
      data_t tos = ds_.back(); 
      ds_.pop_back(); 
      --sp_;
      return tos;
    }


    // TOP

    template<class T>
      typename enable_if<_type_sym<T>::value == VT_DOUBLE, T>::type& top()
    { return ds_.back().val_.d_; }

   template<class T>
      typename enable_if<_type_sym<T>::value == VT_INT, T>::type& top()
    { return ds_.back().val_.i_; }
  
    template<class T>
      typename enable_if<_type_sym<T>::value == VT_STRING, T>::type& top()
    { return ds_.back().val_.id_; }


    void unwind_until(size_t sp) 
    {
      assert(ds_.size() >= sp);
      while(ds_.size() > sp)
        ds_.pop_back();
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
  
    int sp_;

    vector<data_t> ds_;
    
    /**
      * Execution (control-flow) stack
      */

    vector<tuple<bcode_t *, data_t::off_t, size_t, data_t::id_t> > cfs_;
    
    
    /**
      * Execution contexts
      */
    
    ctx_t *lctx_;

    vector<ctx_t> ctx_;       // Natural contexts

    vector<ctx_p_t> ctx_p_;   // Context pointers (skip-list):  
                              //    Adds new layer of indirection preserving actual
                              //    context-id-value being referred in BC
    
  };
  

//  template<class T>
//    void BytecodeInterpreter::var(T **ref, data_t::id_t var_id)
//  {
//
//    if (var_id > lctx_->size())
//      lctx_->resize(var_id + 1);
//   
//    
//    switch (_type_sym<T>::value) {
//      case VT_DOUBLE:
//        *ref = reinterpret_cast<T *>(&(*lctx_)[var_id].val_.d_);
//        break;
//      case VT_INT:
//        *ref = reinterpret_cast<T *>(&(*lctx_)[var_id].val_.i_);
//        break;
//      case VT_STRING:
//        *ref = reinterpret_cast<T *>(&(*lctx_)[var_id].val_.id_);
//        break;
//      default:
//        assert(false);
//    }
//  }

  template<class T>
    T& BytecodeInterpreter::var(data_t::id_t var_id)
  {

    if (var_id + 1u > lctx_->size())
      lctx_->resize(var_id + 1u);
   
    
    switch (_type_sym<T>::value) {
      case VT_DOUBLE:
        return reinterpret_cast<T&>((*lctx_)[var_id].val_.d_);
      case VT_INT:
        return reinterpret_cast<T&>((*lctx_)[var_id].val_.i_);
      case VT_STRING:
        return reinterpret_cast<T&>((*lctx_)[var_id].val_.id_);
      default:
        assert(false);
    }
  }

//  template<class T>
//    void BytecodeInterpreter::cvar(T **ref, data_t::id_t var_id, data_t::id_t ctx_id)
//  {
//
//    // Seek particular context being the last instantiated
//    struct _Ctx_spy {
//      _Ctx_spy(data_t::id_t id) : id_(id) {}
//      bool operator()(const ctx_p_t &ctx_p) {
//        return ctx_p.first == id_;
//      }
//      data_t::id_t id_;
//    } _ctx_spy(ctx_id);
//
//    vector<ctx_p_t>::iterator t;
//    for (t = --ctx_p_.end(); t != --ctx_p_.begin(); --t)
//      if (t->first == ctx_id) break;
//
//    ctx_t &ref_ctx =  ctx_[   // Seek the last instantiated context of particular interest
//                        t->second
//                      ];
//
//    if (var_id > ref_ctx.size())
//      ref_ctx.resize(var_id + 1);
//   
//    
//    switch (_type_sym<T>::value) {
//      case VT_DOUBLE:
//        *ref = reinterpret_cast<T *>(&ref_ctx[var_id].val_.d_);
//        break;
//      case VT_INT:
//        *ref = reinterpret_cast<T *>(&ref_ctx[var_id].val_.i_);
//        break;
//      case VT_STRING:
//        *ref = reinterpret_cast<T *>(&ref_ctx[var_id].val_.id_);
//        break;
//      default:
//        assert(false);
//    }
//  }

  
  template<class T>
    T& BytecodeInterpreter::cvar(data_t::id_t var_id, data_t::id_t ctx_id)
  {

    // Seek particular context being the last instantiated
    struct _Ctx_spy {
      _Ctx_spy(data_t::id_t id) : id_(id) {}
      bool operator()(const ctx_p_t &ctx_p) {
        return ctx_p.first == id_;
      }
      data_t::id_t id_;
    } _ctx_spy(ctx_id);

    vector<ctx_p_t>::iterator t;
    for (t = --ctx_p_.end(); t != --ctx_p_.begin(); --t)
      if (t->first == ctx_id) break;

    ctx_t &ref_ctx =  ctx_[   // Seek the last instantiated context of particular interest
                        t->second
                      ];

    if (var_id + 1u > ref_ctx.size())
      ref_ctx.resize(var_id + 1u);
   
    
    switch (_type_sym<T>::value) {
      case VT_DOUBLE:
        return reinterpret_cast<T&>(ref_ctx[var_id].val_.d_);
      case VT_INT:
        return reinterpret_cast<T&>(ref_ctx[var_id].val_.i_);
      case VT_STRING:
        return reinterpret_cast<T&>(ref_ctx[var_id].val_.id_);
      default:
        assert(false);
    }
  }

}

#endif // #ifndef _MATHVM_INTERPRETER_H_301212_
