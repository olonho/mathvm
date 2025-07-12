use thiserror::Error;

/// Result type used throughout the MathVM codebase
pub type Result<T> = std::result::Result<T, Error>;

/// Error types that can occur during parsing, translation, or execution
#[derive(Error, Debug)]
pub enum Error {
    #[error("Parse error at position {position}: {message}")]
    ParseError { message: String, position: usize },

    #[error("Type error: {0}")]
    TypeError(String),

    #[error("Runtime error: {0}")]
    RuntimeError(String),

    #[error("Undefined variable: {0}")]
    UndefinedVariable(String),

    #[error("Undefined function: {0}")]
    UndefinedFunction(String),

    #[error("Invalid bytecode instruction: {0}")]
    InvalidBytecode(u8),

    #[error("Stack underflow")]
    StackUnderflow,

    #[error("Stack overflow")]
    StackOverflow,

    #[error("Division by zero")]
    DivisionByZero,

    #[error("Invalid cast from {from} to {to}")]
    InvalidCast { from: String, to: String },

    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("Internal error: {0}")]
    InternalError(String),

    #[error("Return: {0:?}")]
    Return(crate::types::Value),
}

impl Error {
    pub fn parse_error(message: &str, position: usize) -> Self {
        Error::ParseError {
            message: message.to_string(),
            position,
        }
    }

    pub fn type_error(message: &str) -> Self {
        Error::TypeError(message.to_string())
    }

    pub fn runtime_error(message: &str) -> Self {
        Error::RuntimeError(message.to_string())
    }

    pub fn position(&self) -> Option<usize> {
        match self {
            Error::ParseError { position, .. } => Some(*position),
            _ => None,
        }
    }
}
