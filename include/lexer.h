#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace pyplusplus {

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    IDENTIFIER,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    POWER, FLOOR_DIVIDE,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN,
    EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    
    // Delimiters
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACKET, RIGHT_BRACKET,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, COLON, DOT, SEMICOLON,
    
    // Keywords
    AND, OR, NOT, IF, ELIF, ELSE,
    FOR, WHILE, BREAK, CONTINUE,
    DEF, RETURN, CLASS, PASS,
    IMPORT, FROM, AS, IN, IS,
    LAMBDA, TRY, EXCEPT, FINALLY, RAISE,
    WITH, YIELD, GLOBAL, NONLOCAL,
    ASSERT, DEL, PASS,
    
    // Async/await
    ASYNC, AWAIT,
    
    // Pattern matching
    MATCH, CASE,
    
    // Type hints
    TYPE, VAR, FINAL, OVERRIDE,
    
    // Special
    NEWLINE, INDENT, DEDENT,
    EOF_TOKEN,
    
    // Comparison operators
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT,
    LEFT_SHIFT, RIGHT_SHIFT,
    AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN, LEFT_SHIFT_ASSIGN, RIGHT_SHIFT_ASSIGN,
    
    // Arrow
    ARROW,
    
    // Special literals
    TRUE, FALSE, NONE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType type, const std::string& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
    
    std::string toString() const;
};

class Lexer {
private:
    std::string source;
    size_t position;
    int line;
    int column;
    std::vector<int> indent_stack;
    bool at_start_of_line;
    
    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    bool match(char expected);
    void skipWhitespace();
    void skipComment();
    Token scanNumber();
    Token scanString(char quote);
    Token scanIdentifier();
    Token scanOperator();
    
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();
    
private:
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
};

} // namespace pyplusplus