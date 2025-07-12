use crate::ast::{AstNode, AstVisitor, BinaryOp, UnaryOp};
use crate::bytecode::{Bytecode, Instruction};
use crate::error::{Error, Result};
use crate::types::{Signature, Value, VarType};
use std::collections::HashMap;
use std::io::{self, Write};

/// A function in the interpreter
#[derive(Debug, Clone)]
pub struct Function {
    pub name: String,
    pub signature: Signature,
    pub body: AstNode,
}

/// Variable scope for the interpreter
#[derive(Debug, Clone)]
pub struct Scope {
    variables: HashMap<String, Value>,
}

impl Scope {
    pub fn new() -> Self {
        Scope {
            variables: HashMap::new(),
        }
    }
    
    pub fn define(&mut self, name: String, value: Value) {
        self.variables.insert(name, value);
    }
    
    pub fn get(&self, name: &str) -> Option<&Value> {
        self.variables.get(name)
    }
    
    pub fn set(&mut self, name: &str, value: Value) -> Result<()> {
        if self.variables.contains_key(name) {
            self.variables.insert(name.to_string(), value);
            Ok(())
        } else {
            Err(Error::UndefinedVariable(name.to_string()))
        }
    }
}

impl Default for Scope {
    fn default() -> Self {
        Self::new()
    }
}

/// Stack-based bytecode interpreter
pub struct BytecodeInterpreter {
    stack: Vec<Value>,
    strings: Vec<String>,
    #[allow(dead_code)]
    scopes: Vec<Scope>,
    #[allow(dead_code)]
    functions: HashMap<String, Function>,
    output: Vec<String>,
}

impl BytecodeInterpreter {
    pub fn new() -> Self {
        BytecodeInterpreter {
            stack: Vec::new(),
            strings: Vec::new(),
            scopes: vec![Scope::new()],
            functions: HashMap::new(),
            output: Vec::new(),
        }
    }
    
