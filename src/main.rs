use clap::{Arg, Command};
use mathvm::prelude::*;
use std::fs;
use std::path::Path;

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
            eprintln!("Error: File '{}' not found", file_path);
            std::process::exit(1);
        }
        fs::read_to_string(file_path).map_err(|e| {
            eprintln!("Error reading file '{}': {}", file_path, e);
            std::process::exit(1);
        }).unwrap()
    } else {
        // Default expression from the original C++ code
        "double x; double y; x += 8.0; y = 2.0; print('Hello, x=', x, ' y=', y, '\\n');".to_string()
    };

    let mut vm = VM::new();
    
    match mode {
        "printer" => {
            // TODO: Implement printer mode
            // Parse the source code and print it back out
            println!("TODO: Implement printer mode");
            println!("This should parse '{}' and print it back", source_code);
        }
        "jit" => {
            // TODO: Implement JIT mode
            println!("TODO: Implement JIT mode");
            println!("JIT compilation is not yet implemented");
        }
        "interpreter" | _ => {
            // TODO: Implement interpreter mode
            println!("TODO: Implement interpreter mode");
            match vm.execute(&source_code) {
                Ok(_) => println!("Execution completed successfully"),
                Err(e) => {
                    eprintln!("Execution error: {}", e);
                    std::process::exit(1);
                }
            }
        }
    }

    Ok(())
}