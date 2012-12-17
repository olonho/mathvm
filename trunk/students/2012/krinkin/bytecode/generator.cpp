#include <cassert>

#include <AsmJit/Assembler.h>
#include <AsmJit/Logger.h>

#include "generator.h"

using namespace AsmJit;

void Generator::translate(AstFunction *top, std::map<AstNode *, VarType> *mapping, BCCode *code)
{
    m_mapping = mapping;
    m_code = code;
    
    m_code->declare_function(top);
    m_code->push_scope(top->owner());
    m_code->push_bytecode(m_code->lookup_function(top));
    top->node()->body()->visit(this);
    m_code->pop_bytecode();
    
    m_code->pop_scope();
}

void Generator::visitBinaryOpNode(BinaryOpNode *node)
{
    VarType target_type = type(node);
	switch (node->kind())
	{
	case tOR: case tAND:
	{
		eval_logic(node->left());
		
		annotate(LOCK_RESULT);
		
		bytecode()->addInsn(BC_ILOAD0);
		if (node->kind() == tOR) bytecode()->addInsn(BC_IFICMPNE);
		else bytecode()->addInsn(BC_IFICMPE);
		uint32_t lazy_jump = bytecode()->current();
		bytecode()->addInt16(0);
		bytecode()->addInsn(BC_POP);
		eval_logic(node->right());
		
		annotate(SAVE_RESULT);
		
		bytecode()->addInsn(BC_SWAP);
		int16_t offset = bytecode()->current() - lazy_jump;
		bytecode()->setInt16(lazy_jump, offset);
		bytecode()->addInsn(BC_POP);
		
		annotate(UNLOCK_RESULT);
		break;
	}
	case tMUL: case tDIV:
	case tADD: case tSUB:
	case tMOD:
	{
	    switch (target_type)
	    {
	    case VT_INT:
	        eval_int(node->right());
	        eval_int(node->left());
	        switch (node->kind())
	        {
	        case tMUL:
	            bytecode()->addInsn(BC_IMUL);
	            break;
	        case tDIV:
	            bytecode()->addInsn(BC_IDIV);
	            break;
	        case tADD:
	            bytecode()->addInsn(BC_IADD);
	            break;
	        case tSUB:
	            bytecode()->addInsn(BC_ISUB);
	            break;
            case tMOD:
	            bytecode()->addInsn(BC_IMOD);
	            break;
            default: assert(0);
	        }
	        break;
        case VT_DOUBLE:
	        eval_double(node->right());
	        eval_double(node->left());
	        switch (node->kind())
	        {
	        case tMUL:
	            bytecode()->addInsn(BC_DMUL);
	            break;
	        case tDIV:
	            bytecode()->addInsn(BC_DDIV);
	            break;
	        case tADD:
	            bytecode()->addInsn(BC_DADD);
	            break;
	        case tSUB:
	            bytecode()->addInsn(BC_DSUB);
	            break;
            default: assert(0);
	        }
	        break;
        default: assert(0);
	    }
		break;
	}
	case tEQ: case tNEQ:
	case tGT: case tGE:
	case tLT: case tLE:
	{
	    if (type(node->right()) == VT_DOUBLE || type(node->left()) == VT_DOUBLE)
	    {
	        if (node->kind() != tNEQ)
	        {
	            annotate(LOCK_RESULT);
	            
	            bytecode()->addInsn(BC_ILOAD0);
            }
            eval_double(node->left());
            eval_double(node->right());
            bytecode()->addInsn(BC_DCMP);
	        if (node->kind() != tNEQ)
	        {
	            switch (node->kind())
	            {
                case tEQ:
                    bytecode()->addInsn(BC_IFICMPE);
                    break;
                case tGT:
                    bytecode()->addInsn(BC_IFICMPL);
                    break;
                case tGE:
                    bytecode()->addInsn(BC_IFICMPLE);
                    break;
                case tLT:
                    bytecode()->addInsn(BC_IFICMPG);
                    break;
                case tLE:
                    bytecode()->addInsn(BC_IFICMPGE);
                    break;
                default: assert(0);
	            }
                uint32_t jump1 = bytecode()->current();
                bytecode()->addInt16(0);
                bytecode()->addInsn(BC_POP);
                bytecode()->addInsn(BC_JA);
                uint32_t jump2 = bytecode()->current();
                bytecode()->addInt16(0);
                int16_t offset = bytecode()->current() - jump1;
                bytecode()->setInt16(jump1, offset);
                bytecode()->addInsn(BC_POP);
                bytecode()->addInsn(BC_POP);
                bytecode()->addInsn(BC_ILOAD1);
                
                annotate(SAVE_RESULT);
                
                offset = bytecode()->current() - jump2;
                bytecode()->setInt16(jump2, offset);
                
                annotate(UNLOCK_RESULT);
	        }
	    }
	    else if (type(node->right()) == VT_INT || type(node->left()) == VT_INT)
	    {
	        eval_int(node->left());
	        
	        annotate(LOCK_RESULT);
	        
	        eval_int(node->right());
            switch (node->kind())
            {
            case tEQ:
                bytecode()->addInsn(BC_IFICMPE);
                break;
            case tNEQ:
                bytecode()->addInsn(BC_IFICMPNE);
                break;
            case tGT:
                bytecode()->addInsn(BC_IFICMPL);
                break;
            case tGE:
                bytecode()->addInsn(BC_IFICMPLE);
                break;
            case tLT:
                bytecode()->addInsn(BC_IFICMPG);
                break;
            case tLE:
                bytecode()->addInsn(BC_IFICMPGE);
                break;
            default: assert(0);
            }
            uint32_t jump1 = bytecode()->current();
            bytecode()->addInt16(0);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_ILOAD0);
            
            annotate(SAVE_RESULT);
            
            bytecode()->addInsn(BC_JA);
            uint32_t jump2 = bytecode()->current();
            bytecode()->addInt16(0);
            int16_t offset = bytecode()->current() - jump1;
            bytecode()->setInt16(jump1, offset);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_ILOAD1);
            
            annotate(SAVE_RESULT);
            
            offset = bytecode()->current() - jump2;
            bytecode()->setInt16(jump2, offset);
            
            annotate(UNLOCK_RESULT);
	    }
	    else assert(0);
		break;
	}
	case tDECRSET: case tINCRSET: case tASSIGN:
	{
	    AstVar const * const var = ((LoadNode *)node->left())->var();
        if (node->kind() == tASSIGN)
        {
            switch (var->type())
            {
            case VT_STRING:
                eval_string(node->right());
                dup_string();
		        break;
            case VT_DOUBLE:
                eval_double(node->right());
                dup_double();
		        break;
            case VT_INT:
                eval_int(node->right());
                dup_int();
		        break;
        	default: assert(0);
            }
        }
        else
        {
            switch (var->type())
            {
            case VT_DOUBLE:
                eval_double(node->right());
                eval_double(node->left());
                if (node->kind() == tINCRSET) bytecode()->addInsn(BC_DADD);
                else bytecode()->addInsn(BC_DSUB);
                dup_double();
		        break;
            case VT_INT:
                eval_int(node->right());
                eval_int(node->left());
                if (node->kind() == tINCRSET) bytecode()->addInsn(BC_IADD);
                else bytecode()->addInsn(BC_ISUB);
                dup_int();
		        break;
            default: assert(0);
            }
        }
        save_variable(var);
		break;
	}
	default: assert(0);
	}
}

