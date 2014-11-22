#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include <cstdio>

namespace mathvm{
	class PrinterVisitor: public AstVisitor{
		private:
		typedef unsigned int uint;
		const char* varTypeToString(VarType t){
			switch(t){
				case VT_VOID: return "void";
				case VT_DOUBLE: return "double";
				case VT_INT: return "int";
				case VT_STRING: return "string";
				default: return "badtype";
			}
			return "?:(";
		}
		const char * kindToString(TokenKind k){
			switch(k){
			#define CASE_ELEM(t, s, p) case(t): return s;
				FOR_TOKENS(CASE_ELEM)
				
			#undef CASE_ELEM
				default: return "";
			}
			return "?:(";
		}
		const char * escape(const string & s){
			string buf;
			for (uint i = 0 ; i < s.size(); ++i){
				if (s[i] == '\n'){
					buf += "\\n";
				}else
				if (s[i] == '\\'){
					buf += "\\\\";
				}else{
					buf += s[i];
				}
			}
			return buf.c_str();
		}
		void printScope(Scope* s){
			Scope::VarIterator vi(s);
			while (vi.hasNext()){
				AstVar * v = vi.next();
				printf("%s %s;\n", varTypeToString(v->type()), 
									v->name().c_str());
			}
			Scope::FunctionIterator fi(s);
			while (fi.hasNext()){
				fi.next()->node()->visit(this);
			}
		}
		public:
		virtual void visitFunctionNode(FunctionNode * n){
			if (n->name() != "<top>"){
				printf("function %s %s(", varTypeToString(n->returnType())
										, n->name().c_str());
				for (uint i = 0; i < n->parametersNumber(); ++i){
					if ( i != 0 ) printf(",");
					printf("%s %s", varTypeToString(n->parameterType(i))
									, n->parameterName(i).c_str());
				}
				printf("){\n");
			}
			n->visitChildren(this);
			if (n->name() != "<top>"){printf("}\n");}
		}

        virtual void visitBinaryOpNode(BinaryOpNode *n){
			printf("(");
			n->left()->visit(this);
			printf(" %s ", kindToString(n->kind()));
			n->right()->visit(this);
			printf(")");
		}
        virtual void visitUnaryOpNode(UnaryOpNode *n){
			printf("%s", kindToString(n->kind()));
			n->visitChildren(this);	
		}
        virtual void visitStringLiteralNode(StringLiteralNode *n){
			printf("\'%s\'", escape(n->literal()));
		}
        virtual void visitDoubleLiteralNode(DoubleLiteralNode *n){
			printf("%f", n->literal());	
		}
        virtual void visitIntLiteralNode(IntLiteralNode *n){
			printf("%ld", n->literal());
		}
        virtual void visitLoadNode(LoadNode *n){
			printf("%s", n->var()->name().c_str());
		}
        virtual void visitStoreNode(StoreNode *n){
			printf("%s %s ", n->var()->name().c_str(), kindToString(n->op()));
			n->value()->visit(this);
		}
        virtual void visitForNode(ForNode *n){
			printf("for (%s in", n->var()->name().c_str());
			n->inExpr()->visit(this);
			printf(") {\n");
			n->body()->visit(this);
			printf("}\n");
		}
        virtual void visitWhileNode(WhileNode *n){
			printf("while (");
			n->whileExpr()->visit(this);
			printf(") {\n");
			n->loopBlock()->visit(this);
			printf("}\n");
		}
        virtual void visitIfNode(IfNode *n){
			printf("if (");
			n->ifExpr()->visit(this);
			printf(") {\n");
			n->thenBlock()->visit(this);
			if (n->elseBlock()){
				printf("} else {\n");
				n->elseBlock()->visit(this);
			} 
			printf("}\n");
		}
        virtual void visitBlockNode(BlockNode *n){
			printScope(n->scope());
			for (uint i = 0; i < n->nodes(); ++i){
				n->nodeAt(i)->visit(this);
				printf(";\n");
			}
		}
        virtual void visitReturnNode(ReturnNode *n){
			printf("return ");
			n->returnExpr()->visit(this);
		}
        virtual void visitCallNode(CallNode *n){
			printf("%s(", n->name().c_str());
			for(uint i = 0; i < n->parametersNumber(); ++i){
				if (i != 0) printf(", ");
				n->parameterAt(i)->visit(this);
			}
			printf(")");			
		}
        virtual void visitNativeCallNode(NativeCallNode *n){
			printf("%s", n->nativeName().c_str());
		}
        virtual void visitPrintNode(PrintNode *n){
			printf("print(");
			for (uint i = 0; i < n->operands(); ++i){
				if (i != 0) printf(", ");
				n->operandAt(i)->visit(this);
			}
			printf(")");
		}

	};
	class AstPrinter: public Translator{
	 public:
	  virtual Status* translate(const string & program, Code* *code){
		Parser parser;
		Status* status = parser.parseProgram(program);
		if (status) return status;
		// impl printer using parser.top() as root of the AST
		parser.top()->node()->visit(new PrinterVisitor());
		return new Status("No executable code produced");
	  }
	};

	
	Translator* Translator::create(const string& impl){
	 if (impl == "printer"){
	   return new AstPrinter();
	 } else {
	  return NULL;
	 }
	}
}  
