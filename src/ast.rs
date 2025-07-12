use crate::types::{VarType, Signature};

/// Binary operators in the MVM language
#[derive(Debug, Clone, PartialEq)]
pub enum BinaryOp {
    // Arithmetic
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    
    // Comparison
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    
    // Logical
    And,
    Or,
    
    // Bitwise
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    
    // Assignment
    Assign,
    AssignAdd,
    AssignSubtract,
    AssignMultiply,
    AssignDivide,
}

/// Unary operators in the MVM language
#[derive(Debug, Clone, PartialEq)]
pub enum UnaryOp {
    Plus,
    Minus,
    Not,
}

/// AST node representing any expression
#[derive(Debug, Clone)]
pub enum AstNode {
    /// Program root - contains a list of statements
    Program(Vec<AstNode>),
    
    /// Block statement - contains a list of statements in a new scope
    Block(Vec<AstNode>),
    
    /// Variable declaration
    VarDeclaration {
        var_type: VarType,
        name: String,
        initializer: Option<Box<AstNode>>,
    },
    
    /// Function declaration
    FunctionDeclaration {
        name: String,
        signature: Signature,
        body: Box<AstNode>,
    },
    
    /// Function call
    FunctionCall {
        name: String,
        arguments: Vec<AstNode>,
    },
    
    /// Print statement
    Print(Vec<AstNode>),
    
    /// If statement
    If {
        condition: Box<AstNode>,
        then_branch: Box<AstNode>,
        else_branch: Option<Box<AstNode>>,
    },
    
    /// While loop
    While {
        condition: Box<AstNode>,
        body: Box<AstNode>,
    },
    
    /// For loop
    For {
        variable: String,
        start: Box<AstNode>,
        end: Box<AstNode>,
        body: Box<AstNode>,
    },
    
    /// Return statement
    Return(Option<Box<AstNode>>),
    
    /// Expression statement
    Expression(Box<AstNode>),
    
    /// Binary operation
    BinaryOp {
        left: Box<AstNode>,
        op: BinaryOp,
        right: Box<AstNode>,
    },
    
    /// Unary operation
    UnaryOp {
        op: UnaryOp,
        operand: Box<AstNode>,
    },
    
    /// Variable reference
    Variable(String),
    
    /// Integer literal
    IntLiteral(i64),
    
    /// Double literal
    DoubleLiteral(f64),
    
    /// String literal
    StringLiteral(String),
}

impl AstNode {
    /// Create a new program node
    pub fn program(statements: Vec<AstNode>) -> Self {
        AstNode::Program(statements)
    }
    
    /// Create a new block node
    pub fn block(statements: Vec<AstNode>) -> Self {
        AstNode::Block(statements)
    }
    
    /// Create a new variable declaration
    pub fn var_declaration(var_type: VarType, name: String, initializer: Option<AstNode>) -> Self {
        AstNode::VarDeclaration {
            var_type,
            name,
            initializer: initializer.map(Box::new),
        }
    }
    
    /// Create a new function declaration
    pub fn function_declaration(name: String, signature: Signature, body: AstNode) -> Self {
        AstNode::FunctionDeclaration {
            name,
            signature,
            body: Box::new(body),
        }
    }
    
    /// Create a new function call
    pub fn function_call(name: String, arguments: Vec<AstNode>) -> Self {
        AstNode::FunctionCall { name, arguments }
    }
    
    /// Create a new print statement
    pub fn print(arguments: Vec<AstNode>) -> Self {
        AstNode::Print(arguments)
    }
    
    /// Create a new if statement
    pub fn if_statement(condition: AstNode, then_branch: AstNode, else_branch: Option<AstNode>) -> Self {
        AstNode::If {
            condition: Box::new(condition),
            then_branch: Box::new(then_branch),
            else_branch: else_branch.map(Box::new),
        }
    }
    
    /// Create a new while loop
    pub fn while_loop(condition: AstNode, body: AstNode) -> Self {
        AstNode::While {
            condition: Box::new(condition),
            body: Box::new(body),
        }
    }
    
    /// Create a new for loop
    pub fn for_loop(variable: String, start: AstNode, end: AstNode, body: AstNode) -> Self {
        AstNode::For {
            variable,
            start: Box::new(start),
            end: Box::new(end),
            body: Box::new(body),
        }
    }
    
    /// Create a new return statement
    pub fn return_statement(value: Option<AstNode>) -> Self {
        AstNode::Return(value.map(Box::new))
    }
    
    /// Create a new expression statement
    pub fn expression(expr: AstNode) -> Self {
        AstNode::Expression(Box::new(expr))
    }
    
    /// Create a new binary operation
    pub fn binary_op(left: AstNode, op: BinaryOp, right: AstNode) -> Self {
        AstNode::BinaryOp {
            left: Box::new(left),
            op,
            right: Box::new(right),
        }
    }
    
    /// Create a new unary operation
    pub fn unary_op(op: UnaryOp, operand: AstNode) -> Self {
        AstNode::UnaryOp {
            op,
            operand: Box::new(operand),
        }
    }
    
    /// Create a new variable reference
    pub fn variable(name: String) -> Self {
        AstNode::Variable(name)
    }
    