void Generator::visitUnaryOpNode(UnaryOpNode *node)
{
    switch (type(node))
    {
    case VT_DOUBLE:
        eval_double(node->operand());
        if (node->kind() == tSUB)
        {
            bytecode()->addInsn(BC_DNEG);
        }
        else if (node->kind() == tNOT)
        {
            bytecode()->addInsn(BC_DLOAD0);
            bytecode()->addInsn(BC_DCMP);
            
            annotate(LOCK_RESULT);
            
            bytecode()->addInsn(BC_ILOAD0);
            bytecode()->addInsn(BC_IFICMPE);
            uint32_t jump1 = bytecode()->current();
            bytecode()->addInt16(0);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_JA);
            uint32_t jump2 = bytecode()->current();
            bytecode()->addInt16(0);
            int16_t offset = bytecode()->current() - jump1;
            bytecode()->setInt16(jump1, offset);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_ILOAD1);
            
            annotate(SAVE_RESULT);
            
            offset = bytecode()->current() - jump2;
            bytecode()->setInt16(jump2, offset);
            
            annotate(UNLOCK_RESULT);
        }
        break;
    case VT_INT:
        if (node->kind() == tSUB)
        {
            eval_int(node->operand());
            bytecode()->addInsn(BC_INEG);
        }
        else if (node->kind() == tNOT)
        {
            bytecode()->addInsn(BC_ILOAD0);
            
            annotate(LOCK_RESULT);
            
            eval_int(node->operand());
            bytecode()->addInsn(BC_IFICMPE);
            uint32_t jump1 = bytecode()->current();
            bytecode()->addInt16(0);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_JA);
            uint32_t jump2 = bytecode()->current();
            bytecode()->addInt16(0);
            int16_t offset = bytecode()->current() - jump1;
            bytecode()->setInt16(jump1, offset);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_POP);
            bytecode()->addInsn(BC_ILOAD1);
            
            annotate(SAVE_RESULT);
            
            offset = bytecode()->current() - jump2;
            bytecode()->setInt16(jump2, offset);
            
            annotate(UNLOCK_RESULT);
        }
        break;
	default: assert(0);
    }
}

