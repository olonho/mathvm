//! MathVM Command Line Interface
//!
//! This binary provides a command-line interface for the MathVM virtual machine.
//! It supports multiple execution modes including interpretation and code printing.

use clap::{Arg, Command};
use mathvm::prelude::*;
use std::fs;
use std::path::Path;

/// Main entry point for the MVM command-line interface
fn main() -> Result<()> {
    let matches = Command::new("mvm")
        .version("0.1.0")
        .about("MathVM - A Virtual Machine for Mathematical Expressions")
        .arg(
            Arg::new("mode")
                .short('m')
                .long("mode")
                .value_name("MODE")
                .help("Execution mode: interpreter, jit, or printer")
                .default_value("interpreter"),
        )
        .arg(
            Arg::new("file")
                .help("MVM source file to execute")
                .required(false)
                .index(1),
        )
        .arg(
            Arg::new("printer")
                .short('p')
                .help("Print mode - parse and print the source code")
                .action(clap::ArgAction::SetTrue),
        )
        .arg(
            Arg::new("jit")
                .short('j')
                .help("JIT mode - compile and execute with JIT")
                .action(clap::ArgAction::SetTrue),
        )
        .get_matches();

    let mode = if matches.get_flag("printer") {
        "printer"
    } else if matches.get_flag("jit") {
        "jit"
    } else {
        matches.get_one::<String>("mode").unwrap()
    };

    let source_code = if let Some(file_path) = matches.get_one::<String>("file") {
        if !Path::new(file_path).exists() {
            eprintln!("Error: File '{file_path}' not found");
            std::process::exit(1);
        }
        fs::read_to_string(file_path)
            .map_err(|e| {
                eprintln!("Error reading file '{file_path}': {e}");
                std::process::exit(1);
            })
            .unwrap()
    } else {
        // Default expression from the original C++ code
        "double x; double y; x += 8.0; y = 2.0; print('Hello, x=', x, ' y=', y, '\\n');".to_string()
    };

    let mut vm = VM::new();

    match mode {
        "printer" => match vm.print_source(&source_code) {
            Ok(formatted) => println!("{formatted}"),
            Err(e) => {
                eprintln!("Parse error: {e}");
                std::process::exit(1);
            }
        },
        "jit" => {
            println!("JIT compilation is not yet implemented");
            std::process::exit(1);
        }
        _ => match vm.execute(&source_code) {
            Ok(_) => {}
            Err(e) => {
                eprintln!("Execution error: {e}");
                std::process::exit(1);
            }
        },
    }

    Ok(())
}
