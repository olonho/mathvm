use crate::error::{Error, Result};
use std::fmt;

/// Bytecode instructions for the MVM virtual machine
/// These correspond to the C++ FOR_BYTECODES macro
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Instruction {
    // Invalid instruction
    Invalid = 0,
    
    // Load constants
    DLoad,      // Load double constant (8 bytes follow)
    ILoad,      // Load int constant (8 bytes follow)
    SLoad,      // Load string constant (2 bytes string ID follow)
    DLoad0,     // Load double 0
    ILoad0,     // Load int 0
    SLoad0,     // Load empty string
    DLoad1,     // Load double 1
    ILoad1,     // Load int 1
    DLoadM1,    // Load double -1
    ILoadM1,    // Load int -1
    
    // Arithmetic operations
    DAdd,       // Add two doubles
    IAdd,       // Add two integers
    DSub,       // Subtract two doubles
    ISub,       // Subtract two integers
    DMul,       // Multiply two doubles
    IMul,       // Multiply two integers
    DDiv,       // Divide two doubles
    IDiv,       // Divide two integers
    IMod,       // Modulo operation on integers
    DNeg,       // Negate double
    INeg,       // Negate integer
    
    // Bitwise operations
    IAor,       // Bitwise OR
    IAand,      // Bitwise AND
    IAxor,      // Bitwise XOR
    
    // Print operations
    IPrint,     // Print integer
    DPrint,     // Print double
    SPrint,     // Print string
    
    // Type conversions
    I2D,        // Convert int to double
    D2I,        // Convert double to int
    S2I,        // Convert string to int
    
    // Stack operations
    Swap,       // Swap two top values
    Pop,        // Remove top value
    
    // Variable operations (optimized for first 4 variables)
    LoadDVar0, LoadDVar1, LoadDVar2, LoadDVar3,
    LoadIVar0, LoadIVar1, LoadIVar2, LoadIVar3,
    LoadSVar0, LoadSVar1, LoadSVar2, LoadSVar3,
    StoreDVar0, StoreDVar1, StoreDVar2, StoreDVar3,
    StoreIVar0, StoreIVar1, StoreIVar2, StoreIVar3,
    StoreSVar0, StoreSVar1, StoreSVar2, StoreSVar3,
    
    // General variable operations
    LoadDVar,   // Load double variable (2 bytes var ID follow)
    LoadIVar,   // Load int variable (2 bytes var ID follow)
    LoadSVar,   // Load string variable (2 bytes var ID follow)
    StoreDVar,  // Store double variable (2 bytes var ID follow)
    StoreIVar,  // Store int variable (2 bytes var ID follow)
    StoreSVar,  // Store string variable (2 bytes var ID follow)
    
    // Context variable operations
    LoadCtxDVar,  // Load double from context (2 bytes ctx, 2 bytes var ID)
    LoadCtxIVar,  // Load int from context (2 bytes ctx, 2 bytes var ID)
    LoadCtxSVar,  // Load string from context (2 bytes ctx, 2 bytes var ID)
    StoreCtxDVar, // Store double to context (2 bytes ctx, 2 bytes var ID)
    StoreCtxIVar, // Store int to context (2 bytes ctx, 2 bytes var ID)
    StoreCtxSVar, // Store string to context (2 bytes ctx, 2 bytes var ID)
    
    // Comparison operations
    DCmp,       // Compare doubles
    ICmp,       // Compare integers
    
    // Control flow
    JA,         // Jump always (2 bytes offset follow)
    IfICmpNE,   // Jump if integers not equal (2 bytes offset follow)
    IfICmpE,    // Jump if integers equal (2 bytes offset follow)
    IfICmpG,    // Jump if upper > lower (2 bytes offset follow)
    IfICmpGE,   // Jump if upper >= lower (2 bytes offset follow)
    IfICmpL,    // Jump if upper < lower (2 bytes offset follow)
    IfICmpLE,   // Jump if upper <= lower (2 bytes offset follow)
    
    // Debugging and execution control
    Dump,       // Dump top of stack value
    Stop,       // Stop execution
    Call,       // Call function (2 bytes function ID follow)
    CallNative, // Call native function (2 bytes function ID follow)
    Return,     // Return from function
    Break,      // Debugger breakpoint
}

