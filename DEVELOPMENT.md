# MathVM Development Guide

Quick reference for students working on the MathVM assignment.

## Project Structure

```
mathvm/
├── src/                    # Rust source code
│   ├── lib.rs             # Library root and prelude
│   ├── main.rs            # CLI application entry point
│   ├── parser.rs          # Lexer and parser (COMPLETE)
│   ├── ast.rs             # Abstract Syntax Tree (COMPLETE)
│   ├── types.rs           # Type system and values (COMPLETE)
│   ├── error.rs           # Error handling (COMPLETE)
│   ├── bytecode.rs        # Bytecode instructions (COMPLETE)
│   ├── interpreter.rs     # ⚠️  NEEDS IMPLEMENTATION
│   ├── translator.rs      # ⚠️  NEEDS IMPLEMENTATION
│   └── vm.rs              # Virtual machine interface (COMPLETE)
├── tests/                 # Test suite
│   ├── integration_tests.rs # Rust test harness
│   ├── mark3/             # Basic features tests
│   ├── mark4/             # Advanced features tests
│   └── mark5/             # Native functions tests
├── examples/              # Example MVM programs
├── ASSIGNMENT.md          # Detailed assignment instructions
└── README.md              # Project overview
```

## Quick Start Commands

```bash
# Build the project
cargo build

# Run all tests (most will fail initially)
cargo test

# Run specific test category
cargo test mark3

# Run with output visible
cargo test test_basic_arithmetic -- --nocapture

# Check code style
cargo clippy

# Format code
cargo fmt

# Run a MVM program
cargo run -- examples/basic.mvm

# Pretty-print mode
cargo run -- -p examples/basic.mvm
```

## Implementation Order

### 1. Start with AST Interpreter (`src/interpreter.rs`)

Focus on the `AstInterpreter` struct and its `AstVisitor` implementation:

```rust
impl AstVisitor<Result<Value>> for AstInterpreter {
    // Start with these methods:
    fn visit_int_literal(&mut self, value: i64) -> Result<Value> {
        // TODO: Return Value::Int(value)
    }
    
    fn visit_var_declaration(&mut self, var_type: VarType, name: &str, initializer: &Option<Box<AstNode>>) -> Result<Value> {
        // TODO: Handle variable declarations
    }
    
    fn visit_binary_op(&mut self, left: &AstNode, op: &BinaryOp, right: &AstNode) -> Result<Value> {
        // TODO: Handle arithmetic operations
    }
}
```

### 2. Test Your Progress

```bash
# Test basic literals
cargo test test_basic_arithmetic

# Test variable declarations
cargo test test_variable_assignment

# Test specific MVM files
cargo test test_mark3_literal
```

### 3. Move to Bytecode Generation (`src/translator.rs`)

Focus on the `BytecodeGenerator` struct:

```rust
impl AstVisitor<Result<VarType>> for BytecodeGenerator {
    fn visit_int_literal(&mut self, value: i64) -> Result<VarType> {
        // TODO: Generate ILOAD instruction
        self.bytecode.add_instruction(Instruction::ILoad);
        self.bytecode.add_i64(value);
        Ok(VarType::Int)
    }
}
```

## Key Concepts

### Value Types
```rust
pub enum Value {
    Void,
    Int(i64),           // 64-bit integer
    Double(f64),        // IEEE 754 double
    String(String),     // UTF-8 string
}
```

### Variable Scoping
```rust
// Push new scope for blocks
self.push_scope();
// ... execute block contents
self.pop_scope();
```

### Type Conversions
```rust
match (left_type, right_type) {
    (VarType::Int, VarType::Double) => {
        // Convert int to double
        left_value.convert_to(VarType::Double)?
    }
    // ... other cases
}
```

### Bytecode Instructions
```rust
// Load constants
Instruction::ILoad0     // Push 0 (int)
Instruction::ILoad1     // Push 1 (int)
Instruction::ILoad      // Push int from instruction stream

// Arithmetic
Instruction::IAdd       // Pop two ints, push sum
Instruction::DSub       // Pop two doubles, push difference

// Variables
Instruction::StoreIVar0 // Store int to variable 0
Instruction::LoadDVar   // Load double from variable (ID follows)
```

## Debugging Tips

### 1. Enable Debug Output
```rust
println!("DEBUG: Visiting node: {:?}", node);
println!("DEBUG: Stack: {:?}", self.stack);
```

### 2. Test Incrementally
Start with the simplest test:
```mvm
print(42);
```

Then gradually add complexity:
```mvm
int x = 42;
print(x);
```

### 3. Use the Pretty Printer
```bash
cargo run -- -p tests/mark3/add.mvm
```

This shows you exactly how the parser interprets the code.

### 4. Check Bytecode Generation
```rust
let vm = VM::new();
let bytecode = vm.compile("int x = 42;")?;
println!("{}", vm.disassemble_bytecode(&bytecode));
```

## Common Errors and Solutions

### "UndefinedVariable" Error
- Make sure you're storing variables in the current scope
- Check that `visit_var_declaration` actually calls `define_variable`

### "StackUnderflow" Error
- Ensure you push values before trying to pop them
- Each bytecode instruction should maintain stack balance

### "TypeError" Error
- Implement proper type conversions
- Handle int ↔ double conversions explicitly

### Test Failures
- Read the test output carefully - it shows expected vs actual
- Check the `.expect` files to understand expected output format
- Make sure print statements output exactly what's expected

## Testing Strategy

1. **Start small**: Get `literal.mvm` working first
2. **One feature at a time**: Don't try to implement everything
3. **AST first**: Get AST interpreter working before bytecode
4. **Use existing tests**: Don't write new tests, fix existing ones

## Performance Tips

- The AST interpreter will be slower but easier to debug
- The bytecode interpreter should be much faster
- Don't optimize prematurely - correctness first!

## Getting Unstuck

1. **Read the working code**: Look at `parser.rs` for patterns
2. **Check the tests**: They show exactly what behavior is expected
3. **Use print debugging**: Liberally add debug prints
4. **Start over on a small piece**: Sometimes a fresh approach helps
5. **Ask for help**: Use office hours or course forum

## Final Checklist

Before submission:
- [ ] `cargo build` completes without errors
- [ ] `cargo test` shows reasonable progress
- [ ] `cargo clippy` has no warnings
- [ ] `cargo fmt` applied
- [ ] Code is commented and readable
- [ ] Git repository is clean and committed

Good luck! Remember, this is about learning - don't just copy code, understand what each piece does.