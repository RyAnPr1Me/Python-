#pragma once
#include "types.h"
#include "ast.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace pyplusplus {

struct TypeConstraint {
    enum class Kind {
        EQUALITY,
        SUBTYPE,
        SUPERTYPE
    };
    
    Kind kind;
    std::shared_ptr<Type> type1;
    std::shared_ptr<Type> type2;
    
    TypeConstraint(Kind kind, std::shared_ptr<Type> type1, std::shared_ptr<Type> type2)
        : kind(kind), type1(std::move(type1)), type2(std::move(type2)) {}
};

class TypeInference : public ASTVisitor {
private:
    TypeSystem& type_system;
    std::unordered_map<const ASTNode*, std::shared_ptr<Type>> types;
    std::unordered_map<std::string, std::shared_ptr<Type>> variables;
    std::vector<TypeConstraint> constraints;
    std::shared_ptr<Type> current_return_type;
    bool in_loop = false;
    bool in_function = false;
    
    // Constraint solving
    void addConstraint(TypeConstraint::Kind kind, std::shared_ptr<Type> type1, std::shared_ptr<Type> type2);
    bool solveConstraints();
    bool unify(std::shared_ptr<Type> type1, std::shared_ptr<Type> type2);
    std::shared_ptr<Type> substitute(std::shared_ptr<Type> type, 
                                     const std::unordered_map<std::string, std::shared_ptr<Type>>& substitution);
    
    // Type inference helpers
    std::shared_ptr<Type> inferBinaryOperation(TokenType op, std::shared_ptr<Type> left, std::shared_ptr<Type> right);
    std::shared_ptr<Type> inferUnaryOperation(TokenType op, std::shared_ptr<Type> operand);
    std::shared_ptr<Type> inferCall(std::shared_ptr<Type> callee, const std::vector<std::shared_ptr<Type>>& args);
    std::shared_ptr<Type> inferSubscript(std::shared_ptr<Type> object, std::shared_ptr<Type> index);
    std::shared_ptr<Type> inferAttribute(std::shared_ptr<Type> object, const std::string& attr);
    
public:
    explicit TypeInference(TypeSystem& type_system) : type_system(type_system) {}
    
    // Main inference method
    bool infer(Module& module);
    
    // Type access
    std::shared_ptr<Type> getType(const ASTNode* node) const {
        auto it = types.find(node);
        return it != types.end() ? it->second : type_system.getUnknownType();
    }
    
    std::shared_ptr<Type> getVariableType(const std::string& name) const {
        auto it = variables.find(name);
        return it != variables.end() ? it->second : type_system.getUnknownType();
    }
    
    // Visitor methods
    void visit(LiteralExpression& expr) override;
    void visit(IdentifierExpression& expr) override;
    void visit(BinaryExpression& expr) override;
    void visit(UnaryExpression& expr) override;
    void visit(CallExpression& expr) override;
    void visit(AttributeExpression& expr) override;
    void visit(SubscriptExpression& expr) override;
    void visit(ListExpression& expr) override;
    void visit(DictExpression& expr) override;
    
    void visit(ExpressionStatement& stmt) override;
    void visit(AssignmentStatement& stmt) override;
    void visit(IfStatement& stmt) override;
    void visit(WhileStatement& stmt) override;
    void visit(ForStatement& stmt) override;
    void visit(FunctionDef& stmt) override;
    void visit(ReturnStatement& stmt) override;
    void visit(BreakStatement& stmt) override;
    void visit(ContinueStatement& stmt) override;
    void visit(PassStatement& stmt) override;
};

class TypeChecker : public ASTVisitor {
private:
    TypeSystem& type_system;
    TypeInference& inference;
    std::vector<std::string> errors;
    
    void addError(const std::string& message, int line, int column);
    bool checkSubtype(std::shared_ptr<Type> subtype, std::shared_ptr<Type> supertype, 
                      const std::string& context, int line, int column);
    
public:
    TypeChecker(TypeSystem& type_system, TypeInference& inference) 
        : type_system(type_system), inference(inference) {}
    
    bool check(Module& module);
    const std::vector<std::string>& getErrors() const { return errors; }
    
    // Visitor methods
    void visit(LiteralExpression& expr) override;
    void visit(IdentifierExpression& expr) override;
    void visit(BinaryExpression& expr) override;
    void visit(UnaryExpression& expr) override;
    void visit(CallExpression& expr) override;
    void visit(AttributeExpression& expr) override;
    void visit(SubscriptExpression& expr) override;
    void visit(ListExpression& expr) override;
    void visit(DictExpression& expr) override;
    
    void visit(ExpressionStatement& stmt) override;
    void visit(AssignmentStatement& stmt) override;
    void visit(IfStatement& stmt) override;
    void visit(WhileStatement& stmt) override;
    void visit(ForStatement& stmt) override;
    void visit(FunctionDef& stmt) override;
    void visit(ReturnStatement& stmt) override;
    void visit(BreakStatement& stmt) override;
    void visit(ContinueStatement& stmt) override;
    void visit(PassStatement& stmt) override;
};

} // namespace pyplusplus