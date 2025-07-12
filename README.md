# MathVM - Educational Virtual Machine in Rust

A simple, educational Virtual Machine for Mathematical Expressions implemented in Rust.

## Overview

MathVM is an educational stack-based virtual machine designed for learning about language implementation, virtual machines, and compiler construction. It executes programs written in the MVM language and features both AST-based interpretation and bytecode compilation/execution.

**This project is designed for educational purposes** to demonstrate:
- Language design and implementation
- Parsing and abstract syntax trees
- Virtual machine architecture
- Interpreter vs. compiler approaches
- Type systems and runtime environments

## What You'll Learn

By studying this codebase, you'll understand:
- How to build a lexer and recursive descent parser
- How to design and traverse abstract syntax trees
- How to implement tree-walking interpreters
- How to design bytecode instruction sets
- How to build stack-based virtual machines
- How scoping and variable resolution work
- How function calls and returns are implemented

## MVM Language

The MVM language is a C-style language with comprehensive support for:

### Data Types
- `int`: 64-bit signed integer
- `double`: IEEE 754 double precision floating point  
- `string`: Immutable UTF-8 strings

### Language Features
- Variable declarations with initialization: `int x = 42;`
- Arithmetic operations: `+`, `-`, `*`, `/`, `%` with proper precedence
- Comparison operations: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operations: `&&`, `||`, `!`
- Bitwise operations: `&`, `|`, `^`
- Assignment operations: `=`, `+=`, `-=`, `*=`, `/=`
- Control flow: `if/else`, `while`, `for` loops with proper scoping
- Functions with parameters, return values, and closures
- Print statements with multiple arguments
- Automatic type conversions between compatible types

### Example Programs

```mvm
// Variables and arithmetic
int x = 10;
double y = 3.14;
print('Result: ', x + y * 2, '\n');

// Control flow
if (x > 5) {
    print('x is greater than 5\n');
    for (i in 1..x) {
        print('i = ', i, '\n');
    }
}

// Functions with recursion
function int factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

print('factorial(10) = ', factorial(10), '\n');
```

## Architecture

This educational VM demonstrates several key concepts in language implementation:

### Core Components

1. **Lexer & Parser** (`src/parser.rs`)
   - Recursive descent parser for the MVM language
   - Basic error reporting with position information
   - Demonstrates parsing techniques for expressions and statements

2. **Abstract Syntax Tree** (`src/ast.rs`)
   - AST representation with visitor pattern
   - Shows how to structure language constructs in memory
   - Includes a pretty printer for educational purposes

3. **Type System** (`src/types.rs`)
   - Simple type system with int, double, string, and void
   - Demonstrates type checking and conversions
   - Function signatures and parameter handling

4. **Execution Engines**
   - **AST Interpreter** (`src/interpreter.rs`): Direct AST execution (tree-walking interpreter)
   - **Bytecode System** (`src/bytecode.rs` + `src/translator.rs`): Demonstrates compilation to bytecode
   - **Bytecode Interpreter**: Stack-based bytecode execution

5. **Virtual Machine** (`src/vm.rs`)
   - Unified interface for different execution modes
   - Demonstrates how to build a simple VM API

## Educational Value

This project demonstrates several important concepts in programming language implementation:

- **Lexical Analysis**: How to break source code into tokens
- **Parsing**: Converting tokens into an Abstract Syntax Tree
- **AST Design**: Representing language constructs in memory
- **Interpreters**: Tree-walking interpretation vs. bytecode execution
- **Type Systems**: Static typing with runtime type checking
- **Scoping**: Variable scope and environment management
- **Function Calls**: Parameter passing and return values
- **Error Handling**: Parse errors and runtime exceptions

## Usage

### Building and Running

```bash
# Clone and build
git clone <repository-url>
cd mathvm
cargo build --release

# Run a program
cargo run -- tests/mark3/add.mvm

# Run in pretty-print mode
cargo run -- -p tests/mark3/function.mvm
```

### Library Usage

```rust
use mathvm::prelude::*;

// Simple execution
let mut vm = VM::new();
let result = vm.execute("print('Hello, ', 2 + 2);")?;
```

## Testing

```bash
# Run all tests
cargo test

# Run integration tests
cargo test integration_tests

# Run with output visible
cargo test -- --nocapture
```

## Development

### Building from Source

```bash
# Debug build
cargo build

# Optimized release build  
cargo build --release

# Run linting
cargo clippy

# Format code
cargo fmt
```

## License

MIT License - see [LICENSE](LICENSE) file for details.

---

**MathVM** - An educational virtual machine implementation in Rust for learning language design and implementation.