void Generator::visitStringLiteralNode(StringLiteralNode *node)
{
	if (node->literal().empty()) bytecode()->addInsn(BC_SLOAD0);
	else
	{
		bytecode()->addInsn(BC_SLOAD);
		bytecode()->addInt16(m_code->makeStringConstant(node->literal()));
	}
}

void Generator::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
	if (node->literal() == 0.0) bytecode()->addInsn(BC_DLOAD0);
	else if (node->literal() == 1.0) bytecode()->addInsn(BC_DLOAD1);
	else if (node->literal() == -1.0) bytecode()->addInsn(BC_DLOADM1);
	else
	{
		bytecode()->addInsn(BC_DLOAD);
		bytecode()->addTyped(node->literal());
	}
}

void Generator::visitIntLiteralNode(IntLiteralNode *node)
{
	if (node->literal() == 0) bytecode()->addInsn(BC_ILOAD0);
	else if (node->literal() == 1) bytecode()->addInsn(BC_ILOAD1);
	else if (node->literal() == -1) bytecode()->addInsn(BC_ILOADM1);
	else
	{
		bytecode()->addInsn(BC_ILOAD);
		bytecode()->addTyped(node->literal());
	}
}

void Generator::visitLoadNode(LoadNode *node)
{
    load_variable(node->var());
}

void Generator::visitStoreNode(StoreNode *node)
{
    AstVar const * const var = node->var();
    if (node->op() == tASSIGN)
    {
        switch (var->type())
        {
        case VT_STRING:
            eval_string(node->value());
	        break;
        case VT_DOUBLE:
            eval_double(node->value());
	        break;
        case VT_INT:
            eval_int(node->value());
	        break;
    	default: assert(0);
        }
    }
    else
    {
        switch (var->type())
        {
        case VT_DOUBLE:
            eval_double(node->value());
            load_variable(var);
            if (node->op() == tINCRSET) bytecode()->addInsn(BC_DADD);
            else bytecode()->addInsn(BC_DSUB);
	        break;
        case VT_INT:
            eval_int(node->value());
            load_variable(var);
            if (node->op() == tINCRSET) bytecode()->addInsn(BC_IADD);
            else bytecode()->addInsn(BC_ISUB);
	        break;
        default: assert(0);
        }
    }
    save_variable(var);
}

void Generator::visitForNode(ForNode *node)
{
    BinaryOpNode *bin = node->inExpr()->asBinaryOpNode();
    assert(type(bin->left()) == VT_INT && type(bin->right()) == VT_INT);
    AstVar const * const var = node->var();
    uint32_t jump, repeat;
    eval_int(bin->right());
    eval_int(bin->left());
    repeat = bytecode()->current();
    dup_int();
    save_variable(var);
    bytecode()->addInsn(BC_IFICMPG);
    jump = bytecode()->current();
    bytecode()->addInt16(0);
    node->body()->visit(this);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    bytecode()->addInsn(BC_JA);
    bytecode()->addInt16(repeat - bytecode()->current());
    bytecode()->setInt16(jump, bytecode()->current() - jump);
    bytecode()->addInsn(BC_POP);
    bytecode()->addInsn(BC_POP);
}

void Generator::visitWhileNode(WhileNode *node)
{
    bytecode()->addInsn(BC_ILOAD0);
    uint32_t repeat = bytecode()->current();
    eval_logic(node->whileExpr());
    bytecode()->addInsn(BC_IFICMPE);
    uint32_t jump = bytecode()->current();
    bytecode()->addInt16(0);
    node->loopBlock()->visit(this);
    bytecode()->addInsn(BC_POP);    
    bytecode()->addInsn(BC_JA);
    bytecode()->addInt16(repeat - bytecode()->current());
    bytecode()->setInt16(jump, bytecode()->current() - jump);
    bytecode()->addInsn(BC_POP);
}

