#pragma once
#include "ast.h"
#include "lexer.h"
#include <vector>
#include <memory>
#include <stdexcept>

namespace pyplusplus {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    
    // Helper methods
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    // Parsing methods
    std::unique_ptr<Module> parseModule();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Statement> parseIfStatement();
    std::unique_ptr<Statement> parseWhileStatement();
    std::unique_ptr<Statement> parseForStatement();
    std::unique_ptr<Statement> parseFunctionDef();
    std::unique_ptr<Statement> parseReturnStatement();
    std::unique_ptr<Statement> parseBreakStatement();
    std::unique_ptr<Statement> parseContinueStatement();
    std::unique_ptr<Statement> parsePassStatement();
    std::unique_ptr<Statement> parseExpressionStatement();
    std::unique_ptr<Statement> parseAssignmentStatement();
    
    std::vector<std::unique_ptr<Statement>> parseBlock();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseAssignment();
    std::unique_ptr<Expression> parseOr();
    std::unique_ptr<Expression> parseAnd();
    std::unique_ptr<Expression> parseNot();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseBitwiseOr();
    std::unique_ptr<Expression> parseBitwiseXor();
    std::unique_ptr<Expression> parseBitwiseAnd();
    std::unique_ptr<Expression> parseShift();
    std::unique_ptr<Expression> parseAddition();
    std::unique_ptr<Expression> parseMultiplication();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePower();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parseCall(std::unique_ptr<Expression> callee);
    std::unique_ptr<Expression> parseAttribute(std::unique_ptr<Expression> object);
    std::unique_ptr<Expression> parseSubscript(std::unique_ptr<Expression> object);
    std::unique_ptr<Expression> parseList();
    std::unique_ptr<Expression> parseDict();
    std::unique_ptr<Expression> parseLambda();
    
    std::vector<std::unique_ptr<Expression>> parseArgumentList();
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parseParameterList();
    std::unique_ptr<Expression> parseTypeAnnotation();
    
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}
    
    std::unique_ptr<Module> parse() {
        try {
            return parseModule();
        } catch (const std::runtime_error& e) {
            // TODO: Better error handling
            throw;
        }
    }
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, int line, int column)
        : std::runtime_error(message + " at line " + std::to_string(line) + ", column " + std::to_string(column)) {}
};

} // namespace pyplusplus