impl Instruction {
    /// Convert a byte value to an instruction
    pub fn from_byte(byte: u8) -> Result<Self> {
        match byte {
            0 => Ok(Instruction::Invalid),
            1 => Ok(Instruction::DLoad),
            2 => Ok(Instruction::ILoad),
            3 => Ok(Instruction::SLoad),
            4 => Ok(Instruction::DLoad0),
            5 => Ok(Instruction::ILoad0),
            6 => Ok(Instruction::SLoad0),
            7 => Ok(Instruction::DLoad1),
            8 => Ok(Instruction::ILoad1),
            9 => Ok(Instruction::DLoadM1),
            10 => Ok(Instruction::ILoadM1),
            11 => Ok(Instruction::DAdd),
            12 => Ok(Instruction::IAdd),
            13 => Ok(Instruction::DSub),
            14 => Ok(Instruction::ISub),
            15 => Ok(Instruction::DMul),
            16 => Ok(Instruction::IMul),
            17 => Ok(Instruction::DDiv),
            18 => Ok(Instruction::IDiv),
            19 => Ok(Instruction::IMod),
            20 => Ok(Instruction::DNeg),
            21 => Ok(Instruction::INeg),
            22 => Ok(Instruction::IAor),
            23 => Ok(Instruction::IAand),
            24 => Ok(Instruction::IAxor),
            25 => Ok(Instruction::IPrint),
            26 => Ok(Instruction::DPrint),
            27 => Ok(Instruction::SPrint),
            28 => Ok(Instruction::I2D),
            29 => Ok(Instruction::D2I),
            30 => Ok(Instruction::S2I),
            31 => Ok(Instruction::Swap),
            32 => Ok(Instruction::Pop),
            _ => Err(Error::InvalidBytecode(byte)),
        }
    }

    /// Get the byte value of this instruction
    pub fn to_byte(&self) -> u8 {
        *self as u8
    }

    /// Get the name of this instruction
    pub fn name(&self) -> &'static str {
        match self {
            Instruction::Invalid => "INVALID",
            Instruction::DLoad => "DLOAD",
            Instruction::ILoad => "ILOAD",
            Instruction::SLoad => "SLOAD",
            Instruction::DLoad0 => "DLOAD0",
            Instruction::ILoad0 => "ILOAD0",
            Instruction::SLoad0 => "SLOAD0",
            Instruction::DLoad1 => "DLOAD1",
            Instruction::ILoad1 => "ILOAD1",
            Instruction::DLoadM1 => "DLOADM1",
            Instruction::ILoadM1 => "ILOADM1",
            Instruction::DAdd => "DADD",
            Instruction::IAdd => "IADD",
            Instruction::DSub => "DSUB",
            Instruction::ISub => "ISUB",
            Instruction::DMul => "DMUL",
            Instruction::IMul => "IMUL",
            Instruction::DDiv => "DDIV",
            Instruction::IDiv => "IDIV",
            Instruction::IMod => "IMOD",
            Instruction::DNeg => "DNEG",
            Instruction::INeg => "INEG",
            Instruction::IAor => "IAOR",
            Instruction::IAand => "IAAND",
            Instruction::IAxor => "IAXOR",
            Instruction::IPrint => "IPRINT",
            Instruction::DPrint => "DPRINT",
            Instruction::SPrint => "SPRINT",
            Instruction::I2D => "I2D",
            Instruction::D2I => "D2I",
            Instruction::S2I => "S2I",
            Instruction::Swap => "SWAP",
            Instruction::Pop => "POP",
            _ => "UNKNOWN",
        }
    }

    /// Get the size of this instruction in bytes (including operands)
    pub fn size(&self) -> usize {
        match self {
            Instruction::DLoad | Instruction::ILoad => 9, // 1 + 8 bytes
            Instruction::SLoad => 3, // 1 + 2 bytes
            Instruction::LoadDVar | Instruction::LoadIVar | Instruction::LoadSVar => 3, // 1 + 2 bytes
            Instruction::StoreDVar | Instruction::StoreIVar | Instruction::StoreSVar => 3, // 1 + 2 bytes
            Instruction::LoadCtxDVar | Instruction::LoadCtxIVar | Instruction::LoadCtxSVar => 5, // 1 + 2 + 2 bytes
            Instruction::StoreCtxDVar | Instruction::StoreCtxIVar | Instruction::StoreCtxSVar => 5, // 1 + 2 + 2 bytes
            Instruction::JA | Instruction::IfICmpNE | Instruction::IfICmpE | 
            Instruction::IfICmpG | Instruction::IfICmpGE | Instruction::IfICmpL | 
            Instruction::IfICmpLE => 3, // 1 + 2 bytes
            Instruction::Call | Instruction::CallNative => 3, // 1 + 2 bytes
            _ => 1, // Single byte instruction
        }
    }
}

impl fmt::Display for Instruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.name())
    }
}

/// Bytecode container - holds a sequence of instructions and data
#[derive(Debug, Clone)]
pub struct Bytecode {
    data: Vec<u8>,
}

