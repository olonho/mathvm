use crate::ast::*;
use crate::error::{Error, Result};
use crate::types::{Signature, VarType};

/// Token types for the MVM language
#[derive(Debug, Clone, PartialEq)]
pub enum TokenType {
    // Literals
    IntLiteral(i64),
    DoubleLiteral(f64),
    StringLiteral(String),

    // Identifiers and keywords
    Identifier(String),

    // Keywords
    Int,
    Double,
    String,
    Void,
    Function,
    Native,
    If,
    Else,
    While,
    For,
    In,
    Return,
    Print,

    // Operators
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Assign,
    AssignAdd,
    AssignSubtract,
    AssignMultiply,
    AssignDivide,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or,
    Not,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,

    // Delimiters
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Comma,
    Semicolon,
    Colon,
    DotDot,

    // Special
    Newline,
    Eof,

    // Error
    Error(String),
}

/// A token with position information
#[derive(Debug, Clone)]
pub struct Token {
    pub token_type: TokenType,
    pub position: usize,
    pub line: usize,
    pub column: usize,
}

/// Lexer for the MVM language
pub struct Lexer {
    input: Vec<char>,
    position: usize,
    line: usize,
    column: usize,
}

impl Lexer {
    pub fn new(input: &str) -> Self {
        Lexer {
            input: input.chars().collect(),
            position: 0,
            line: 1,
            column: 1,
        }
    }

    pub fn tokenize(&mut self) -> Result<Vec<Token>> {
        let mut tokens = Vec::new();

        while !self.is_at_end() {
            let token = self.next_token()?;
            if !matches!(token.token_type, TokenType::Newline) {
                tokens.push(token);
            }
        }

        tokens.push(Token {
            token_type: TokenType::Eof,
            position: self.position,
            line: self.line,
            column: self.column,
        });

        Ok(tokens)
    }

    fn next_token(&mut self) -> Result<Token> {
        self.skip_whitespace();

        let start_pos = self.position;
        let start_line = self.line;
        let start_column = self.column;

        if self.is_at_end() {
            return Ok(Token {
                token_type: TokenType::Eof,
                position: start_pos,
                line: start_line,
                column: start_column,
            });
        }

        let ch = self.advance();

        let token_type = match ch {
            '+' => {
                if self.match_char('=') {
                    TokenType::AssignAdd
                } else {
                    TokenType::Plus
                }
            }
            '-' => {
                if self.match_char('=') {
                    TokenType::AssignSubtract
                } else {
                    TokenType::Minus
                }
            }
            '*' => {
                if self.match_char('=') {
                    TokenType::AssignMultiply
                } else {
                    TokenType::Multiply
                }
            }
            '/' => {
                if self.match_char('/') {
                    // Single line comment
                    while self.peek() != '\n' && !self.is_at_end() {
                        self.advance();
                    }
                    return self.next_token();
                } else if self.match_char('*') {
                    // Multi-line comment
                    while !self.is_at_end() {
                        if self.peek() == '*' && self.peek_next() == '/' {
                            self.advance(); // consume '*'
                            self.advance(); // consume '/'
                            break;
                        }
                        self.advance();
                    }
                    return self.next_token();
                } else if self.match_char('=') {
                    TokenType::AssignDivide
                } else {
                    TokenType::Divide
                }
            }
            '%' => TokenType::Modulo,
            '=' => {
                if self.match_char('=') {
                    TokenType::Equal
                } else {
                    TokenType::Assign
                }
            }
            '!' => {
                if self.match_char('=') {
                    TokenType::NotEqual
                } else {
                    TokenType::Not
                }
            }
            '<' => {
                if self.match_char('=') {
                    TokenType::LessEqual
                } else {
                    TokenType::Less
                }
            }
            '>' => {
                if self.match_char('=') {
                    TokenType::GreaterEqual
                } else {
                    TokenType::Greater
                }
            }
            '&' => {
                if self.match_char('&') {
                    TokenType::And
                } else {
                    TokenType::BitwiseAnd
                }
            }
            '|' => {
                if self.match_char('|') {
                    TokenType::Or
                } else {
                    TokenType::BitwiseOr
                }
            }
            '^' => TokenType::BitwiseXor,
            '(' => TokenType::LeftParen,
            ')' => TokenType::RightParen,
            '{' => TokenType::LeftBrace,
            '}' => TokenType::RightBrace,
            '[' => TokenType::LeftBracket,
            ']' => TokenType::RightBracket,
            ',' => TokenType::Comma,
            ';' => TokenType::Semicolon,
            ':' => TokenType::Colon,
            '.' => {
                if self.match_char('.') {
                    TokenType::DotDot
                } else {
                    return Err(Error::parse_error("Unexpected character '.'", start_pos));
                }
            }
            '\n' => {
                self.line += 1;
                self.column = 1;
                TokenType::Newline
            }
            '\'' => {
                let string_value = self.read_string_literal()?;
                TokenType::StringLiteral(string_value)
            }
            '"' => {
                let string_value = self.read_string_literal_double()?;
                TokenType::StringLiteral(string_value)
            }
            _ if ch.is_ascii_digit() => {
                self.position -= 1; // Back up to re-read the digit
                self.column -= 1;
                self.read_number()?
            }
            _ if ch.is_ascii_alphabetic() || ch == '_' => {
                self.position -= 1; // Back up to re-read the character
                self.column -= 1;
                self.read_identifier_or_keyword()
            }
            _ => {
                return Err(Error::parse_error(
                    &format!("Unexpected character '{ch}'"),
                    start_pos,
                ));
            }
        };

        Ok(Token {
            token_type,
            position: start_pos,
            line: start_line,
            column: start_column,
        })
    }

