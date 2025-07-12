use mathvm::prelude::*;
use std::fs;
use std::path::Path;
use tempfile::NamedTempFile;

/// Test helper to run a single MVM test case
fn run_test_case(test_name: &str, test_dir: &str) -> Result<bool> {
    let mvm_file = format!("{}/{}.mvm", test_dir, test_name);
    let expect_file = format!("{}/{}.expect", test_dir, test_name);
    
    if !Path::new(&mvm_file).exists() {
        return Err(Error::runtime_error(&format!("Test file {} not found", mvm_file)));
    }
    
    if !Path::new(&expect_file).exists() {
        return Err(Error::runtime_error(&format!("Expected output file {} not found", expect_file)));
    }
    
    let source = fs::read_to_string(&mvm_file)?;
    let expected = fs::read_to_string(&expect_file)?;
    
    let mut vm = VM::new();
    
    // Execute the program
    match vm.execute(&source) {
        Ok(_) => {
            let output = vm.get_output();
            let actual = output.join("");
            
            // Compare output (ignoring whitespace differences for now)
            let expected_trimmed = expected.trim();
            let actual_trimmed = actual.trim();
            
            Ok(expected_trimmed == actual_trimmed)
        }
        Err(e) => {
            eprintln!("Execution error for {}: {}", test_name, e);
            Ok(false)
        }
    }
}

/// Test the printer mode (parse and print back)
fn test_printer_mode(test_name: &str, test_dir: &str) -> Result<bool> {
    let mvm_file = format!("{}/{}.mvm", test_dir, test_name);
    
    if !Path::new(&mvm_file).exists() {
        return Err(Error::runtime_error(&format!("Test file {} not found", mvm_file)));
    }
    
    let source = fs::read_to_string(&mvm_file)?;
    let vm = VM::new();
    
    // Parse and print the source
    let printed = vm.print_source(&source)?;
    
    // Create a temporary file with the printed output
    let temp_file = NamedTempFile::new()?;
    fs::write(temp_file.path(), &printed)?;
    
    // Parse and print the printed output
    let source2 = fs::read_to_string(temp_file.path())?;
    let printed2 = vm.print_source(&source2)?;
    
    // The two printed versions should be identical
    Ok(printed == printed2)
}

#[test]
fn test_mark3_add() {
    let result = run_test_case("add", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "add test failed");
}

#[test]
fn test_mark3_assign() {
    let result = run_test_case("assign", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "assign test failed");
}

#[test]
fn test_mark3_expr() {
    let result = run_test_case("expr", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "expr test failed");
}

#[test]
fn test_mark3_literal() {
    let result = run_test_case("literal", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "literal test failed");
}

#[test]
fn test_mark3_mul() {
    let result = run_test_case("mul", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "mul test failed");
}

#[test]
fn test_mark3_sub() {
    let result = run_test_case("sub", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "sub test failed");
}

#[test]
fn test_mark3_div() {
    let result = run_test_case("div", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "div test failed");
}

#[test]
fn test_mark3_if() {
    let result = run_test_case("if", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "if test failed");
}

#[test]
fn test_mark3_while() {
    let result = run_test_case("while", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "while test failed");
}

#[test]
fn test_mark3_for() {
    let result = run_test_case("for", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "for test failed");
}

#[test]
fn test_mark3_function() {
    let result = run_test_case("function", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "function test failed");
}

#[test]
fn test_mark3_bitwise() {
    let result = run_test_case("bitwise", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "bitwise test failed");
}

// Mark 4 tests (more advanced features)
#[test]
fn test_mark4_ackermann() {
    let result = run_test_case("ackermann", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "ackermann test failed");
}

#[test]
fn test_mark4_fibonacci() {
    let result = run_test_case("fib", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "fib test failed");
}

#[test]
fn test_mark4_closure() {
    let result = run_test_case("closure", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "closure test failed");
}

#[test]
fn test_mark4_function_call() {
    let result = run_test_case("function-call", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "function-call test failed");
}

#[test]
fn test_mark4_function_cast() {
    let result = run_test_case("function-cast", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "function-cast test failed");
}

#[test]
fn test_mark4_complex() {
    let result = run_test_case("complex", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "complex test failed");
}

#[test]
fn test_mark4_complex2() {
    let result = run_test_case("complex2", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "complex2 test failed");
}

#[test]
fn test_mark4_casts() {
    let result = run_test_case("casts", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "casts test failed");
}

// Mark 5 tests (native functions)
#[test]
fn test_mark5_function_native() {
    let result = run_test_case("function_native", "tests/mark5");
    assert!(result.is_ok() && result.unwrap(), "function_native test failed");
}

// Printer mode tests
#[test]
fn test_printer_mode_basic() {
    let result = test_printer_mode("add", "tests/mark3");
    assert!(result.is_ok() && result.unwrap(), "printer mode test failed");
}

#[test]
fn test_printer_mode_complex() {
    let result = test_printer_mode("complex", "tests/mark4");
    assert!(result.is_ok() && result.unwrap(), "printer mode complex test failed");
}

// Basic functionality tests
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

// Helper function to run all tests for a given directory
pub fn run_all_tests_in_directory(test_dir: &str) -> Result<Vec<(String, bool)>> {
    let mut results = Vec::new();
    
    let dir_path = Path::new(test_dir);
    if !dir_path.exists() {
        return Err(Error::runtime_error(&format!("Test directory {} not found", test_dir)));
    }
    
    let entries = fs::read_dir(dir_path)?;
    for entry in entries {
        let entry = entry?;
        let path = entry.path();
        
        if path.extension().map_or(false, |ext| ext == "mvm") {
            let test_name = path.file_stem().unwrap().to_string_lossy().to_string();
            let result = run_test_case(&test_name, test_dir)?;
            results.push((test_name, result));
        }
    }
    
    Ok(results)
}

// Utility test to check if all required test files exist
#[test]
fn test_files_exist() {
    let test_dirs = ["tests/mark3", "tests/mark4", "tests/mark5"];
    
    for test_dir in &test_dirs {
        let dir_path = Path::new(test_dir);
        assert!(dir_path.exists(), "Test directory {} should exist", test_dir);
        
        // Check that the directory contains .mvm files
        let entries: Vec<_> = fs::read_dir(dir_path)
            .unwrap()
            .filter_map(|e| e.ok())
            .filter(|e| e.path().extension().map_or(false, |ext| ext == "mvm"))
            .collect();
        
        assert!(!entries.is_empty(), "Test directory {} should contain .mvm files", test_dir);
    }
}