impl Bytecode {
    /// Create a new empty bytecode container
    pub fn new() -> Self {
        Bytecode { data: Vec::new() }
    }

    /// Add a single byte to the bytecode
    pub fn add_byte(&mut self, byte: u8) {
        self.data.push(byte);
    }

    /// Add an instruction to the bytecode
    pub fn add_instruction(&mut self, instruction: Instruction) {
        self.data.push(instruction.to_byte());
    }

    /// Add a 16-bit value to the bytecode (little-endian)
    pub fn add_u16(&mut self, value: u16) {
        self.data.extend_from_slice(&value.to_le_bytes());
    }

    /// Add a 32-bit value to the bytecode (little-endian)
    pub fn add_u32(&mut self, value: u32) {
        self.data.extend_from_slice(&value.to_le_bytes());
    }

    /// Add a 64-bit integer to the bytecode (little-endian)
    pub fn add_i64(&mut self, value: i64) {
        self.data.extend_from_slice(&value.to_le_bytes());
    }

    /// Add a 64-bit float to the bytecode (little-endian)
    pub fn add_f64(&mut self, value: f64) {
        self.data.extend_from_slice(&value.to_le_bytes());
    }

    /// Get a byte at the specified index
    pub fn get_byte(&self, index: usize) -> Result<u8> {
        self.data.get(index).copied().ok_or_else(|| {
            Error::runtime_error(&format!("Bytecode index {} out of bounds", index))
        })
    }

    /// Get an instruction at the specified index
    pub fn get_instruction(&self, index: usize) -> Result<Instruction> {
        let byte = self.get_byte(index)?;
        Instruction::from_byte(byte)
    }

    /// Get a 16-bit value at the specified index (little-endian)
    pub fn get_u16(&self, index: usize) -> Result<u16> {
        if index + 1 >= self.data.len() {
            return Err(Error::runtime_error(&format!("Bytecode index {} out of bounds", index)));
        }
        Ok(u16::from_le_bytes([self.data[index], self.data[index + 1]]))
    }

    /// Get a 64-bit integer at the specified index (little-endian)
    pub fn get_i64(&self, index: usize) -> Result<i64> {
        if index + 7 >= self.data.len() {
            return Err(Error::runtime_error(&format!("Bytecode index {} out of bounds", index)));
        }
        let mut bytes = [0u8; 8];
        bytes.copy_from_slice(&self.data[index..index + 8]);
        Ok(i64::from_le_bytes(bytes))
    }

    /// Get a 64-bit float at the specified index (little-endian)
    pub fn get_f64(&self, index: usize) -> Result<f64> {
        if index + 7 >= self.data.len() {
            return Err(Error::runtime_error(&format!("Bytecode index {} out of bounds", index)));
        }
        let mut bytes = [0u8; 8];
        bytes.copy_from_slice(&self.data[index..index + 8]);
        Ok(f64::from_le_bytes(bytes))
    }

    /// Get the length of the bytecode
    pub fn len(&self) -> usize {
        self.data.len()
    }

    /// Check if the bytecode is empty
    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    /// Get the current position (end of bytecode)
    pub fn current_position(&self) -> usize {
        self.data.len()
    }

    /// Dump the bytecode for debugging
    pub fn dump(&self) -> String {
        let mut result = String::new();
        let mut i = 0;
        while i < self.data.len() {
            let byte = self.data[i];
            if let Ok(instruction) = Instruction::from_byte(byte) {
                result.push_str(&format!("{:04x}: {}", i, instruction));
                match instruction {
                    Instruction::DLoad => {
                        if i + 8 < self.data.len() {
                            let value = self.get_f64(i + 1).unwrap_or(0.0);
                            result.push_str(&format!(" {}", value));
                            i += 9;
                        } else {
                            i += 1;
                        }
                    }
                    Instruction::ILoad => {
                        if i + 8 < self.data.len() {
                            let value = self.get_i64(i + 1).unwrap_or(0);
                            result.push_str(&format!(" {}", value));
                            i += 9;
                        } else {
                            i += 1;
                        }
                    }
                    Instruction::SLoad => {
                        if i + 2 < self.data.len() {
                            let value = self.get_u16(i + 1).unwrap_or(0);
                            result.push_str(&format!(" {}", value));
                            i += 3;
                        } else {
                            i += 1;
                        }
                    }
                    _ => {
                        i += instruction.size();
                    }
                }
            } else {
                result.push_str(&format!("{:04x}: UNKNOWN({})", i, byte));
                i += 1;
            }
            result.push('\n');
        }
        result
    }
}

impl Default for Bytecode {
    fn default() -> Self {
        Self::new()
    }
}