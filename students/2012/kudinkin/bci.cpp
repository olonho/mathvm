
#include "bci.h"


namespace mathvm
{
 
  const char * const BytecodeInterpreter::top_func_name_ = "<top>";
  
  
  void BytecodeInterpreter::exec(BytecodeExecutable *bce) {
    
    BytecodeInterpreter bi(bce, cerr, cerr);
    
    bi.exec();
    
  }
  
  
  void BytecodeInterpreter::exec() {
    
    BytecodeFunction &top_func = *static_cast<BytecodeFunction *>(bce_->functionByName(top_func_name_));
    
    Bytecode &top_func_bcode = *top_func.bytecode();
    
    //
    // ...
    //
    
    Bytecode *bcs = &top_func_bcode;
    

    for (uint32_t ip=0; ip < bcs->length();)
    {

      // INVARIANT:
      //
      //    Tip of the state-machine's (SM) tape holds next instruction
      //    to be executed ('retired' from the pipeline)
      //
      
      Instruction ni = bcs->getInsn(ip++);
      
      switch(ni)
      {
        
        /**
          * Withdraws pipeline, stops execution
          */
        case BC_INVALID:

          if (ip == bcs->length() - 1)
            break;
          else {
            std::cerr << "IP: " << ip << std::endl; 
            assert(false);
          }
        
        /**
          * Loads DOUBLE inlined on TOS
          */
        case BC_DLOAD:
          
//          ds_.emplace();
//          ds_.top().val_.d_ = bcs->getDouble(ip);

          push<data_t::double_t>(bcs->getDouble(ip));
          
          ip += sizeof(data_t::double_t);
          
          break;

        /** 
          * Loads INTEGER inlined on TOS
          */
        case BC_ILOAD:
          
//          ds_.emplace();
//          ds_.top().val_.i_ = bcs->getInt64(ip);

          push<data_t::int_t>(bcs->getInt64(ip));

          ip += sizeof(data_t::int_t);
          
          break;
          
        /**
         * Loads CONSTANT-STRING ID inlined on TOS
         */
        case BC_SLOAD:
          
//          ds_.emplace();
//          ds_.top().val_.id_ = bcs->getUInt16(ip);

          push<data_t::id_t>(bcs->getUInt16(ip));
          
          ip += sizeof(data_t::id_t);
          
          break;
          
        /**
         * Loads DOUBLE ZERO (0.0) on TOS
         */
        case BC_DLOAD0:
          
//          ds_.emplace();
//          ds_.top().val_.d_ = 0.0;
        
          push<data_t::double_t>(0.0);
          
          break;
          
        /**
         * Loads INTEGER ZERO (0) on TOS
         */
        case BC_ILOAD0:
          
//          ds_.emplace();
//          ds_.top().val_.i_ = 0;

          push<data_t::int_t>(0);
          
          break;
          
        /**
         * Loads CONSTANT-STRING ID inlined on TOS
         */
        case BC_SLOAD0:
          
//          ds_.emplace();
//          ds_.top().val_.id_ = 0;

          push<data_t::id_t>(0);
          
          break;
          
        /**
         * Loads DOUBLE UNIT (1.0) on TOS
         */
        case BC_DLOAD1:
          
//          ds_.emplace();
//          ds_.top().val_.d_ = 1.0;

          push<data_t::double_t>(1.0);

          break;
          
        /**
         * Loads INTEGER UNIT (1) on TOS
         */
        case BC_ILOAD1:
          
//          ds_.emplace();
//          ds_.top().val_.i_ = 1;

          push<data_t::int_t>(1);
          
          break;

        /**
         * Loads DOUBLE NEGATED UNIT (-1.0) on TOS
         */
        case BC_DLOADM1:
          
//          ds_.emplace();
//          ds_.top().val_.d_ = -1.0;

          push<data_t::double_t>(-1.0);
          
          break;
          
        /**
         * Loads INTEGER NEGATED UNIT (-1) on TOS
         */
        case BC_ILOADM1:
          
//          ds_.emplace();
//          ds_.top().val_.i_ = -1;
          
          push<data_t::int_t>(-1);

          break;
          
        /**
          * ...
          */
        case BC_DADD:
        {
          data_t::double_t lop, rop;

//          lop = ds_.top().val_.d_; ds_.pop();
//          rop = ds_.top().val_.d_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.d_ = lop + rop;

          lop = pop<data_t::double_t>();
          rop = pop<data_t::double_t>();
        
          push<data_t::double_t>(lop + rop);
          
          break;
        }

        /**
         * ...
         */
        case BC_IADD:
        {
          data_t::int_t lop, rop;

//          lop = ds_.top().val_.i_; ds_.pop();
//          rop = ds_.top().val_.i_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.i_ = lop + rop;

          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
        
          push<data_t::int_t>(lop + rop);
 
          break;
        }
          
        /**
         * ...
         */
        case BC_DSUB:
        {
          data_t::double_t lop, rop;
          
//          lop = ds_.top().val_.d_; ds_.pop();
//          rop = ds_.top().val_.d_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.d_ = lop - rop;

          lop = pop<data_t::double_t>();
          rop = pop<data_t::double_t>();
        
          push<data_t::double_t>(lop - rop);
          
          break;
        }
          
          /**
           * ...
           */
        case BC_ISUB:
        {
          data_t::int_t lop, rop;
          
//          lop = ds_.top().val_.i_; ds_.pop();
//          rop = ds_.top().val_.i_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.i_ = lop - rop;

          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
        
          push<data_t::int_t>(lop - rop);
          
          break;
        }
          
        /**
         * ...
         */
        case BC_DMUL:
        {
          data_t::double_t lop, rop;
          
//          lop = ds_.top().val_.d_; ds_.pop();
//          rop = ds_.top().val_.d_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.d_ = lop * rop;

          lop = pop<data_t::double_t>();
          rop = pop<data_t::double_t>();
        
          push<data_t::double_t>(lop * rop);
           
          break;
        }
          
        /**
         * ...
         */
        case BC_IMUL:
        {
          data_t::int_t lop, rop;
          
//          lop = ds_.top().val_.i_; ds_.pop();
//          rop = ds_.top().val_.i_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.i_ = lop * rop;

          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
        
          push<data_t::int_t>(lop * rop);
          
          break;
        }
          
        /**
         * ...
         */
        case BC_DDIV:
        {
          data_t::double_t lop, rop;
          
//          lop = ds_.top().val_.d_; ds_.pop();
//          rop = ds_.top().val_.d_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.d_ = lop / rop;

          lop = pop<data_t::double_t>();
          rop = pop<data_t::double_t>();
        
          push<data_t::double_t>(lop / rop);
          
          break;
        }
          
        /**
         * ...
         */
        case BC_IDIV:
        {
          data_t::int_t lop, rop;
          
//          lop = ds_.top().val_.i_; ds_.pop();
//          rop = ds_.top().val_.i_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.i_ = lop / rop;

          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
        
          push<data_t::int_t>(lop / rop);

          
          break;
        }
          
        /**
          * ...
          */
        case BC_IMOD:
        {
          data_t::int_t lop, rop;
          
//          lop = ds_.top().val_.i_; ds_.pop();
//          rop = ds_.top().val_.i_; ds_.pop();
//          
//          ds_.emplace();
//          ds_.top().val_.i_ = lop % rop;

          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
        
          push<data_t::int_t>(lop % rop);

          
          break;
        }
          
        /**
          * ...
          */
        case BC_DNEG:

          top<data_t::double_t>() *= -1.0;

          break;
          
        /**
          * ...
          */
        case BC_INEG:

          top<data_t::int_t>() *= -1.0;
          
          break;
          
        /**
          * ...
          */
        case BC_DPRINT:
          
          stdout_ << pop<data_t::double_t>();

          break;
          
        /**
          * ...
          */
        case BC_IPRINT:
          
          stdout_ << pop<data_t::int_t>();
          
          break;
          
        /**
          * ...
          */
        case BC_SPRINT:
          
          stdout_ << bce_->constantById(pop<data_t::id_t>());
          
          break;

        /**
          * ...
          */
        case BC_I2D:
          
          push<data_t::double_t>(static_cast<data_t::double_t>(pop<data_t::int_t>()));
          
          break;

        /**
          * ...
          */
        case BC_D2I:
          
          push<data_t::int_t>(static_cast<data_t::int_t>(pop<data_t::double_t>()));
          
          break;
          
        /**
          * ...
          */
        case BC_SWAP:
        {
          data_t lop, rop;
          
          lop = pop();
          rop = pop();
          
          ds_.push(lop);
          ds_.push(rop);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_POP:
          
          pop();

          break;
        
        
        /**
          * ...
          */
        case BC_DCMP:
        {
          data_t::double_t lop, rop, diff;
          
          lop = pop<data_t::double_t>();
          rop = pop<data_t::double_t>();
          
          diff = lop - rop;
          
          push<data_t::double_t>(diff == 0.0 ? diff : diff < 0.0 ? -1.0 : 1.0);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_ICMP:
        {
          data_t::int_t lop, rop, diff;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          diff = lop - rop;
          
          push<data_t::int_t>(diff == 0 ? diff : diff < 0 ? -1 : 1);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_JA:

          ip += bcs->getInt16(ip);
          
          break;
        
        /**
          * ...
          */
        case BC_IFICMPNE:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop != rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_IFICMPE:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop == rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_IFICMPG:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop > rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_IFICMPGE:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop >= rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_IFICMPL:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop < rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          
        /**
          * ...
          */
        case BC_IFICMPLE:
        {
          data_t::int_t lop, rop;
          
          lop = pop<data_t::int_t>();
          rop = pop<data_t::int_t>();
          
          if (lop <= rop)
            ip += bcs->getInt16(ip);
          else
            ip += sizeof(data_t::off_t);
          
          break;
        }
          

        ////////////////////////////////////////////////////////////////////////
        
        //
        // VARIABLE RELATED INSTRUCTIONS
        //
          
        //
        // LOCAL0-3  LOAD-/STORE-INSTRUCTIONS (ULTRA-FAST/-SHORT)
        //

        case BC_LOADDVAR0:
        case BC_LOADIVAR0:
        case BC_LOADSVAR0:
          push((*lctx_)[0]);
          break;
          

        case BC_LOADDVAR1:
        case BC_LOADIVAR1:
        case BC_LOADSVAR1:
          push((*lctx_)[1]);
          break;
          
          
        case BC_LOADDVAR2:
        case BC_LOADIVAR2:
        case BC_LOADSVAR2:
          push((*lctx_)[1]);
          break;
          
        case BC_STOREDVAR0:
        case BC_STOREIVAR0:
        case BC_STORESVAR0:
          (*lctx_)[0] = pop();
          break;
          
          
        case BC_STOREDVAR1:
        case BC_STOREIVAR1:
        case BC_STORESVAR1:
          (*lctx_)[1] = pop();
          break;
          
        case BC_STOREDVAR2:
        case BC_STOREIVAR2:
        case BC_STORESVAR2:
          (*lctx_)[2] = pop();
          break;

        //
        // LOCAL LOAD-/STORE-INSTRUCTIONS (MODERATELY FAST/SHORT)
        //
        
        case BC_LOADDVAR:
        {
          data_t::double_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);

          var<data_t::double_t>(&vref, vid);
          
          push<data_t::double_t>(*vref);
          
          break;
        }
        
        case BC_LOADIVAR:
        {
          data_t::int_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::int_t>(&vref, vid);
          
          push<data_t::int_t>(*vref);
          
          break;
        }
          
        case BC_LOADSVAR:
        {
          data_t::id_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::id_t>(&vref, vid);
          
          push<data_t::id_t>(*vref);
          
          break;
        }
        
          
        case BC_STOREDVAR:
        {
          data_t::double_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::double_t>(&vref, vid);
          
          *vref = pop<data_t::double_t>();
          
          break;
        }
          
        case BC_STOREIVAR:
        {
          data_t::int_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::int_t>(&vref, vid);

          *vref = pop<data_t::int_t>();
          
          break;
        }
          
        case BC_STORESVAR:
        {
          data_t::id_t *vref;
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::id_t>(&vref, vid);
          
          *vref = pop<data_t::id_t>();
          
          break;
        }
          

        //
        // CONTEXT-AWARE LOAD-/STORE-INSTRUCTIONS (SLOWEST/LONGEST)
        //
          
        case BC_LOADCTXDVAR:
        {
          data_t::double_t  *vref;
          
          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          var<data_t::double_t>(&vref, vid, ctxid);
          
          push<data_t::double_t>(*vref);
          
          break;
        }
          
        case BC_LOADCTXIVAR:
        {
          data_t::int_t *vref;

          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          var<data_t::int_t>(&vref, vid, ctxid);
          
          push<data_t::int_t>(*vref);
          
          break;
        }
          
        case BC_LOADCTXSVAR:
        {
          data_t::id_t  *vref;
          
          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          var<data_t::id_t>(&vref, vid, ctxid);
          
          push<data_t::id_t>(*vref);
          
          break;
        }
          
        case BC_STORECTXDVAR:
        {
          data_t::double_t  *vref;
          
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          var<data_t::double_t>(&vref, vid, ctxid);
          
          *vref = pop<data_t::double_t>();
          
          break;
        }
          
        case BC_STORECTXIVAR:
        {
          data_t::int_t *vref;
          
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          var<data_t::int_t>(&vref, vid, ctxid);
          
          *vref = pop<data_t::int_t>();
          
          break;
        }
          
        case BC_STORECTXSVAR:
        {
          data_t::id_t  *vref;
          
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          var<data_t::id_t>(&vref, vid, ctxid);
          
          *vref = pop<data_t::id_t>();
          
          break;
        }
          
        ////////////////////////////////////////////////////////////////////////
          
        /**
          * ...
          */
        case BC_CALL:
        {
          data_t::id_t fid = bcs->getUInt16(ip);

          ip += sizeof(data_t::id_t);

          
          BytecodeFunction *callee = static_cast<BytecodeFunction *>(bce_->functionById(fid));
          
          //
          // PUSH OLD CONTEXT AND CREATE NEW ONE
          //

          cfs_.push(make_tuple(bcs, ip, sp_));

          ctx_.emplace_back(ctx_t(3));

          lctx_ = &ctx_.back();

          // ...
          
          bcs = callee->bytecode();
          
          ip = 0;
          
          break;
        }
          
        case BC_RETURN:
        {

          //
          // RESTORE OLD CONTEXT AND DISPOSE NEW ONE
          //

          ctx_.pop_back();

          lctx_ = &ctx_.back();

          tie(bcs, ip, sp_) = cfs_.top();

          cfs_.pop();

          // EXPERIMENTAL

          // Now, invokation outcome being delivered through register-like facility (!)
          
          //unwind(sp_);

          break;

        }
          
        default:
          assert(false);
          
      }
      
    }
    
  }
  
}