    /// Execute bytecode instructions
    pub fn execute(&mut self, bytecode: &Bytecode) -> Result<()> {
        let mut ip = 0; // instruction pointer
        
        while ip < bytecode.len() {
            let instruction = bytecode.get_instruction(ip)?;
            ip += 1;
            
            match instruction {
                Instruction::Invalid => {
                    return Err(Error::runtime_error("Invalid instruction"));
                }
                
                // Load constants
                Instruction::DLoad => {
                    let value = bytecode.get_f64(ip)?;
                    self.stack.push(Value::Double(value));
                    ip += 8;
                }
                Instruction::ILoad => {
                    let value = bytecode.get_i64(ip)?;
                    self.stack.push(Value::Int(value));
                    ip += 8;
                }
                Instruction::SLoad => {
                    let string_id = bytecode.get_u16(ip)? as usize;
                    if string_id < self.strings.len() {
                        self.stack.push(Value::String(self.strings[string_id].clone()));
                    } else {
                        return Err(Error::runtime_error("Invalid string constant ID"));
                    }
                    ip += 2;
                }
                Instruction::DLoad0 => {
                    self.stack.push(Value::Double(0.0));
                }
                Instruction::ILoad0 => {
                    self.stack.push(Value::Int(0));
                }
                Instruction::SLoad0 => {
                    self.stack.push(Value::String(String::new()));
                }
                Instruction::DLoad1 => {
                    self.stack.push(Value::Double(1.0));
                }
                Instruction::ILoad1 => {
                    self.stack.push(Value::Int(1));
                }
                Instruction::DLoadM1 => {
                    self.stack.push(Value::Double(-1.0));
                }
                Instruction::ILoadM1 => {
                    self.stack.push(Value::Int(-1));
                }
                
                // Arithmetic operations
                Instruction::DAdd => {
                    let b = self.pop_double()?;
                    let a = self.pop_double()?;
                    self.stack.push(Value::Double(a + b));
                }
                Instruction::IAdd => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a + b));
                }
                Instruction::DSub => {
                    let b = self.pop_double()?;
                    let a = self.pop_double()?;
                    self.stack.push(Value::Double(a - b));
                }
                Instruction::ISub => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a - b));
                }
                Instruction::DMul => {
                    let b = self.pop_double()?;
                    let a = self.pop_double()?;
                    self.stack.push(Value::Double(a * b));
                }
                Instruction::IMul => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a * b));
                }
                Instruction::DDiv => {
                    let b = self.pop_double()?;
                    let a = self.pop_double()?;
                    if b == 0.0 {
                        return Err(Error::DivisionByZero);
                    }
                    self.stack.push(Value::Double(a / b));
                }
                Instruction::IDiv => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    if b == 0 {
                        return Err(Error::DivisionByZero);
                    }
                    self.stack.push(Value::Int(a / b));
                }
                Instruction::IMod => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    if b == 0 {
                        return Err(Error::DivisionByZero);
                    }
                    self.stack.push(Value::Int(a % b));
                }
                Instruction::DNeg => {
                    let a = self.pop_double()?;
                    self.stack.push(Value::Double(-a));
                }
                Instruction::INeg => {
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(-a));
                }
                
                // Bitwise operations
                Instruction::IAor => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a | b));
                }
                Instruction::IAand => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a & b));
                }
                Instruction::IAxor => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    self.stack.push(Value::Int(a ^ b));
                }
                
                // Print operations
                Instruction::IPrint => {
                    let value = self.pop_int()?;
                    self.output.push(value.to_string());
                    print!("{}", value);
                    io::stdout().flush().unwrap();
                }
                Instruction::DPrint => {
                    let value = self.pop_double()?;
                    self.output.push(value.to_string());
                    print!("{}", value);
                    io::stdout().flush().unwrap();
                }
                Instruction::SPrint => {
                    let value = self.pop_string()?;
                    self.output.push(value.clone());
                    print!("{}", value);
                    io::stdout().flush().unwrap();
                }
                
                // Type conversions
                Instruction::I2D => {
                    let value = self.pop_int()?;
                    self.stack.push(Value::Double(value as f64));
                }
                Instruction::D2I => {
                    let value = self.pop_double()?;
                    self.stack.push(Value::Int(value as i64));
                }
                Instruction::S2I => {
                    let value = self.pop_string()?;
                    let int_value = value.parse::<i64>().map_err(|_| {
                        Error::runtime_error("Cannot convert string to integer")
                    })?;
                    self.stack.push(Value::Int(int_value));
                }
                
                // Stack operations
                Instruction::Swap => {
                    if self.stack.len() < 2 {
                        return Err(Error::StackUnderflow);
                    }
                    let len = self.stack.len();
                    self.stack.swap(len - 1, len - 2);
                }
                Instruction::Pop => {
                    self.pop_value()?;
                }
                
                // Variable operations (simplified - TODO: implement proper variable storage)
                Instruction::LoadDVar0 | Instruction::LoadDVar1 | Instruction::LoadDVar2 | Instruction::LoadDVar3 => {
                    // TODO: Implement variable loading
                    return Err(Error::runtime_error("Variable operations not yet implemented"));
                }
                
                // Comparison operations
                Instruction::DCmp => {
                    let b = self.pop_double()?;
                    let a = self.pop_double()?;
                    let result = if a < b {
                        -1
                    } else if a > b {
                        1
                    } else {
                        0
                    };
                    self.stack.push(Value::Int(result));
                }
                Instruction::ICmp => {
                    let b = self.pop_int()?;
                    let a = self.pop_int()?;
                    let result = if a < b {
                        -1
                    } else if a > b {
                        1
                    } else {
                        0
                    };
                    self.stack.push(Value::Int(result));
                }
                
                // Control flow (TODO: implement proper control flow)
                Instruction::Stop => {
                    break;
                }
                
                // Debugging
                Instruction::Dump => {
                    if let Some(value) = self.stack.last() {
                        println!("DEBUG: {}", value);
                    }
                }
                
                _ => {
                    return Err(Error::runtime_error(&format!(
                        "Unimplemented instruction: {:?}",
                        instruction
                    )));
                }
            }
        }
        
        Ok(())
    }
    
    /// Add a string constant and return its ID
    pub fn add_string_constant(&mut self, s: String) -> u16 {
        self.strings.push(s);
        (self.strings.len() - 1) as u16
    }
    
    /// Get the captured output from print statements
    pub fn get_output(&self) -> Vec<String> {
        self.output.clone()
    }
    
    /// Clear the output buffer
    pub fn clear_output(&mut self) {
        self.output.clear();
    }
    
    fn pop_value(&mut self) -> Result<Value> {
        self.stack.pop().ok_or(Error::StackUnderflow)
    }
    
    fn pop_int(&mut self) -> Result<i64> {
        let value = self.pop_value()?;
        value.as_int()
    }
    
    fn pop_double(&mut self) -> Result<f64> {
        let value = self.pop_value()?;
        value.as_double()
    }
    
    fn pop_string(&mut self) -> Result<String> {
        let value = self.pop_value()?;
        value.as_string()
    }
}

