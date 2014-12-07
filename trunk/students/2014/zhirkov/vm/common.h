#pragma once

#define FOR_NODES(DO)                           \
            DO(BinaryOpNode, "binary")          \
            DO(UnaryOpNode, "unary")            \
            DO(StringLiteralNode, "string literal") \
            DO(DoubleLiteralNode, "double literal") \
            DO(IntLiteralNode, "int literal")   \
            DO(LoadNode, "loadval")             \
            DO(StoreNode, "storeval")           \
            DO(ForNode, "for")                  \
            DO(WhileNode, "while")              \
            DO(IfNode, "if")                    \
            DO(BlockNode, "block")              \
            DO(FunctionNode, "function")        \
            DO(ReturnNode, "return")            \
            DO(CallNode, "call")                \
            DO(NativeCallNode, "native call")   \
            DO(PrintNode, "print")

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);


