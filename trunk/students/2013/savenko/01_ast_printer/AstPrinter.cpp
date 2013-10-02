#include "AstPrinter.h"
#include <iostream>

namespace mathvm {

namespace _ast_printer_utils {
  
  const std::string VT_INVALID_STR("invalid");
  const std::string VT_VOID_STR("void");
  const std::string VT_DOUBLE_STR("double");
  const std::string VT_INT_STR("int");
  const std::string VT_STRING_STR("string");
  
  std::string const & getVarTypeString(VarType type) {
    switch(type) {
      case VT_VOID    : return VT_VOID_STR;
      case VT_DOUBLE  : return VT_DOUBLE_STR;
      case VT_INT     : return VT_INT_STR;
      case VT_STRING  : return VT_STRING_STR;
      default         : return VT_INVALID_STR;
    }
  }
    
  void printFunctionParameters(FunctionNode * functionNode) {
    for (uint32_t i = 0; i != functionNode->parametersNumber(); ++i) {
      std::cout << getVarTypeString(functionNode->parameterType(i))
                << " "
                << functionNode->parameterName(i);
      if (i != functionNode->parametersNumber()) {
        std::cout << ", ";
      }
    }
  }

}

// visitor implementation

using namespace _ast_printer_utils;

void AstPrinter::visitBinaryOpNode(BinaryOpNode * binaryOpNode) {
  std::cout << "binop" << std::endl;
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode * unaryOpNode) {
  std::cout << "uop" << std::endl;
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode * stringLiteralNode) {
  std::cout << "string literal" << std::endl;
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode * doubleLiteralNode) {
  std::cout << "double literal" << std::endl;
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode * IntLiteralNode) {
  std::cout << "IntLiteral" << std::endl;
}

void AstPrinter::visitLoadNode(LoadNode * LoadNode) {
  std::cout << "Load" << std::endl;
}

void AstPrinter::visitStoreNode(StoreNode * StoreNode) {
  std::cout << "Store" << std::endl;
}

void AstPrinter::visitForNode(ForNode * ForNode) {
  std::cout << "For" << std::endl;
}

void AstPrinter::visitWhileNode(WhileNode * WhileNode) {
  std::cout << "While" << std::endl;
}

void AstPrinter::visitIfNode(IfNode * IfNode) {
  std::cout << "If" << std::endl;
}

void AstPrinter::visitBlockNode(BlockNode * BlockNode) {
  std::cout << "Block" << std::endl;
}

void AstPrinter::visitFunctionNode(FunctionNode * functionNode) {
  std::cout << "function "
            << getVarTypeString(functionNode->returnType())
            << " " 
            << functionNode->name()
            << "(";
  printFunctionParameters(functionNode);         
  std::cout << ")"
            << std::endl;
}

void AstPrinter::visitReturnNode(ReturnNode * ReturnNode) {
  std::cout << "Return" << std::endl;
}

void AstPrinter::visitCallNode(CallNode * CallNode) {
  std::cout << "Call" << std::endl;
}

void AstPrinter::visitNativeCallNode(NativeCallNode * NativeCallNode) {
  std::cout << "NativeCall" << std::endl;
}

void AstPrinter::visitPrintNode(PrintNode * PrintNode) {
  std::cout << "Print" << std::endl;
}


}
