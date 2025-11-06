#pragma once
#include "types.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <optional>

namespace pyplusplus {

// Advanced type system with full PEP 484 type hint support
class AdvancedTypeSystem : public TypeSystem {
private:
    // Generic type parameters
    std::unordered_map<std::string, std::shared_ptr<TypeVariable>> type_params;
    
    // Generic types
    std::unordered_map<std::string, std::shared_ptr<ClassType>> generic_types;
    
    // Type aliases
    std::unordered_map<std::string, std::shared_ptr<Type>> type_aliases;
    
    // Protocol types (structural subtyping)
    std::unordered_map<std::string, std::shared_ptr<ClassType>> protocols;
    
    // TypedDict types
    std::unordered_map<std::string, std::shared_ptr<ClassType>> typed_dicts;
    
    // Literal types
    std::unordered_map<std::string, std::shared_ptr<Type>> literal_types;
    
public:
    AdvancedTypeSystem();
    
    // Type hint parsing
    std::shared_ptr<Type> parseTypeHint(const std::string& hint);
    std::shared_ptr<Type> parseGenericType(const std::string& name, 
                                          const std::vector<std::shared_ptr<Type>>& args);
    
    // Generic type creation
    std::shared_ptr<ClassType> createGenericType(const std::string& name, 
                                                const std::vector<std::string>& type_params);
    std::shared_ptr<Type> instantiateGeneric(std::shared_ptr<ClassType> generic_type,
                                             const std::vector<std::shared_ptr<Type>>& type_args);
    
    // Protocol support
    std::shared_ptr<ClassType> createProtocol(const std::string& name);
    bool isSubtypeOfProtocol(const Type* type, const ClassType* protocol);
    
    // TypedDict support
    std::shared_ptr<ClassType> createTypedDict(const std::string& name,
                                               const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& fields,
                                               const std::vector<std::string>& required_fields = {});
    
    // Literal types
    std::shared_ptr<Type> createLiteralType(const std::vector<std::variant<int, std::string, bool>>& values);
    
    // Type aliases
    void registerTypeAlias(const std::string& name, std::shared_ptr<Type> type);
    std::shared_ptr<Type> resolveTypeAlias(const std::string& name);
    
    // Advanced subtyping
    bool isStructuralSubtype(const Type* subtype, const Type* supertype);
    bool matchesProtocol(const Type* type, const ClassType* protocol);
    
    // Type inference enhancements
    std::shared_ptr<Type> inferReturnType(const FunctionType* func_type, 
                                          const std::vector<std::shared_ptr<Type>>& arg_types);
    std::shared_ptr<Type> inferGeneratorType(std::shared_ptr<Type> yield_type);
    std::shared_ptr<Type> inferAsyncType(std::shared_ptr<Type> return_type);
    
    // Union and intersection types
    std::shared_ptr<Type> createIntersectionType(const std::vector<std::shared_ptr<Type>>& types);
    bool isIntersectionSubtype(const Type* type, const IntersectionType* intersection);
    
    // Callable types
    std::shared_ptr<Type> createCallableType(const std::vector<std::shared_ptr<Type>>& param_types,
                                            std::shared_ptr<Type> return_type,
                                            const std::vector<std::string>& param_names = {},
                                            const std::vector<bool>& param_defaults = {});
    
    // Type narrowing (for type guards)
    std::shared_ptr<Type> narrowType(std::shared_ptr<Type> type, 
                                      const std::string& condition);
    
    // Type compatibility checking
    bool isTypeCompatible(const Type* source, const Type* target, bool allow_coercion = true);
    std::shared_ptr<Type> getCommonType(const std::vector<std::shared_ptr<Type>>& types);
    
    // Type validation
    bool validateTypeHint(std::shared_ptr<Type> type, std::vector<std::string>& errors);
    bool validateGenericInstantiation(std::shared_ptr<ClassType> generic_type,
                                     const std::vector<std::shared_ptr<Type>>& type_args,
                                     std::vector<std::string>& errors);
};

// Intersection type for structural subtyping
class IntersectionType : public Type {
private:
    std::vector<std::shared_ptr<Type>> types;
    
public:
    explicit IntersectionType(std::vector<std::shared_ptr<Type>> types)
        : types(std::move(types)) {}
    
    Kind getKind() const override { return Kind::INTERSECTION; }
    static Kind staticKind() { return Kind::INTERSECTION; }
    std::string toString() const override {
        std::string result = "Intersection[";
        for (size_t i = 0; i < types.size(); ++i) {
            if (i > 0) result += " & ";
            result += types[i]->toString();
        }
        result += "]";
        return result;
    }
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::INTERSECTION) return false;
        const auto& other_intersection = static_cast<const IntersectionType&>(other);
        if (types.size() != other_intersection.types.size()) return false;
        for (size_t i = 0; i < types.size(); ++i) {
            if (!types[i]->equals(*other_intersection.types[i])) return false;
        }
        return true;
    }
    
    const std::vector<std::shared_ptr<Type>>& getTypes() const { return types; }
};

// Protocol type for structural subtyping
class ProtocolType : public ClassType {
private:
    std::unordered_map<std::string, std::shared_ptr<FunctionType>> required_methods;
    std::unordered_map<std::string, std::shared_ptr<Type>> required_attributes;
    bool is_runtime_checkable;
    
public:
    ProtocolType(const std::string& name, bool runtime_checkable = false)
        : ClassType(name), is_runtime_checkable(runtime_checkable) {}
    
    Kind getKind() const override { return Kind::PROTOCOL; }
    static Kind staticKind() { return Kind::PROTOCOL; }
    
