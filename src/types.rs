use crate::error::{Error, Result};
use std::fmt;

/// Variable types supported by the MVM language
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum VarType {
    /// No type (void)
    Void,
    /// 64-bit signed integer
    Int,
    /// IEEE 754 double precision floating point
    Double,
    /// Immutable string
    String,
}

impl VarType {
    /// Convert a type name string to VarType
    pub fn from_name(name: &str) -> Result<Self> {
        match name {
            "void" => Ok(VarType::Void),
            "int" => Ok(VarType::Int),
            "double" => Ok(VarType::Double),
            "string" => Ok(VarType::String),
            _ => Err(Error::type_error(&format!("Unknown type: {}", name))),
        }
    }

    /// Get the string representation of this type
    pub fn name(&self) -> &'static str {
        match self {
            VarType::Void => "void",
            VarType::Int => "int",
            VarType::Double => "double",
            VarType::String => "string",
        }
    }

    /// Check if this type can be converted to another type
    pub fn can_convert_to(&self, other: VarType) -> bool {
        match (self, other) {
            // Same type
            (a, b) if a == &b => true,
            // Integer to double
            (VarType::Int, VarType::Double) => true,
            // Double to integer (with potential precision loss)
            (VarType::Double, VarType::Int) => true,
            // String to integer (parsing)
            (VarType::String, VarType::Int) => true,
            // String to double (parsing)
            (VarType::String, VarType::Double) => true,
            // Other conversions are not supported
            _ => false,
        }
    }
}

impl fmt::Display for VarType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.name())
    }
}

/// A value in the MVM runtime
#[derive(Debug, Clone)]
pub enum Value {
    Void,
    Int(i64),
    Double(f64),
    String(String),
}

impl Value {
    /// Get the type of this value
    pub fn get_type(&self) -> VarType {
        match self {
            Value::Void => VarType::Void,
            Value::Int(_) => VarType::Int,
            Value::Double(_) => VarType::Double,
            Value::String(_) => VarType::String,
        }
    }

    /// Convert this value to another type
    pub fn convert_to(&self, target_type: VarType) -> Result<Value> {
        match (self, target_type) {
            // Same type
            (val, t) if val.get_type() == t => Ok(val.clone()),
            
            // Integer to double
            (Value::Int(i), VarType::Double) => Ok(Value::Double(*i as f64)),
            
            // Double to integer (truncate)
            (Value::Double(d), VarType::Int) => Ok(Value::Int(*d as i64)),
            
            // String to integer
            (Value::String(s), VarType::Int) => {
                s.parse::<i64>()
                    .map(Value::Int)
                    .map_err(|_| Error::InvalidCast {
                        from: "string".to_string(),
                        to: "int".to_string(),
                    })
            }
            
            // String to double
            (Value::String(s), VarType::Double) => {
                s.parse::<f64>()
                    .map(Value::Double)
                    .map_err(|_| Error::InvalidCast {
                        from: "string".to_string(),
                        to: "double".to_string(),
                    })
            }
            
            // Other conversions are not supported
            _ => Err(Error::InvalidCast {
                from: self.get_type().name().to_string(),
                to: target_type.name().to_string(),
            }),
        }
    }

    /// Extract integer value, converting if necessary
    pub fn as_int(&self) -> Result<i64> {
        match self {
            Value::Int(i) => Ok(*i),
            Value::Double(d) => Ok(*d as i64),
            Value::String(s) => s.parse::<i64>().map_err(|_| Error::InvalidCast {
                from: "string".to_string(),
                to: "int".to_string(),
            }),
            Value::Void => Err(Error::InvalidCast {
                from: "void".to_string(),
                to: "int".to_string(),
            }),
        }
    }

    /// Extract double value, converting if necessary
    pub fn as_double(&self) -> Result<f64> {
        match self {
            Value::Double(d) => Ok(*d),
            Value::Int(i) => Ok(*i as f64),
            Value::String(s) => s.parse::<f64>().map_err(|_| Error::InvalidCast {
                from: "string".to_string(),
                to: "double".to_string(),
            }),
            Value::Void => Err(Error::InvalidCast {
                from: "void".to_string(),
                to: "double".to_string(),
            }),
        }
    }

    /// Extract string value, converting if necessary
    pub fn as_string(&self) -> Result<String> {
        match self {
            Value::String(s) => Ok(s.clone()),
            Value::Int(i) => Ok(i.to_string()),
            Value::Double(d) => Ok(format!("{}", d)),
            Value::Void => Err(Error::InvalidCast {
                from: "void".to_string(),
                to: "string".to_string(),
            }),
        }
    }

    /// Check if this value is truthy (for conditional expressions)
    pub fn is_truthy(&self) -> bool {
        match self {
            Value::Void => false,
            Value::Int(i) => *i != 0,
            Value::Double(d) => *d != 0.0,
            Value::String(s) => !s.is_empty(),
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Void => write!(f, "void"),
            Value::Int(i) => write!(f, "{}", i),
            Value::Double(d) => write!(f, "{}", d),
            Value::String(s) => write!(f, "{}", s),
        }
    }
}

/// Function signature - return type and parameter types
#[derive(Debug, Clone)]
pub struct Signature {
    pub return_type: VarType,
    pub parameters: Vec<(String, VarType)>,
}

impl Signature {
    pub fn new(return_type: VarType) -> Self {
        Signature {
            return_type,
            parameters: Vec::new(),
        }
    }

    pub fn with_parameter(mut self, name: String, param_type: VarType) -> Self {
        self.parameters.push((name, param_type));
        self
    }

    pub fn parameter_count(&self) -> usize {
        self.parameters.len()
    }

    pub fn parameter_type(&self, index: usize) -> Option<VarType> {
        self.parameters.get(index).map(|(_, t)| *t)
    }

    pub fn parameter_name(&self, index: usize) -> Option<&str> {
        self.parameters.get(index).map(|(n, _)| n.as_str())
    }
}