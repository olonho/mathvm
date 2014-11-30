#include "mathvm.h"
#include "my_interpreter.h"
#include <stdexcept>
#include <iostream>

namespace mathvm{
    using Interpriter::StackItem;
    Status * ExecutableCode::execute(std::vector<Var *> &){
        BytecodeFunction * main = (BytecodeFunction *) functionById(0);
        currentScope = new Interpriter::Scope(main);
        std::vector<StackItem> stack;
        while(true){
            switch(currentScope->getInstruction()){
            case BC_INVALID: case BC_S2I: case BC_SLOAD0:
                throw std::runtime_error("Invalid instruction");
            case BC_DLOAD:{
                stack.push_back(StackItem::fromDouble(currentScope->getDouble())); break;
            }
            case BC_ILOAD:{
                stack.push_back(StackItem::fromInt(currentScope->getInt())); break;
            }
            case BC_SLOAD:{
                stack.push_back(StackItem::fromUint(currentScope->getUint())); break;
            }
            case BC_DLOAD0:{
                stack.push_back(StackItem::fromDouble(0)); break;
            }
            case BC_ILOAD0:{
                stack.push_back(StackItem::fromInt(0)); break;
            }
            case BC_DLOAD1:{
                stack.push_back(StackItem::fromDouble(1)); break;
            }
            case BC_ILOAD1:{
                stack.push_back(StackItem::fromInt(1)); break;
            }
            case BC_DLOADM1:{
                stack.push_back(StackItem::fromDouble(-1)); break;
            }
            case BC_ILOADM1:{
                stack.push_back(StackItem::fromInt(-1)); break;
            }
            case BC_DADD:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left+right));
                break;
            }
            case BC_IADD:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left+right));
                break;
            }
            case BC_DSUB:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left-right));
                break;
            }
            case BC_ISUB:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left-right));
                break;
            }
            case BC_DMUL:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left*right));
                break;
            }
            case BC_IMUL:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left*right));
                break;
            }
            case BC_DDIV:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left/right));
                break;
            }
            case BC_IDIV:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left/right));
                break;
            }
            case BC_IMOD:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left%right));
                break;
            }
            case BC_DNEG:{
                double val = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(-val));
                break;
            }
            case BC_INEG:{
                int64_t val = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(-val));
                break;
            }
            case BC_IAOR:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left|right));
                break;
            }
            case BC_IAAND:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left&right));
                break;
            }
            case BC_IAXOR:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left^right));
                break;
            }
            case BC_IPRINT:{
                std::cout << stack[stack.size()-1].asInt();
                stack.pop_back();
                break;
            }
            case BC_DPRINT:{
                std::cout << stack[stack.size()-1].asDouble();
                stack.pop_back();
                break;
            }
            case BC_SPRINT:{
                std::cout << constantById(stack[stack.size()-1].asUint());
                stack.pop_back();
                break;
            }
            case BC_I2D:{
                int64_t val = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(val));
                break;
            }
            case BC_D2I:{
                double val = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(val));
                break;
            }
            case BC_SWAP:{
                std::swap(stack[stack.size()-1], stack[stack.size()-2]); break;
            }
            case BC_POP:{
                stack.pop_back();
            }
            case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
            {
                stack.push_back(currentScope->getVar()); break;
            }
            case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:{
                currentScope->setVar(stack[stack.size()-1]);
                stack.pop_back();
                break;
            }
            case BC_DCMP:{
                double upper = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double lower = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(upper == lower ? 0 : upper < lower ? -1 : 1));
                break;
            }
            case BC_ICMP:{
                int64_t upper = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t lower = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(upper == lower ? 0 : upper < lower ? -1 : 1));
                break;
            }
            case BC_JA:{
                int16_t offset = currentScope->getInt16();
                currentScope->jump(offset  - sizeof(int16_t));
                break;
            }
            case BC_IFICMPNE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() != stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() == stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPG:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() > stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPGE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() >= stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPL:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() < stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPLE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() <= stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_CALL:{
                uint16_t id = currentScope->getUint();
                BytecodeFunction *fn = (BytecodeFunction*)functionById(id);
                currentScope = new Interpriter::Scope(fn, currentScope);
                break;
            }
            case BC_RETURN:{
                Interpriter::Scope * oldScope = currentScope;
                currentScope = currentScope->parent();
                delete oldScope;
                break;
            }
            case BC_STOP:{
                return Status::Ok();
            }
            default:{
                std::cerr << "Unimplemented instruction" << std::endl;
            }
            }

        }
        return Status::Ok();
    }
}
