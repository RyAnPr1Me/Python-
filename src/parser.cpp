#include "parser.h"
#include <iostream>

namespace pyplusplus {

const Token& Parser::peek() const {
    if (isAtEnd()) return tokens.back();
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || peek().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        current++;
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            current++;
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return tokens[current++];
    
    throw ParseError(message, peek().line, peek().column);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::NEWLINE) return;
        
        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::DEF:
            case TokenType::IF:
            case TokenType::FOR:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::BREAK:
            case TokenType::CONTINUE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

std::unique_ptr<Module> Parser::parseModule() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    while (!isAtEnd()) {
        if (match(TokenType::NEWLINE)) continue;
        
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError& error) {
            std::cerr << "Parse error: " << error.what() << std::endl;
            synchronize();
        }
    }
    
    return std::make_unique<Module>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::IF)) return parseIfStatement();
    if (match(TokenType::WHILE)) return parseWhileStatement();
    if (match(TokenType::FOR)) return parseForStatement();
    if (match(TokenType::DEF)) return parseFunctionDef();
    if (match(TokenType::RETURN)) return parseReturnStatement();
    if (match(TokenType::BREAK)) return parseBreakStatement();
    if (match(TokenType::CONTINUE)) return parseContinueStatement();
    if (match(TokenType::PASS)) return parsePassStatement();
    
    // Try to parse assignment first
    try {
        return parseAssignmentStatement();
    } catch (const ParseError&) {
        // Fall back to expression statement
        return parseExpressionStatement();
    }
}

std::unique_ptr<Statement> Parser::parseIfStatement() {
    auto condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after if condition");
    
    auto then_branch = parseBlock();
    
    std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>> elif_branches;
    
    while (match(TokenType::ELIF)) {
        auto elif_condition = parseExpression();
        consume(TokenType::COLON, "Expected ':' after elif condition");
        auto elif_body = parseBlock();
        elif_branches.push_back(std::make_unique<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>(
            std::move(elif_condition), std::move(elif_body)));
    }
    
    std::vector<std::unique_ptr<Statement>> else_branch;
    if (match(TokenType::ELSE)) {
        consume(TokenType::COLON, "Expected ':' after else");
        else_branch = parseBlock();
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_branch), 
                                         std::move(elif_branches), std::move(else_branch));
}

std::unique_ptr<Statement> Parser::parseWhileStatement() {
    auto condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after while condition");
    auto body = parseBlock();
    
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parseForStatement() {
    auto target = parseExpression();
    consume(TokenType::IN, "Expected 'in' in for loop");
    auto iterable = parseExpression();
    consume(TokenType::COLON, "Expected ':' after for loop iterable");
    auto body = parseBlock();
    
    return std::make_unique<ForStatement>(std::move(target), std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::parseFunctionDef() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    auto parameters = parseParameterList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    std::unique_ptr<Expression> return_type = nullptr;
    if (match(TokenType::ARROW)) {
        return_type = parseTypeAnnotation();
    }
    
    consume(TokenType::COLON, "Expected ':' after function signature");
    auto body = parseBlock();
    
    return std::make_unique<FunctionDef>(name.value, std::move(parameters), 
                                         std::move(return_type), std::move(body));
}

std::unique_ptr<Statement> Parser::parseReturnStatement() {
    Token return_token = previous();
    std::unique_ptr<Expression> value = nullptr;
    
    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN)) {
        value = parseExpression();
    }
    
    return std::make_unique<ReturnStatement>(std::move(value), return_token.line, return_token.column);
}

std::unique_ptr<Statement> Parser::parseBreakStatement() {
    Token break_token = previous();
    return std::make_unique<BreakStatement>(break_token.line, break_token.column);
}

std::unique_ptr<Statement> Parser::parseContinueStatement() {
    Token continue_token = previous();
    return std::make_unique<ContinueStatement>(continue_token.line, continue_token.column);
}

