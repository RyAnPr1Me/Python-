#include "type_inference.h"
#include <iostream>
#include <algorithm>

namespace pyplusplus {

void TypeInference::addConstraint(TypeConstraint::Kind kind, std::shared_ptr<Type> type1, std::shared_ptr<Type> type2) {
    constraints.emplace_back(kind, std::move(type1), std::move(type2));
}

bool TypeInference::solveConstraints() {
    bool changed = true;
    int iterations = 0;
    const int max_iterations = 1000; // Prevent infinite loops
    
    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;
        
        for (auto& constraint : constraints) {
            switch (constraint.kind) {
                case TypeConstraint::Kind::EQUALITY:
                    if (unify(constraint.type1, constraint.type2)) {
                        changed = true;
                    }
                    break;
                case TypeConstraint::Kind::SUBTYPE:
                    // For subtype constraints, we need to ensure type1 is subtype of type2
                    // This is more complex and may require type variable instantiation
                    break;
                case TypeConstraint::Kind::SUPERTYPE:
                    // Similar to subtype, but reversed
                    break;
            }
        }
    }
    
    return iterations < max_iterations;
}

bool TypeInference::unify(std::shared_ptr<Type> type1, std::shared_ptr<Type> type2) {
    if (type1->equals(*type2)) {
        return false; // No change needed
    }
    
    // Handle type variables
    if (type1->getKind() == Type::Kind::TYPE_VARIABLE) {
        // Substitute type variable with type2
        for (auto& [node, type] : types) {
            if (type->equals(*type1)) {
                type = type2;
            }
        }
        for (auto& [name, type] : variables) {
            if (type->equals(*type1)) {
                type = type2;
            }
        }
        return true;
    }
    
    if (type2->getKind() == Type::Kind::TYPE_VARIABLE) {
        // Substitute type variable with type1
        for (auto& [node, type] : types) {
            if (type->equals(*type2)) {
                type = type1;
            }
        }
        for (auto& [name, type] : variables) {
            if (type->equals(*type2)) {
                type = type1;
            }
        }
        return true;
    }
    
    // Handle union types
    if (type1->getKind() == Type::Kind::UNION && type2->getKind() == Type::Kind::UNION) {
        auto union1 = static_cast<const UnionType*>(type1.get());
        auto union2 = static_cast<const UnionType*>(type2.get());
        
        // Create a new union containing all types from both
        std::vector<std::shared_ptr<Type>> all_types = union1->getTypes();
        for (const auto& type : union2->getTypes()) {
            bool found = false;
            for (const auto& existing : all_types) {
                if (type->equals(*existing)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                all_types.push_back(type);
            }
        }
        
        auto new_union = type_system.createUnionType(std::move(all_types));
        if (!new_union->equals(*type1) && !new_union->equals(*type2)) {
            // Update all references to the old unions
            for (auto& [node, type] : types) {
                if (type->equals(*type1) || type->equals(*type2)) {
                    type = new_union;
                }
            }
            for (auto& [name, type] : variables) {
                if (type->equals(*type1) || type->equals(*type2)) {
                    type = new_union;
                }
            }
            return true;
        }
    }
    
    return false;
}

std::shared_ptr<Type> TypeInference::substitute(
    std::shared_ptr<Type> type, 
    const std::unordered_map<std::string, std::shared_ptr<Type>>& substitution) {
    
    if (type->getKind() == Type::Kind::TYPE_VARIABLE) {
        auto type_var = static_cast<const TypeVariable*>(type.get());
        auto it = substitution.find(type_var->getName());
        if (it != substitution.end()) {
            return it->second;
        }
    }
    
    return type;
}

std::shared_ptr<Type> TypeInference::inferBinaryOperation(TokenType op, std::shared_ptr<Type> left, std::shared_ptr<Type> right) {
    switch (op) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
        case TokenType::POWER:
        case TokenType::FLOOR_DIVIDE:
            // Arithmetic operations
            if (left->getKind() == Type::Kind::INT && right->getKind() == Type::Kind::INT) {
                if (op == TokenType::DIVIDE) {
                    return type_system.getFloatType(); // Python's / always returns float
                }
                return type_system.getIntType();
            }
            if ((left->getKind() == Type::Kind::INT || left->getKind() == Type::Kind::FLOAT) &&
                (right->getKind() == Type::Kind::INT || right->getKind() == Type::Kind::FLOAT)) {
                return type_system.getFloatType();
            }
            if (left->getKind() == Type::Kind::STRING && right->getKind() == Type::Kind::STRING && op == TokenType::PLUS) {
                return type_system.getStringType();
            }
            if (left->getKind() == Type::Kind::LIST && right->getKind() == Type::Kind::LIST && op == TokenType::PLUS) {
                auto list1 = static_cast<const ListType*>(left.get());
                auto list2 = static_cast<const ListType*>(right.get());
                if (list1->getElementType()->equals(*list2->getElementType())) {
                    return type_system.createListType(std::shared_ptr<Type>(const_cast<Type*>(list1->getElementType()), [](Type*){}));
                }
            }
            return type_system.getUnknownType();
            
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
        case TokenType::IS:
        case TokenType::IN:
            // Comparison operations always return bool
            return type_system.getBoolType();
            
        case TokenType::BIT_AND:
        case TokenType::BIT_OR:
        case TokenType::BIT_XOR:
        case TokenType::LEFT_SHIFT:
        case TokenType::RIGHT_SHIFT:
            // Bitwise operations
            if (left->getKind() == Type::Kind::INT && right->getKind() == Type::Kind::INT) {
                return type_system.getIntType();
            }
            return type_system.getUnknownType();
            
        default:
            return type_system.getUnknownType();
    }
}

