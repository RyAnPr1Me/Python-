#include "advanced_types.h"
#include <sstream>
#include <algorithm>
#include <regex>

namespace pyplusplus {

AdvancedTypeSystem::AdvancedTypeSystem() : TypeSystem() {
    // Initialize built-in generic types and protocols
}

std::shared_ptr<Type> AdvancedTypeSystem::parseTypeHint(const std::string& hint) {
    TypeHintParser parser(*this);
    return parser.parse(hint);
}

std::shared_ptr<Type> AdvancedTypeSystem::parseGenericType(const std::string& name, 
                                                          const std::vector<std::shared_ptr<Type>>& args) {
    // Handle built-in generic types
    if (name == "list" && args.size() == 1) {
        return createListType(args[0]);
    } else if (name == "dict" && args.size() == 2) {
        return createDictType(args[0], args[1]);
    } else if (name == "tuple") {
        // Handle tuple types
        if (args.empty()) {
            return createListType(getAnyType()); // Empty tuple
        }
        // For now, treat as list of first type
        return createListType(args[0]);
    } else if (name == "set" && args.size() == 1) {
        return createListType(args[0]); // Simplified: treat set as list
    } else if (name == "frozenset" && args.size() == 1) {
        return createListType(args[0]); // Simplified: treat frozenset as list
    }
    
    // Check for user-defined generic types
    auto it = generic_types.find(name);
    if (it != generic_types.end()) {
        return instantiateGeneric(it->second, args);
    }
    
    return getUnknownType();
}

std::shared_ptr<ClassType> AdvancedTypeSystem::createGenericType(const std::string& name, 
                                                                  const std::vector<std::string>& type_params) {
    auto generic_type = std::make_shared<ClassType>(name);
    
    // Create type variables for parameters
    for (const auto& param : type_params) {
        auto type_var = createTypeVariable(param);
        this->type_params[param] = type_var;
        generic_type->addAttribute(param, type_var);
    }
    
    generic_types[name] = generic_type;
    return generic_type;
}

std::shared_ptr<Type> AdvancedTypeSystem::instantiateGeneric(std::shared_ptr<ClassType> generic_type,
                                                             const std::vector<std::shared_ptr<Type>>& type_args) {
    // Create a new class type with instantiated type arguments
    auto instantiated = std::make_shared<ClassType>(generic_type->getName() + "<instantiated>");
    
    // Copy base classes and methods
    // For now, just return the generic type as-is
    return generic_type;
}

std::shared_ptr<ClassType> AdvancedTypeSystem::createProtocol(const std::string& name) {
    auto protocol = std::make_shared<ProtocolType>(name);
    protocols[name] = protocol;
    return protocol;
}

bool AdvancedTypeSystem::isSubtypeOfProtocol(const Type* type, const ClassType* protocol) {
    if (!protocol || protocol->getKind() != Type::Kind::PROTOCOL) {
        return false;
    }
    
    const auto* protocol_type = static_cast<const ProtocolType*>(protocol);
    return matchesProtocol(type, protocol_type);
}

std::shared_ptr<ClassType> AdvancedTypeSystem::createTypedDict(const std::string& name,
                                                               const std::vector<std::pair<std::string, std::shared_ptr<Type>>>& fields,
                                                               const std::vector<std::string>& required_fields) {
    auto typed_dict = std::make_shared<TypedDictType>(name);
    
    for (const auto& [field_name, field_type] : fields) {
        bool is_required = std::find(required_fields.begin(), required_fields.end(), field_name) != required_fields.end();
        typed_dict->addField(field_name, field_type, is_required);
    }
    
    typed_dicts[name] = typed_dict;
    return typed_dict;
}

std::shared_ptr<Type> AdvancedTypeSystem::createLiteralType(const std::vector<std::variant<int, std::string, bool>>& values) {
    std::vector<std::variant<int, double, std::string, bool, std::nullptr_t>> literal_values;
    for (const auto& value : values) {
        if (std::holds_alternative<int>(value)) {
            literal_values.push_back(std::get<int>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            literal_values.push_back(std::get<std::string>(value));
        } else if (std::holds_alternative<bool>(value)) {
            literal_values.push_back(std::get<bool>(value));
        }
    }
    return std::make_shared<LiteralType>(literal_values);
}

void AdvancedTypeSystem::registerTypeAlias(const std::string& name, std::shared_ptr<Type> type) {
    type_aliases[name] = std::move(type);
}

std::shared_ptr<Type> AdvancedTypeSystem::resolveTypeAlias(const std::string& name) {
    auto it = type_aliases.find(name);
    return it != type_aliases.end() ? it->second : nullptr;
}

bool AdvancedTypeSystem::isStructuralSubtype(const Type* subtype, const Type* supertype) {
    if (!subtype || !supertype) return false;
    
    // Handle protocol checking
    if (supertype->getKind() == Type::Kind::PROTOCOL) {
        return matchesProtocol(subtype, static_cast<const ProtocolType*>(supertype));
    }
    
    // Handle TypedDict checking
    if (supertype->getKind() == Type::Kind::TYPEDDICT) {
        if (subtype->getKind() != Type::Kind::TYPEDDICT) return false;
        // Check field compatibility
        return true; // Simplified
    }
    
    // Default to nominal subtyping
    return isSubtype(*subtype, *supertype);
}

bool AdvancedTypeSystem::matchesProtocol(const Type* type, const ProtocolType* protocol) {
    if (!type || !protocol) return false;
    
    // Check if type has all required methods
    for (const auto& [method_name, method_type] : protocol->getRequiredMethods()) {
        if (type->getKind() == Type::Kind::CLASS) {
            const auto* class_type = static_cast<const ClassType*>(type);
            const auto* method = class_type->getMethod(method_name);
            if (!method || !method->equals(*method_type)) {
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Check if type has all required attributes
    for (const auto& [attr_name, attr_type] : protocol->getRequiredAttributes()) {
        if (type->getKind() == Type::Kind::CLASS) {
            const auto* class_type = static_cast<const ClassType*>(type);
            const auto* attr = class_type->getAttributeType(attr_name);
            if (!attr || !attr->equals(*attr_type)) {
                return false;
            }
        } else {
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<Type> AdvancedTypeSystem::inferReturnType(const FunctionType* func_type, 
                                                          const std::vector<std::shared_ptr<Type>>& arg_types) {
    if (!func_type) return getUnknownType();
    
    // Simple return type inference based on parameter types
    // For now, just return the declared return type
    return std::shared_ptr<Type>(const_cast<Type*>(func_type->getReturnType()), [](Type*){});
}

std::shared_ptr<Type> AdvancedTypeSystem::inferGeneratorType(std::shared_ptr<Type> yield_type) {
    if (!yield_type) return getUnknownType();
    
    // Generator[YieldType, SendType, ReturnType]
    // For now, just create Iterator[YieldType]
    auto iterator_class = createClass("Iterator");
    return iterator_class;
}

std::shared_ptr<Type> AdvancedTypeSystem::inferAsyncType(std::shared_ptr<Type> return_type) {
    if (!return_type) return getUnknownType();
    
    // Coroutine[ReturnType, SendType, ReturnType]
    // For now, just wrap in Awaitable
    auto awaitable_class = createClass("Awaitable");
    return awaitable_class;
}

std::shared_ptr<Type> AdvancedTypeSystem::createIntersectionType(const std::vector<std::shared_ptr<Type>>& types) {
    if (types.empty()) return getAnyType();
    if (types.size() == 1) return types[0];
    
    return std::make_shared<IntersectionType>(types);
}

bool AdvancedTypeSystem::isIntersectionSubtype(const Type* type, const IntersectionType* intersection) {
    if (!type || !intersection) return false;
    
    // Type is subtype of intersection if it's subtype of all types in the intersection
    for (const auto& intersection_type : intersection->getTypes()) {
        if (!isSubtype(*type, *intersection_type)) {
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<Type> AdvancedTypeSystem::createCallableType(const std::vector<std::shared_ptr<Type>>& param_types,
                                                             std::shared_ptr<Type> return_type,
                                                             const std::vector<std::string>& param_names,
                                                             const std::vector<bool>& param_defaults) {
    return createFunctionType(param_types, return_type);
}

std::shared_ptr<Type> AdvancedTypeSystem::narrowType(std::shared_ptr<Type> type, 
                                                      const std::string& condition) {
    if (!type) return getUnknownType();
    
    // Type narrowing based on conditions like isinstance() checks
    // For now, return the original type
    return type;
}

bool AdvancedTypeSystem::isTypeCompatible(const Type* source, const Type* target, bool allow_coercion) {
    if (!source || !target) return false;
    
    if (source->equals(*target)) return true;
    
    if (allow_coercion) {
        return isSubtype(*source, *target);
    }
    
    return false;
}

std::shared_ptr<Type> AdvancedTypeSystem::getCommonType(const std::vector<std::shared_ptr<Type>>& types) {
    if (types.empty()) return getUnknownType();
    if (types.size() == 1) return types[0];
    
    std::shared_ptr<Type> common = types[0];
    for (size_t i = 1; i < types.size(); ++i) {
        common = join(*common, *types[i]);
        if (common->getKind() == Type::Kind::ANY) {
            break; // Can't get more specific than Any
        }
    }
    
    return common;
}

bool AdvancedTypeSystem::validateTypeHint(std::shared_ptr<Type> type, std::vector<std::string>& errors) {
    if (!type) {
        errors.push_back("Type hint is null");
        return false;
    }
    
    // Basic validation - check for recursive types, invalid combinations, etc.
    return true;
}

bool AdvancedTypeSystem::validateGenericInstantiation(std::shared_ptr<ClassType> generic_type,
                                                       const std::vector<std::shared_ptr<Type>>& type_args,
                                                       std::vector<std::string>& errors) {
    if (!generic_type) {
        errors.push_back("Generic type is null");
        return false;
    }
    
    // Validate that the number of type arguments matches the number of type parameters
    // For now, just return true
    return true;
}

// TypeHintParser implementation
char TypeHintParser::peek() const {
    return position < input.length() ? input[position] : '\0';
}

char TypeHintParser::advance() {
    return position < input.length() ? input[position++] : '\0';
}

bool TypeHintParser::match(char expected) {
    if (peek() == expected) {
        advance();
        return true;
    }
    return false;
}

void TypeHintParser::skipWhitespace() {
    while (isspace(peek())) advance();
}

std::shared_ptr<Type> TypeHintParser::parse(const std::string& type_hint) {
    input = type_hint;
    position = 0;
    return parseType();
}

std::shared_ptr<Type> TypeHintParser::parseType() {
    skipWhitespace();
    return parseUnionType();
}

std::shared_ptr<Type> TypeHintParser::parseUnionType() {
    auto type = parseIntersectionType();
    
    skipWhitespace();
    if (match('|')) {
        std::vector<std::shared_ptr<Type>> union_types;
        union_types.push_back(type);
        
        do {
            skipWhitespace();
            union_types.push_back(parseIntersectionType());
            skipWhitespace();
        } while (match('|'));
        
        return type_system.createUnionType(std::move(union_types));
    }
    
    return type;
}

std::shared_ptr<Type> TypeHintParser::parseIntersectionType() {
    auto type = parsePrimaryType();
    
    // For now, skip intersection parsing (would use '&' operator)
    return type;
}

std::shared_ptr<Type> TypeHintParser::parsePrimaryType() {
    skipWhitespace();
    
    if (match('(')) {
        auto type = parseType();
        skipWhitespace();
        if (!match(')')) {
            return type_system.getUnknownType(); // Error
        }
        return type;
    }
    
    // Check for literals
    if (input.substr(position, 7) == "Literal") {
        position += 7;
        return parseLiteralType();
    }
    
    // Check for Callable
    if (input.substr(position, 8) == "Callable") {
        position += 8;
        return parseCallableType();
    }
    
    // Check for Tuple
    if (input.substr(position, 5) == "Tuple") {
        position += 5;
        return parseTupleType();
    }
    
    // Check for generic types
    auto identifier = parseIdentifier();
    skipWhitespace();
    
    if (match('[')) {
        auto type_args = parseTypeList();
        skipWhitespace();
        if (!match(']')) {
            return type_system.getUnknownType(); // Error
        }
        return type_system.parseGenericType(identifier, type_args);
    }
    
    // Check for built-in types
    if (identifier == "int") return type_system.getIntType();
    if (identifier == "float") return type_system.getFloatType();
    if (identifier == "str") return type_system.getStringType();
    if (identifier == "bool") return type_system.getBoolType();
    if (identifier == "None") return type_system.getNoneType();
    if (identifier == "Any") return type_system.getAnyType();
    
    // Check for type aliases
    auto alias = type_system.resolveTypeAlias(identifier);
    if (alias) return alias;
    
    // Default to unknown
    return type_system.getUnknownType();
}

std::shared_ptr<Type> TypeHintParser::parseLiteralType() {
    skipWhitespace();
    if (!match('[')) {
        return type_system.getUnknownType();
    }
    
    std::vector<std::variant<int, std::string, bool>> values;
    
    do {
        skipWhitespace();
        
        // Parse literal values
        if (isdigit(peek()) || peek() == '-') {
            // Number literal
            std::string num_str;
            while (isdigit(peek()) || peek() == '-' || peek() == '.') {
                num_str += advance();
            }
            try {
                int value = std::stoi(num_str);
                values.push_back(value);
            } catch (...) {
                // Try float
                try {
                    double value = std::stod(num_str);
                    // For now, skip float literals
                } catch (...) {
                    return type_system.getUnknownType();
                }
            }
        } else if (match('"')) {
            // String literal
            std::string str_value;
            while (peek() != '"' && peek() != '\0') {
                str_value += advance();
            }
            if (!match('"')) {
                return type_system.getUnknownType();
            }
            values.push_back(str_value);
        } else if (input.substr(position, 4) == "True") {
            position += 4;
            values.push_back(true);
        } else if (input.substr(position, 5) == "False") {
            position += 5;
            values.push_back(false);
        }
        
        skipWhitespace();
    } while (match(','));
    
    skipWhitespace();
    if (!match(']')) {
        return type_system.getUnknownType();
    }
    
    return type_system.createLiteralType(values);
}

std::shared_ptr<Type> TypeHintParser::parseCallableType() {
    skipWhitespace();
    if (!match('[')) {
        return type_system.getUnknownType();
    }
    
    // For now, just return Any
    skipWhitespace();
    if (!match(']')) {
        return type_system.getUnknownType();
    }
    
    return type_system.getAnyType();
}

std::shared_ptr<Type> TypeHintParser::parseTupleType() {
    skipWhitespace();
    if (!match('[')) {
        return type_system.getUnknownType();
    }
    
    auto type_args = parseTypeList();
    skipWhitespace();
    if (!match(']')) {
        return type_system.getUnknownType();
    }
    
    // For now, treat tuple as list of first type
    if (type_args.empty()) {
        return type_system.createListType(type_system.getAnyType());
    }
    return type_system.createListType(type_args[0]);
}

std::string TypeHintParser::parseIdentifier() {
    skipWhitespace();
    std::string identifier;
    while (isalnum(peek()) || peek() == '_') {
        identifier += advance();
    }
    return identifier;
}

std::vector<std::shared_ptr<Type>> TypeHintParser::parseTypeList() {
    std::vector<std::shared_ptr<Type>> types;
    
    do {
        skipWhitespace();
        types.push_back(parseType());
        skipWhitespace();
    } while (match(','));
    
    return types;
}

// AdvancedTypeChecker implementation
bool AdvancedTypeChecker::checkModule(Module& module) {
    errors.clear();
    warnings.clear();
    
    // For now, just return true
    // In a full implementation, this would walk the AST and check all type annotations
    return errors.empty();
}

void AdvancedTypeChecker::narrowType(const std::string& variable_name, std::shared_ptr<Type> narrowed_type) {
    narrowed_types[variable_name] = narrowed_type;
}

void AdvancedTypeChecker::resetNarrowing() {
    narrowed_types.clear();
}

void AdvancedTypeChecker::bindTypeParameter(const std::string& param_name, std::shared_ptr<Type> type) {
    type_bindings[param_name] = type;
}

void AdvancedTypeChecker::unbindTypeParameter(const std::string& param_name) {
    type_bindings.erase(param_name);
}

std::shared_ptr<Type> AdvancedTypeChecker::resolveTypeBinding(const std::string& param_name) {
    auto it = type_bindings.find(param_name);
    return it != type_bindings.end() ? it->second : nullptr;
}

bool AdvancedTypeChecker::checkTypeCompatibility(std::shared_ptr<Type> source, std::shared_ptr<Type> target,
                                                  const std::string& context, int line, int column) {
    if (!source || !target) return false;
    
    if (!type_system.isTypeCompatible(source.get(), target.get())) {
        std::ostringstream oss;
        oss << "Type mismatch in " << context << " at line " << line << ", column " << column 
            << ": expected " << target->toString() << ", got " << source->toString();
        errors.push_back(oss.str());
        return false;
    }
    
    return true;
}

bool AdvancedTypeChecker::checkGenericConstraints(std::shared_ptr<ClassType> generic_type,
                                                  const std::vector<std::shared_ptr<Type>>& type_args) {
    std::vector<std::string> errors;
    return type_system.validateGenericInstantiation(generic_type, type_args, errors);
}

bool AdvancedTypeChecker::checkProtocolImplementation(const ClassType* class_type, const ProtocolType* protocol) {
    return type_system.matchesProtocol(class_type, protocol);
}

} // namespace pyplusplus