impl Default for BytecodeInterpreter {
    fn default() -> Self {
        Self::new()
    }
}

/// AST-based interpreter that directly executes the AST
pub struct AstInterpreter {
    scopes: Vec<Scope>,
    functions: HashMap<String, Function>,
    output: Vec<String>,
}

impl AstInterpreter {
    pub fn new() -> Self {
        AstInterpreter {
            scopes: vec![Scope::new()],
            functions: HashMap::new(),
            output: Vec::new(),
        }
    }
    
    /// Execute an AST node
    pub fn execute(&mut self, node: &AstNode) -> Result<Value> {
        self.visit(node)
    }
    
    /// Get the captured output from print statements
    pub fn get_output(&self) -> Vec<String> {
        self.output.clone()
    }
    
    /// Clear the output buffer
    pub fn clear_output(&mut self) {
        self.output.clear();
    }
    
    #[allow(dead_code)]
    fn current_scope(&self) -> &Scope {
        self.scopes.last().unwrap()
    }
    
    fn current_scope_mut(&mut self) -> &mut Scope {
        self.scopes.last_mut().unwrap()
    }
    
    fn push_scope(&mut self) {
        self.scopes.push(Scope::new());
    }
    
    fn pop_scope(&mut self) {
        if self.scopes.len() > 1 {
            self.scopes.pop();
        }
    }
    
    fn define_variable(&mut self, name: String, value: Value) {
        self.current_scope_mut().define(name, value);
    }
    
    fn get_variable(&self, name: &str) -> Result<Value> {
        for scope in self.scopes.iter().rev() {
            if let Some(value) = scope.get(name) {
                return Ok(value.clone());
            }
        }
        Err(Error::UndefinedVariable(name.to_string()))
    }
    
    fn set_variable(&mut self, name: &str, value: Value) -> Result<()> {
        for scope in self.scopes.iter_mut().rev() {
            if scope.variables.contains_key(name) {
                scope.set(name, value)?;
                return Ok(());
            }
        }
        Err(Error::UndefinedVariable(name.to_string()))
    }
}

impl Default for AstInterpreter {
    fn default() -> Self {
        Self::new()
    }
}