std::shared_ptr<Type> TypeInference::inferUnaryOperation(TokenType op, std::shared_ptr<Type> operand) {
    switch (op) {
        case TokenType::MINUS:
        case TokenType::PLUS:
            if (operand->getKind() == Type::Kind::INT || operand->getKind() == Type::Kind::FLOAT) {
                return operand;
            }
            return type_system.getUnknownType();
            
        case TokenType::NOT:
            return type_system.getBoolType();
            
        case TokenType::BIT_NOT:
            if (operand->getKind() == Type::Kind::INT) {
                return type_system.getIntType();
            }
            return type_system.getUnknownType();
            
        default:
            return type_system.getUnknownType();
    }
}

std::shared_ptr<Type> TypeInference::inferCall(std::shared_ptr<Type> callee, const std::vector<std::shared_ptr<Type>>& args) {
    if (callee->getKind() == Type::Kind::FUNCTION) {
        auto func_type = static_cast<const FunctionType*>(callee.get());
        // TODO: Check argument compatibility
        return std::shared_ptr<Type>(const_cast<Type*>(func_type->getReturnType()), [](Type*){});
    }
    
    // Built-in functions
    // TODO: Implement built-in function inference
    
    return type_system.getUnknownType();
}

std::shared_ptr<Type> TypeInference::inferSubscript(std::shared_ptr<Type> object, std::shared_ptr<Type> index) {
    if (object->getKind() == Type::Kind::LIST) {
        auto list_type = static_cast<const ListType*>(object.get());
        return std::shared_ptr<Type>(const_cast<Type*>(list_type->getElementType()), [](Type*){});
    }
    
    if (object->getKind() == Type::Kind::DICT) {
        auto dict_type = static_cast<const DictType*>(object.get());
        return std::shared_ptr<Type>(const_cast<Type*>(dict_type->getValueType()), [](Type*){});
    }
    
    if (object->getKind() == Type::Kind::STRING) {
        return type_system.getStringType();
    }
    
    return type_system.getUnknownType();
}

