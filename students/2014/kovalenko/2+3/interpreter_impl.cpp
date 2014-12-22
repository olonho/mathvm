#include "mathvm.h"
#include "interpreter_impl.h"
#include <stdexcept>
#include <iostream>

namespace mathvm{
    using namespace Interpreter;
    Status * ExecutableCode::execute(std::vector<Var *> &){
        BytecodeFunction * main = (BytecodeFunction *) functionById(0);
        currentScope = new ScopeData(main);
        std::vector<SVal> my_stack;

        while(1){
          switch(currentScope->getBytecodeInstruction()){

            case BC_CALL:{
              uint16_t id = currentScope->getUint();
              BytecodeFunction *fn = (BytecodeFunction*)functionById(id);
              currentScope = new ScopeData(fn, currentScope);
              break;
            }

            case BC_RETURN:{
              Interpreter::ScopeData * prevScope = currentScope;
              currentScope = currentScope->parent();
              delete prevScope;
              break;
            }

            case BC_STOP:{
              return Status::Ok();
            }

            case BC_DLOAD:{
              my_stack.push_back(SVal::saveD(currentScope->getDouble())); break;
            }

            case BC_ILOAD:{
              my_stack.push_back(SVal::saveI(currentScope->getInt())); break;
            }

            case BC_SLOAD:{
              my_stack.push_back(SVal::saveU(currentScope->getUint())); break;
            }

            case BC_DLOAD0:{
              my_stack.push_back(SVal::saveD(0)); break;
            }

            case BC_ILOAD0:{
              my_stack.push_back(SVal::saveI(0)); break;
            }

            case BC_DLOAD1:{
              my_stack.push_back(SVal::saveD(1)); break;
            }

            case BC_ILOAD1:{
              my_stack.push_back(SVal::saveI(1)); break;
            }

            case BC_DLOADM1:{
              my_stack.push_back(SVal::saveD(-1)); break;
            }

            case BC_ILOADM1:{
              my_stack.push_back(SVal::saveI(-1)); break;
            }

            case BC_DADD:{
                double lhs = my_stack.back().d();
                my_stack.pop_back(); //maybe I should write poems, not C++ code?
                double rhs = my_stack.back().d();
                my_stack.pop_back(); //yo aha aha
                my_stack.push_back(SVal::saveD(lhs+rhs));
                break;
            }

            case BC_IADD:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs+rhs));
                break;
            }

            case BC_DSUB:{
                double lhs = my_stack.back().d();
                my_stack.pop_back();
                double rhs = my_stack.back().d();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveD(lhs-rhs));
                break;
            }

            case BC_ISUB:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs-rhs));
                break;
            }

            case BC_DMUL:{
                double lhs = my_stack.back().d();
                my_stack.pop_back();
                double rhs = my_stack.back().d();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveD(lhs*rhs));
                break;
            }

            case BC_IMUL:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs*rhs));
                break;
            }

            case BC_DDIV:{
                double lhs = my_stack.back().d();
                my_stack.pop_back();
                double rhs = my_stack.back().d();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveD(lhs/rhs));
                break;
            }

            case BC_IDIV:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs/rhs));
                break;
            }

            case BC_IMOD:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs%rhs));
                break;
            }

            case BC_DNEG:{
                double val = my_stack.back().d();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveD(-val));
                break;
            }

            case BC_INEG:{
                int64_t val = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(-val));
                break;
            }

            case BC_IAOR:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs|rhs));
                break;
            }

            case BC_IAAND:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs&rhs));
                break;
            }

            case BC_IAXOR:{
                int64_t lhs = my_stack.back().i();
                my_stack.pop_back();
                int64_t rhs = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(lhs^rhs));
                break;
            }

            case BC_IPRINT:{
                std::cout << my_stack.back().i();
                my_stack.pop_back();
                break;
            }

            case BC_DPRINT:{
                std::cout << my_stack.back().d();
                my_stack.pop_back();
                break;
            }

            case BC_SPRINT:{
                std::cout << constantById(my_stack.back().u());
                my_stack.pop_back();
                break;
            }

            case BC_I2D:{
                int64_t val = my_stack.back().i();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveD(val));
                break;
            }

            case BC_D2I:{
                double val = my_stack.back().d();
                my_stack.pop_back();
                my_stack.push_back(SVal::saveI(val));
                break;
            }

            case BC_SWAP:{
                std::swap(my_stack.back(), my_stack.at(my_stack.size()-2)); break;
            }

            case BC_POP:{
                my_stack.pop_back();
                break;
            }

            case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
            {
                my_stack.push_back(currentScope->getVariable());
                break;
            }

            case BC_LOADCTXDVAR: case BC_LOADCTXIVAR: case BC_LOADCTXSVAR:
            {
                my_stack.push_back(currentScope->getContextVariable());
                break;
            }

            case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:{
                currentScope->setVar(my_stack.back());
                my_stack.pop_back();
                break;
            }

            case BC_STORECTXDVAR: case BC_STORECTXIVAR: case BC_STORECTXSVAR:{
                currentScope->setContextVariable(my_stack.back());
                my_stack.pop_back();
                break;
            }

            case BC_DCMP:{
                double h = my_stack.back().d();
                my_stack.pop_back();
                double l = my_stack.back().d();
                my_stack.pop_back();
                int v = h == l ? 0 : h < l ? -1 : 1;
                my_stack.push_back(SVal::saveI(v));
                break;
            }

            case BC_ICMP:{
                int64_t h = my_stack.back().i();
                my_stack.pop_back();
                int64_t l = my_stack.back().i();
                my_stack.pop_back();
                int v = h == l ? 0 : h < l ? -1 : 1;
                my_stack.push_back(SVal::saveI(v));
                break;
            }

            case BC_JA:{
                int16_t o = currentScope->getInt16();
                currentScope->jump(o - sizeof(int16_t));
                break;
            }

            case BC_IFICMPNE:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() != my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_IFICMPE:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() == my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_IFICMPG:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() > my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_IFICMPGE:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() >= my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_IFICMPL:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() < my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_IFICMPLE:{
                int16_t o = currentScope->getInt16();
                if(my_stack.back().i() <= my_stack.at(my_stack.size()-2).i())
                    currentScope->jump(o - sizeof(int16_t));
                my_stack.pop_back();
                my_stack.pop_back();
                break;
            }

            case BC_INVALID: case BC_S2I: case BC_SLOAD0:
             throw std::runtime_error("INVALID");

            default:{
                std::cerr << "Unknown instruction" << std::endl;
            }
          }
      }
        return Status::Ok();
    }
}
