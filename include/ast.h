#pragma once
#include <vector>
#include <memory>
#include <string>
#include <variant>
#include "lexer.h"

namespace pyplusplus {

// Forward declarations
class ASTVisitor;
class Expression;
class Statement;

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual int getLine() const = 0;
    virtual int getColumn() const = 0;
};

// Expression classes
class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

class LiteralExpression : public Expression {
private:
    std::variant<int, double, std::string, bool, std::nullptr_t> value;
    int line;
    int column;

public:
    LiteralExpression(int value, int line, int column) : value(value), line(line), column(column) {}
    LiteralExpression(double value, int line, int column) : value(value), line(line), column(column) {}
    LiteralExpression(const std::string& value, int line, int column) : value(value), line(line), column(column) {}
    LiteralExpression(bool value, int line, int column) : value(value), line(line), column(column) {}
    LiteralExpression(std::nullptr_t, int line, int column) : value(nullptr), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const auto& getValue() const { return value; }
    bool isInt() const { return std::holds_alternative<int>(value); }
    bool isFloat() const { return std::holds_alternative<double>(value); }
    bool isString() const { return std::holds_alternative<std::string>(value); }
    bool isBool() const { return std::holds_alternative<bool>(value); }
    bool isNone() const { return std::holds_alternative<std::nullptr_t>(value); }
    
    int asInt() const { return std::get<int>(value); }
    double asFloat() const { return std::get<double>(value); }
    const std::string& asString() const { return std::get<std::string>(value); }
    bool asBool() const { return std::get<bool>(value); }
};

class IdentifierExpression : public Expression {
private:
    std::string name;
    int line;
    int column;

public:
    IdentifierExpression(const std::string& name, int line, int column) 
        : name(name), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::string& getName() const { return name; }
};

class BinaryExpression : public Expression {
private:
    std::unique_ptr<Expression> left;
    TokenType op;
    std::unique_ptr<Expression> right;

public:
    BinaryExpression(std::unique_ptr<Expression> left, TokenType op, std::unique_ptr<Expression> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return left->getLine(); }
    int getColumn() const override { return left->getColumn(); }
    
    Expression* getLeft() const { return left.get(); }
    TokenType getOperator() const { return op; }
    Expression* getRight() const { return right.get(); }
};

class UnaryExpression : public Expression {
private:
    TokenType op;
    std::unique_ptr<Expression> operand;

public:
    UnaryExpression(TokenType op, std::unique_ptr<Expression> operand)
        : op(op), operand(std::move(operand)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return operand->getLine(); }
    int getColumn() const override { return operand->getColumn(); }
    
    TokenType getOperator() const { return op; }
    Expression* getOperand() const { return operand.get(); }
};

class CallExpression : public Expression {
private:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;

public:
    CallExpression(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return callee->getLine(); }
    int getColumn() const override { return callee->getColumn(); }
    
    Expression* getCallee() const { return callee.get(); }
    const std::vector<std::unique_ptr<Expression>>& getArguments() const { return arguments; }
};

class AttributeExpression : public Expression {
private:
    std::unique_ptr<Expression> object;
    std::string attribute;

public:
    AttributeExpression(std::unique_ptr<Expression> object, const std::string& attribute)
        : object(std::move(object)), attribute(attribute) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return object->getLine(); }
    int getColumn() const override { return object->getColumn(); }
    
    Expression* getObject() const { return object.get(); }
    const std::string& getAttribute() const { return attribute; }
};

class SubscriptExpression : public Expression {
private:
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;

public:
    SubscriptExpression(std::unique_ptr<Expression> object, std::unique_ptr<Expression> index)
        : object(std::move(object)), index(std::move(index)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return object->getLine(); }
    int getColumn() const override { return object->getColumn(); }
    
    Expression* getObject() const { return object.get(); }
    Expression* getIndex() const { return index.get(); }
};