std::shared_ptr<Type> TypeInference::inferAttribute(std::shared_ptr<Type> object, const std::string& attr) {
    if (object->getKind() == Type::Kind::CLASS) {
        auto class_type = static_cast<const ClassType*>(object.get());
        if (auto method = class_type->getMethod(attr)) {
            return std::shared_ptr<FunctionType>(const_cast<FunctionType*>(method), [](FunctionType*){});
        }
        if (auto attr_type = class_type->getAttributeType(attr)) {
            return std::shared_ptr<Type>(const_cast<Type*>(attr_type), [](Type*){});
        }
    }
    
    // Built-in object attributes
    // TODO: Implement built-in attribute inference
    
    return type_system.getUnknownType();
}

bool TypeInference::infer(Module& module) {
    // First pass: collect function signatures and variable declarations
    for (const auto& stmt : module.getStatements()) {
        if (auto func_def = dynamic_cast<FunctionDef*>(stmt.get())) {
            // Create function type
            std::vector<std::shared_ptr<Type>> param_types;
            for (const auto& [name, type_annotation] : func_def->getParameters()) {
                if (type_annotation) {
                    // TODO: Parse type annotation
                    param_types.push_back(type_system.getUnknownType());
                } else {
                    param_types.push_back(type_system.createTypeVariable(name));
                }
            }
            
            auto return_type = func_def->getReturnType() ? 
                getType(func_def->getReturnType()) : type_system.createTypeVariable("return");
            
            auto func_type = type_system.createFunctionType(std::move(param_types), return_type);
            variables[func_def->getName()] = func_type;
        }
    }
    
    // Second pass: infer statement types
    for (const auto& stmt : module.getStatements()) {
        stmt->accept(*this);
    }
    
    // Solve constraints
    return solveConstraints();
}

// Expression visitors
void TypeInference::visit(LiteralExpression& expr) {
    std::shared_ptr<Type> type;
    if (expr.isInt()) {
        type = type_system.getIntType();
    } else if (expr.isFloat()) {
        type = type_system.getFloatType();
    } else if (expr.isString()) {
        type = type_system.getStringType();
    } else if (expr.isBool()) {
        type = type_system.getBoolType();
    } else if (expr.isNone()) {
        type = type_system.getNoneType();
    } else {
        type = type_system.getUnknownType();
    }
    
    types[&expr] = type;
}

void TypeInference::visit(IdentifierExpression& expr) {
    auto it = variables.find(expr.getName());
    if (it != variables.end()) {
        types[&expr] = it->second;
    } else {
        // Create a new type variable for unknown identifiers
        auto type_var = type_system.createTypeVariable(expr.getName());
        variables[expr.getName()] = type_var;
        types[&expr] = type_var;
    }
}

void TypeInference::visit(BinaryExpression& expr) {
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
    
    auto left_type = getType(expr.getLeft());
    auto right_type = getType(expr.getRight());
    auto result_type = inferBinaryOperation(expr.getOperator(), left_type, right_type);
    
    types[&expr] = result_type;
}

void TypeInference::visit(UnaryExpression& expr) {
    expr.getOperand()->accept(*this);
    auto operand_type = getType(expr.getOperand());
    auto result_type = inferUnaryOperation(expr.getOperator(), operand_type);
    
    types[&expr] = result_type;
}

void TypeInference::visit(CallExpression& expr) {
    expr.getCallee()->accept(*this);
    auto callee_type = getType(expr.getCallee());
    
    std::vector<std::shared_ptr<Type>> arg_types;
    for (const auto& arg : expr.getArguments()) {
        arg->accept(*this);
        arg_types.push_back(getType(arg));
    }
    
    auto result_type = inferCall(callee_type, arg_types);
    types[&expr] = result_type;
}

void TypeInference::visit(AttributeExpression& expr) {
    expr.getObject()->accept(*this);
    auto object_type = getType(expr.getObject());
    auto result_type = inferAttribute(object_type, expr.getAttribute());
    
    types[&expr] = result_type;
}

void TypeInference::visit(SubscriptExpression& expr) {
    expr.getObject()->accept(*this);
    expr.getIndex()->accept(*this);
    
    auto object_type = getType(expr.getObject());
    auto index_type = getType(expr.getIndex());
    auto result_type = inferSubscript(object_type, index_type);
    
    types[&expr] = result_type;
}