void Generator::visitIfNode(IfNode *node)
{
    bool else_exists = node->elseBlock() && node->elseBlock()->nodes();
    bytecode()->addInsn(BC_ILOAD0);
    eval_logic(node->ifExpr());
    bytecode()->addInsn(BC_IFICMPE);
    uint32_t jump_to_else = bytecode()->current();
    bytecode()->addInt16(0);
    node->thenBlock()->visit(this);
    uint32_t jump_through_else = 0;
    if (else_exists)
    {
        bytecode()->addInsn(BC_JA);
        jump_through_else = bytecode()->current();
        bytecode()->addInt16(0);
    }
    bytecode()->setInt16(jump_to_else, bytecode()->current() - jump_to_else);
	if (else_exists)
	{
	    node->elseBlock()->visit(this);
	    bytecode()->setInt16(jump_through_else, bytecode()->current() - jump_through_else);
    }
    bytecode()->addInsn(BC_POP);
	bytecode()->addInsn(BC_POP);
}

void Generator::visitBlockNode(BlockNode *node)
{
    push_scope(node->scope());
    visit_scope(node->scope());
	for (uint32_t i = 0; i != node->nodes(); ++i)
		node->nodeAt(i)->visit(this);
    pop_scope();
}

void Generator::visitFunctionNode(FunctionNode *node)
{
	AstFunction *function = lookup_function(node->name());
	push_bytecode(lookup_function(function));
	if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
		node->body()->nodeAt(0)->visit(this);
	else
	{
		push_scope(node->body()->scope()->parent());
		visit_scope(node->body()->scope()->parent());
		for (uint32_t i = 0; i != node->parametersNumber(); ++i)
		{
			AstVar *local = lookup_variable(node->parameterName(i));
			save_variable(local);
		}
		node->body()->visit(this);
		pop_scope();
	}
	pop_bytecode();
}

void Generator::visitReturnNode(ReturnNode *node)
{
    switch (type(node))
    {
    case VT_INT:
        eval_int(node->returnExpr());
        bytecode()->addInsn(BC_STOREIVAR0);
        break;
    case VT_DOUBLE:
        eval_double(node->returnExpr());
        bytecode()->addInsn(BC_STOREDVAR0);
        break;
    case VT_STRING:
        eval_string(node->returnExpr());
        bytecode()->addInsn(BC_STORESVAR0);
        break;
    default: break;
    }
    bytecode()->addInsn(BC_RETURN);
}

void Generator::visitCallNode(CallNode *node)
{
	AstFunction *function = lookup_function(node->name());
	uint16_t id = lookup_function(function)->id();
	for (uint32_t i = 0; i != node->parametersNumber(); ++i)
	{
	    uint32_t pos = node->parametersNumber() - i - 1;
	    switch (function->parameterType(pos))
	    {
	    case VT_INT:
	        eval_int(node->parameterAt(pos));
	        break;
        case VT_DOUBLE:
	        eval_double(node->parameterAt(pos));
	        break;
        case VT_STRING:
	        eval_string(node->parameterAt(pos));
	        break;
        default: assert(0);
	    }
	}
	bytecode()->addInsn(BC_CALL);
	bytecode()->addInt16(id);
	switch (function->returnType())
	{
	case VT_INT:
	    bytecode()->addInsn(BC_LOADIVAR0);
	    break;
	case VT_DOUBLE:
	    bytecode()->addInsn(BC_LOADDVAR0);
	    break;
	case VT_STRING:
	    bytecode()->addInsn(BC_LOADSVAR0);
	    break;
    default: break;
	}
}

void Generator::visitNativeCallNode(NativeCallNode *node)
{
    static XMMReg dregs[] = {
        xmm0 , xmm1 , xmm2 , xmm3 ,
        xmm4 , xmm5 , xmm6 , xmm7 ,
        xmm8 , xmm9 , xmm10, xmm11,
        xmm12, xmm13, xmm14, xmm15 };
    static GPReg cregs[] = {
        ndi, nsi, ndx, ncx, r8, r9
    };
    Assembler asmb;
    asmb.push(nbp);
    asmb.mov(nbp, nsp);
    if (node->nativeSignature().size() > 1)
    {
        asmb.mov(nax, ndi);
        size_t dregpos = 0, cregpos = 0, shift = 0, it = 1;
        for (; it < node->nativeSignature().size(); ++it)
        {
            if (node->nativeSignature()[it].first == VT_DOUBLE)
            {
                asmb.movsd(dregs[dregpos], qword_ptr(nax, shift));
                shift += sizeof(double);
                ++dregpos;
            }
            else
            {
                asmb.mov(cregs[cregpos], qword_ptr(nax, shift));
                ++cregpos;
                if (node->nativeSignature()[it].first == VT_INT) shift += sizeof(int64_t);
                else shift += sizeof(char const *);
            }
        }
    }
    asmb.call(imm((sysint_t)node->info()));
    asmb.mov(nsp, nbp);
    asmb.pop(nbp);
    asmb.ret();
	uint16_t id = m_code->makeNativeFunction(node->nativeName(),
	                                         node->nativeSignature(),
	                                         asmb.make());
	bytecode()->addInsn(BC_CALLNATIVE);
	bytecode()->addInt16(id);
	bytecode()->addInsn(BC_RETURN);
}

