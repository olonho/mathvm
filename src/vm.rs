use crate::ast::AstPrinter;
use crate::bytecode::Bytecode;
use crate::error::Result;
use crate::interpreter::{AstInterpreter, BytecodeInterpreter};
use crate::parser::parse;
use crate::translator::{BytecodeGenerator, Translator};
use crate::types::Value;

/// Virtual Machine that can execute MVM programs
pub struct VM {
    ast_interpreter: AstInterpreter,
    bytecode_interpreter: BytecodeInterpreter,
}

impl VM {
    /// Create a new VM instance
    pub fn new() -> Self {
        VM {
            ast_interpreter: AstInterpreter::new(),
            bytecode_interpreter: BytecodeInterpreter::new(),
        }
    }
    
    /// Execute MVM source code using the AST interpreter
    pub fn execute(&mut self, source: &str) -> Result<Value> {
        let ast = parse(source)?;
        self.ast_interpreter.execute(&ast)
    }
    
    /// Execute MVM source code using the bytecode interpreter
    pub fn execute_bytecode(&mut self, source: &str) -> Result<()> {
        let ast = parse(source)?;
        let mut generator = BytecodeGenerator::new();
        generator.translate(&ast)?;
        
        // Copy string constants to the interpreter
        // TODO: This is a simplified implementation
        // In a real implementation, you'd properly handle string constants
        
        let bytecode = generator.get_bytecode();
        self.bytecode_interpreter.execute(bytecode)
    }
    
    /// Parse and pretty-print MVM source code
    pub fn print_source(&self, source: &str) -> Result<String> {
        let ast = parse(source)?;
        let mut printer = AstPrinter::new();
        Ok(printer.print(&ast))
    }
    
    /// Compile MVM source code to bytecode
    pub fn compile(&self, source: &str) -> Result<Bytecode> {
        let ast = parse(source)?;
        let mut generator = BytecodeGenerator::new();
        generator.translate(&ast)?;
        Ok(generator.take_bytecode())
    }
    
    /// Get the output from the last execution
    pub fn get_output(&self) -> Vec<String> {
        self.ast_interpreter.get_output()
    }
    
    /// Clear the output buffer
    pub fn clear_output(&mut self) {
        self.ast_interpreter.clear_output();
        self.bytecode_interpreter.clear_output();
    }
    
    /// Disassemble bytecode for debugging
    pub fn disassemble_bytecode(&self, bytecode: &Bytecode) -> String {
        bytecode.dump()
    }
}

impl Default for VM {
    fn default() -> Self {
        Self::new()
    }
}

/// Helper function to execute MVM source code directly
pub fn execute_source(source: &str) -> Result<Value> {
    let mut vm = VM::new();
    vm.execute(source)
}

/// Helper function to compile MVM source code to bytecode
pub fn compile_source(source: &str) -> Result<Bytecode> {
    let vm = VM::new();
    vm.compile(source)
}

/// Helper function to pretty-print MVM source code
pub fn print_source(source: &str) -> Result<String> {
    let vm = VM::new();
    vm.print_source(source)
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_basic_arithmetic() {
        let mut vm = VM::new();
        let result = vm.execute("print(1 + 2 * 3);");
        assert!(result.is_ok());
    }
    
    #[test]
    fn test_variable_declaration() {
        let mut vm = VM::new();
        let result = vm.execute("int x = 42; print(x);");
        assert!(result.is_ok());
    }
    
    #[test]
    fn test_string_literals() {
        let mut vm = VM::new();
        let result = vm.execute("print('Hello, World!');");
        assert!(result.is_ok());
    }
    
    #[test]
    fn test_pretty_printing() {
        let vm = VM::new();
        let result = vm.print_source("int x = 42; print(x);");
        assert!(result.is_ok());
    }
    
    #[test]
    fn test_compilation() {
        let vm = VM::new();
        let result = vm.compile("int x = 42; print(x);");
        assert!(result.is_ok());
    }
}