use mathvm::prelude::*;

/// Unit tests for core VM functionality
/// These tests verify individual components and basic functionality

#[test]
fn test_basic_arithmetic() {
    let mut vm = VM::new();
    let result = vm.execute("double x = 2.0; double y = 3.0; print(x + y);");
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("5"));
}

#[test]
fn test_variable_assignment() {
    let mut vm = VM::new();
    let result = vm.execute("int x = 10; x = x + 5; print(x);");
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("15"));
}

#[test]
fn test_string_operations() {
    let mut vm = VM::new();
    let result = vm.execute("string s = 'Hello'; print(s);");
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("Hello"));
}

#[test]
fn test_type_conversions() {
    let mut vm = VM::new();
    let result = vm.execute("int i = 42; double d = i; print(d);");
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("42"));
}

#[test]
fn test_compilation_to_bytecode() {
    let vm = VM::new();
    let result = vm.compile("int x = 42; print(x);");
    assert!(result.is_ok());

    let bytecode = result.unwrap();
    assert!(!bytecode.is_empty());

    // Check that the bytecode contains some expected instructions
    let disassembly = vm.disassemble_bytecode(&bytecode);
    assert!(disassembly.contains("ILOAD"));
    assert!(disassembly.contains("IPRINT"));
    assert!(disassembly.contains("STOP"));
}

#[test]
fn test_error_handling() {
    let mut vm = VM::new();

    // Test undefined variable
    let result = vm.execute("print(undefined_var);");
    assert!(result.is_err());

    // Test type error
    let result = vm.execute("int x = 'not a number';");
    assert!(result.is_err());

    // Test division by zero
    let result = vm.execute("int x = 10 / 0;");
    assert!(result.is_err());
}

#[test]
fn test_parser_error_handling() {
    let mut vm = VM::new();

    // Test syntax error
    let result = vm.execute("int x = ;");
    assert!(result.is_err());

    // Test missing semicolon
    let result = vm.execute("int x = 42 print(x);");
    assert!(result.is_err());

    // Test unmatched parentheses
    let result = vm.execute("print(1 + 2;");
    assert!(result.is_err());
}

#[test]
fn test_comprehensive_example() {
    let mut vm = VM::new();
    let source = r#"
        int x = 10;
        double y = 3.14;
        string msg = 'Result: ';
        
        x = x * 2;
        y = y + 1.0;
        
        print(msg);
        print(x);
        print(' ');
        print(y);
        print('\n');
    "#;

    let result = vm.execute(source);
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());

    let combined_output = output.join("");
    assert!(combined_output.contains("Result: "));
    assert!(combined_output.contains("20"));
    assert!(combined_output.contains("4.14"));
}

#[test]
fn test_function_calls() {
    let mut vm = VM::new();
    let source = r#"
        function int add(int a, int b) {
            return a + b;
        }
        
        int result = add(5, 3);
        print(result);
    "#;

    let result = vm.execute(source);
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("8"));
}

#[test]
fn test_recursive_functions() {
    let mut vm = VM::new();
    let source = r#"
        function int factorial(int n) {
            if (n <= 1) {
                return 1;
            }
            return n * factorial(n - 1);
        }
        
        print(factorial(5));
    "#;

    let result = vm.execute(source);
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("120"));
}

#[test]
fn test_loop_constructs() {
    let mut vm = VM::new();
    let source = r#"
        int sum = 0;
        int i;
        for (i in 1..5) {
            sum = sum + i;
        }
        print(sum);
    "#;

    let result = vm.execute(source);
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("15")); // 1+2+3+4+5 = 15
}

#[test]
fn test_conditional_statements() {
    let mut vm = VM::new();
    let source = r#"
        int x = 10;
        if (x > 5) {
            print('greater');
        } else {
            print('lesser');
        }
    "#;

    let result = vm.execute(source);
    assert!(result.is_ok());

    let output = vm.get_output();
    assert!(!output.is_empty());
    assert!(output[0].contains("greater"));
}