void Generator::visitPrintNode(PrintNode *node)
{
	for (uint32_t i = 0; i != node->operands(); ++i)
	{
		switch (type(node->operandAt(i)))
		{
		case VT_INT:
		    eval_int(node->operandAt(i));
			bytecode()->addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
		    eval_double(node->operandAt(i));
			bytecode()->addInsn(BC_DPRINT);
			break;
		case VT_STRING:
		    eval_string(node->operandAt(i));
			bytecode()->addInsn(BC_SPRINT);
			break;
		default: assert(0);
		}
	}
}

void Generator::visit_scope(Scope *scope)
{
	Scope::VarIterator ivar(scope);
	while (ivar.hasNext())
	{
		AstVar *var = ivar.next();
		declare_variable(var);
	}
	
	Scope::FunctionIterator ifun(scope);
	while (ifun.hasNext()) declare_function(ifun.next());
	
	ifun = Scope::FunctionIterator(scope);
	while (ifun.hasNext()) ifun.next()->node()->visit(this);
}

void Generator::save_variable(AstVar const * const var)
{
    std::pair<uint16_t, uint16_t> var_id = lookup_variable(var);
	switch (var->type())
	{
	case VT_INT:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_STORECTXIVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STOREIVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_DOUBLE:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_STORECTXDVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STOREDVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_STRING:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_STORECTXSVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_STORESVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	default: assert(0);
	}
}

void Generator::load_variable(AstVar const * const var)
{
	std::pair<uint16_t, uint16_t> var_id = lookup_variable(var);
	switch (var->type())
	{
	case VT_INT:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_LOADCTXIVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADIVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_DOUBLE:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_LOADCTXDVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADDVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	case VT_STRING:
		if (var_id.first != current_id())
		{
			bytecode()->addInsn(BC_LOADCTXSVAR);
			bytecode()->addInt16(var_id.first);
			bytecode()->addInt16(var_id.second);
		}
		else
		{
			bytecode()->addInsn(BC_LOADSVAR);
			bytecode()->addInt16(var_id.second);
		}
		break;
	default: assert(0);
	}
}

void Generator::eval_int(AstNode *node)
{
    node->visit(this);
    switch (type(node))
    {
    case VT_DOUBLE:
        bytecode()->addInsn(BC_D2I);
        break;
    case VT_STRING:
        bytecode()->addInsn(BC_S2I);
        break;
    case VT_VOID:
    case VT_INVALID:
        assert(0);
        break;
    default: break;
    }
}

void Generator::eval_double(AstNode *node)
{
    node->visit(this);
    switch (type(node))
    {
    case VT_INT:
        bytecode()->addInsn(BC_I2D);
        break;
    case VT_STRING:
    case VT_VOID:
    case VT_INVALID:
        assert(0);
        break;
    default: break;
    }
}

void Generator::eval_string(AstNode *node)
{
    node->visit(this);
    switch (type(node))
    {
    case VT_INT:
    case VT_DOUBLE:
    case VT_VOID:
    case VT_INVALID:
        assert(0);
        break;
    default: break;
    }
}

void Generator::eval_logic(AstNode *node)
{
    switch (type(node))
    {
    case VT_DOUBLE:
        bytecode()->addInsn(BC_DLOAD0);
        eval_double(node);
        bytecode()->addInsn(BC_DCMP);
        break;
    case VT_INT: case VT_STRING:
        eval_int(node);
        break;
    default: assert(0);
    }
}

void Generator::dup_string()
{
    bytecode()->addInsn(BC_STORESVAR0);
    bytecode()->addInsn(BC_LOADSVAR0);
    bytecode()->addInsn(BC_LOADSVAR0);
}

void Generator::dup_int()
{
    bytecode()->addInsn(BC_STOREIVAR0);
    bytecode()->addInsn(BC_LOADIVAR0);
    bytecode()->addInsn(BC_LOADIVAR0);
}

void Generator::dup_double()
{
    bytecode()->addInsn(BC_STOREDVAR0);
    bytecode()->addInsn(BC_LOADDVAR0);
    bytecode()->addInsn(BC_LOADDVAR0);
}