impl AstVisitor<Result<Value>> for AstInterpreter {
    fn visit(&mut self, node: &AstNode) -> Result<Value> {
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
    
    fn visit_program(&mut self, statements: &[AstNode]) -> Result<Value> {
        let mut result = Value::Void;
        for statement in statements {
            result = self.visit(statement)?;
        }
        Ok(result)
    }
    
    fn visit_block(&mut self, statements: &[AstNode]) -> Result<Value> {
        self.push_scope();
        let mut result = Value::Void;
        for statement in statements {
            result = self.visit(statement)?;
        }
        self.pop_scope();
        Ok(result)
    }
    
    fn visit_var_declaration(&mut self, var_type: VarType, name: &str, initializer: &Option<Box<AstNode>>) -> Result<Value> {
        let value = if let Some(init) = initializer {
            self.visit(init)?
        } else {
            match var_type {
                VarType::Int => Value::Int(0),
                VarType::Double => Value::Double(0.0),
                VarType::String => Value::String(String::new()),
                VarType::Void => Value::Void,
            }
        };
        
        self.define_variable(name.to_string(), value);
        Ok(Value::Void)
    }
    
    fn visit_function_declaration(&mut self, name: &str, signature: &Signature, body: &AstNode) -> Result<Value> {
        let function = Function {
            name: name.to_string(),
            signature: signature.clone(),
            body: body.clone(),
        };
        self.functions.insert(name.to_string(), function);
        Ok(Value::Void)
    }
    
    fn visit_function_call(&mut self, _name: &str, _arguments: &[AstNode]) -> Result<Value> {
        // TODO: Implement function calls
        Err(Error::runtime_error("Function calls not yet implemented"))
    }
    
    fn visit_print(&mut self, arguments: &[AstNode]) -> Result<Value> {
        let mut output = String::new();
        for arg in arguments {
            let value = self.visit(arg)?;
            let str_value = value.as_string()?;
            output.push_str(&str_value);
            print!("{}", str_value);
        }
        self.output.push(output);
        io::stdout().flush().unwrap();
        Ok(Value::Void)
    }
    
    fn visit_if(&mut self, condition: &AstNode, then_branch: &AstNode, else_branch: &Option<Box<AstNode>>) -> Result<Value> {
        let condition_value = self.visit(condition)?;
        
        if condition_value.is_truthy() {
            self.visit(then_branch)
        } else if let Some(else_part) = else_branch {
            self.visit(else_part)
        } else {
            Ok(Value::Void)
        }
    }
    
    fn visit_while(&mut self, condition: &AstNode, body: &AstNode) -> Result<Value> {
        let mut result = Value::Void;
        while self.visit(condition)?.is_truthy() {
            result = self.visit(body)?;
        }
        Ok(result)
    }
    
    fn visit_for(&mut self, variable: &str, start: &AstNode, end: &AstNode, body: &AstNode) -> Result<Value> {
        let start_value = self.visit(start)?.as_int()?;
        let end_value = self.visit(end)?.as_int()?;
        
        self.push_scope();
        
        let mut result = Value::Void;
        for i in start_value..=end_value {
            self.define_variable(variable.to_string(), Value::Int(i));
            result = self.visit(body)?;
        }
        
        self.pop_scope();
        Ok(result)
    }
    
    fn visit_return(&mut self, value: &Option<Box<AstNode>>) -> Result<Value> {
        if let Some(val) = value {
            self.visit(val)
        } else {
            Ok(Value::Void)
        }
    }
    
    fn visit_expression(&mut self, expr: &AstNode) -> Result<Value> {
        self.visit(expr)
    }
    
    fn visit_binary_op(&mut self, left: &AstNode, op: &BinaryOp, right: &AstNode) -> Result<Value> {
        match op {
            BinaryOp::Assign => {
                let value = self.visit(right)?;
                if let AstNode::Variable(name) = left {
                    self.set_variable(name, value.clone())?;
                    Ok(value)
                } else {
                    Err(Error::runtime_error("Invalid assignment target"))
                }
            }
            BinaryOp::AssignAdd => {
                let right_value = self.visit(right)?;
                if let AstNode::Variable(name) = left {
                    let left_value = self.get_variable(name)?;
                    let result = match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Value::Int(a + b),
                        (Value::Double(a), Value::Double(b)) => Value::Double(a + b),
                        (Value::Int(a), Value::Double(b)) => Value::Double(a as f64 + b),
                        (Value::Double(a), Value::Int(b)) => Value::Double(a + b as f64),
                        _ => return Err(Error::runtime_error("Invalid operands for addition")),
                    };
                    self.set_variable(name, result.clone())?;
                    Ok(result)
                } else {
                    Err(Error::runtime_error("Invalid assignment target"))
                }
            }
            _ => {
                let left_value = self.visit(left)?;
                let right_value = self.visit(right)?;
                
                match op {
                    BinaryOp::Add => {
                        match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a + b)),
                            (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a + b)),
                            (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 + b)),
                            (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a + b as f64)),
                            _ => Err(Error::runtime_error("Invalid operands for addition")),
                        }
                    }
                    BinaryOp::Subtract => {
                        match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a - b)),
                            (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a - b)),
                            (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 - b)),
                            (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a - b as f64)),
                            _ => Err(Error::runtime_error("Invalid operands for subtraction")),
                        }
                    }
                    BinaryOp::Multiply => {
                        match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a * b)),
                            (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a * b)),
                            (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 * b)),
                            (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a * b as f64)),
                            _ => Err(Error::runtime_error("Invalid operands for multiplication")),
                        }
                    }
                    BinaryOp::Divide => {
                        match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => {
                                if b == 0 {
                                    Err(Error::DivisionByZero)
                                } else {
                                    Ok(Value::Int(a / b))
                                }
                            }
                            (Value::Double(a), Value::Double(b)) => {
                                if b == 0.0 {
                                    Err(Error::DivisionByZero)
                                } else {
                                    Ok(Value::Double(a / b))
                                }
                            }
                            (Value::Int(a), Value::Double(b)) => {
                                if b == 0.0 {
                                    Err(Error::DivisionByZero)
                                } else {
                                    Ok(Value::Double(a as f64 / b))
                                }
                            }
                            (Value::Double(a), Value::Int(b)) => {
                                if b == 0 {
                                    Err(Error::DivisionByZero)
                                } else {
                                    Ok(Value::Double(a / b as f64))
                                }
                            }
                            _ => Err(Error::runtime_error("Invalid operands for division")),
                        }
                    }
                    BinaryOp::Equal => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a == b,
                            (Value::Double(a), Value::Double(b)) => a == b,
                            (Value::String(a), Value::String(b)) => a == b,
                            _ => false,
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::NotEqual => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a != b,
                            (Value::Double(a), Value::Double(b)) => a != b,
                            (Value::String(a), Value::String(b)) => a != b,
                            _ => true,
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::Less => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a < b,
                            (Value::Double(a), Value::Double(b)) => a < b,
                            (Value::Int(a), Value::Double(b)) => (a as f64) < b,
                            (Value::Double(a), Value::Int(b)) => a < (b as f64),
                            _ => return Err(Error::runtime_error("Invalid operands for comparison")),
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::Greater => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a > b,
                            (Value::Double(a), Value::Double(b)) => a > b,
                            (Value::Int(a), Value::Double(b)) => (a as f64) > b,
                            (Value::Double(a), Value::Int(b)) => a > (b as f64),
                            _ => return Err(Error::runtime_error("Invalid operands for comparison")),
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::And => {
                        let result = left_value.is_truthy() && right_value.is_truthy();
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::Or => {
                        let result = left_value.is_truthy() || right_value.is_truthy();
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    _ => Err(Error::runtime_error("Unimplemented binary operator")),
                }
            }
        }
    }
    
    fn visit_unary_op(&mut self, op: &UnaryOp, operand: &AstNode) -> Result<Value> {
        let value = self.visit(operand)?;
        
        match op {
            UnaryOp::Minus => {
                match value {
                    Value::Int(i) => Ok(Value::Int(-i)),
                    Value::Double(d) => Ok(Value::Double(-d)),
                    _ => Err(Error::runtime_error("Invalid operand for unary minus")),
                }
            }
            UnaryOp::Plus => {
                match value {
                    Value::Int(_) | Value::Double(_) => Ok(value),
                    _ => Err(Error::runtime_error("Invalid operand for unary plus")),
                }
            }
            UnaryOp::Not => {
                Ok(Value::Int(if value.is_truthy() { 0 } else { 1 }))
            }
        }
    }
    
    fn visit_variable(&mut self, name: &str) -> Result<Value> {
        self.get_variable(name)
    }
    
    fn visit_int_literal(&mut self, value: i64) -> Result<Value> {
        Ok(Value::Int(value))
    }
    
    fn visit_double_literal(&mut self, value: f64) -> Result<Value> {
        Ok(Value::Double(value))
    }
    
    fn visit_string_literal(&mut self, value: &str) -> Result<Value> {
        Ok(Value::String(value.to_string()))
    }
}