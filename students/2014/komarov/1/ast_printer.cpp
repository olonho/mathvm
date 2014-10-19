#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast.h"

using namespace mathvm;

class AstPrinterVisitor: public AstVisitor {

public:
	AstPrinterVisitor(std::ostream &out_stream_): 
		out_stream(out_stream_) {
	}

	virtual void visitBinaryOpNode(BinaryOpNode *node) {
		out_stream << "(";		
		node->left()->visit(this);
		out_stream << " " << tokenOp(node->kind()) << " ";
		node->right()->visit(this);
		out_stream << ")";
	}

	virtual void visitUnaryOpNode(UnaryOpNode *node) {
		out_stream << " " << tokenOp(node->kind()) << " ";
		node->operand()->visit(this);	
	}   

	virtual void visitStringLiteralNode(StringLiteralNode *node) {
		out_stream << "\'" << escape(node->literal()) << "\'";
	}
   
   virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
		out_stream << showpoint << node->literal();
	}
   
	virtual void visitIntLiteralNode(IntLiteralNode *node) {
		out_stream << node->literal();
	}
   
	virtual void visitLoadNode(LoadNode *node) {
		out_stream << node->var()->name();
	} 
  
	virtual void visitStoreNode(StoreNode *node) {
		out_stream << node->var()->name() << " "
			<< tokenOp(node->op()) << " ";
		node->value()->visit(this);
	}      

	virtual void visitForNode(ForNode *node) {
		out_stream << "for (" 
			<< node->var()->name() << " in ";
		node->inExpr()->visit(this);
		out_stream << ") {\n";		
		node->body()->visit(this);
		out_stream << "}\n";		
	}   

	virtual void visitWhileNode(WhileNode *node) {
		out_stream << "while (";
		node->whileExpr()->visit(this);
		out_stream << ") {\n";		
		node->loopBlock()->visit(this);
		out_stream << "}\n";		
	}   

	virtual void visitIfNode(IfNode *node) {
		out_stream << "if (";
		node->ifExpr()->visit(this);
		out_stream << ") {\n";		
		node->thenBlock()->visit(this);
		out_stream << "}\n";	
		AstNode* else_block = node->elseBlock();
		if(else_block) {
			out_stream << "else {\n";	
			else_block->visit(this);
			out_stream << "}\n";	
		}		
	}   

	virtual void visitBlockNode(BlockNode *node) {
		Scope::VarIterator var_it(node->scope());
		while(var_it.hasNext()) {
			AstVar* var = var_it.next();
         out_stream << typeToName(var->type()) 
				<< " " << var->name() << ";\n";		
		}

		Scope::FunctionIterator fun_it(node->scope());
		while(fun_it.hasNext()) {
			AstFunction* fun = fun_it.next();
			fun->node()->visit(this);         
		}
		
		for (uint32_t i = 0; i < node->nodes(); i++) {
            AstNode* cur_node = node->nodeAt(i);
           	cur_node->visit(this);
				if (	cur_node->isLoadNode() || 
						cur_node->isStoreNode() ||
                	cur_node->isReturnNode() || 
                	cur_node->isCallNode() || 
                	cur_node->isPrintNode()				) {
                	out_stream << ";\n";
            }
      }		
	}      

	virtual void visitFunctionNode(FunctionNode *node) {
		out_stream << "function " 
			<< typeToName(node->returnType()) << " "
			<< node->name() << "(";		
		for(uint32_t i=0; i < node->parametersNumber(); i++) {
			if(i != 0) {
				out_stream	<< ", ";			
			}
			out_stream << typeToName(node->parameterType(i))
				<< " " << node->parameterName(i); 
		}
		out_stream << ") ";

		BlockNode* block = node->body();
		
		if(block->nodeAt(0) && block->nodeAt(0)->isNativeCallNode()) {
			block->nodeAt(0)->visit(this);		
		} else {
			out_stream << "{\n";
			block->visit(this);
			out_stream << "}\n";
		}
	}  

	virtual void visitReturnNode(ReturnNode *node) {
		out_stream << "return";		
		AstNode* return_expr = node->returnExpr();
		if(return_expr) {
			out_stream << " ";		
			return_expr->visit(this);
		}
	}   

	virtual void visitCallNode(CallNode *node) {
		out_stream << node->name() << "(";
		for (uint32_t i = 0; i < node->parametersNumber(); i++) {
			if(i != 0) {
				out_stream << ", ";	
			}
      	node->parameterAt(i)->visit(this);
      }			
		out_stream << ")";
	} 

   virtual void visitNativeCallNode(NativeCallNode *node) {
	 out_stream << "native \'"
    				<< node->nativeName() << "\';\n";
	} 
  
   virtual void visitPrintNode(PrintNode *node) {
		out_stream << "print(";	
		for (uint32_t i = 0; i < node->operands(); i++) {
			if(i != 0) {
				out_stream << ", ";	
			}
      	node->operandAt(i)->visit(this);
      }
		out_stream << ")";	
	}   

private:
	std::ostream &out_stream;
	
	string escape(string const& str) {
		string ans;
		for(unsigned int i = 0; i < str.size(); i++) {
			switch(str[i]){
				case '\\': ans += "\\\\"; break;
				case '\n': ans += "\\n"; break;	
				case '\t': ans += "\\t"; break;
				case '\r': ans += "\\r"; break;
				case '\'': ans += "\\'"; break;
				default: ans += str[i];
			}
		}
		return ans;
	}
};


class AstPrinter: public Translator {
public:
	virtual Status* translate(const string& program, Code** code) {
   	Parser parser;
      Status* status = parser.parseProgram(program);
      if (status != NULL && status->isError())
      	return status;

		AstPrinterVisitor printer_vistor(std::cout);
		BlockNode* top_block = parser.top()->node()->body();
		top_block->visit(&printer_vistor);

		return new Status();
    }
};


Translator *Translator::create(const string &impl) {
	if (impl == "printer") {
		return new AstPrinter();
   } else {
   	return NULL;
   }
}