    /// Create a new integer literal
    pub fn int_literal(value: i64) -> Self {
        AstNode::IntLiteral(value)
    }
    
    /// Create a new double literal
    pub fn double_literal(value: f64) -> Self {
        AstNode::DoubleLiteral(value)
    }
    
    /// Create a new string literal
    pub fn string_literal(value: String) -> Self {
        AstNode::StringLiteral(value)
    }
    
    /// Check if this node is a statement (can appear at the top level)
    pub fn is_statement(&self) -> bool {
        matches!(self, 
            AstNode::VarDeclaration { .. } |
            AstNode::FunctionDeclaration { .. } |
            AstNode::Print(_) |
            AstNode::If { .. } |
            AstNode::While { .. } |
            AstNode::For { .. } |
            AstNode::Return(_) |
            AstNode::Expression(_) |
            AstNode::Block(_)
        )
    }
    
    /// Check if this node is an expression (has a value)
    pub fn is_expression(&self) -> bool {
        matches!(self,
            AstNode::BinaryOp { .. } |
            AstNode::UnaryOp { .. } |
            AstNode::Variable(_) |
            AstNode::IntLiteral(_) |
            AstNode::DoubleLiteral(_) |
            AstNode::StringLiteral(_) |
            AstNode::FunctionCall { .. }
        )
    }
}

/// Visitor trait for traversing the AST
pub trait AstVisitor<T> {
    fn visit(&mut self, node: &AstNode) -> T;
    
    fn visit_program(&mut self, statements: &[AstNode]) -> T;
    fn visit_block(&mut self, statements: &[AstNode]) -> T;
    fn visit_var_declaration(&mut self, var_type: VarType, name: &str, initializer: &Option<Box<AstNode>>) -> T;
    fn visit_function_declaration(&mut self, name: &str, signature: &Signature, body: &AstNode) -> T;
    fn visit_function_call(&mut self, name: &str, arguments: &[AstNode]) -> T;
    fn visit_print(&mut self, arguments: &[AstNode]) -> T;
    fn visit_if(&mut self, condition: &AstNode, then_branch: &AstNode, else_branch: &Option<Box<AstNode>>) -> T;
    fn visit_while(&mut self, condition: &AstNode, body: &AstNode) -> T;
    fn visit_for(&mut self, variable: &str, start: &AstNode, end: &AstNode, body: &AstNode) -> T;
    fn visit_return(&mut self, value: &Option<Box<AstNode>>) -> T;
    fn visit_expression(&mut self, expr: &AstNode) -> T;
    fn visit_binary_op(&mut self, left: &AstNode, op: &BinaryOp, right: &AstNode) -> T;
    fn visit_unary_op(&mut self, op: &UnaryOp, operand: &AstNode) -> T;
    fn visit_variable(&mut self, name: &str) -> T;
    fn visit_int_literal(&mut self, value: i64) -> T;
    fn visit_double_literal(&mut self, value: f64) -> T;
    fn visit_string_literal(&mut self, value: &str) -> T;
}

/// Pretty printer for the AST
pub struct AstPrinter {
    indent: usize,
}

impl AstPrinter {
    pub fn new() -> Self {
        AstPrinter { indent: 0 }
    }
    
    pub fn print(&mut self, node: &AstNode) -> String {
        self.visit(node)
    }
    
    fn indent_str(&self) -> String {
        "  ".repeat(self.indent)
    }
    
    fn with_indent<F>(&mut self, f: F) -> String
    where
        F: FnOnce(&mut Self) -> String,
    {
        self.indent += 1;
        let result = f(self);
        self.indent -= 1;
        result
    }
}

impl Default for AstPrinter {
    fn default() -> Self {
        Self::new()
    }
}

impl AstVisitor<String> for AstPrinter {
    fn visit(&mut self, node: &AstNode) -> String {
        match node {
            AstNode::Program(statements) => self.visit_program(statements),
            AstNode::Block(statements) => self.visit_block(statements),
            AstNode::VarDeclaration { var_type, name, initializer } => {
                self.visit_var_declaration(*var_type, name, initializer)
            }
            AstNode::FunctionDeclaration { name, signature, body } => {
                self.visit_function_declaration(name, signature, body)
            }
            AstNode::FunctionCall { name, arguments } => {
                self.visit_function_call(name, arguments)
            }
            AstNode::Print(arguments) => self.visit_print(arguments),
            AstNode::If { condition, then_branch, else_branch } => {
                self.visit_if(condition, then_branch, else_branch)
            }
            AstNode::While { condition, body } => {
                self.visit_while(condition, body)
            }
            AstNode::For { variable, start, end, body } => {
                self.visit_for(variable, start, end, body)
            }
            AstNode::Return(value) => self.visit_return(value),
            AstNode::Expression(expr) => self.visit_expression(expr),
            AstNode::BinaryOp { left, op, right } => {
                self.visit_binary_op(left, op, right)
            }
            AstNode::UnaryOp { op, operand } => {
                self.visit_unary_op(op, operand)
            }
            AstNode::Variable(name) => self.visit_variable(name),
            AstNode::IntLiteral(value) => self.visit_int_literal(*value),
            AstNode::DoubleLiteral(value) => self.visit_double_literal(*value),
            AstNode::StringLiteral(value) => self.visit_string_literal(value),
        }
    }
    
