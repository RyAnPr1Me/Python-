#include "lexer.h"
#include <stdexcept>
#include <cctype>
#include <unordered_map>

namespace pyplusplus {

std::string Token::toString() const {
    std::string typeStr;
    switch (type) {
        case TokenType::INTEGER: typeStr = "INTEGER"; break;
        case TokenType::FLOAT: typeStr = "FLOAT"; break;
        case TokenType::STRING: typeStr = "STRING"; break;
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::PLUS: typeStr = "PLUS"; break;
        case TokenType::MINUS: typeStr = "MINUS"; break;
        case TokenType::MULTIPLY: typeStr = "MULTIPLY"; break;
        case TokenType::DIVIDE: typeStr = "DIVIDE"; break;
        case TokenType::MODULO: typeStr = "MODULO"; break;
        case TokenType::POWER: typeStr = "POWER"; break;
        case TokenType::FLOOR_DIVIDE: typeStr = "FLOOR_DIVIDE"; break;
        case TokenType::ASSIGN: typeStr = "ASSIGN"; break;
        case TokenType::EQUAL: typeStr = "EQUAL"; break;
        case TokenType::NOT_EQUAL: typeStr = "NOT_EQUAL"; break;
        case TokenType::LESS: typeStr = "LESS"; break;
        case TokenType::GREATER: typeStr = "GREATER"; break;
        case TokenType::LEFT_PAREN: typeStr = "LEFT_PAREN"; break;
        case TokenType::RIGHT_PAREN: typeStr = "RIGHT_PAREN"; break;
        case TokenType::LEFT_BRACKET: typeStr = "LEFT_BRACKET"; break;
        case TokenType::RIGHT_BRACKET: typeStr = "RIGHT_BRACKET"; break;
        case TokenType::LEFT_BRACE: typeStr = "LEFT_BRACE"; break;
        case TokenType::RIGHT_BRACE: typeStr = "RIGHT_BRACE"; break;
        case TokenType::COMMA: typeStr = "COMMA"; break;
        case TokenType::COLON: typeStr = "COLON"; break;
        case TokenType::DOT: typeStr = "DOT"; break;
        case TokenType::NEWLINE: typeStr = "NEWLINE"; break;
        case TokenType::INDENT: typeStr = "INDENT"; break;
        case TokenType::DEDENT: typeStr = "DEDENT"; break;
        case TokenType::EOF_TOKEN: typeStr = "EOF"; break;
        default: typeStr = "UNKNOWN"; break;
    }
    return typeStr + "(" + value + ")";
}

Lexer::Lexer(const std::string& source) 
    : source(source), position(0), line(1), column(1), at_start_of_line(true) {
    indent_stack.push_back(0);
}

char Lexer::current() const {
    return position < source.length() ? source[position] : '\0';
}

char Lexer::peek(size_t offset) const {
    size_t pos = position + offset;
    return pos < source.length() ? source[pos] : '\0';
}

void Lexer::advance() {
    if (position < source.length()) {
        if (current() == '\n') {
            line++;
            column = 1;
            at_start_of_line = true;
        } else {
            column++;
            at_start_of_line = false;
        }
        position++;
    }
}

bool Lexer::match(char expected) {
    if (current() == expected) {
        advance();
        return true;
    }
    return false;
}

bool Lexer::isAtEnd() const {
    return position >= source.length();
}

bool Lexer::isDigit(char c) const {
    return std::isdigit(c);
}

bool Lexer::isAlpha(char c) const {
    return std::isalpha(c) || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(current()) && current() != '\n') {
        advance();
    }
}

void Lexer::skipComment() {
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

Token Lexer::scanNumber() {
    size_t start = position;
    bool isFloat = false;
    
    while (!isAtEnd() && (isDigit(current()) || current() == '.')) {
        if (current() == '.') {
            if (isFloat) break; // Second dot, stop
            isFloat = true;
        }
        advance();
    }
    
    // Check for exponent
    if (!isAtEnd() && (current() == 'e' || current() == 'E')) {
        isFloat = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        while (!isAtEnd() && isDigit(current())) {
            advance();
        }
    }
    
    std::string value = source.substr(start, position - start);
    return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, value, line, column);
}