void TypeInference::visit(ListExpression& expr) {
    std::shared_ptr<Type> element_type = type_system.getUnknownType();
    
    if (!expr.getElements().empty()) {
        expr.getElements()[0]->accept(*this);
        element_type = getType(expr.getElements()[0]);
        
        // Add constraints for all elements to have the same type
        for (size_t i = 1; i < expr.getElements().size(); ++i) {
            expr.getElements()[i]->accept(*this);
            addConstraint(TypeConstraint::Kind::EQUALITY, element_type, getType(expr.getElements()[i]));
        }
    }
    
    types[&expr] = type_system.createListType(element_type);
}

void TypeInference::visit(DictExpression& expr) {
    std::shared_ptr<Type> key_type = type_system.getUnknownType();
    std::shared_ptr<Type> value_type = type_system.getUnknownType();
    
    if (!expr.getEntries().empty()) {
        expr.getEntries()[0].first->accept(*this);
        expr.getEntries()[0].second->accept(*this);
        key_type = getType(expr.getEntries()[0].first);
        value_type = getType(expr.getEntries()[0].second);
        
        // Add constraints for all entries
        for (size_t i = 1; i < expr.getEntries().size(); ++i) {
            expr.getEntries()[i].first->accept(*this);
            expr.getEntries()[i].second->accept(*this);
            addConstraint(TypeConstraint::Kind::EQUALITY, key_type, getType(expr.getEntries()[i].first));
            addConstraint(TypeConstraint::Kind::EQUALITY, value_type, getType(expr.getEntries()[i].second));
        }
    }
    
    types[&expr] = type_system.createDictType(key_type, value_type);
}

// Statement visitors
void TypeInference::visit(ExpressionStatement& stmt) {
    stmt.getExpression()->accept(*this);
}

void TypeInference::visit(AssignmentStatement& stmt) {
    stmt.getValue()->accept(*this);
    auto value_type = getType(stmt.getValue());
    
    for (const auto& target : stmt.getTargets()) {
        target->accept(*this);
        if (auto ident = dynamic_cast<IdentifierExpression*>(target)) {
            variables[ident->getName()] = value_type;
            addConstraint(TypeConstraint::Kind::EQUALITY, getType(target), value_type);
        }
        // TODO: Handle other assignment targets (subscript, attribute)
    }
}

void TypeInference::visit(IfStatement& stmt) {
    stmt.getCondition()->accept(*this);
    
    for (const auto& then_stmt : stmt.getThenBranch()) {
        then_stmt->accept(*this);
    }
    
    for (const auto& elif_branch : stmt.getElifBranches()) {
        elif_branch.first->accept(*this);
        for (const auto& elif_stmt : elif_branch.second) {
            elif_stmt->accept(*this);
        }
    }
    
    for (const auto& else_stmt : stmt.getElseBranch()) {
        else_stmt->accept(*this);
    }
}

void TypeInference::visit(WhileStatement& stmt) {
    stmt.getCondition()->accept(*this);
    
    bool old_in_loop = in_loop;
    in_loop = true;
    
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
    
    in_loop = old_in_loop;
}

void TypeInference::visit(ForStatement& stmt) {
    stmt.getIterable()->accept(*this);
    auto iterable_type = getType(stmt.getIterable());
    
    // Infer target type from iterable
    std::shared_ptr<Type> element_type;
    if (iterable_type->getKind() == Type::Kind::LIST) {
        auto list_type = static_cast<const ListType*>(iterable_type.get());
        element_type = std::shared_ptr<Type>(const_cast<Type*>(list_type->getElementType()), [](Type*){});
    } else {
        element_type = type_system.createTypeVariable("element");
    }
    
    stmt.getTarget()->accept(*this);
    if (auto ident = dynamic_cast<IdentifierExpression*>(stmt.getTarget())) {
        variables[ident->getName()] = element_type;
        addConstraint(TypeConstraint::Kind::EQUALITY, getType(stmt.getTarget()), element_type);
    }
    
    bool old_in_loop = in_loop;
    in_loop = true;
    
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
    
    in_loop = old_in_loop;
}