    fn visit_program(&mut self, statements: &[AstNode]) -> String {
        statements.iter()
            .map(|stmt| self.visit(stmt))
            .collect::<Vec<_>>()
            .join("\n")
    }
    
    fn visit_block(&mut self, statements: &[AstNode]) -> String {
        let mut result = format!("{}\n", self.indent_str());
        result.push_str(&self.with_indent(|printer| {
            statements.iter()
                .map(|stmt| format!("{}{}", printer.indent_str(), printer.visit(stmt)))
                .collect::<Vec<_>>()
                .join("\n")
        }));
        result.push_str(&format!("\n{}}}", self.indent_str()));
        result
    }
    
    fn visit_var_declaration(&mut self, var_type: VarType, name: &str, initializer: &Option<Box<AstNode>>) -> String {
        match initializer {
            Some(init) => format!("{} {} = {};", var_type, name, self.visit(init)),
            None => format!("{} {};", var_type, name),
        }
    }
    
    fn visit_function_declaration(&mut self, name: &str, signature: &Signature, body: &AstNode) -> String {
        let params = signature.parameters.iter()
            .map(|(param_name, param_type)| format!("{} {}", param_type, param_name))
            .collect::<Vec<_>>()
            .join(", ");
        
        format!("function {} {}({}) {}", signature.return_type, name, params, self.visit(body))
    }
    
    fn visit_function_call(&mut self, name: &str, arguments: &[AstNode]) -> String {
        let args = arguments.iter()
            .map(|arg| self.visit(arg))
            .collect::<Vec<_>>()
            .join(", ");
        format!("{}({})", name, args)
    }
    
    fn visit_print(&mut self, arguments: &[AstNode]) -> String {
        let args = arguments.iter()
            .map(|arg| self.visit(arg))
            .collect::<Vec<_>>()
            .join(", ");
        format!("print({});", args)
    }
    
    fn visit_if(&mut self, condition: &AstNode, then_branch: &AstNode, else_branch: &Option<Box<AstNode>>) -> String {
        let mut result = format!("if ({}) {}", self.visit(condition), self.visit(then_branch));
        if let Some(else_part) = else_branch {
            result.push_str(&format!(" else {}", self.visit(else_part)));
        }
        result
    }
    
    fn visit_while(&mut self, condition: &AstNode, body: &AstNode) -> String {
        format!("while ({}) {}", self.visit(condition), self.visit(body))
    }
    
    fn visit_for(&mut self, variable: &str, start: &AstNode, end: &AstNode, body: &AstNode) -> String {
        format!("for ({} in {}..{}) {}", variable, self.visit(start), self.visit(end), self.visit(body))
    }
    
    fn visit_return(&mut self, value: &Option<Box<AstNode>>) -> String {
        match value {
            Some(val) => format!("return {};", self.visit(val)),
            None => "return;".to_string(),
        }
    }
    
    fn visit_expression(&mut self, expr: &AstNode) -> String {
        format!("{};", self.visit(expr))
    }
    
    fn visit_binary_op(&mut self, left: &AstNode, op: &BinaryOp, right: &AstNode) -> String {
        let op_str = match op {
            BinaryOp::Add => "+",
            BinaryOp::Subtract => "-",
            BinaryOp::Multiply => "*",
            BinaryOp::Divide => "/",
            BinaryOp::Modulo => "%",
            BinaryOp::Equal => "==",
            BinaryOp::NotEqual => "!=",
            BinaryOp::Less => "<",
            BinaryOp::LessEqual => "<=",
            BinaryOp::Greater => ">",
            BinaryOp::GreaterEqual => ">=",
            BinaryOp::And => "&&",
            BinaryOp::Or => "||",
            BinaryOp::BitwiseAnd => "&",
            BinaryOp::BitwiseOr => "|",
            BinaryOp::BitwiseXor => "^",
            BinaryOp::Assign => "=",
            BinaryOp::AssignAdd => "+=",
            BinaryOp::AssignSubtract => "-=",
            BinaryOp::AssignMultiply => "*=",
            BinaryOp::AssignDivide => "/=",
        };
        format!("({} {} {})", self.visit(left), op_str, self.visit(right))
    }
    
    fn visit_unary_op(&mut self, op: &UnaryOp, operand: &AstNode) -> String {
        let op_str = match op {
            UnaryOp::Plus => "+",
            UnaryOp::Minus => "-",
            UnaryOp::Not => "!",
        };
        format!("({}{})", op_str, self.visit(operand))
    }
    
    fn visit_variable(&mut self, name: &str) -> String {
        name.to_string()
    }
    
    fn visit_int_literal(&mut self, value: i64) -> String {
        value.to_string()
    }
    
    fn visit_double_literal(&mut self, value: f64) -> String {
        value.to_string()
    }
    
    fn visit_string_literal(&mut self, value: &str) -> String {
        format!("'{}'", value)
    }
}