Token Lexer::scanString(char quote) {
    advance(); // Skip opening quote
    size_t start = position;
    
    while (!isAtEnd() && current() != quote) {
        if (current() == '\\') {
            advance(); // Skip escape character
            if (!isAtEnd()) advance(); // Skip escaped character
        } else {
            advance();
        }
    }
    
    if (isAtEnd()) {
        throw std::runtime_error("Unterminated string at line " + std::to_string(line));
    }
    
    std::string value = source.substr(start, position - start);
    advance(); // Skip closing quote
    
    return Token(TokenType::STRING, value, line, column);
}

Token Lexer::scanIdentifier() {
    size_t start = position;
    
    while (!isAtEnd() && isAlphaNumeric(current())) {
        advance();
    }
    
    std::string value = source.substr(start, position - start);
    
    // Check for keywords
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"and", TokenType::AND}, {"or", TokenType::OR}, {"not", TokenType::NOT},
        {"if", TokenType::IF}, {"elif", TokenType::ELIF}, {"else", TokenType::ELSE},
        {"for", TokenType::FOR}, {"while", TokenType::WHILE}, {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE}, {"def", TokenType::DEF}, {"return", TokenType::RETURN},
        {"class", TokenType::CLASS}, {"pass", TokenType::PASS}, {"import", TokenType::IMPORT},
        {"from", TokenType::FROM}, {"as", TokenType::AS}, {"in", TokenType::IN},
        {"is", TokenType::IS}, {"lambda", TokenType::LAMBDA}, {"try", TokenType::TRY},
        {"except", TokenType::EXCEPT}, {"finally", TokenType::FINALLY}, {"raise", TokenType::RAISE},
        {"with", TokenType::WITH}, {"yield", TokenType::YIELD}, {"global", TokenType::GLOBAL},
        {"nonlocal", TokenType::NONLOCAL}, {"assert", TokenType::ASSERT}, {"del", TokenType::DEL},
        {"True", TokenType::TRUE}, {"False", TokenType::FALSE}, {"None", TokenType::NONE}
    };
    
    auto it = keywords.find(value);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    
    return Token(type, value, line, column);
}

