
#include "bci.h"


namespace mathvm
{
 
  const char * const BytecodeInterpreter::top_func_name_ = "<top>";

  /**
    * ...
    */ 
  void BytecodeInterpreter::exec(BytecodeExecutable *bce) {
    
    BytecodeInterpreter bi(bce, cout, cerr);
    
    bi.exec();
    
  }
  
  
  /**
    *
    */
  void BytecodeInterpreter::exec() {
    
    BytecodeFunction &top_func = *static_cast<BytecodeFunction *>(bce_->functionByName(top_func_name_));
    
    //
    // ...
    //
    
    Bytecode *bcs   = top_func.bytecode();
    
    data_t::id_t cfid = top_func.id(); 

    for (uint32_t ip=0; ip < bcs->length();)
    {

      // INVARIANT:
      //
      //    Tip of the state-machine's (SM) tape holds next instruction
      //    to be executed ('retired' from the pipeline)
      //
      
      Instruction ni = bcs->getInsn(ip++);
    
      //
      // TRACING FACILITIES: FIXME
      //

      //std::cerr << bce_->functionById(cfid)->name() << ": " << ip << ", " << bcName(ni) << std::endl;
      //for (auto i = ds_.begin(); i != ds_.end(); ++i)
      //  std::cerr << i->val_.d_ << ", ";
      //std::cerr << std::endl;      


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
          
          push<data_t::double_t>(bcs->getDouble(ip));
          
          ip += sizeof(data_t::double_t);
          
          break;

        /** 
          * Loads INTEGER inlined on TOS
          */
        case BC_ILOAD:
          
          push<data_t::int_t>(bcs->getInt64(ip));

          ip += sizeof(data_t::int_t);
          
          break;
          
        /**
         * Loads CONSTANT-STRING ID inlined on TOS
         */
        case BC_SLOAD:
          
          push<data_t::id_t>(bcs->getUInt16(ip));
          
          ip += sizeof(data_t::id_t);
          
          break;
          
        /**
         * Loads DOUBLE ZERO (0.0) on TOS
         */
        case BC_DLOAD0:
          
          push<data_t::double_t>(0.0);
          
          break;
          
        /**
         * Loads INTEGER ZERO (0) on TOS
         */
        case BC_ILOAD0:
          
          push<data_t::int_t>(0);
          
          break;
          
        /**
         * Loads CONSTANT-STRING ID inlined on TOS
         */
        case BC_SLOAD0:
          
          push<data_t::id_t>(0);
          
          break;
          
        /**
         * Loads DOUBLE UNIT (1.0) on TOS
         */
        case BC_DLOAD1:
          
          push<data_t::double_t>(1.0);

          break;
          
        /**
         * Loads INTEGER UNIT (1) on TOS
         */
        case BC_ILOAD1:
          
          push<data_t::int_t>(1);
          
          break;

        /**
         * Loads DOUBLE NEGATED UNIT (-1.0) on TOS
         */
        case BC_DLOADM1:
          
          push<data_t::double_t>(-1.0);
          
          break;
          
        /**
         * Loads INTEGER NEGATED UNIT (-1) on TOS
         */
        case BC_ILOADM1:
          
          push<data_t::int_t>(-1);

          break;
          
        /**
          * ...
          */
        case BC_DADD:
        {
          data_t::double_t lop, rop;

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
          
          push(lop);
          push(rop);
          
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
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          push<data_t::double_t>(var<data_t::double_t>(vid));
          
          break;
        }
        
        case BC_LOADIVAR:
        {
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);

          push<data_t::int_t>(var<data_t::int_t>(vid));
          
          break;
        }
          
        case BC_LOADSVAR:
        {
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          push<data_t::id_t>(var<data_t::id_t>(vid));
          
          break;
        }
        
          
        case BC_STOREDVAR:
        {
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::double_t>(vid) = pop<data_t::double_t>();
          
          break;
        }
          
        case BC_STOREIVAR:
        {
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);

          var<data_t::int_t>(vid) = pop<data_t::int_t>();

          break;
        }
          
        case BC_STORESVAR:
        {
          data_t::id_t vid = bcs->getUInt16(ip);
          
          ip += sizeof(data_t::id_t);
          
          var<data_t::id_t>(vid) = pop<data_t::id_t>();
          
          break;
        }
          

        //
        // CONTEXT-AWARE LOAD-/STORE-INSTRUCTIONS (SLOWEST/LONGEST)
        //
          
        case BC_LOADCTXDVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          push<data_t::double_t>(cvar<data_t::double_t>(vid, ctxid));
          
          break;
        }
          
        case BC_LOADCTXIVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          push<data_t::int_t>(cvar<data_t::int_t>(vid, ctxid));
          
          break;
        }
          
        case BC_LOADCTXSVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip); ip += sizeof(data_t::id_t);
          
          push<data_t::id_t>(cvar<data_t::id_t>(vid, ctxid));
          
          break;
        }
          
        case BC_STORECTXDVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          cvar<data_t::double_t>(vid, ctxid) = pop<data_t::double_t>();
          
          break;
        }
          
        case BC_STORECTXIVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          cvar<data_t::int_t>(vid, ctxid) = pop<data_t::int_t>();
          
          break;
        }
          
        case BC_STORECTXSVAR:
        {
          data_t::id_t  ctxid = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          data_t::id_t  vid   = bcs->getUInt16(ip);  ip += sizeof(data_t::id_t);
          
          cvar<data_t::id_t>(vid, ctxid) = pop<data_t::id_t>();
          
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

          int argc = callee->parametersNumber();
      
          assert(argc >= 0);
          assert(sp_ - argc >= 0);

          cfs_.push_back(
            make_tuple(bcs, ip, sp_ - argc, cfid)
          );

          // FIXME : current-function-id stored to facilitate tracing procedure
          cfid = fid;

          ctx_.emplace_back(ctx_t(4));

          lctx_ = &ctx_.back();

          // Check whether pointer at this particular context 
          // being established previously
          
          if (fid != ctx_p_.back().first)   // Create new pointer
            ctx_p_.push_back(make_pair(fid, ctx_.size() - 1));
          else                              // Advance pointer
            ctx_p_.back().second = ctx_.size() - 1;

          // ...

          bcs = callee->bytecode();
          
          ip = 0;
          
          break;
        }

        case BC_RETURN:
//        case BC_IRETURN:
//        case BC_DRETURN:
        {
          data_t pret_val;

          // POP return-value if any

          // FIXME
          data_t::id_t r_cfid = cfid;

          if (bce_->functionById(r_cfid)->returnType() != VT_VOID) 
          {
              pret_val = pop();
          }

//          switch(ni) {
//            case BC_IRETURN:
//            case BC_DRETURN:
//              pret_val = pop();
//            default: ; 
//          }

          // Dispose context

          ctx_.pop_back();

          lctx_ = &ctx_.back();


          int psp = sp_;
      
          tie(bcs, ip, sp_, cfid) = cfs_.back();

          assert(psp >= sp_);


          if (r_cfid != cfid)   // Prune pointers
            ctx_p_.pop_back();
          else                  // Shift pointer
            ctx_p_.back().second = ctx_.size() - 1;

  
          cfs_.pop_back();

          // Transition stack into "intouched" state

          unwind_until(sp_);


          // And PUSH it back

          if (bce_->functionById(r_cfid)->returnType() != VT_VOID) 
          {
              push(pret_val);
          }


//          switch(ni) {
//            case BC_IRETURN:
//            case BC_DRETURN:
//              push(pret_val);
//            default: ;
//          }

          break;

        }


        default:
          assert(false);
          
      }
      
    }
    
  }
  
}