std::unique_ptr<Statement> Parser::parsePassStatement() {
    Token pass_token = previous();
    return std::make_unique<PassStatement>(pass_token.line, pass_token.column);
}

std::unique_ptr<Statement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::unique_ptr<Statement> Parser::parseAssignmentStatement() {
    auto expr = parseAssignment();
    return std::make_unique<ExpressionStatement>(std::move(expr));
}

std::vector<std::unique_ptr<Statement>> Parser::parseBlock() {
    std::vector<std::unique_ptr<Statement>> statements;
    
    // Expect INDENT after colon
    if (check(TokenType::NEWLINE)) {
        advance(); // Skip newline
        consume(TokenType::INDENT, "Expected indented block");
    } else {
        // Single statement on same line
        statements.push_back(parseStatement());
        return statements;
    }
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (match(TokenType::NEWLINE)) continue;
        
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError& error) {
            std::cerr << "Parse error: " << error.what() << std::endl;
            synchronize();
        }
    }
    
    if (check(TokenType::DEDENT)) {
        advance(); // Skip DEDENT
    }
    
    return statements;
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expression> Parser::parseAssignment() {
    auto expr = parseOr();
    
    if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN, 
               TokenType::MULTIPLY_ASSIGN, TokenType::DIVIDE_ASSIGN})) {
        Token op = previous();
        auto value = parseAssignment();
        
        if (expr->getLine() == 0) { // Simple check for lvalue
            // TODO: Better lvalue checking
            return std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(value));
        }
        
        throw ParseError("Invalid assignment target", op.line, op.column);
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseOr() {
    auto expr = parseAnd();
    
    while (match(TokenType::OR)) {
        Token op = previous();
        auto right = parseAnd();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAnd() {
    auto expr = parseNot();
    
    while (match(TokenType::AND)) {
        Token op = previous();
        auto right = parseNot();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseNot() {
    if (match(TokenType::NOT)) {
        Token op = previous();
        auto right = parseNot();
        return std::make_unique<UnaryExpression>(op.type, std::move(right));
    }
    
    return parseComparison();
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseBitwiseOr();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL, TokenType::LESS, 
                  TokenType::LESS_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL,
                  TokenType::IS, TokenType::IN})) {
        Token op = previous();
        auto right = parseBitwiseOr();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseOr() {
    auto expr = parseBitwiseXor();
    
    while (match(TokenType::BIT_OR)) {
        Token op = previous();
        auto right = parseBitwiseXor();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseXor() {
    auto expr = parseBitwiseAnd();
    
    while (match(TokenType::BIT_XOR)) {
        Token op = previous();
        auto right = parseBitwiseAnd();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseAnd() {
    auto expr = parseShift();
    
    while (match(TokenType::BIT_AND)) {
        Token op = previous();
        auto right = parseShift();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseShift() {
    auto expr = parseAddition();
    
    while (match({TokenType::LEFT_SHIFT, TokenType::RIGHT_SHIFT})) {
        Token op = previous();
        auto right = parseAddition();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAddition() {
    auto expr = parseMultiplication();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        auto right = parseMultiplication();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseMultiplication() {
    auto expr = parseUnary();
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO, TokenType::FLOOR_DIVIDE})) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::MINUS, TokenType::PLUS, TokenType::BIT_NOT})) {
        Token op = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpression>(op.type, std::move(right));
    }
    
    return parsePower();
}

std::unique_ptr<Expression> Parser::parsePower() {
    auto expr = parsePrimary();
    
    if (match(TokenType::POWER)) {
        Token op = previous();
        auto right = parseUnary(); // Power is right-associative
        expr = std::make_unique<BinaryExpression>(std::move(expr), op.type, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::TRUE)) {
        return std::make_unique<LiteralExpression>(true, previous().line, previous().column);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<LiteralExpression>(false, previous().line, previous().column);
    }
    
    if (match(TokenType::NONE)) {
        return std::make_unique<LiteralExpression>(nullptr, previous().line, previous().column);
    }
    
    if (match(TokenType::INTEGER)) {
        Token token = previous();
        return std::make_unique<LiteralExpression>(std::stoi(token.value), token.line, token.column);
    }
    
    if (match(TokenType::FLOAT)) {
        Token token = previous();
        return std::make_unique<LiteralExpression>(std::stod(token.value), token.line, token.column);
    }
    
    if (match(TokenType::STRING)) {
        Token token = previous();
        return std::make_unique<LiteralExpression>(token.value, token.line, token.column);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        Token token = previous();
        return std::make_unique<IdentifierExpression>(token.value, token.line, token.column);
    }
    
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match(TokenType::LEFT_BRACKET)) {
        return parseList();
    }
    
    if (match(TokenType::LEFT_BRACE)) {
        return parseDict();
    }
    
    if (match(TokenType::LAMBDA)) {
        return parseLambda();
    }
    
    throw ParseError("Expected expression", peek().line, peek().column);
}

std::unique_ptr<Expression> Parser::parseCall(std::unique_ptr<Expression> callee) {
    auto arguments = parseArgumentList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
    return std::make_unique<CallExpression>(std::move(callee), std::move(arguments));
}

std::unique_ptr<Expression> Parser::parseAttribute(std::unique_ptr<Expression> object) {
    consume(TokenType::IDENTIFIER, "Expected attribute name after '.'");
    Token attr = previous();
    return std::make_unique<AttributeExpression>(std::move(object), attr.value);
}

std::unique_ptr<Expression> Parser::parseSubscript(std::unique_ptr<Expression> object) {
    auto index = parseExpression();
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after subscript");
    return std::make_unique<SubscriptExpression>(std::move(object), std::move(index));
}

std::unique_ptr<Expression> Parser::parseList() {
    std::vector<std::unique_ptr<Expression>> elements;
    
    if (!check(TokenType::RIGHT_BRACKET)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after list elements");
    return std::make_unique<ListExpression>(std::move(elements), previous().line, previous().column);
}

std::unique_ptr<Expression> Parser::parseDict() {
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> entries;
    
    if (!check(TokenType::RIGHT_BRACE)) {
        do {
            auto key = parseExpression();
            consume(TokenType::COLON, "Expected ':' in dictionary entry");
            auto value = parseExpression();
            entries.push_back(std::make_pair(std::move(key), std::move(value)));
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after dictionary entries");
    return std::make_unique<DictExpression>(std::move(entries), previous().line, previous().column);
}

std::unique_ptr<Expression> Parser::parseLambda() {
    auto parameters = parseParameterList();
    consume(TokenType::COLON, "Expected ':' after lambda parameters");
    auto body = parseExpression();
    
    // Create a lambda function
    // TODO: Implement proper lambda representation
    return std::make_unique<IdentifierExpression>("lambda", 0, 0);
}

std::vector<std::unique_ptr<Expression>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<Expression>> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    
    return arguments;
}

std::vector<std::pair<std::string, std::unique_ptr<Expression>>> Parser::parseParameterList() {
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            consume(TokenType::IDENTIFIER, "Expected parameter name");
            std::string name = previous().value;
            
            std::unique_ptr<Expression> type_annotation = nullptr;
            if (match(TokenType::COLON)) {
                type_annotation = parseTypeAnnotation();
            }
            
            parameters.push_back(std::make_pair(name, std::move(type_annotation)));
        } while (match(TokenType::COMMA));
    }
    
    return parameters;
}

std::unique_ptr<Expression> Parser::parseTypeAnnotation() {
    // Simple type annotation parsing
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<IdentifierExpression>(previous().value, previous().line, previous().column);
    }
    
    throw ParseError("Expected type annotation", peek().line, peek().column);
}

} // namespace pyplusplus