    fn skip_whitespace(&mut self) {
        while !self.is_at_end() {
            let ch = self.peek();
            if ch.is_whitespace() && ch != '\n' {
                self.advance();
            } else {
                break;
            }
        }
    }

    fn read_string_literal(&mut self) -> Result<String> {
        let mut value = String::new();

        while self.peek() != '\'' && !self.is_at_end() {
            let ch = self.advance();
            if ch == '\\' {
                let escaped = self.advance();
                match escaped {
                    'n' => value.push('\n'),
                    't' => value.push('\t'),
                    'r' => value.push('\r'),
                    '\\' => value.push('\\'),
                    '\'' => value.push('\''),
                    _ => {
                        value.push('\\');
                        value.push(escaped);
                    }
                }
            } else {
                value.push(ch);
            }
        }

        if self.is_at_end() {
            return Err(Error::parse_error(
                "Unterminated string literal",
                self.position,
            ));
        }

        self.advance(); // consume closing quote
        Ok(value)
    }

    fn read_string_literal_double(&mut self) -> Result<String> {
        let mut value = String::new();

        while self.peek() != '"' && !self.is_at_end() {
            let ch = self.advance();
            if ch == '\\' {
                let escaped = self.advance();
                match escaped {
                    'n' => value.push('\n'),
                    't' => value.push('\t'),
                    'r' => value.push('\r'),
                    '\\' => value.push('\\'),
                    '"' => value.push('"'),
                    _ => {
                        value.push('\\');
                        value.push(escaped);
                    }
                }
            } else {
                value.push(ch);
            }
        }

        if self.is_at_end() {
            return Err(Error::parse_error(
                "Unterminated string literal",
                self.position,
            ));
        }

        self.advance(); // consume closing quote
        Ok(value)
    }

    fn read_number(&mut self) -> Result<TokenType> {
        let mut has_dot = false;
        let mut has_e = false;
        let start_pos = self.position;

        // Read integer part
        while self.peek().is_ascii_digit() {
            self.advance();
        }

        // Check for decimal point
        if self.peek() == '.' && self.peek_next().is_ascii_digit() {
            has_dot = true;
            self.advance(); // consume '.'
            while self.peek().is_ascii_digit() {
                self.advance();
            }
        }

        // Check for scientific notation
        if self.peek() == 'e' || self.peek() == 'E' {
            has_e = true;
            self.advance(); // consume 'e'
            if self.peek() == '+' || self.peek() == '-' {
                self.advance(); // consume sign
            }
            if !self.peek().is_ascii_digit() {
                return Err(Error::parse_error("Invalid number format", start_pos));
            }
            while self.peek().is_ascii_digit() {
                self.advance();
            }
        }

        let number_str: String = self.input[start_pos..self.position].iter().collect();

        if has_dot || has_e {
            let value = number_str
                .parse::<f64>()
                .map_err(|_| Error::parse_error("Invalid double literal", start_pos))?;
            Ok(TokenType::DoubleLiteral(value))
        } else {
            let value = number_str
                .parse::<i64>()
                .map_err(|_| Error::parse_error("Invalid integer literal", start_pos))?;
            Ok(TokenType::IntLiteral(value))
        }
    }

