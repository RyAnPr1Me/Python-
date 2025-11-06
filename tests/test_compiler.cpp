#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "type_inference.h"
#include "ir.h"
#include "runtime.h"
#include <memory>

using namespace pyplusplus;

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
};

TEST_F(LexerTest, BasicTokens) {
    Lexer lexer("1 + 2 * 3");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].value, "1");
    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[2].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[2].value, "2");
    EXPECT_EQ(tokens[3].type, TokenType::MULTIPLY);
    EXPECT_EQ(tokens[4].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[4].value, "3");
    EXPECT_EQ(tokens[5].type, TokenType::NEWLINE);
    EXPECT_EQ(tokens[6].type, TokenType::EOF_TOKEN);
}

TEST_F(LexerTest, StringLiterals) {
    Lexer lexer("\"hello\" 'world'");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].value, "hello");
    EXPECT_EQ(tokens[1].type, TokenType::STRING);
    EXPECT_EQ(tokens[1].value, "world");
}

TEST_F(LexerTest, Keywords) {
    Lexer lexer("if else for while def return");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 11);
    EXPECT_EQ(tokens[0].type, TokenType::IF);
    EXPECT_EQ(tokens[1].type, TokenType::ELSE);
    EXPECT_EQ(tokens[2].type, TokenType::FOR);
    EXPECT_EQ(tokens[3].type, TokenType::WHILE);
    EXPECT_EQ(tokens[4].type, TokenType::DEF);
    EXPECT_EQ(tokens[5].type, TokenType::RETURN);
}

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
};

TEST_F(ParserTest, SimpleExpression) {
    Lexer lexer("1 + 2 * 3");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto module = parser.parse();
    
    ASSERT_NE(module, nullptr);
    ASSERT_EQ(module->getStatements().size(), 1);
    
    auto stmt = dynamic_cast<ExpressionStatement*>(module->getStatements()[0].get());
    ASSERT_NE(stmt, nullptr);
    
    auto expr = dynamic_cast<BinaryExpression*>(stmt->getExpression());
    ASSERT_NE(expr, nullptr);
    EXPECT_EQ(expr->getOperator(), TokenType::PLUS);
    
    auto left = dynamic_cast<LiteralExpression*>(expr->getLeft());
    ASSERT_NE(left, nullptr);
    EXPECT_TRUE(left->isInt());
    EXPECT_EQ(left->asInt(), 1);
    
    auto right = dynamic_cast<BinaryExpression*>(expr->getRight());
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->getOperator(), TokenType::MULTIPLY);
}

TEST_F(ParserTest, FunctionDefinition) {
    Lexer lexer("def add(a, b):\n    return a + b");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto module = parser.parse();
    
    ASSERT_NE(module, nullptr);
    ASSERT_EQ(module->getStatements().size(), 1);
    
    auto func_def = dynamic_cast<FunctionDef*>(module->getStatements()[0].get());
    ASSERT_NE(func_def, nullptr);
    EXPECT_EQ(func_def->getName(), "add");
    EXPECT_EQ(func_def->getParameters().size(), 2);
    EXPECT_EQ(func_def->getParameters()[0].first, "a");
    EXPECT_EQ(func_def->getParameters()[1].first, "b");
}

class TypeInferenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
        type_system = std::make_unique<TypeSystem>();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
    
    std::unique_ptr<TypeSystem> type_system;
};

TEST_F(TypeInferenceTest, BasicTypes) {
    Lexer lexer("x = 42\ny = 3.14\nz = \"hello\"");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto module = parser.parse();
    
    ASSERT_NE(module, nullptr);
    
    TypeInference inference(*type_system);
    bool success = inference.infer(*module);
    EXPECT_TRUE(success);
    
    // Check variable types
    auto x_type = inference.getVariableType("x");
    EXPECT_TRUE(x_type->is(*type_system->getIntType()));
    
    auto y_type = inference.getVariableType("y");
    EXPECT_TRUE(y_type->is(*type_system->getFloatType()));
    
    auto z_type = inference.getVariableType("z");
    EXPECT_TRUE(z_type->is(*type_system->getStringType()));
}

class RuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
};

