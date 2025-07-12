# MathVM - Rust Implementation

A Virtual Machine for Mathematical Expressions implemented in Rust for the Virtual Machines course.

## Overview

MathVM is a simple stack-based virtual machine that executes programs written in the MVM language. This is a complete rewrite of the original C++ implementation in Rust, designed as a course assignment stub for students to learn about virtual machine implementation.

## MVM Language

The MVM language is a simple C-style language with the following features:

### Data Types
- `int`: 64-bit signed integer
- `double`: IEEE 754 double precision floating point
- `string`: Immutable UTF-8 strings

### Language Features
- Variable declarations with optional initialization
- Arithmetic operations: `+`, `-`, `*`, `/`, `%`
- Comparison operations: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical operations: `&&`, `||`, `!`
- Bitwise operations: `&`, `|`, `^`
- Assignment operations: `=`, `+=`, `-=`, `*=`, `/=`
- Control flow: `if/else`, `while`, `for` loops
- Functions with parameters and return values
- Print statements for output

### Example Programs

```mvm
// Basic arithmetic
int x = 10;
double y = 3.14;
print('x + y = ', x + y, '\n');

// Control flow
if (x > 5) {
    print('x is greater than 5\n');
} else {
    print('x is not greater than 5\n');
}

// Loops
for (i in 1..10) {
    print('i = ', i, '\n');
}

// Functions
function int factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

print('factorial(5) = ', factorial(5), '\n');
```

## Architecture

The MathVM implementation consists of several key components:

### 1. Lexer and Parser (`src/parser.rs`)
- Tokenizes MVM source code
- Parses tokens into an Abstract Syntax Tree (AST)
- Handles syntax errors with detailed error messages

### 2. Abstract Syntax Tree (`src/ast.rs`)
- Represents the parsed program structure
- Includes visitor pattern for AST traversal
- Pretty printer for source code formatting

### 3. Type System (`src/types.rs`)
- Defines the MVM type system
- Handles type conversions and checking
- Runtime value representation

### 4. Bytecode (`src/bytecode.rs`)
- Stack-based bytecode instruction set
- Corresponds to the original C++ bytecode specification
- Includes instruction encoding/decoding

### 5. Interpreters (`src/interpreter.rs`)
- **AST Interpreter**: Directly executes the AST
- **Bytecode Interpreter**: Executes compiled bytecode
- Both support variable scoping and function calls

### 6. Bytecode Generator (`src/translator.rs`)
- Converts AST to bytecode instructions
- Handles optimizations for common patterns
- Manages string constants and variable allocation

### 7. Virtual Machine (`src/vm.rs`)
- Main interface for executing MVM programs
- Supports multiple execution modes
- Provides debugging and introspection capabilities

## Usage

### Building the Project

```bash
# Build the project
cargo build

# Build with optimizations
cargo build --release

# Run tests
cargo test

# Run with verbose output
cargo test -- --nocapture
```

### Running MVM Programs

```bash
# Execute a MVM file
cargo run -- program.mvm

# Use interpreter mode (default)
cargo run -- --mode interpreter program.mvm

# Use printer mode (parse and pretty-print)
cargo run -- -p program.mvm

# Use JIT mode (when implemented)
cargo run -- -j program.mvm
```

### Library Usage

```rust
use mathvm::prelude::*;

// Execute MVM source code
let mut vm = VM::new();
let result = vm.execute("int x = 42; print(x);")?;

// Compile to bytecode
let bytecode = vm.compile("int x = 42; print(x);")?;

// Pretty-print source code
let formatted = vm.print_source("int x=42;print(x);")?;
```

## Assignment Tasks

Students are expected to complete the following tasks:

### Task 1: Basic Language Features
- [ ] Implement variable declarations and assignments
- [ ] Implement arithmetic operations
- [ ] Implement print statements
- [ ] Handle basic type conversions

### Task 2: Control Flow
- [ ] Implement if/else statements
- [ ] Implement while loops
- [ ] Implement for loops
- [ ] Handle proper control flow in bytecode

### Task 3: Functions
- [ ] Implement function declarations
- [ ] Implement function calls
- [ ] Handle parameter passing and return values
- [ ] Implement proper scoping rules

### Task 4: Advanced Features
- [ ] Implement closures and lambda functions
- [ ] Implement native function calls
- [ ] Optimize bytecode generation
- [ ] Add debugging capabilities

### Task 5: JIT Compilation (Optional)
- [ ] Implement Just-In-Time compilation
- [ ] Generate native machine code
- [ ] Optimize hot code paths

## Testing

The project includes comprehensive tests that mirror the original C++ test suite:

### Test Categories
- **Mark 3**: Basic language features (variables, arithmetic, control flow)
- **Mark 4**: Advanced features (functions, closures, complex expressions)
- **Mark 5**: Native functions and system integration

### Running Tests

```bash
# Run all tests
cargo test

# Run specific test category
cargo test mark3

# Run integration tests
cargo test integration_tests

# Run with output
cargo test -- --nocapture
```

### Test Files

Test files are located in the `tests/` directory and include:
- `mark3/`: Basic language feature tests
- `mark4/`: Advanced feature tests
- `mark5/`: Native function tests
- `integration_tests.rs`: Rust integration tests

Each test consists of a `.mvm` file (source code) and a `.expect` file (expected output).

## Error Handling

The implementation provides detailed error messages for:
- Syntax errors with line/column information
- Type errors with context
- Runtime errors with stack traces
- Undefined variable/function errors

## Performance Considerations

The Rust implementation offers several advantages:
- Memory safety without garbage collection
- Zero-cost abstractions
- Efficient bytecode execution
- Parallelizable compilation phases

## Development Guidelines

### Code Style
- Follow Rust naming conventions
- Use `clippy` for linting
- Format code with `rustfmt`
- Write comprehensive documentation

### Testing
- Write unit tests for all public functions
- Add integration tests for end-to-end scenarios
- Use property-based testing where appropriate
- Maintain test coverage above 80%

### Documentation
- Document all public APIs
- Include usage examples
- Maintain this README
- Add inline comments for complex algorithms

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Add tests for new functionality
5. Run `cargo test` and `cargo clippy`
6. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Original C++ implementation by the Virtual Machines course instructors
- Rust community for excellent tooling and documentation
- Students and teaching assistants for feedback and contributions

## Getting Help

If you encounter issues:
1. Check the test output for specific error messages
2. Review the documentation in `src/lib.rs`
3. Look at the example programs in `tests/`
4. Ask questions in the course forum or office hours

Remember: This is a learning exercise. The goal is to understand how virtual machines work, not just to pass the tests. Take time to understand each component and how they work together.

## Implementation Status

### ✅ Completed
- [x] Basic project structure and build system
- [x] Lexer and parser for MVM language
- [x] AST representation and pretty printer
- [x] Type system and value representation
- [x] Bytecode instruction set
- [x] Basic AST interpreter
- [x] Bytecode generator (partial)
- [x] Test framework and integration tests
- [x] Error handling and reporting

### 🚧 In Progress
- [ ] Complete bytecode interpreter
- [ ] Control flow implementation
- [ ] Function calls and scoping
- [ ] Native function interface

### 📋 TODO
- [ ] JIT compiler integration
- [ ] Performance optimizations
- [ ] Advanced debugging features
- [ ] IDE integration support

The implementation provides a solid foundation for students to build upon. The core infrastructure is in place, and the main task is to complete the missing functionality in the interpreters and bytecode generator.