void TypeInference::visit(FunctionDef& stmt) {
    bool old_in_function = in_function;
    in_function = true;
    
    // Set parameter types
    for (const auto& [name, type_annotation] : stmt.getParameters()) {
        if (type_annotation) {
            // TODO: Parse type annotation
            variables[name] = type_system.createTypeVariable(name);
        } else {
            variables[name] = type_system.createTypeVariable(name);
        }
    }
    
    // Infer body
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
    
    in_function = old_in_function;
}

void TypeInference::visit(ReturnStatement& stmt) {
    if (stmt.getValue()) {
        stmt.getValue()->accept(*this);
        if (current_return_type) {
            addConstraint(TypeConstraint::Kind::EQUALITY, current_return_type, getType(stmt.getValue()));
        }
    }
}

void TypeInference::visit(BreakStatement& stmt) {
    // No type inference needed
}

void TypeInference::visit(ContinueStatement& stmt) {
    // No type inference needed
}

void TypeInference::visit(PassStatement& stmt) {
    // No type inference needed
}

// TypeChecker implementation
void TypeChecker::addError(const std::string& message, int line, int column) {
    errors.push_back("Error at line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message);
}

bool TypeChecker::checkSubtype(std::shared_ptr<Type> subtype, std::shared_ptr<Type> supertype, 
                               const std::string& context, int line, int column) {
    if (!type_system.isSubtype(*subtype, *supertype)) {
        addError("Type mismatch in " + context + ": expected " + supertype->toString() + 
                ", got " + subtype->toString(), line, column);
        return false;
    }
    return true;
}

bool TypeChecker::check(Module& module) {
    errors.clear();
    
    for (const auto& stmt : module.getStatements()) {
        stmt->accept(*this);
    }
    
    return errors.empty();
}

// TypeChecker visitor methods (simplified)
void TypeChecker::visit(LiteralExpression& expr) {
    // Literals are always valid
}

void TypeChecker::visit(IdentifierExpression& expr) {
    // Check if variable is defined
    if (inference.getVariableType(expr.getName())->getKind() == Type::Kind::UNKNOWN) {
        addError("Undefined variable: " + expr.getName(), expr.getLine(), expr.getColumn());
    }
}

void TypeChecker::visit(BinaryExpression& expr) {
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);
    
    auto left_type = inference.getType(expr.getLeft());
    auto right_type = inference.getType(expr.getRight());
    
    // Check operator compatibility
    switch (expr.getOperator()) {
        case TokenType::PLUS:
            if (left_type->getKind() == Type::Kind::STRING && right_type->getKind() != Type::Kind::STRING) {
                addError("Cannot concatenate string with " + right_type->toString(), expr.getLine(), expr.getColumn());
            }
            break;
        // TODO: Add more operator checks
        default:
            break;
    }
}

void TypeChecker::visit(UnaryExpression& expr) {
    expr.getOperand()->accept(*this);
}

void TypeChecker::visit(CallExpression& expr) {
    expr.getCallee()->accept(*this);
    
    auto callee_type = inference.getType(expr.getCallee());
    if (callee_type->getKind() != Type::Kind::FUNCTION) {
        addError("Cannot call non-function: " + callee_type->toString(), expr.getLine(), expr.getColumn());
        return;
    }
    
    auto func_type = static_cast<const FunctionType*>(callee_type.get());
    const auto& param_types = func_type->getParameterTypes();
    const auto& args = expr.getArguments();
    
    if (args.size() < param_types.size()) {
        addError("Too few arguments for function call", expr.getLine(), expr.getColumn());
    } else if (args.size() > param_types.size() && !func_type->isVariadic()) {
        addError("Too many arguments for function call", expr.getLine(), expr.getColumn());
    }
    
    for (size_t i = 0; i < std::min(args.size(), param_types.size()); ++i) {
        args[i]->accept(*this);
        auto arg_type = inference.getType(args[i]);
        checkSubtype(arg_type, std::shared_ptr<Type>(const_cast<Type*>(param_types[i].get()), [](Type*){}),
                    "function argument", args[i]->getLine(), args[i]->getColumn());
    }
}