    void addRequiredMethod(const std::string& name, std::shared_ptr<FunctionType> method) {
        required_methods[name] = std::move(method);
    }
    
    void addRequiredAttribute(const std::string& name, std::shared_ptr<Type> type) {
        required_attributes[name] = std::move(type);
    }
    
    bool isRuntimeCheckable() const { return is_runtime_checkable; }
    const std::unordered_map<std::string, std::shared_ptr<FunctionType>>& getRequiredMethods() const {
        return required_methods;
    }
    const std::unordered_map<std::string, std::shared_ptr<Type>>& getRequiredAttributes() const {
        return required_attributes;
    }
};

// TypedDict type
class TypedDictType : public ClassType {
private:
    std::unordered_map<std::string, std::shared_ptr<Type>> fields;
    std::unordered_set<std::string> required_fields;
    bool total; // Whether all fields are required by default
    
public:
    TypedDictType(const std::string& name, bool total = true)
        : ClassType(name), total(total) {}
    
    Kind getKind() const override { return Kind::TYPEDDICT; }
    static Kind staticKind() { return Kind::TYPEDDICT; }
    
    void addField(const std::string& name, std::shared_ptr<Type> type, bool required = true) {
        fields[name] = std::move(type);
        if (required) {
            required_fields.insert(name);
        }
    }
    
    void setFieldOptional(const std::string& name) {
        required_fields.erase(name);
    }
    
    bool isFieldRequired(const std::string& name) const {
        return required_fields.find(name) != required_fields.end();
    }
    
    const std::unordered_map<std::string, std::shared_ptr<Type>>& getFields() const {
        return fields;
    }
    
    const std::unordered_set<std::string>& getRequiredFields() const {
        return required_fields;
    }
    
    bool isTotal() const { return total; }
};

// Literal type
class LiteralType : public Type {
private:
    std::vector<std::variant<int, double, std::string, bool, std::nullptr_t>> values;
    
public:
    explicit LiteralType(const std::vector<std::variant<int, double, std::string, bool, std::nullptr_t>>& values)
        : values(values) {}
    
    Kind getKind() const override { return Kind::LITERAL; }
    static Kind staticKind() { return Kind::LITERAL; }
    
    std::string toString() const override {
        std::string result = "Literal[";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) result += ", ";
            std::visit([&result](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, int>) {
                    result += std::to_string(value);
                } else if constexpr (std::is_same_v<T, double>) {
                    result += std::to_string(value);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    result += "\"" + value + "\"";
                } else if constexpr (std::is_same_v<T, bool>) {
                    result += value ? "True" : "False";
                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    result += "None";
                }
            }, values[i]);
        }
        result += "]";
        return result;
    }
    
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::LITERAL) return false;
        const auto& other_literal = static_cast<const LiteralType&>(other);
        if (values.size() != other_literal.values.size()) return false;
        for (size_t i = 0; i < values.size(); ++i) {
            // Compare variants
            if (values[i].index() != other_literal.values[i].index()) return false;
            // Add actual value comparison based on type
        }
        return true;
    }
    
    const std::vector<std::variant<int, double, std::string, bool, std::nullptr_t>>& getValues() const {
        return values;
    }
};

// Type hint parser
class TypeHintParser {
private:
    AdvancedTypeSystem& type_system;
    std::string input;
    size_t position;
    
    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    
    std::shared_ptr<Type> parseType();
    std::shared_ptr<Type> parseUnionType();
    std::shared_ptr<Type> parseIntersectionType();
    std::shared_ptr<Type> parsePrimaryType();
    std::shared_ptr<Type> parseGenericType();
    std::shared_ptr<Type> parseCallableType();
    std::shared_ptr<Type> parseTupleType();
    std::shared_ptr<Type> parseLiteralType();
    std::string parseIdentifier();
    std::vector<std::shared_ptr<Type>> parseTypeList();
    
public:
    explicit TypeHintParser(AdvancedTypeSystem& type_system) : type_system(type_system) {}
    
    std::shared_ptr<Type> parse(const std::string& type_hint);
};

// Type checker with advanced features
class AdvancedTypeChecker {
private:
    AdvancedTypeSystem& type_system;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Type narrowing context
    std::unordered_map<std::string, std::shared_ptr<Type>> narrowed_types;
    
    // Generic instantiation context
    std::unordered_map<std::string, std::shared_ptr<Type>> type_bindings;
    
    bool checkTypeCompatibility(std::shared_ptr<Type> source, std::shared_ptr<Type> target,
                               const std::string& context, int line, int column);
    bool checkGenericConstraints(std::shared_ptr<ClassType> generic_type,
                                const std::vector<std::shared_ptr<Type>>& type_args);
    bool checkProtocolImplementation(const ClassType* class_type, const ProtocolType* protocol);
    
public:
    explicit AdvancedTypeChecker(AdvancedTypeSystem& type_system) : type_system(type_system) {}
    
    bool checkModule(Module& module);
    const std::vector<std::string>& getErrors() const { return errors; }
    const std::vector<std::string>& getWarnings() const { return warnings; }
    
    // Type narrowing support
    void narrowType(const std::string& variable_name, std::shared_ptr<Type> narrowed_type);
    void resetNarrowing();
    
    // Generic instantiation support
    void bindTypeParameter(const std::string& param_name, std::shared_ptr<Type> type);
    void unbindTypeParameter(const std::string& param_name);
    std::shared_ptr<Type> resolveTypeBinding(const std::string& param_name);
};

} // namespace pyplusplus