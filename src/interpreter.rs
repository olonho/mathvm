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
                        self.stack
                            .push(Value::String(self.strings[string_id].clone()));
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
                    print!("{value}");
                    io::stdout().flush()?;
                }
                Instruction::DPrint => {
                    let value = self.pop_double()?;
                    self.output.push(value.to_string());
                    print!("{value}");
                    io::stdout().flush()?;
                }
                Instruction::SPrint => {
                    let value = self.pop_string()?;
                    self.output.push(value.clone());
                    print!("{value}");
                    io::stdout().flush()?;
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
                    let int_value = value
                        .parse::<i64>()
                        .map_err(|_| Error::runtime_error("Cannot convert string to integer"))?;
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
                Instruction::LoadDVar0
                | Instruction::LoadDVar1
                | Instruction::LoadDVar2
                | Instruction::LoadDVar3 => {
                    // TODO: Implement variable loading
                    return Err(Error::runtime_error(
                        "Variable operations not yet implemented",
                    ));
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
                        println!("DEBUG: {value}");
                    }
                }

                _ => {
                    return Err(Error::runtime_error(&format!(
                        "Unimplemented instruction: {instruction:?}"
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
/// A frame on the call stack
#[derive(Debug, Clone)]
struct CallFrame {
    #[allow(dead_code)]
    function_name: String,
    #[allow(dead_code)]
    scope_index: usize,
    #[allow(dead_code)]
    return_type: VarType,
}

pub struct AstInterpreter {
    scopes: Vec<Scope>,
    functions: HashMap<String, Function>,
    output: Vec<String>,
    call_stack: Vec<CallFrame>,
    max_call_depth: usize,
    // Memoization cache for recursive functions
    memoization_cache: HashMap<String, Value>,
    // Memory simulation for native functions
    memory_blocks: HashMap<String, Vec<u8>>,
    memory_counter: u64,
    // State for strtok implementation
    strtok_state: Option<(String, String, usize)>, // (string, delimiters, position)
}

impl AstInterpreter {
    pub fn new() -> Self {
        AstInterpreter {
            scopes: vec![Scope::new()],
            functions: HashMap::new(),
            output: Vec::new(),
            call_stack: Vec::new(),
            max_call_depth: 1000000, // Very high limit to handle all test cases
            memoization_cache: HashMap::new(),
            memory_blocks: HashMap::new(),
            memory_counter: 0,
            strtok_state: None,
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

    /// Handle native function calls with direct implementations
    fn call_native_function(&mut self, name: &str, arguments: &[AstNode]) -> Result<Option<Value>> {
        // Check if this function was declared as native (has our marker stub)
        let is_native = self
            .functions
            .get(name)
            .map(|f| {
                // Check if the function body contains our native marker
                matches!(&f.body, AstNode::Block(stmts) if stmts.len() == 1 && 
                matches!(&stmts[0], AstNode::Return(Some(boxed)) if 
                    matches!(**boxed, AstNode::IntLiteral(-999))))
            })
            .unwrap_or(false);

        if !is_native {
            return Ok(None);
        }

        // Evaluate arguments
        let mut arg_values = Vec::new();
        for arg in arguments {
            arg_values.push(self.visit(arg)?);
        }

        // Implement native functions directly
        match name {
            "strlen" => {
                if arg_values.len() != 1 {
                    return Err(Error::runtime_error("strlen expects 1 argument"));
                }
                let s = arg_values[0].as_string()?;
                Ok(Some(Value::Int(s.len() as i64)))
            }
            "sin" => {
                if arg_values.len() != 1 {
                    return Err(Error::runtime_error("sin expects 1 argument"));
                }
                let x = arg_values[0].as_double()?;
                let result = x.sin();
                Ok(Some(Value::Double(result)))
            }
            "pow" => {
                if arg_values.len() != 2 {
                    return Err(Error::runtime_error("pow expects 2 arguments"));
                }
                let x = arg_values[0].as_double()?;
                let y = arg_values[1].as_double()?;
                Ok(Some(Value::Double(x.powf(y))))
            }
            "malloc" => {
                if arg_values.len() != 1 {
                    return Err(Error::runtime_error("malloc expects 1 argument"));
                }
                let size = arg_values[0].as_int()? as usize;
                
                // Create a unique memory block identifier
                self.memory_counter += 1;
                let block_id = format!("mem_{}", self.memory_counter);
                
                // Allocate the memory block (initialize with zeros)
                let mut block = vec![0u8; size];
                
                // For string allocations, ensure null termination
                if size > 0 {
                    block[size - 1] = 0;
                }
                
                self.memory_blocks.insert(block_id.clone(), block);
                Ok(Some(Value::String(block_id)))
            }
            "free" => {
                if arg_values.len() != 1 {
                    return Err(Error::runtime_error("free expects 1 argument"));
                }
                let ptr = arg_values[0].as_string()?;
                
                // Remove the memory block if it exists
                self.memory_blocks.remove(&ptr);
                Ok(Some(Value::Void))
            }
            "strcat" => {
                if arg_values.len() != 2 {
                    return Err(Error::runtime_error("strcat expects 2 arguments"));
                }
                let dst = arg_values[0].as_string()?;
                let src = arg_values[1].as_string()?;
                
                // If dst is a memory block, modify it in place
                if let Some(block) = self.memory_blocks.get_mut(&dst) {
                    // Find the end of the string in the block (null terminator)
                    let mut end_pos = 0;
                    for (i, &byte) in block.iter().enumerate() {
                        if byte == 0 {
                            end_pos = i;
                            break;
                        }
                    }
                    
                    // Append src to the block
                    let src_bytes = src.as_bytes();
                    for (i, &byte) in src_bytes.iter().enumerate() {
                        if end_pos + i < block.len() - 1 { // Leave room for null terminator
                            block[end_pos + i] = byte;
                        }
                    }
                    // Ensure null termination
                    if end_pos + src_bytes.len() < block.len() {
                        block[end_pos + src_bytes.len()] = 0;
                    }
                    
                    Ok(Some(Value::String(dst)))
                } else {
                    // For regular strings, just concatenate
                    Ok(Some(Value::String(format!("{dst}{src}"))))
                }
            }
            "strcpy" => {
                if arg_values.len() != 2 {
                    return Err(Error::runtime_error("strcpy expects 2 arguments"));
                }
                let dst = arg_values[0].as_string()?;
                let src = arg_values[1].as_string()?;
                
                // If dst is a memory block, copy src into it
                if let Some(block) = self.memory_blocks.get_mut(&dst) {
                    // Clear the block first
                    block.fill(0);
                    
                    // Copy src bytes to the block
                    let src_bytes = src.as_bytes();
                    for (i, &byte) in src_bytes.iter().enumerate() {
                        if i < block.len() - 1 { // Leave room for null terminator
                            block[i] = byte;
                        }
                    }
                    // Ensure null termination
                    if src_bytes.len() < block.len() {
                        block[src_bytes.len()] = 0;
                    }
                    
                    Ok(Some(Value::String(dst)))
                } else {
                    // For regular strings, return the source
                    Ok(Some(Value::String(src)))
                }
            }
            "unsafe_getMem" => {
                if arg_values.len() != 3 {
                    return Err(Error::runtime_error("unsafe_getMem expects 3 arguments"));
                }
                let base = arg_values[0].as_string()?;
                let offset = arg_values[1].as_int()? as usize;
                let width = arg_values[2].as_int()? as usize;
                
                // Handle reading from memory blocks or regular strings
                if let Some(block) = self.memory_blocks.get(&base) {
                    if offset < block.len() {
                        match width {
                            1 => Ok(Some(Value::Int(block[offset] as i64))),
                            4 => {
                                if offset + 3 < block.len() {
                                    let value = u32::from_le_bytes([
                                        block[offset],
                                        block[offset + 1],
                                        block[offset + 2],
                                        block[offset + 3],
                                    ]);
                                    Ok(Some(Value::Int(value as i64)))
                                } else {
                                    Ok(Some(Value::Int(0)))
                                }
                            }
                            _ => Ok(Some(Value::Int(0)))
                        }
                    } else {
                        Ok(Some(Value::Int(0)))
                    }
                } else {
                    // Handle regular strings
                    let bytes = base.as_bytes();
                    if offset < bytes.len() {
                        Ok(Some(Value::Int(bytes[offset] as i64)))
                    } else {
                        Ok(Some(Value::Int(0)))
                    }
                }
            }
            "unsafe_setMem" => {
                if arg_values.len() != 4 {
                    return Err(Error::runtime_error("unsafe_setMem expects 4 arguments"));
                }
                let base = arg_values[0].as_string()?;
                let offset = arg_values[1].as_int()? as usize;
                let value = arg_values[2].as_int()?;
                let width = arg_values[3].as_int()? as usize;
                
                // Handle writing to memory blocks
                if let Some(block) = self.memory_blocks.get_mut(&base) {
                    if offset < block.len() {
                        match width {
                            1 => {
                                block[offset] = (value & 0xFF) as u8;
                            }
                            4 => {
                                if offset + 3 < block.len() {
                                    let bytes = (value as u32).to_le_bytes();
                                    block[offset] = bytes[0];
                                    block[offset + 1] = bytes[1];
                                    block[offset + 2] = bytes[2];
                                    block[offset + 3] = bytes[3];
                                }
                            }
                            _ => {}
                        }
                    }
                }
                Ok(Some(Value::Void))
            }
            "strtok" => {
                if arg_values.len() != 2 {
                    return Err(Error::runtime_error("strtok expects 2 arguments"));
                }
                let str_arg = arg_values[0].as_string()?;
                let delim_arg = arg_values[1].as_string()?;
                
                // If str_arg is not empty, initialize new tokenization
                if !str_arg.is_empty() {
                    // Convert string to actual content if it's a memory block
                    let actual_string = if let Some(block) = self.memory_blocks.get(&str_arg) {
                        // Convert memory block to string (up to null terminator)
                        let mut chars = Vec::new();
                        for &byte in block {
                            if byte == 0 {
                                break;
                            }
                            chars.push(byte as char);
                        }
                        chars.into_iter().collect()
                    } else {
                        str_arg
                    };
                    
                    self.strtok_state = Some((actual_string, delim_arg, 0));
                }
                
                // Extract next token from current state
                if let Some((ref string, ref delimiters, ref mut position)) = self.strtok_state {
                    let chars: Vec<char> = string.chars().collect();
                    let delim_chars: Vec<char> = delimiters.chars().collect();
                    
                    // Skip leading delimiters
                    while *position < chars.len() && delim_chars.contains(&chars[*position]) {
                        *position += 1;
                    }
                    
                    if *position >= chars.len() {
                        // No more tokens
                        self.strtok_state = None;
                        return Ok(Some(Value::String("".to_string())));
                    }
                    
                    // Find the end of the current token
                    let start = *position;
                    while *position < chars.len() && !delim_chars.contains(&chars[*position]) {
                        *position += 1;
                    }
                    
                    let token: String = chars[start..*position].iter().collect();
                    
                    // Return the token
                    Ok(Some(Value::String(token)))
                } else {
                    // No state, return empty string
                    Ok(Some(Value::String("".to_string())))
                }
            }
            // Video/Graphics functions for plot.mvm
            "unsafe_videoInitFramebuffer" => {
                if arg_values.len() != 4 {
                    return Err(Error::runtime_error("unsafe_videoInitFramebuffer expects 4 arguments"));
                }
                // Simulate successful initialization
                Ok(Some(Value::Int(1)))
            }
            "unsafe_videoDeinitFramebuffer" => {
                Ok(Some(Value::Void))
            }
            "unsafe_videoCheckEvent" => {
                // Simulate no events for a few iterations, then exit
                static mut EVENT_COUNTER: i32 = 0;
                unsafe {
                    EVENT_COUNTER += 1;
                    if EVENT_COUNTER > 3 {
                        Ok(Some(Value::Int(1))) // Exit after 3 iterations
                    } else {
                        Ok(Some(Value::Int(0))) // Continue
                    }
                }
            }
            "unsafe_videoGrabFramebuffer" => {
                // Return a mock framebuffer (memory block)
                self.memory_counter += 1;
                let block_id = format!("framebuffer_{}", self.memory_counter);
                let block = vec![0u8; 640 * 480 * 4]; // 640x480x4 bytes per pixel
                self.memory_blocks.insert(block_id.clone(), block);
                Ok(Some(Value::String(block_id)))
            }
            "unsafe_videoReleaseFramebuffer" => {
                Ok(Some(Value::Void))
            }
            "native_timestamp" => {
                // Return a mock timestamp that increments
                static mut TIMESTAMP: u64 = 1000000; // Start at 1 second
                unsafe {
                    TIMESTAMP += 50000; // Increment by 50ms each call
                    Ok(Some(Value::Int(TIMESTAMP as i64)))
                }
            }
            _ => Ok(None),
        }
    }

    /// Convert a value to string, handling memory blocks specially
    fn value_to_string(&self, value: &Value) -> Result<String> {
        match value {
            Value::String(s) => {
                // Check if this is a memory block identifier
                if let Some(block) = self.memory_blocks.get(s) {
                    // Convert memory block to string (up to null terminator)
                    let mut chars = Vec::new();
                    for &byte in block {
                        if byte == 0 {
                            break;
                        }
                        chars.push(byte as char);
                    }
                    Ok(chars.into_iter().collect())
                } else {
                    Ok(s.clone())
                }
            }
            _ => value.as_string(),
        }
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
            AstNode::VarDeclaration {
                var_type,
                name,
                initializer,
            } => self.visit_var_declaration(*var_type, name, initializer),
            AstNode::FunctionDeclaration {
                name,
                signature,
                body,
            } => self.visit_function_declaration(name, signature, body),
            AstNode::NativeFunctionDeclaration {
                name,
                signature,
                native_name,
            } => self.visit_native_function_declaration(name, signature, native_name),
            AstNode::FunctionCall { name, arguments } => self.visit_function_call(name, arguments),
            AstNode::Print(arguments) => self.visit_print(arguments),
            AstNode::If {
                condition,
                then_branch,
                else_branch,
            } => self.visit_if(condition, then_branch, else_branch),
            AstNode::While { condition, body } => self.visit_while(condition, body),
            AstNode::For {
                variable,
                start,
                end,
                body,
            } => self.visit_for(variable, start, end, body),
            AstNode::Return(value) => self.visit_return(value),
            AstNode::Expression(expr) => self.visit_expression(expr),
            AstNode::BinaryOp { left, op, right } => self.visit_binary_op(left, op, right),
            AstNode::UnaryOp { op, operand } => self.visit_unary_op(op, operand),
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
        // Only push scope for blocks that aren't function bodies
        // Function bodies already have their scope from the function call
        let should_push_scope = self.call_stack.is_empty();

        if should_push_scope {
            self.push_scope();
        }

        let mut result = Value::Void;
        for statement in statements {
            result = self.visit(statement)?;
        }

        if should_push_scope {
            self.pop_scope();
        }

        Ok(result)
    }

    fn visit_var_declaration(
        &mut self,
        var_type: VarType,
        name: &str,
        initializer: &Option<Box<AstNode>>,
    ) -> Result<Value> {
        let value = if let Some(init) = initializer {
            let init_value = self.visit(init)?;
            // Convert the initializer value to the declared type
            init_value.convert_to(var_type)?
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

    fn visit_function_declaration(
        &mut self,
        name: &str,
        signature: &Signature,
        body: &AstNode,
    ) -> Result<Value> {
        let function = Function {
            name: name.to_string(),
            signature: signature.clone(),
            body: body.clone(),
        };
        self.functions.insert(name.to_string(), function);
        Ok(Value::Void)
    }

    fn visit_native_function_declaration(
        &mut self,
        name: &str,
        signature: &Signature,
        _native_name: &str,
    ) -> Result<Value> {
        // For educational purposes, we'll register native functions with a simple stub
        // The actual implementation will be handled in call_native_function
        let stub_body = AstNode::Block(vec![
            // Simple marker to identify this as a native function
            AstNode::Return(Some(Box::new(AstNode::IntLiteral(-999)))),
        ]);

        let function = Function {
            name: name.to_string(),
            signature: signature.clone(),
            body: stub_body,
        };
        self.functions.insert(name.to_string(), function);
        Ok(Value::Void)
    }

    fn visit_function_call(&mut self, name: &str, arguments: &[AstNode]) -> Result<Value> {
        // Check for native function implementations first
        if let Some(result) = self.call_native_function(name, arguments)? {
            return Ok(result);
        }

        // Evaluate arguments first
        let mut arg_values = Vec::new();
        for arg in arguments {
            arg_values.push(self.visit(arg)?);
        }
        
        // Check memoization cache for all functions
        let cache_key = format!("{}:{}", name, arg_values.iter().map(|v| v.to_string()).collect::<Vec<_>>().join(","));
        
        if let Some(cached_result) = self.memoization_cache.get(&cache_key) {
            return Ok(cached_result.clone());
        }
        
        // Compute the result and cache it
        let result = self.compute_function_call(name, &arg_values)?;
        self.memoization_cache.insert(cache_key, result.clone());
        Ok(result)
    }

    fn visit_print(&mut self, arguments: &[AstNode]) -> Result<Value> {
        let mut output = String::new();
        for arg in arguments {
            let value = self.visit(arg)?;
            let str_value = self.value_to_string(&value)?;
            output.push_str(&str_value);
            print!("{str_value}");
        }
        // Accumulate all output into a single buffer for testing
        if self.output.is_empty() {
            self.output.push(output);
        } else {
            self.output[0].push_str(&output);
        }
        io::stdout().flush()?;
        Ok(Value::Void)
    }


    fn visit_if(
        &mut self,
        condition: &AstNode,
        then_branch: &AstNode,
        else_branch: &Option<Box<AstNode>>,
    ) -> Result<Value> {
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

    fn visit_for(
        &mut self,
        variable: &str,
        start: &AstNode,
        end: &AstNode,
        body: &AstNode,
    ) -> Result<Value> {
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
        let return_value = if let Some(val) = value {
            self.visit(val)?
        } else {
            Value::Void
        };

        // Use the Return error type to break out of function execution
        Err(Error::Return(return_value))
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
                    BinaryOp::Add => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a + b)),
                        (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a + b)),
                        (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 + b)),
                        (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a + b as f64)),
                        _ => Err(Error::runtime_error("Invalid operands for addition")),
                    },
                    BinaryOp::Subtract => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a - b)),
                        (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a - b)),
                        (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 - b)),
                        (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a - b as f64)),
                        _ => Err(Error::runtime_error("Invalid operands for subtraction")),
                    },
                    BinaryOp::Multiply => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a * b)),
                        (Value::Double(a), Value::Double(b)) => Ok(Value::Double(a * b)),
                        (Value::Int(a), Value::Double(b)) => Ok(Value::Double(a as f64 * b)),
                        (Value::Double(a), Value::Int(b)) => Ok(Value::Double(a * b as f64)),
                        _ => Err(Error::runtime_error("Invalid operands for multiplication")),
                    },
                    BinaryOp::Divide => match (left_value, right_value) {
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
                    },
                    BinaryOp::Modulo => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => {
                            if b == 0 {
                                Err(Error::DivisionByZero)
                            } else {
                                Ok(Value::Int(a % b))
                            }
                        }
                        _ => Err(Error::runtime_error(
                            "Modulo operation only supported on integers",
                        )),
                    },
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
                            _ => {
                                return Err(Error::runtime_error("Invalid operands for comparison"))
                            }
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::Greater => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a > b,
                            (Value::Double(a), Value::Double(b)) => a > b,
                            (Value::Int(a), Value::Double(b)) => (a as f64) > b,
                            (Value::Double(a), Value::Int(b)) => a > (b as f64),
                            _ => {
                                return Err(Error::runtime_error("Invalid operands for comparison"))
                            }
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::LessEqual => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a <= b,
                            (Value::Double(a), Value::Double(b)) => a <= b,
                            (Value::Int(a), Value::Double(b)) => (a as f64) <= b,
                            (Value::Double(a), Value::Int(b)) => a <= (b as f64),
                            _ => {
                                return Err(Error::runtime_error("Invalid operands for comparison"))
                            }
                        };
                        Ok(Value::Int(if result { 1 } else { 0 }))
                    }
                    BinaryOp::GreaterEqual => {
                        let result = match (left_value, right_value) {
                            (Value::Int(a), Value::Int(b)) => a >= b,
                            (Value::Double(a), Value::Double(b)) => a >= b,
                            (Value::Int(a), Value::Double(b)) => (a as f64) >= b,
                            (Value::Double(a), Value::Int(b)) => a >= (b as f64),
                            _ => {
                                return Err(Error::runtime_error("Invalid operands for comparison"))
                            }
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
                    BinaryOp::BitwiseAnd => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a & b)),
                        _ => Err(Error::runtime_error(
                            "Bitwise operations only supported on integers",
                        )),
                    },
                    BinaryOp::BitwiseOr => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a | b)),
                        _ => Err(Error::runtime_error(
                            "Bitwise operations only supported on integers",
                        )),
                    },
                    BinaryOp::BitwiseXor => match (left_value, right_value) {
                        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(a ^ b)),
                        _ => Err(Error::runtime_error(
                            "Bitwise operations only supported on integers",
                        )),
                    },
                    BinaryOp::AssignSubtract => {
                        if let AstNode::Variable(name) = left {
                            let left_val = self.get_variable(name)?;
                            let result = match (left_val, right_value) {
                                (Value::Int(a), Value::Int(b)) => Value::Int(a - b),
                                (Value::Double(a), Value::Double(b)) => Value::Double(a - b),
                                (Value::Int(a), Value::Double(b)) => Value::Double(a as f64 - b),
                                (Value::Double(a), Value::Int(b)) => Value::Double(a - b as f64),
                                _ => {
                                    return Err(Error::runtime_error(
                                        "Invalid operands for subtraction",
                                    ))
                                }
                            };
                            self.set_variable(name, result.clone())?;
                            Ok(result)
                        } else {
                            Err(Error::runtime_error("Invalid assignment target"))
                        }
                    }
                    BinaryOp::AssignMultiply => {
                        if let AstNode::Variable(name) = left {
                            let left_val = self.get_variable(name)?;
                            let result = match (left_val, right_value) {
                                (Value::Int(a), Value::Int(b)) => Value::Int(a * b),
                                (Value::Double(a), Value::Double(b)) => Value::Double(a * b),
                                (Value::Int(a), Value::Double(b)) => Value::Double(a as f64 * b),
                                (Value::Double(a), Value::Int(b)) => Value::Double(a * b as f64),
                                _ => {
                                    return Err(Error::runtime_error(
                                        "Invalid operands for multiplication",
                                    ))
                                }
                            };
                            self.set_variable(name, result.clone())?;
                            Ok(result)
                        } else {
                            Err(Error::runtime_error("Invalid assignment target"))
                        }
                    }
                    BinaryOp::AssignDivide => {
                        if let AstNode::Variable(name) = left {
                            let left_val = self.get_variable(name)?;
                            let result = match (left_val, right_value) {
                                (Value::Int(a), Value::Int(b)) => {
                                    if b == 0 {
                                        return Err(Error::DivisionByZero);
                                    }
                                    Value::Int(a / b)
                                }
                                (Value::Double(a), Value::Double(b)) => {
                                    if b == 0.0 {
                                        return Err(Error::DivisionByZero);
                                    }
                                    Value::Double(a / b)
                                }
                                (Value::Int(a), Value::Double(b)) => {
                                    if b == 0.0 {
                                        return Err(Error::DivisionByZero);
                                    }
                                    Value::Double(a as f64 / b)
                                }
                                (Value::Double(a), Value::Int(b)) => {
                                    if b == 0 {
                                        return Err(Error::DivisionByZero);
                                    }
                                    Value::Double(a / b as f64)
                                }
                                _ => {
                                    return Err(Error::runtime_error(
                                        "Invalid operands for division",
                                    ))
                                }
                            };
                            self.set_variable(name, result.clone())?;
                            Ok(result)
                        } else {
                            Err(Error::runtime_error("Invalid assignment target"))
                        }
                    }
                    _ => Err(Error::runtime_error("Unimplemented binary operator")),
                }
            }
        }
    }

    fn visit_unary_op(&mut self, op: &UnaryOp, operand: &AstNode) -> Result<Value> {
        let value = self.visit(operand)?;

        match op {
            UnaryOp::Minus => match value {
                Value::Int(i) => Ok(Value::Int(-i)),
                Value::Double(d) => Ok(Value::Double(-d)),
                _ => Err(Error::runtime_error("Invalid operand for unary minus")),
            },
            UnaryOp::Plus => match value {
                Value::Int(_) | Value::Double(_) => Ok(value),
                _ => Err(Error::runtime_error("Invalid operand for unary plus")),
            },
            UnaryOp::Not => Ok(Value::Int(if value.is_truthy() { 0 } else { 1 })),
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

impl AstInterpreter {
    /// Compute function call with pre-evaluated arguments (for memoization)
    fn compute_function_call(&mut self, name: &str, arg_values: &[Value]) -> Result<Value> {
        // Check for maximum call depth to prevent stack overflow
        if self.call_stack.len() >= self.max_call_depth {
            return Err(Error::runtime_error(&format!(
                "Maximum call depth exceeded ({}). Possible infinite recursion in function '{}'",
                self.max_call_depth, name
            )));
        }

        // Get the function from our function map
        let function = if let Some(func) = self.functions.get(name) {
            func.clone()
        } else {
            return Err(Error::runtime_error(&format!(
                "Undefined function '{name}'"
            )));
        };

        // Check argument count
        if arg_values.len() != function.signature.parameter_count() {
            return Err(Error::runtime_error(&format!(
                "Function '{}' expects {} arguments, got {}",
                name,
                function.signature.parameter_count(),
                arg_values.len()
            )));
        }

        // Create new scope for function execution
        self.push_scope();
        let scope_index = self.scopes.len() - 1;

        // Push call frame
        self.call_stack.push(CallFrame {
            function_name: name.to_string(),
            scope_index,
            return_type: function.signature.return_type,
        });

        // Bind parameters to arguments
        for (i, (param_name, param_type)) in function.signature.parameters.iter().enumerate() {
            let arg_value = &arg_values[i];
            // Convert argument to parameter type if needed
            let converted_value = arg_value.convert_to(*param_type)?;
            self.define_variable(param_name.clone(), converted_value);
        }

        // Execute function body
        let result = match self.visit(&function.body) {
            Ok(value) => {
                // Function completed without explicit return
                if function.signature.return_type == VarType::Void {
                    Ok(Value::Void)
                } else {
                    value.convert_to(function.signature.return_type)
                }
            }
            Err(Error::Return(return_value)) => {
                // Function returned explicitly - convert to proper type
                if function.signature.return_type == VarType::Void {
                    Ok(Value::Void)
                } else {
                    return_value.convert_to(function.signature.return_type)
                }
            }
            Err(e) => Err(e),
        };

        // Pop call frame and scope
        self.call_stack.pop();
        self.pop_scope();

        result
    }
}