class ListExpression : public Expression {
private:
    std::vector<std::unique_ptr<Expression>> elements;
    int line;
    int column;

public:
    ListExpression(std::vector<std::unique_ptr<Expression>> elements, int line, int column)
        : elements(std::move(elements)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::unique_ptr<Expression>>& getElements() const { return elements; }
};

class DictExpression : public Expression {
private:
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> entries;
    int line;
    int column;

public:
    DictExpression(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> entries, int line, int column)
        : entries(std::move(entries)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& getEntries() const { return entries; }
};

class TupleExpression : public Expression {
private:
    std::vector<std::unique_ptr<Expression>> elements;
    int line;
    int column;

public:
    TupleExpression(std::vector<std::unique_ptr<Expression>> elements, int line, int column)
        : elements(std::move(elements)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::unique_ptr<Expression>>& getElements() const { return elements; }
};

class SetExpression : public Expression {
private:
    std::vector<std::unique_ptr<Expression>> elements;
    int line;
    int column;

public:
    SetExpression(std::vector<std::unique_ptr<Expression>> elements, int line, int column)
        : elements(std::move(elements)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::unique_ptr<Expression>>& getElements() const { return elements; }
};

class LambdaExpression : public Expression {
private:
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters;
    std::unique_ptr<Expression> body;
    int line;
    int column;

public:
    LambdaExpression(std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters,
                     std::unique_ptr<Expression> body, int line, int column)
        : parameters(std::move(parameters)), body(std::move(body)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getParameters() const { return parameters; }
    Expression* getBody() const { return body.get(); }
};

class ComprehensionExpression : public Expression {
private:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> iterable;
    std::vector<std::unique_ptr<Expression>> ifs;
    int line;
    int column;

public:
    ComprehensionExpression(std::unique_ptr<Expression> target, std::unique_ptr<Expression> iterable,
                            std::vector<std::unique_ptr<Expression>> ifs, int line, int column)
        : target(std::move(target)), iterable(std::move(iterable)), ifs(std::move(ifs)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getTarget() const { return target.get(); }
    Expression* getIterable() const { return iterable.get(); }
    const std::vector<std::unique_ptr<Expression>>& getIfs() const { return ifs; }
};

class ListComprehensionExpression : public Expression {
private:
    std::unique_ptr<Expression> element;
    std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions;
    int line;
    int column;

public:
    ListComprehensionExpression(std::unique_ptr<Expression> element,
                               std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions, int line, int column)
        : element(std::move(element)), comprehensions(std::move(comprehensions)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getElement() const { return element.get(); }
    const std::vector<std::unique_ptr<ComprehensionExpression>>& getComprehensions() const { return comprehensions; }
};

class DictComprehensionExpression : public Expression {
private:
    std::unique_ptr<Expression> key;
    std::unique_ptr<Expression> value;
    std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions;
    int line;
    int column;

public:
    DictComprehensionExpression(std::unique_ptr<Expression> key, std::unique_ptr<Expression> value,
                                std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions, int line, int column)
        : key(std::move(key)), value(std::move(value)), comprehensions(std::move(comprehensions)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getKey() const { return key.get(); }
    Expression* getValue() const { return value.get(); }
    const std::vector<std::unique_ptr<ComprehensionExpression>>& getComprehensions() const { return comprehensions; }
};

class SetComprehensionExpression : public Expression {
private:
    std::unique_ptr<Expression> element;
    std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions;
    int line;
    int column;

public:
    SetComprehensionExpression(std::unique_ptr<Expression> element,
                               std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions, int line, int column)
        : element(std::move(element)), comprehensions(std::move(comprehensions)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getElement() const { return element.get(); }
    const std::vector<std::unique_ptr<ComprehensionExpression>>& getComprehensions() const { return comprehensions; }
};

class GeneratorExpression : public Expression {
private:
    std::unique_ptr<Expression> element;
    std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions;
    int line;
    int column;

public:
    GeneratorExpression(std::unique_ptr<Expression> element,
                       std::vector<std::unique_ptr<ComprehensionExpression>> comprehensions, int line, int column)
        : element(std::move(element)), comprehensions(std::move(comprehensions)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getElement() const { return element.get(); }
    const std::vector<std::unique_ptr<ComprehensionExpression>>& getComprehensions() const { return comprehensions; }
};

class YieldExpression : public Expression {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    YieldExpression(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class YieldFromExpression : public Expression {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    YieldFromExpression(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class AwaitExpression : public Expression {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    AwaitExpression(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class ConditionalExpression : public Expression {
private:
    std::unique_ptr<Expression> test;
    std::unique_ptr<Expression> body;
    std::unique_ptr<Expression> orelse;

public:
    ConditionalExpression(std::unique_ptr<Expression> test, std::unique_ptr<Expression> body, std::unique_ptr<Expression> orelse)
        : test(std::move(test)), body(std::move(body)), orelse(std::move(orelse)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return test->getLine(); }
    int getColumn() const override { return test->getColumn(); }
    
    Expression* getTest() const { return test.get(); }
    Expression* getBody() const { return body.get(); }
    Expression* getOrElse() const { return orelse.get(); }
};

class CompareExpression : public Expression {
private:
    std::unique_ptr<Expression> left;
    std::vector<std::pair<TokenType, std::unique_ptr<Expression>>> comparators;

public:
    CompareExpression(std::unique_ptr<Expression> left, 
                     std::vector<std::pair<TokenType, std::unique_ptr<Expression>>> comparators)
        : left(std::move(left)), comparators(std::move(comparators)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return left->getLine(); }
    int getColumn() const override { return left->getColumn(); }
    
    Expression* getLeft() const { return left.get(); }
    const std::vector<std::pair<TokenType, std::unique_ptr<Expression>>>& getComparators() const { return comparators; }
};

class BooleanExpression : public Expression {
private:
    std::vector<std::pair<TokenType, std::unique_ptr<Expression>>> values;

public:
    BooleanExpression(std::vector<std::pair<TokenType, std::unique_ptr<Expression>>> values)
        : values(std::move(values)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return values.empty() ? 0 : values[0].second->getLine(); }
    int getColumn() const override { return values.empty() ? 0 : values[0].second->getColumn(); }
    
    const std::vector<std::pair<TokenType, std::unique_ptr<Expression>>>& getValues() const { return values; }
};

// Statement classes
class Statement : public ASTNode {
public:
    virtual ~Statement() = default;
};

class ExpressionStatement : public Statement {
private:
    std::unique_ptr<Expression> expression;

public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expression)
        : expression(std::move(expression)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return expression->getLine(); }
    int getColumn() const override { return expression->getColumn(); }
    
    Expression* getExpression() const { return expression.get(); }
};

class AssignmentStatement : public Statement {
private:
    std::vector<std::unique_ptr<Expression>> targets;
    std::unique_ptr<Expression> value;

public:
    AssignmentStatement(std::vector<std::unique_ptr<Expression>> targets, std::unique_ptr<Expression> value)
        : targets(std::move(targets)), value(std::move(value)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return targets[0]->getLine(); }
    int getColumn() const override { return targets[0]->getColumn(); }
    
    const std::vector<std::unique_ptr<Expression>>& getTargets() const { return targets; }
    Expression* getValue() const { return value.get(); }
};

class IfStatement : public Statement {
private:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> then_branch;
    std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>> elif_branches;
    std::vector<std::unique_ptr<Statement>> else_branch;

public:
    IfStatement(std::unique_ptr<Expression> condition, 
                std::vector<std::unique_ptr<Statement>> then_branch,
                std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>> elif_branches,
                std::vector<std::unique_ptr<Statement>> else_branch)
        : condition(std::move(condition)), then_branch(std::move(then_branch)), 
          elif_branches(std::move(elif_branches)), else_branch(std::move(else_branch)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return condition->getLine(); }
    int getColumn() const override { return condition->getColumn(); }
    
    Expression* getCondition() const { return condition.get(); }
    const std::vector<std::unique_ptr<Statement>>& getThenBranch() const { return then_branch; }
    const std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>>& getElifBranches() const { return elif_branches; }
    const std::vector<std::unique_ptr<Statement>>& getElseBranch() const { return else_branch; }
};

class WhileStatement : public Statement {
private:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;

public:
    WhileStatement(std::unique_ptr<Expression> condition, std::vector<std::unique_ptr<Statement>> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return condition->getLine(); }
    int getColumn() const override { return condition->getColumn(); }
    
    Expression* getCondition() const { return condition.get(); }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class ForStatement : public Statement {
private:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> iterable;
    std::vector<std::unique_ptr<Statement>> body;

public:
    ForStatement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> iterable, 
                 std::vector<std::unique_ptr<Statement>> body)
        : target(std::move(target)), iterable(std::move(iterable)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return target->getLine(); }
    int getColumn() const override { return target->getColumn(); }
    
    Expression* getTarget() const { return target.get(); }
    Expression* getIterable() const { return iterable.get(); }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class FunctionDef : public Statement {
private:
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters;
    std::unique_ptr<Expression> return_type;
    std::vector<std::unique_ptr<Statement>> body;

public:
    FunctionDef(const std::string& name, 
                std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters,
                std::unique_ptr<Expression> return_type,
                std::vector<std::unique_ptr<Statement>> body)
        : name(name), parameters(std::move(parameters)), return_type(std::move(return_type)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return 0; } // TODO: Store line info
    int getColumn() const override { return 0; }
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getParameters() const { return parameters; }
    Expression* getReturnType() const { return return_type.get(); }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class ReturnStatement : public Statement {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    ReturnStatement(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class BreakStatement : public Statement {
private:
    int line;
    int column;

public:
    BreakStatement(int line, int column) : line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
};

class ContinueStatement : public Statement {
private:
    int line;
    int column;

public:
    ContinueStatement(int line, int column) : line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
};

class PassStatement : public Statement {
private:
    int line;
    int column;

public:
    PassStatement(int line, int column) : line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
};

class ClassDef : public Statement {
private:
    std::string name;
    std::vector<std::unique_ptr<Expression>> base_classes;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators;
    std::unique_ptr<Expression> metaclass;
    std::vector<std::unique_ptr<Statement>> body;

public:
    ClassDef(const std::string& name, 
             std::vector<std::unique_ptr<Expression>> base_classes,
             std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators,
             std::unique_ptr<Expression> metaclass,
             std::vector<std::unique_ptr<Statement>> body)
        : name(name), base_classes(std::move(base_classes)), decorators(std::move(decorators)), 
          metaclass(std::move(metaclass)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return 0; } // TODO: Store line info
    int getColumn() const override { return 0; }
    
    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<Expression>>& getBaseClasses() const { return base_classes; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getDecorators() const { return decorators; }
    Expression* getMetaclass() const { return metaclass.get(); }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class AsyncFunctionDef : public Statement {
private:
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters;
    std::unique_ptr<Expression> return_type;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators;
    std::vector<std::unique_ptr<Statement>> body;

public:
    AsyncFunctionDef(const std::string& name, 
                     std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters,
                     std::unique_ptr<Expression> return_type,
                     std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators,
                     std::vector<std::unique_ptr<Statement>> body)
        : name(name), parameters(std::move(parameters)), return_type(std::move(return_type)), 
          decorators(std::move(decorators)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return 0; } // TODO: Store line info
    int getColumn() const override { return 0; }
    
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getParameters() const { return parameters; }
    Expression* getReturnType() const { return return_type.get(); }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getDecorators() const { return decorators; }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class DecoratedFunctionDef : public FunctionDef {
private:
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators;

public:
    DecoratedFunctionDef(const std::string& name, 
                         std::vector<std::pair<std::string, std::unique_ptr<Expression>>> parameters,
                         std::unique_ptr<Expression> return_type,
                         std::vector<std::pair<std::string, std::unique_ptr<Expression>>> decorators,
                         std::vector<std::unique_ptr<Statement>> body)
        : FunctionDef(name, std::move(parameters), std::move(return_type), std::move(body)), 
          decorators(std::move(decorators)) {}
    
    void accept(ASTVisitor& visitor) override;
    
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getDecorators() const { return decorators; }
};

class AsyncForStatement : public Statement {
private:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> iterable;
    std::vector<std::unique_ptr<Statement>> body;

public:
    AsyncForStatement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> iterable, 
                      std::vector<std::unique_ptr<Statement>> body)
        : target(std::move(target)), iterable(std::move(iterable)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return target->getLine(); }
    int getColumn() const override { return target->getColumn(); }
    
    Expression* getTarget() const { return target.get(); }
    Expression* getIterable() const { return iterable.get(); }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class AsyncWithStatement : public Statement {
private:
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> items;
    std::vector<std::unique_ptr<Statement>> body;

public:
    AsyncWithStatement(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> items,
                       std::vector<std::unique_ptr<Statement>> body)
        : items(std::move(items)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return items.empty() ? 0 : items[0].first->getLine(); }
    int getColumn() const override { return items.empty() ? 0 : items[0].first->getColumn(); }
    
    const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& getItems() const { return items; }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class WithStatement : public Statement {
private:
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> items;
    std::vector<std::unique_ptr<Statement>> body;

public:
    WithStatement(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> items,
                  std::vector<std::unique_ptr<Statement>> body)
        : items(std::move(items)), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return items.empty() ? 0 : items[0].first->getLine(); }
    int getColumn() const override { return items.empty() ? 0 : items[0].first->getColumn(); }
    
    const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& getItems() const { return items; }
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
};

class TryStatement : public Statement {
private:
    std::vector<std::unique_ptr<Statement>> body;
    std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>> handlers;
    std::vector<std::unique_ptr<Statement>> else_body;
    std::vector<std::unique_ptr<Statement>> finally_body;

public:
    TryStatement(std::vector<std::unique_ptr<Statement>> body,
                 std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>> handlers,
                 std::vector<std::unique_ptr<Statement>> else_body,
                 std::vector<std::unique_ptr<Statement>> finally_body)
        : body(std::move(body)), handlers(std::move(handlers)), else_body(std::move(else_body)), finally_body(std::move(finally_body)) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return body.empty() ? 0 : body[0]->getLine(); }
    int getColumn() const override { return body.empty() ? 0 : body[0]->getColumn(); }
    
    const std::vector<std::unique_ptr<Statement>>& getBody() const { return body; }
    const std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>>& getHandlers() const { return handlers; }
    const std::vector<std::unique_ptr<Statement>>& getElseBody() const { return else_body; }
    const std::vector<std::unique_ptr<Statement>>& getFinallyBody() const { return finally_body; }
};

class RaiseStatement : public Statement {
private:
    std::unique_ptr<Expression> exception;
    std::unique_ptr<Expression> cause;
    int line;
    int column;

public:
    RaiseStatement(std::unique_ptr<Expression> exception, std::unique_ptr<Expression> cause, int line, int column)
        : exception(std::move(exception)), cause(std::move(cause)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getException() const { return exception.get(); }
    Expression* getCause() const { return cause.get(); }
};

class AssertStatement : public Statement {
private:
    std::unique_ptr<Expression> test;
    std::unique_ptr<Expression> msg;
    int line;
    int column;

public:
    AssertStatement(std::unique_ptr<Expression> test, std::unique_ptr<Expression> msg, int line, int column)
        : test(std::move(test)), msg(std::move(msg)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getTest() const { return test.get(); }
    Expression* getMsg() const { return msg.get(); }
};

class GlobalStatement : public Statement {
private:
    std::vector<std::string> names;
    int line;
    int column;

public:
    GlobalStatement(std::vector<std::string> names, int line, int column)
        : names(std::move(names)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::string>& getNames() const { return names; }
};

class NonlocalStatement : public Statement {
private:
    std::vector<std::string> names;
    int line;
    int column;

public:
    NonlocalStatement(std::vector<std::string> names, int line, int column)
        : names(std::move(names)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::string>& getNames() const { return names; }
};

class ImportStatement : public Statement {
private:
    std::vector<std::pair<std::string, std::string>> names;
    int line;
    int column;

public:
    ImportStatement(std::vector<std::pair<std::string, std::string>> names, int line, int column)
        : names(std::move(names)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::pair<std::string, std::string>>& getNames() const { return names; }
};

class ImportFromStatement : public Statement {
private:
    std::string module;
    std::vector<std::pair<std::string, std::string>> names;
    int line;
    int column;

public:
    ImportFromStatement(const std::string& module, std::vector<std::pair<std::string, std::string>> names, int line, int column)
        : module(module), names(std::move(names)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::string& getModule() const { return module; }
    const std::vector<std::pair<std::string, std::string>>& getNames() const { return names; }
};

// Pattern matching (Python 3.10+)
class MatchStatement : public Statement {
private:
    std::unique_ptr<Expression> subject;
    std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>> cases;
    int line;
    int column;

public:
    MatchStatement(std::unique_ptr<Expression> subject,
                   std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>> cases,
                   int line, int column)
        : subject(std::move(subject)), cases(std::move(cases)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getSubject() const { return subject.get(); }
    const std::vector<std::unique_ptr<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>>>& getCases() const { return cases; }
};

// Pattern expressions for pattern matching
class MatchValue : public Expression {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    MatchValue(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class MatchSingleton : public Expression {
private:
    std::unique_ptr<Expression> value;
    int line;
    int column;

public:
    MatchSingleton(std::unique_ptr<Expression> value, int line, int column)
        : value(std::move(value)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getValue() const { return value.get(); }
};

class MatchSequence : public Expression {
private:
    std::vector<std::unique_ptr<Expression>> patterns;
    int line;
    int column;

public:
    MatchSequence(std::vector<std::unique_ptr<Expression>> patterns, int line, int column)
        : patterns(std::move(patterns)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::unique_ptr<Expression>>& getPatterns() const { return patterns; }
};

class MatchMapping : public Expression {
private:
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> patterns;
    std::unique_ptr<Expression> rest;
    int line;
    int column;

public:
    MatchMapping(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> patterns,
                 std::unique_ptr<Expression> rest, int line, int column)
        : patterns(std::move(patterns)), rest(std::move(rest)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>>& getPatterns() const { return patterns; }
    Expression* getRest() const { return rest.get(); }
};

class MatchClass : public Expression {
private:
    std::unique_ptr<Expression> cls;
    std::vector<std::unique_ptr<Expression>> patterns;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> kwd_patterns;
    int line;
    int column;

public:
    MatchClass(std::unique_ptr<Expression> cls,
               std::vector<std::unique_ptr<Expression>> patterns,
               std::vector<std::pair<std::string, std::unique_ptr<Expression>>> kwd_patterns,
               int line, int column)
        : cls(std::move(cls)), patterns(std::move(patterns)), kwd_patterns(std::move(kwd_patterns)), 
          line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getClass() const { return cls.get(); }
    const std::vector<std::unique_ptr<Expression>>& getPatterns() const { return patterns; }
    const std::vector<std::pair<std::string, std::unique_ptr<Expression>>>& getKwdPatterns() const { return kwd_patterns; }
};

class MatchStar : public Expression {
private:
    std::string name;
    int line;
    int column;

public:
    MatchStar(const std::string& name, int line, int column)
        : name(name), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::string& getName() const { return name; }
};

class MatchAs : public Expression {
private:
    std::unique_ptr<Expression> pattern;
    std::string name;
    int line;
    int column;

public:
    MatchAs(std::unique_ptr<Expression> pattern, const std::string& name, int line, int column)
        : pattern(std::move(pattern)), name(name), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    Expression* getPattern() const { return pattern.get(); }
    const std::string& getName() const { return name; }
};

class MatchOr : public Expression {
private:
    std::vector<std::unique_ptr<Expression>> patterns;
    int line;
    int column;

public:
    MatchOr(std::vector<std::unique_ptr<Expression>> patterns, int line, int column)
        : patterns(std::move(patterns)), line(line), column(column) {}
    
    void accept(ASTVisitor& visitor) override;
    int getLine() const override { return line; }
    int getColumn() const override { return column; }
    
    const std::vector<std::unique_ptr<Expression>>& getPatterns() const { return patterns; }
};

// Module class
class Module {
private:
    std::vector<std::unique_ptr<Statement>> statements;

public:
    explicit Module(std::vector<std::unique_ptr<Statement>> statements)
        : statements(std::move(statements)) {}
    
    const std::vector<std::unique_ptr<Statement>>& getStatements() const { return statements; }
};

// Visitor interface
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expressions
    virtual void visit(LiteralExpression& expr) = 0;
    virtual void visit(IdentifierExpression& expr) = 0;
    virtual void visit(BinaryExpression& expr) = 0;
    virtual void visit(UnaryExpression& expr) = 0;
    virtual void visit(CallExpression& expr) = 0;
    virtual void visit(AttributeExpression& expr) = 0;
    virtual void visit(SubscriptExpression& expr) = 0;
    virtual void visit(ListExpression& expr) = 0;
    virtual void visit(DictExpression& expr) = 0;
    virtual void visit(TupleExpression& expr) = 0;
    virtual void visit(SetExpression& expr) = 0;
    virtual void visit(LambdaExpression& expr) = 0;
    virtual void visit(ComprehensionExpression& expr) = 0;
    virtual void visit(ListComprehensionExpression& expr) = 0;
    virtual void visit(DictComprehensionExpression& expr) = 0;
    virtual void visit(SetComprehensionExpression& expr) = 0;
    virtual void visit(GeneratorExpression& expr) = 0;
    virtual void visit(YieldExpression& expr) = 0;
    virtual void visit(YieldFromExpression& expr) = 0;
    virtual void visit(AwaitExpression& expr) = 0;
    virtual void visit(ConditionalExpression& expr) = 0;
    virtual void visit(CompareExpression& expr) = 0;
    virtual void visit(BooleanExpression& expr) = 0;
    
    // Statements
    virtual void visit(ExpressionStatement& stmt) = 0;
    virtual void visit(AssignmentStatement& stmt) = 0;
    virtual void visit(IfStatement& stmt) = 0;
    virtual void visit(WhileStatement& stmt) = 0;
    virtual void visit(ForStatement& stmt) = 0;
    virtual void visit(FunctionDef& stmt) = 0;
    virtual void visit(ReturnStatement& stmt) = 0;
    virtual void visit(BreakStatement& stmt) = 0;
    virtual void visit(ContinueStatement& stmt) = 0;
    virtual void visit(PassStatement& stmt) = 0;
    virtual void visit(ClassDef& stmt) = 0;
    virtual void visit(AsyncFunctionDef& stmt) = 0;
    virtual void visit(DecoratedFunctionDef& stmt) = 0;
    virtual void visit(AsyncForStatement& stmt) = 0;
    virtual void visit(WithStatement& stmt) = 0;
    virtual void visit(AsyncWithStatement& stmt) = 0;
    virtual void visit(TryStatement& stmt) = 0;
    virtual void visit(RaiseStatement& stmt) = 0;
    virtual void visit(AssertStatement& stmt) = 0;
    virtual void visit(GlobalStatement& stmt) = 0;
    virtual void visit(NonlocalStatement& stmt) = 0;
    virtual void visit(ImportStatement& stmt) = 0;
    virtual void visit(ImportFromStatement& stmt) = 0;
    virtual void visit(MatchStatement& stmt) = 0;
    
    // Pattern matching expressions
    virtual void visit(MatchValue& expr) = 0;
    virtual void visit(MatchSingleton& expr) = 0;
    virtual void visit(MatchSequence& expr) = 0;
    virtual void visit(MatchMapping& expr) = 0;
    virtual void visit(MatchClass& expr) = 0;
    virtual void visit(MatchStar& expr) = 0;
    virtual void visit(MatchAs& expr) = 0;
    virtual void visit(MatchOr& expr) = 0;
};

} // namespace pyplusplus