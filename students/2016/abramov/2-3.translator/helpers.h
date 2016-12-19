#ifndef HELPERS_H
#define HELPERS_H

#include "ast.h"
#include "mathvm.h"
#include "generator_exception.h"

namespace mathvm
{
    Instruction getBitInstruction(TokenKind bitToken);
    Instruction getArithmeticInstruction(TokenKind arithmeticToken, VarType type);
    Instruction getArithmeticInstructionInt(TokenKind arithmeticToken);
    Instruction getArithmeticInstructionDouble(TokenKind arithmeticToken);
    Instruction getComapreInstruction(TokenKind token);
    Instruction getPrintInstruction(VarType type);
    Instruction getLocalLoadInstruction(VarType type);
    Instruction getContextLoadInstruction(VarType type);
    Instruction getLocalStoreInstruction(VarType type);
    Instruction getContexStoreInstruction(VarType type);

    Instruction getBitInstruction(TokenKind bitToken)  
    {
        switch (bitToken)
        {
            case tAOR:
                return BC_IAOR;
            case tAAND:
                return BC_IAAND;
            case tAXOR:
                return BC_IAXOR;
            default:
                break;
        }

        throw GeneratorException("Invalid Bit token");
    }

    Instruction getArithmeticInstruction(TokenKind arithmeticToken, VarType type)
    {
        if (type == VT_INT)
        {
            return getArithmeticInstructionInt(arithmeticToken);
        }
        if (type == VT_DOUBLE)
        {
            return getArithmeticInstructionDouble(arithmeticToken);
        }

        throw GeneratorException("Invalid Arithmetic token");
    }

    Instruction getArithmeticInstructionInt(TokenKind arithmeticToken)
    {
        switch (arithmeticToken)
        {
            case tADD:
                return BC_IADD;
            case tSUB:
                return BC_ISUB;
            case tMUL:
                return BC_IMUL;
            case tDIV:
                return BC_IDIV;
            case tMOD:
                return BC_IMOD;
            default:
                throw GeneratorException("Invalid Arithmetic Int token");       
        }
    }

    Instruction getArithmeticInstructionDouble(TokenKind arithmeticToken)
    {
        switch (arithmeticToken)
        {
            case tADD:
                return BC_DADD;
            case tSUB:
                return BC_DSUB;
            case tMUL:
                return BC_DMUL;
            case tDIV:
                return BC_DDIV;
            default:
                throw GeneratorException("Invalid Arithmetic Double token");       
        }
    }

    Instruction getComapreInstruction(TokenKind token) 
    {
        switch (token) 
        {
            case tEQ:
                return BC_IFICMPE;
            case tNEQ:
                return BC_IFICMPNE;
            case tLE:
                return BC_IFICMPLE;
            case tGE:
                return BC_IFICMPGE;
            case tLT:
                return BC_IFICMPL;
            case tGT:
                return BC_IFICMPG;
            default:
                throw GeneratorException("Invalid Compare token");
        }
    }
    
    Instruction getPrintInstruction(VarType type)
    {
        switch (type) 
        {
            case VT_STRING:
                return BC_SPRINT;
            case VT_INT:
                return BC_IPRINT;
            case VT_DOUBLE:
                return BC_DPRINT;
            default:
                throw GeneratorException("Invalid type for print operation");
        }
    }
    
    Instruction getLocalLoadInstruction(VarType type)
    {
        switch (type) 
        {
            case VT_INT:
                return BC_LOADIVAR;
            case VT_DOUBLE:
                return BC_LOADDVAR;
            case VT_STRING:
                return BC_LOADSVAR;
            default:
                throw GeneratorException("Wrong type for Local Load");
        }
    }
    
    Instruction getContextLoadInstruction(VarType type)
    {
        switch (type) 
        {
            case VT_INT:
                return BC_LOADCTXIVAR;
            case VT_DOUBLE:
                return BC_LOADCTXDVAR;
            case VT_STRING:
                return BC_LOADCTXSVAR;
            default:
                throw GeneratorException("Wrong type for Global Load");
        }
        
    }
    
    Instruction getLocalStoreInstruction(VarType type)
    {
        switch (type)
        {
            case VT_INT:
                return BC_STOREIVAR;
            case VT_DOUBLE:
                return BC_STOREDVAR;
            case VT_STRING:
                return BC_STORESVAR;
            default:
                throw GeneratorException("Wrong type for Local Store");
        }
    }
    
    Instruction getContexStoreInstruction(VarType type)
    {
        switch (type)
        {
            case VT_INT:
                return BC_STORECTXIVAR;
            case VT_DOUBLE:
                return BC_STORECTXDVAR;
            case VT_STRING:
                return BC_STORECTXSVAR;
            default:
                throw GeneratorException("Wrong type for Global Store");
        }
    }
}

#endif /* HELPERS_H */