TEST_F(RuntimeTest, BasicObjects) {
    // Test integer creation
    auto int_obj = PyInt::create(42);
    ASSERT_NE(int_obj, nullptr);
    EXPECT_TRUE(int_obj->isInt());
    EXPECT_EQ(int_obj->asInt(), 42);
    EXPECT_EQ(int_obj->repr(), "42");
    
    // Test float creation
    auto float_obj = PyFloat::create(3.14);
    ASSERT_NE(float_obj, nullptr);
    EXPECT_TRUE(float_obj->isFloat());
    EXPECT_DOUBLE_EQ(float_obj->asFloat(), 3.14);
    
    // Test string creation
    auto str_obj = PyString::create("hello");
    ASSERT_NE(str_obj, nullptr);
    EXPECT_TRUE(str_obj->isString());
    EXPECT_EQ(str_obj->getValue(), "hello");
    EXPECT_EQ(str_obj->repr(), "\"hello\"");
    
    // Test boolean creation
    auto true_obj = PyBool::getInstance(true);
    auto false_obj = PyBool::getInstance(false);
    ASSERT_NE(true_obj, nullptr);
    ASSERT_NE(false_obj, nullptr);
    EXPECT_TRUE(true_obj->isBool());
    EXPECT_TRUE(true_obj->asBool());
    EXPECT_FALSE(false_obj->asBool());
    
    // Test list creation
    auto list_obj = PyList::create();
    ASSERT_NE(list_obj, nullptr);
    EXPECT_TRUE(list_obj->isList());
    EXPECT_EQ(list_obj->size(), 0);
    
    list_obj->append(PyInt::create(1));
    list_obj->append(PyInt::create(2));
    EXPECT_EQ(list_obj->size(), 2);
    
    // Test dict creation
    auto dict_obj = PyDict::create();
    ASSERT_NE(dict_obj, nullptr);
    EXPECT_TRUE(dict_obj->isDict());
    EXPECT_EQ(dict_obj->size(), 0);
    
    dict_obj->set("key", PyString::create("value"));
    EXPECT_EQ(dict_obj->size(), 1);
    EXPECT_NE(dict_obj->get("key"), nullptr);
}

TEST_F(RuntimeTest, ReferenceCounting) {
    auto obj = PyInt::create(42);
    EXPECT_EQ(obj->getRefCount(), 1);
    
    obj->ref();
    EXPECT_EQ(obj->getRefCount(), 2);
    
    obj->unref();
    EXPECT_EQ(obj->getRefCount(), 1);
}

class IRTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
        type_system = std::make_unique<TypeSystem>();
        module = std::make_unique<IRModule>("test");
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
    
    std::unique_ptr<TypeSystem> type_system;
    std::unique_ptr<IRModule> module;
};

TEST_F(IRTest, BasicFunction) {
    // Create a simple function: int add(int a, int b) { return a + b; }
    auto func_type = type_system->createFunctionType(
        {type_system->getIntType(), type_system->getIntType()},
        type_system->getIntType()
    );
    
    auto func = module->createFunction("add", func_type);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getName(), "add");
    
    auto entry_block = func->createBasicBlock("entry");
    ASSERT_NE(entry_block, nullptr);
    
    IRBuilder builder(func.get(), entry_block.get());
    
    // Create parameters
    auto param_a = std::make_shared<IRParameter>(0, type_system->getIntType(), "a");
    auto param_b = std::make_shared<IRParameter>(1, type_system->getIntType(), "b");
    
    // Create binary operation
    auto add_result = builder.createBinaryOp(IROp::ADD, param_a, param_b);
    ASSERT_NE(add_result, nullptr);
    
    // Create return
    builder.createReturn(add_result);
    
    // Verify the function structure
    EXPECT_EQ(func->getBasicBlocks().size(), 1);
    EXPECT_EQ(func->getBasicBlocks()[0]->getInstructions().size(), 2);
}

// Integration test
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
};

TEST_F(IntegrationTest, SimpleCompilation) {
    // Test the full compilation pipeline with a simple program
    std::string source = "x = 42\nprint(x)\n";
    
    // Lexical analysis
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    EXPECT_GT(tokens.size(), 0);
    
    // Parsing
    Parser parser(tokens);
    auto module = parser.parse();
    ASSERT_NE(module, nullptr);
    
    // Type inference
    TypeSystem type_system;
    TypeInference inference(type_system);
    bool inference_success = inference.infer(*module);
    EXPECT_TRUE(inference_success);
    
    // Type checking
    TypeChecker checker(type_system, inference);
    bool check_success = checker.check(*module);
    EXPECT_TRUE(check_success);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}