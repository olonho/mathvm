//! # MathVM - A Virtual Machine for Mathematical Expressions
//!
//! This is a stub implementation for the Virtual Machines course assignment.
//! Students should implement the missing functionality to create a working
//! virtual machine for the MVM language.
//!
//! ## MVM Language Features
//!
//! - Three data types: `int` (64-bit), `double` (IEEE 754), `string`
//! - Variable declarations and assignments
//! - Arithmetic operations with proper precedence
//! - Control flow: `if/else`, `for` loops, `while` loops
//! - Functions with parameters and return values
//! - Print statements for output
//!
//! ## Implementation Tasks
//!
//! Students need to implement:
//! 1. **Parser**: Convert MVM source code to AST (Abstract Syntax Tree)
//! 2. **Bytecode Generator**: Convert AST to bytecode instructions
//! 3. **Interpreter**: Execute bytecode instructions
//! 4. **Type System**: Handle type checking and conversions
//! 5. **Memory Management**: Variable storage and scoping
//!
//! ## Getting Started
//!
//! 1. Study the existing type definitions and traits
//! 2. Implement the `Parser` trait to parse MVM source code
//! 3. Implement the `BytecodeGenerator` trait to generate bytecode
//! 4. Implement the `Interpreter` trait to execute bytecode
//! 5. Run tests to verify your implementation works correctly

pub mod ast;
pub mod bytecode;
pub mod error;
pub mod interpreter;
pub mod parser;
pub mod translator;
pub mod types;
pub mod vm;

pub use error::{Error, Result};
pub use types::VarType;
pub use vm::VM;

/// Re-export commonly used types for convenience
pub mod prelude {
    pub use crate::ast::*;
    pub use crate::bytecode::*;
    pub use crate::error::{Error, Result};
    pub use crate::interpreter::*;
    pub use crate::parser::*;
    pub use crate::translator::*;
    pub use crate::types::*;
    pub use crate::vm::*;
}