void TypeChecker::visit(AttributeExpression& expr) {
    expr.getObject()->accept(*this);
    
    auto object_type = inference.getType(expr.getObject());
    if (object_type->getKind() == Type::Kind::CLASS) {
        auto class_type = static_cast<const ClassType*>(object_type.get());
        if (!class_type->getAttributeType(expr.getAttribute()) && !class_type->getMethod(expr.getAttribute())) {
            addError("Class " + class_type->getName() + " has no attribute '" + expr.getAttribute() + "'", 
                    expr.getLine(), expr.getColumn());
        }
    }
}

void TypeChecker::visit(SubscriptExpression& expr) {
    expr.getObject()->accept(*this);
    expr.getIndex()->accept(*this);
    
    auto object_type = inference.getType(expr.getObject());
    if (object_type->getKind() != Type::Kind::LIST && object_type->getKind() != Type::Kind::DICT && 
        object_type->getKind() != Type::Kind::STRING) {
        addError("Cannot subscript object of type " + object_type->toString(), expr.getLine(), expr.getColumn());
    }
}

void TypeChecker::visit(ListExpression& expr) {
    for (const auto& element : expr.getElements()) {
        element->accept(*this);
    }
}

void TypeChecker::visit(DictExpression& expr) {
    for (const auto& entry : expr.getEntries()) {
        entry.first->accept(*this);
        entry.second->accept(*this);
    }
}

void TypeChecker::visit(ExpressionStatement& stmt) {
    stmt.getExpression()->accept(*this);
}

void TypeChecker::visit(AssignmentStatement& stmt) {
    stmt.getValue()->accept(*this);
    
    for (const auto& target : stmt.getTargets()) {
        target->accept(*this);
        // TODO: Check assignment validity
    }
}

void TypeChecker::visit(IfStatement& stmt) {
    stmt.getCondition()->accept(*this);
    
    auto condition_type = inference.getType(stmt.getCondition());
    if (!type_system.isSubtype(*condition_type, *type_system.getBoolType())) {
        addError("If condition must be boolean", stmt.getCondition()->getLine(), stmt.getCondition()->getColumn());
    }
    
    for (const auto& then_stmt : stmt.getThenBranch()) {
        then_stmt->accept(*this);
    }
    
    for (const auto& elif_branch : stmt.getElifBranches()) {
        elif_branch.first->accept(*this);
        for (const auto& elif_stmt : elif_branch.second) {
            elif_stmt->accept(*this);
        }
    }
    
    for (const auto& else_stmt : stmt.getElseBranch()) {
        else_stmt->accept(*this);
    }
}

void TypeChecker::visit(WhileStatement& stmt) {
    stmt.getCondition()->accept(*this);
    
    auto condition_type = inference.getType(stmt.getCondition());
    if (!type_system.isSubtype(*condition_type, *type_system.getBoolType())) {
        addError("While condition must be boolean", stmt.getCondition()->getLine(), stmt.getCondition()->getColumn());
    }
    
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
}

void TypeChecker::visit(ForStatement& stmt) {
    stmt.getIterable()->accept(*this);
    stmt.getTarget()->accept(*this);
    
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
}

void TypeChecker::visit(FunctionDef& stmt) {
    for (const auto& body_stmt : stmt.getBody()) {
        body_stmt->accept(*this);
    }
}

void TypeChecker::visit(ReturnStatement& stmt) {
    if (stmt.getValue()) {
        stmt.getValue()->accept(*this);
    }
}

void TypeChecker::visit(BreakStatement& stmt) {
    if (!inference.in_loop) {
        addError("'break' outside loop", stmt.getLine(), stmt.getColumn());
    }
}

void TypeChecker::visit(ContinueStatement& stmt) {
    if (!inference.in_loop) {
        addError("'continue' outside loop", stmt.getLine(), stmt.getColumn());
    }
}

void TypeChecker::visit(PassStatement& stmt) {
    // Pass is always valid
}

} // namespace pyplusplus