    fn read_identifier_or_keyword(&mut self) -> TokenType {
        let start_pos = self.position;

        while self.peek().is_ascii_alphanumeric() || self.peek() == '_' {
            self.advance();
        }

        let identifier: String = self.input[start_pos..self.position].iter().collect();

        match identifier.as_str() {
            "int" => TokenType::Int,
            "double" => TokenType::Double,
            "string" => TokenType::String,
            "void" => TokenType::Void,
            "function" => TokenType::Function,
            "native" => TokenType::Native,
            "if" => TokenType::If,
            "else" => TokenType::Else,
            "while" => TokenType::While,
            "for" => TokenType::For,
            "in" => TokenType::In,
            "return" => TokenType::Return,
            "print" => TokenType::Print,
            _ => TokenType::Identifier(identifier),
        }
    }

    fn is_at_end(&self) -> bool {
        self.position >= self.input.len()
    }

    fn advance(&mut self) -> char {
        if !self.is_at_end() {
            let ch = self.input[self.position];
            self.position += 1;
            self.column += 1;
            ch
        } else {
            '\0'
        }
    }

    fn peek(&self) -> char {
        if self.is_at_end() {
            '\0'
        } else {
            self.input[self.position]
        }
    }

    fn peek_next(&self) -> char {
        if self.position + 1 >= self.input.len() {
            '\0'
        } else {
            self.input[self.position + 1]
        }
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.is_at_end() || self.input[self.position] != expected {
            false
        } else {
            self.position += 1;
            self.column += 1;
            true
        }
    }
}