Token Lexer::scanOperator() {
    char c = current();
    int startColumn = column;
    
    switch (c) {
        case '+':
            advance();
            if (match('=')) return Token(TokenType::PLUS_ASSIGN, "+=", line, startColumn);
            return Token(TokenType::PLUS, "+", line, startColumn);
            
        case '-':
            advance();
            if (match('=')) return Token(TokenType::MINUS_ASSIGN, "-=", line, startColumn);
            if (match('>')) return Token(TokenType::ARROW, "->", line, startColumn);
            return Token(TokenType::MINUS, "-", line, startColumn);
            
        case '*':
            advance();
            if (match('=')) return Token(TokenType::MULTIPLY_ASSIGN, "*=", line, startColumn);
            if (match('*')) return Token(TokenType::POWER, "**", line, startColumn);
            return Token(TokenType::MULTIPLY, "*", line, startColumn);
            
        case '/':
            advance();
            if (match('=')) return Token(TokenType::DIVIDE_ASSIGN, "/=", line, startColumn);
            if (match('/')) return Token(TokenType::FLOOR_DIVIDE, "//", line, startColumn);
            return Token(TokenType::DIVIDE, "/", line, startColumn);
            
        case '%':
            advance();
            return Token(TokenType::MODULO, "%", line, startColumn);
            
        case '=':
            advance();
            if (match('=')) return Token(TokenType::EQUAL, "==", line, startColumn);
            return Token(TokenType::ASSIGN, "=", line, startColumn);
            
        case '!':
            advance();
            if (match('=')) return Token(TokenType::NOT_EQUAL, "!=", line, startColumn);
            break;
            
        case '<':
            advance();
            if (match('=')) return Token(TokenType::LESS_EQUAL, "<=", line, startColumn);
            if (match('<')) {
                if (match('=')) return Token(TokenType::LEFT_SHIFT_ASSIGN, "<<=", line, startColumn);
                return Token(TokenType::LEFT_SHIFT, "<<", line, startColumn);
            }
            return Token(TokenType::LESS, "<", line, startColumn);
            
        case '>':
            advance();
            if (match('=')) return Token(TokenType::GREATER_EQUAL, ">=", line, startColumn);
            if (match('>')) {
                if (match('=')) return Token(TokenType::RIGHT_SHIFT_ASSIGN, ">>=", line, startColumn);
                return Token(TokenType::RIGHT_SHIFT, ">>", line, startColumn);
            }
            return Token(TokenType::GREATER, ">", line, startColumn);
            
        case '&':
            advance();
            if (match('=')) return Token(TokenType::AND_ASSIGN, "&=", line, startColumn);
            return Token(TokenType::BIT_AND, "&", line, startColumn);
            
        case '|':
            advance();
            if (match('=')) return Token(TokenType::OR_ASSIGN, "|=", line, startColumn);
            return Token(TokenType::BIT_OR, "|", line, startColumn);
            
        case '^':
            advance();
            if (match('=')) return Token(TokenType::XOR_ASSIGN, "^=", line, startColumn);
            return Token(TokenType::BIT_XOR, "^", line, startColumn);
            
        case '~':
            advance();
            return Token(TokenType::BIT_NOT, "~", line, startColumn);
            
        case '(':
            advance();
            return Token(TokenType::LEFT_PAREN, "(", line, startColumn);
            
        case ')':
            advance();
            return Token(TokenType::RIGHT_PAREN, ")", line, startColumn);
            
        case '[':
            advance();
            return Token(TokenType::LEFT_BRACKET, "[", line, startColumn);
            
        case ']':
            advance();
            return Token(TokenType::RIGHT_BRACKET, "]", line, startColumn);
            
        case '{':
            advance();
            return Token(TokenType::LEFT_BRACE, "{", line, startColumn);
            
        case '}':
            advance();
            return Token(TokenType::RIGHT_BRACE, "}", line, startColumn);
            
        case ',':
            advance();
            return Token(TokenType::COMMA, ",", line, startColumn);
            
        case ':':
            advance();
            return Token(TokenType::COLON, ":", line, startColumn);
            
        case '.':
            advance();
            return Token(TokenType::DOT, ".", line, startColumn);
            
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";", line, startColumn);
    }
    
    throw std::runtime_error("Unexpected character: " + std::string(1, c) + " at line " + std::to_string(line));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        // Handle indentation at start of line
        if (at_start_of_line) {
            int indent_level = 0;
            while (!isAtEnd() && current() == ' ') {
                indent_level++;
                advance();
            }
            
            if (isAtEnd() || current() == '\n') {
                // Empty line or whitespace only, skip
                if (!isAtEnd()) {
                    advance(); // Skip newline
                    tokens.push_back(Token(TokenType::NEWLINE, "\\n", line, column));
                }
                continue;
            }
            
            if (current() == '#') {
                skipComment();
                continue;
            }
            
            int current_indent = indent_stack.back();
            if (indent_level > current_indent) {
                indent_stack.push_back(indent_level);
                tokens.push_back(Token(TokenType::INDENT, "", line, column));
            } else if (indent_level < current_indent) {
                while (indent_stack.back() > indent_level) {
                    indent_stack.pop_back();
                    tokens.push_back(Token(TokenType::DEDENT, "", line, column));
                }
                if (indent_stack.back() != indent_level) {
                    throw std::runtime_error("Inconsistent indentation at line " + std::to_string(line));
                }
            }
            at_start_of_line = false;
        }
        
        char c = current();
        
        if (c == '\n') {
            tokens.push_back(Token(TokenType::NEWLINE, "\\n", line, column));
            advance();
            at_start_of_line = true;
        } else if (std::isspace(c)) {
            skipWhitespace();
        } else if (c == '#') {
            skipComment();
        } else if (isDigit(c)) {
            tokens.push_back(scanNumber());
        } else if (c == '"' || c == '\'') {
            tokens.push_back(scanString(c));
        } else if (isAlpha(c)) {
            tokens.push_back(scanIdentifier());
        } else {
            tokens.push_back(scanOperator());
        }
    }
    
    // Add remaining DEDENT tokens
    while (indent_stack.size() > 1) {
        indent_stack.pop_back();
        tokens.push_back(Token(TokenType::DEDENT, "", line, column));
    }
    
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
    return tokens;
}

} // namespace pyplusplus