# MathVM Assignment Instructions

This document provides detailed instructions for students working on the MathVM assignment.

## Getting Started

1. **Clone and build the project:**
   ```bash
   git clone <repository-url>
   cd mathvm
   cargo build
   ```

2. **Run the tests to see current status:**
   ```bash
   cargo test
   ```
   
   Most tests will fail initially - this is expected! Your job is to implement the missing functionality.

3. **Try running a basic program:**
   ```bash
   cargo run -- examples/basic.mvm
   ```

## Your Tasks

### Phase 1: Basic Language Support (30 points)

**Goal**: Implement basic variable declarations, arithmetic, and print statements.

**Files to modify**: 
- `src/interpreter.rs` - Complete the AST interpreter
- `src/translator.rs` - Complete the bytecode generator

**Test command**: `cargo test mark3`

**What to implement:**
1. Variable declarations (`int x = 42;`)
2. Basic arithmetic (`+`, `-`, `*`, `/`)
3. Print statements (`print('hello', x);`)
4. Type conversions between int and double

**Success criteria**: Tests in `tests/mark3/` should pass

### Phase 2: Control Flow (25 points)

**Goal**: Implement if/else statements, while loops, and for loops.

**Files to modify**:
- `src/interpreter.rs` - Add control flow to AST interpreter
- `src/translator.rs` - Add control flow bytecode generation

**Test command**: `cargo test mark3 | grep -E "(if|while|for)"`

**What to implement:**
1. If/else statements with proper condition evaluation
2. While loops with break conditions
3. For loops with range syntax (`for (i in 1..10)`)
4. Proper scoping for loop variables

**Success criteria**: Control flow tests should pass

### Phase 3: Functions (25 points)

**Goal**: Implement function declarations and calls.

**Files to modify**:
- `src/interpreter.rs` - Function call mechanism
- `src/translator.rs` - Function bytecode generation
- `src/types.rs` - May need signature improvements

**Test command**: `cargo test mark4`

**What to implement:**
1. Function declarations with parameters
2. Function calls with argument passing
3. Return statements
4. Proper parameter and local variable scoping

**Success criteria**: Function tests in `tests/mark4/` should pass

### Phase 4: Advanced Features (20 points)

**Goal**: Implement advanced language features.

**Test command**: `cargo test mark4 mark5`

**What to implement:**
1. Closures and lambda functions
2. Native function bindings
3. String operations
4. Type casting
5. Error handling improvements

**Success criteria**: Advanced tests should pass

## Implementation Tips

### Understanding the Architecture

1. **Parser**: Converts source code → AST
   - Already implemented and working
   - You shouldn't need to modify this

2. **AST Interpreter**: Executes AST directly
   - Located in `src/interpreter.rs`
   - Uses the visitor pattern
   - This is where you'll implement most logic

3. **Bytecode Generator**: Converts AST → Bytecode
   - Located in `src/translator.rs`
   - Generates stack-based instructions
   - More complex but offers better performance

4. **Bytecode Interpreter**: Executes bytecode
   - Also in `src/interpreter.rs`
   - Stack-based execution model
   - Can implement this after AST interpreter

### Debugging Tips

1. **Use the pretty printer:**
   ```bash
   cargo run -- -p examples/basic.mvm
   ```

2. **Check bytecode generation:**
   ```rust
   let vm = VM::new();
   let bytecode = vm.compile("int x = 42;")?;
   println!("{}", vm.disassemble_bytecode(&bytecode));
   ```

3. **Add debug prints:**
   ```rust
   println!("DEBUG: Processing variable {}", name);
   ```

4. **Run specific tests:**
   ```bash
   cargo test test_mark3_add -- --nocapture
   ```

### Common Patterns

#### Variable Storage
```rust
// In AST Interpreter
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
```

#### Bytecode Generation
```rust
// Generate bytecode for arithmetic
match (left_type, right_type) {
    (VarType::Int, VarType::Int) => {
        self.bytecode.add_instruction(Instruction::IAdd);
        Ok(VarType::Int)
    }
    (VarType::Double, VarType::Double) => {
        self.bytecode.add_instruction(Instruction::DAdd);
        Ok(VarType::Double)
    }
    // Handle type conversions...
}
```

#### Scope Management
```rust
// For block statements
self.push_scope();
let result = self.visit_block_contents(statements)?;
self.pop_scope();
```

## Testing Strategy

1. **Start with simple tests**: Begin with `add.mvm`, `literal.mvm`
2. **Test incrementally**: Don't try to implement everything at once
3. **Use the REPL**: You can test expressions interactively
4. **Check error messages**: Make sure your errors are helpful

## Common Pitfalls

1. **Type conversions**: Remember to handle int ↔ double conversions
2. **Scoping**: Variables should be properly scoped in blocks and functions
3. **Stack management**: In bytecode, ensure balanced stack operations
4. **Error handling**: Don't panic - return proper Error types

## Submission

1. **Code quality**: Follow Rust conventions, use `cargo fmt` and `cargo clippy`
2. **Tests**: All tests should pass: `cargo test`
3. **Documentation**: Comment complex algorithms
4. **Performance**: Bytecode interpreter should be reasonably fast

## Getting Help

1. **Read the existing code**: Understand patterns used in working parts
2. **Check the tests**: Test files show expected behavior
3. **Use the debugger**: `rust-gdb` or VS Code debugging
4. **Ask questions**: Use course forum or office hours

## Grading Rubric

- **Functionality (60%)**: Do the tests pass?
- **Code Quality (20%)**: Clean, readable, well-structured code
- **Error Handling (10%)**: Proper error messages and handling
- **Performance (10%)**: Reasonable execution speed

Good luck! Remember, this is a learning exercise - focus on understanding how virtual machines work, not just passing tests.