/// Parser for the MVM language
pub struct Parser {
    tokens: Vec<Token>,
    current: usize,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Parser { tokens, current: 0 }
    }

    /// Parse the tokens into an AST
    pub fn parse(&mut self) -> Result<AstNode> {
        let mut statements = Vec::new();

        while !self.is_at_end() {
            statements.push(self.parse_statement()?);
        }

        Ok(AstNode::program(statements))
    }

    fn parse_statement(&mut self) -> Result<AstNode> {
        match &self.peek().token_type {
            TokenType::Int | TokenType::Double | TokenType::String => self.parse_declaration(),
            TokenType::Function => self.parse_function_declaration(),
            TokenType::If => self.parse_if_statement(),
            TokenType::While => self.parse_while_statement(),
            TokenType::For => self.parse_for_statement(),
            TokenType::Return => self.parse_return_statement(),
            TokenType::Print => self.parse_print_statement(),
            TokenType::LeftBrace => self.parse_block_statement(),
            _ => {
                let expr = self.parse_expression()?;
                self.consume(TokenType::Semicolon, "Expected ';' after expression")?;
                Ok(AstNode::expression(expr))
            }
        }
    }

    fn parse_declaration(&mut self) -> Result<AstNode> {
        let var_type = self.parse_type()?;
        let name = self.consume_identifier("Expected variable name")?;

        let initializer = if self.match_token(&TokenType::Assign) {
            Some(self.parse_expression()?)
        } else {
            None
        };

        self.consume(
            TokenType::Semicolon,
            "Expected ';' after variable declaration",
        )?;
        Ok(AstNode::var_declaration(var_type, name, initializer))
    }

    fn parse_function_declaration(&mut self) -> Result<AstNode> {
        self.consume(TokenType::Function, "Expected 'function'")?;
        let return_type = self.parse_type()?;
        let name = self.consume_identifier("Expected function name")?;

        self.consume(TokenType::LeftParen, "Expected '(' after function name")?;

        let mut signature = Signature::new(return_type);

        if !self.check(&TokenType::RightParen) {
            loop {
                let param_type = self.parse_type()?;
                let param_name = self.consume_identifier("Expected parameter name")?;
                signature = signature.with_parameter(param_name, param_type);

                if !self.match_token(&TokenType::Comma) {
                    break;
                }
            }
        }

        self.consume(TokenType::RightParen, "Expected ')' after parameters")?;

        // Check if this is a native function
        if self.match_token(&TokenType::Native) {
            // Parse native function declaration: native 'name';
            let native_name = if let TokenType::StringLiteral(name) = &self.peek().token_type {
                let name = name.clone();
                self.advance();
                name
            } else {
                return Err(Error::parse_error(
                    "Expected string literal after 'native'",
                    self.current,
                ));
            };

            self.consume(
                TokenType::Semicolon,
                "Expected ';' after native function declaration",
            )?;
            Ok(AstNode::native_function_declaration(
                name,
                signature,
                native_name,
            ))
        } else {
            // Regular function with body
            let body = self.parse_block_statement()?;
            Ok(AstNode::function_declaration(name, signature, body))
        }
    }

    fn parse_if_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::If, "Expected 'if'")?;
        self.consume(TokenType::LeftParen, "Expected '(' after 'if'")?;
        let condition = self.parse_expression()?;
        self.consume(TokenType::RightParen, "Expected ')' after if condition")?;

        let then_branch = self.parse_statement()?;
        let else_branch = if self.match_token(&TokenType::Else) {
            Some(self.parse_statement()?)
        } else {
            None
        };

        Ok(AstNode::if_statement(condition, then_branch, else_branch))
    }

    fn parse_while_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::While, "Expected 'while'")?;
        self.consume(TokenType::LeftParen, "Expected '(' after 'while'")?;
        let condition = self.parse_expression()?;
        self.consume(TokenType::RightParen, "Expected ')' after while condition")?;

        let body = self.parse_statement()?;
        Ok(AstNode::while_loop(condition, body))
    }

    fn parse_for_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::For, "Expected 'for'")?;
        self.consume(TokenType::LeftParen, "Expected '(' after 'for'")?;
        let variable = self.consume_identifier("Expected variable name in for loop")?;
        self.consume(TokenType::In, "Expected 'in' after for variable")?;

        let start = self.parse_expression()?;
        self.consume(TokenType::DotDot, "Expected '..' in for range")?;
        let end = self.parse_expression()?;

        self.consume(TokenType::RightParen, "Expected ')' after for range")?;

        let body = self.parse_statement()?;
        Ok(AstNode::for_loop(variable, start, end, body))
    }

    fn parse_return_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::Return, "Expected 'return'")?;

        let value = if self.check(&TokenType::Semicolon) {
            None
        } else {
            Some(self.parse_expression()?)
        };

        self.consume(TokenType::Semicolon, "Expected ';' after return statement")?;
        Ok(AstNode::return_statement(value))
    }

    fn parse_print_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::Print, "Expected 'print'")?;
        self.consume(TokenType::LeftParen, "Expected '(' after 'print'")?;

        let mut arguments = Vec::new();

        if !self.check(&TokenType::RightParen) {
            loop {
                arguments.push(self.parse_expression()?);
                if !self.match_token(&TokenType::Comma) {
                    break;
                }
            }
        }

        self.consume(TokenType::RightParen, "Expected ')' after print arguments")?;
        self.consume(TokenType::Semicolon, "Expected ';' after print statement")?;

        Ok(AstNode::print(arguments))
    }

    fn parse_block_statement(&mut self) -> Result<AstNode> {
        self.consume(TokenType::LeftBrace, "Expected '{'")?;

        let mut statements = Vec::new();

        while !self.check(&TokenType::RightBrace) && !self.is_at_end() {
            statements.push(self.parse_statement()?);
        }

        self.consume(TokenType::RightBrace, "Expected '}'")?;
        Ok(AstNode::block(statements))
    }

    fn parse_expression(&mut self) -> Result<AstNode> {
        self.parse_assignment()
    }

    fn parse_assignment(&mut self) -> Result<AstNode> {
        let expr = self.parse_logical_or()?;

        if let Ok(op) = self.match_assignment_operator() {
            let value = self.parse_assignment()?;
            return Ok(AstNode::binary_op(expr, op, value));
        }

        Ok(expr)
    }

    fn match_assignment_operator(&mut self) -> Result<BinaryOp> {
        match &self.peek().token_type {
            TokenType::Assign => {
                self.advance();
                Ok(BinaryOp::Assign)
            }
            TokenType::AssignAdd => {
                self.advance();
                Ok(BinaryOp::AssignAdd)
            }
            TokenType::AssignSubtract => {
                self.advance();
                Ok(BinaryOp::AssignSubtract)
            }
            TokenType::AssignMultiply => {
                self.advance();
                Ok(BinaryOp::AssignMultiply)
            }
            TokenType::AssignDivide => {
                self.advance();
                Ok(BinaryOp::AssignDivide)
            }
            _ => Err(Error::parse_error(
                "Not an assignment operator",
                self.current,
            )),
        }
    }

    fn parse_logical_or(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_logical_and()?;

        while self.match_token(&TokenType::Or) {
            let right = self.parse_logical_and()?;
            expr = AstNode::binary_op(expr, BinaryOp::Or, right);
        }

        Ok(expr)
    }

    fn parse_logical_and(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_bitwise_or()?;

        while self.match_token(&TokenType::And) {
            let right = self.parse_bitwise_or()?;
            expr = AstNode::binary_op(expr, BinaryOp::And, right);
        }

        Ok(expr)
    }

    fn parse_bitwise_or(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_bitwise_xor()?;

        while self.match_token(&TokenType::BitwiseOr) {
            let right = self.parse_bitwise_xor()?;
            expr = AstNode::binary_op(expr, BinaryOp::BitwiseOr, right);
        }

        Ok(expr)
    }

    fn parse_bitwise_xor(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_bitwise_and()?;

        while self.match_token(&TokenType::BitwiseXor) {
            let right = self.parse_bitwise_and()?;
            expr = AstNode::binary_op(expr, BinaryOp::BitwiseXor, right);
        }

        Ok(expr)
    }

    fn parse_bitwise_and(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_equality()?;

        while self.match_token(&TokenType::BitwiseAnd) {
            let right = self.parse_equality()?;
            expr = AstNode::binary_op(expr, BinaryOp::BitwiseAnd, right);
        }

        Ok(expr)
    }

    fn parse_equality(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_comparison()?;

        while let Some(op) = self.match_equality_operator() {
            let right = self.parse_comparison()?;
            expr = AstNode::binary_op(expr, op, right);
        }

        Ok(expr)
    }

    fn match_equality_operator(&mut self) -> Option<BinaryOp> {
        match &self.peek().token_type {
            TokenType::Equal => {
                self.advance();
                Some(BinaryOp::Equal)
            }
            TokenType::NotEqual => {
                self.advance();
                Some(BinaryOp::NotEqual)
            }
            _ => None,
        }
    }

    fn parse_comparison(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_term()?;

        while let Some(op) = self.match_comparison_operator() {
            let right = self.parse_term()?;
            expr = AstNode::binary_op(expr, op, right);
        }

        Ok(expr)
    }

    fn match_comparison_operator(&mut self) -> Option<BinaryOp> {
        match &self.peek().token_type {
            TokenType::Greater => {
                self.advance();
                Some(BinaryOp::Greater)
            }
            TokenType::GreaterEqual => {
                self.advance();
                Some(BinaryOp::GreaterEqual)
            }
            TokenType::Less => {
                self.advance();
                Some(BinaryOp::Less)
            }
            TokenType::LessEqual => {
                self.advance();
                Some(BinaryOp::LessEqual)
            }
            _ => None,
        }
    }

    fn parse_term(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_factor()?;

        while let Some(op) = self.match_term_operator() {
            let right = self.parse_factor()?;
            expr = AstNode::binary_op(expr, op, right);
        }

        Ok(expr)
    }

    fn match_term_operator(&mut self) -> Option<BinaryOp> {
        match &self.peek().token_type {
            TokenType::Minus => {
                self.advance();
                Some(BinaryOp::Subtract)
            }
            TokenType::Plus => {
                self.advance();
                Some(BinaryOp::Add)
            }
            _ => None,
        }
    }

    fn parse_factor(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_unary()?;

        while let Some(op) = self.match_factor_operator() {
            let right = self.parse_unary()?;
            expr = AstNode::binary_op(expr, op, right);
        }

        Ok(expr)
    }

    fn match_factor_operator(&mut self) -> Option<BinaryOp> {
        match &self.peek().token_type {
            TokenType::Divide => {
                self.advance();
                Some(BinaryOp::Divide)
            }
            TokenType::Multiply => {
                self.advance();
                Some(BinaryOp::Multiply)
            }
            TokenType::Modulo => {
                self.advance();
                Some(BinaryOp::Modulo)
            }
            _ => None,
        }
    }

    fn parse_unary(&mut self) -> Result<AstNode> {
        if let Some(op) = self.match_unary_operator() {
            let expr = self.parse_unary()?;
            Ok(AstNode::unary_op(op, expr))
        } else {
            self.parse_call()
        }
    }

    fn match_unary_operator(&mut self) -> Option<UnaryOp> {
        match &self.peek().token_type {
            TokenType::Not => {
                self.advance();
                Some(UnaryOp::Not)
            }
            TokenType::Minus => {
                self.advance();
                Some(UnaryOp::Minus)
            }
            TokenType::Plus => {
                self.advance();
                Some(UnaryOp::Plus)
            }
            _ => None,
        }
    }

    fn parse_call(&mut self) -> Result<AstNode> {
        let mut expr = self.parse_primary()?;

        while self.match_token(&TokenType::LeftParen) {
            let mut arguments = Vec::new();

            if !self.check(&TokenType::RightParen) {
                loop {
                    arguments.push(self.parse_expression()?);
                    if !self.match_token(&TokenType::Comma) {
                        break;
                    }
                }
            }

            self.consume(TokenType::RightParen, "Expected ')' after arguments")?;

            if let AstNode::Variable(name) = expr {
                expr = AstNode::function_call(name, arguments);
            } else {
                return Err(Error::parse_error("Invalid function call", self.current));
            }
        }

        Ok(expr)
    }

    fn parse_primary(&mut self) -> Result<AstNode> {
        match &self.peek().token_type {
            TokenType::IntLiteral(value) => {
                let value = *value;
                self.advance();
                Ok(AstNode::int_literal(value))
            }
            TokenType::DoubleLiteral(value) => {
                let value = *value;
                self.advance();
                Ok(AstNode::double_literal(value))
            }
            TokenType::StringLiteral(value) => {
                let value = value.clone();
                self.advance();
                Ok(AstNode::string_literal(value))
            }
            TokenType::Identifier(name) => {
                let name = name.clone();
                self.advance();
                Ok(AstNode::variable(name))
            }
            TokenType::LeftParen => {
                self.advance();
                let expr = self.parse_expression()?;
                self.consume(TokenType::RightParen, "Expected ')' after expression")?;
                Ok(expr)
            }
            _ => Err(Error::parse_error("Expected expression", self.current)),
        }
    }

    fn parse_type(&mut self) -> Result<VarType> {
        match &self.peek().token_type {
            TokenType::Int => {
                self.advance();
                Ok(VarType::Int)
            }
            TokenType::Double => {
                self.advance();
                Ok(VarType::Double)
            }
            TokenType::String => {
                self.advance();
                Ok(VarType::String)
            }
            TokenType::Void => {
                self.advance();
                Ok(VarType::Void)
            }
            _ => Err(Error::parse_error("Expected type", self.current)),
        }
    }

    fn consume_identifier(&mut self, message: &str) -> Result<String> {
        match &self.peek().token_type {
            TokenType::Identifier(name) => {
                let name = name.clone();
                self.advance();
                Ok(name)
            }
            _ => Err(Error::parse_error(message, self.current)),
        }
    }

    fn consume(&mut self, token_type: TokenType, message: &str) -> Result<()> {
        if self.check(&token_type) {
            self.advance();
            Ok(())
        } else {
            Err(Error::parse_error(message, self.current))
        }
    }

    fn match_token(&mut self, token_type: &TokenType) -> bool {
        if self.check(token_type) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn check(&self, token_type: &TokenType) -> bool {
        if self.is_at_end() {
            false
        } else {
            std::mem::discriminant(&self.peek().token_type) == std::mem::discriminant(token_type)
        }
    }

    fn advance(&mut self) -> &Token {
        if !self.is_at_end() {
            self.current += 1;
        }
        self.previous()
    }

    fn is_at_end(&self) -> bool {
        matches!(self.peek().token_type, TokenType::Eof)
    }

    fn peek(&self) -> &Token {
        &self.tokens[self.current]
    }

    fn previous(&self) -> &Token {
        &self.tokens[self.current - 1]
    }
}

/// Parse MVM source code into an AST
pub fn parse(source: &str) -> Result<AstNode> {
    let mut lexer = Lexer::new(source);
    let tokens = lexer.tokenize()?;
    let mut parser = Parser::new(tokens);
    parser.parse()
}
