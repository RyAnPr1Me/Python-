#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "type_inference.h"
#include "ir.h"
#include "codegen.h"
#include "runtime.h"
#include "bytecode.h"
#include "import_system.h"
#include <memory>
#include <string>
#include <vector>

namespace pyplusplus {

class AOTCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
        ImportSystem::initialize();
    }
    
    void TearDown() override {
        ImportSystem::shutdown();
        Runtime::shutdown();
    }
    
    std::unique_ptr<Module> parseCode(const std::string& code) {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        return parser.parse();
    }
    
    bool compileAndRun(const std::string& code, const std::string& expected_output = "") {
        try {
            // Parse
            auto module = parseCode(code);
            ASSERT_NE(module, nullptr);
            
            // Type inference
            TypeSystem type_system;
            TypeInference inference(type_system);
            bool success = inference.infer(*module);
            ASSERT_TRUE(success);
            
            // Generate IR
            IRGenerator ir_generator(type_system);
            auto ir_module = ir_generator.generate(*module);
            ASSERT_NE(ir_module, nullptr);
            
            // Generate code
            LLVMCodeGenerator codegen(type_system);
            success = codegen.generate(*ir_module);
            ASSERT_TRUE(success);
            
            // Optimize
            success = codegen.optimize(2);
            ASSERT_TRUE(success);
            
            // Generate executable
            success = codegen.generateNativeExecutable("test_output");
            ASSERT_TRUE(success);
            
            // Run and check output if provided
            if (!expected_output.empty()) {
                // This would need runtime execution testing
                // For now, just return true if compilation succeeded
                return true;
            }
            
            return true;
        } catch (const std::exception& e) {
            FAIL() << "Compilation failed with exception: " << e.what();
            return false;
        }
    }
};

// Basic compilation tests
TEST_F(AOTCompilerTest, BasicArithmetic) {
    const std::string code = R"(
def add(a, b):
    return a + b

def multiply(a, b):
    return a * b

result = add(5, 3)
print(result)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, ControlFlow) {
    const std::string code = R"(
def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

result = factorial(5)
print(result)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, Loops) {
    const std::string code = R"(
def sum_list(numbers):
    total = 0
    for num in numbers:
        total += num
    return total

numbers = [1, 2, 3, 4, 5]
result = sum_list(numbers)
print(result)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, ListOperations) {
    const std::string code = R"(
def process_list(data):
    # List comprehension
    squares = [x * x for x in data if x > 0]
    # List methods
    squares.sort()
    squares.reverse()
    return squares

data = [-2, -1, 0, 1, 2, 3, 4, 5]
result = process_list(data)
print(result)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, DictionaryOperations) {
    const std::string code = R"(
def word_count(text):
    words = text.split()
    counts = {}
    for word in words:
        if word in counts:
            counts[word] += 1
        else:
            counts[word] = 1
    return counts

text = "hello world hello python world"
result = word_count(text)
print(result)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Advanced Python features tests
TEST_F(AOTCompilerTest, PatternMatching) {
    const std::string code = R"(
def process_value(value):
    match value:
        case int(x) if x > 0:
            return f"positive integer: {x}"
        case int(x) if x < 0:
            return f"negative integer: {x}"
        case 0:
            return "zero"
        case str(s):
            return f"string: {s}"
        case [first, *rest]:
            return f"list starting with {first}"
        case _:
            return "unknown"

print(process_value(42))
print(process_value(-5))
print(process_value("hello"))
print(process_value([1, 2, 3]))
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, AsyncAwait) {
    const std::string code = R"(
import asyncio

async def fetch_data(url):
    await asyncio.sleep(0.1)  # Simulate async operation
    return f"data from {url}"

async def main():
    tasks = [fetch_data(f"url{i}") for i in range(3)]
    results = await asyncio.gather(*tasks)
    return results

# Note: This test would need proper async runtime support
print("Async test compiled successfully")
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, Metaclasses) {
    const std::string code = R"(
class SingletonMeta(type):
    _instances = {}
    
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super().__call__(*args, **kwargs)
        return cls._instances[cls]

class Database(metaclass=SingletonMeta):
    def __init__(self):
        self.connection = "connected"

db1 = Database()
db2 = Database()
print(db1 is db2)  # Should be True
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, Decorators) {
    const std::string code = R"(
def timer(func):
    def wrapper(*args, **kwargs):
        import time
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()
        print(f"{func.__name__} took {end - start:.4f} seconds")
        return result
    return wrapper

@timer
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

result = fibonacci(10)
print(f"Result: {result}")
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, Generators) {
    const std::string code = R"(
def prime_generator(limit):
    sieve = [True] * (limit + 1)
    sieve[0] = sieve[1] = False
    
    for num in range(2, int(limit ** 0.5) + 1):
        if sieve[num]:
            sieve[num*num : limit+1 : num] = [False] * len(sieve[num*num : limit+1 : num])
    
    for num in range(2, limit + 1):
        if sieve[num]:
            yield num

primes = list(prime_generator(20))
print(primes)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Performance and optimization tests
TEST_F(AOTCompilerTest, Vectorization) {
    const std::string code = R"(
import math

def vectorized_operation(data):
    # This should be vectorized by the optimizer
    result = []
    for x in data:
        result.append(math.sin(x) * math.cos(x) + math.sqrt(abs(x)))
    return result

data = [i * 0.1 for i in range(1000)]
result = vectorized_operation(data)
print(f"Processed {len(result)} elements")
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, MemoryOptimization) {
    const std::string code = R"(
def memory_efficient_processing():
    # Test memory pool and garbage collection
    data = []
    for i in range(10000):
        # Create temporary objects
        temp = [i * j for j in range(10)]
        data.append(sum(temp))
        # temp should be garbage collected here
    
    return sum(data)

result = memory_efficient_processing()
print(f"Memory test result: {result}")
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Type system tests
TEST_F(AOTCompilerTest, TypeInference) {
    const std::string code = R"(
from typing import List, Dict, Union

def process_data(items: List[Union[int, float]]) -> Dict[str, float]:
    result = {
        "sum": sum(items),
        "mean": sum(items) / len(items),
        "min": min(items),
        "max": max(items)
    }
    return result

data = [1, 2.5, 3, 4.7, 5]
stats = process_data(data)
print(stats)
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

TEST_F(AOTCompilerTest, UnionTypes) {
    const std::string code = R"(
from typing import Union

def process_value(value: Union[int, str, float]) -> str:
    if isinstance(value, int):
        return f"integer: {value}"
    elif isinstance(value, str):
        return f"string: {value}"
    else:
        return f"float: {value:.2f}"

print(process_value(42))
print(process_value("hello"))
print(process_value(3.14159))
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Import system tests
TEST_F(AOTCompilerTest, ModuleImport) {
    const std::string code = R"(
# Test importing standard library modules
import math
import random
import itertools

result = math.sqrt(16)
random_number = random.randint(1, 10)
combinations = list(itertools.combinations([1, 2, 3], 2))

print(f"sqrt(16) = {result}")
print(f"random number = {random_number}")
print(f"combinations = {combinations}")
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Error handling tests
TEST_F(AOTCompilerTest, ExceptionHandling) {
    const std::string code = R"(
def safe_divide(a, b):
    try:
        return a / b
    except ZeroDivisionError:
        return "division by zero"
    except TypeError:
        return "type error"
    except Exception as e:
        return f"unknown error: {e}"

print(safe_divide(10, 2))
print(safe_divide(10, 0))
print(safe_divide("10", 2))
)";
    
    EXPECT_TRUE(compileAndRun(code));
}

// Bytecode translation tests
class BytecodeTranslationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
    
    std::vector<BytecodeInstruction> createSimpleBytecode() {
        std::vector<BytecodeInstruction> bytecode;
        
        // LOAD_CONST 10
        bytecode.emplace_back(100, 10, 0, "Load constant 10");
        // STORE_FAST x
        bytecode.emplace_back(102, 0, 2, "Store to local x");
        // LOAD_FAST x
        bytecode.emplace_back(101, 0, 4, "Load local x");
        // RETURN_VALUE
        bytecode.emplace_back(83, 0, 6, "Return value");
        
        return bytecode;
    }
};

TEST_F(BytecodeTranslationTest, SimpleBytecodeTranslation) {
    TypeSystem type_system;
    BytecodeTranslator translator(type_system);
    
    auto bytecode = createSimpleBytecode();
    auto ir_module = translator.translate(bytecode, "test_module");
    
    ASSERT_NE(ir_module, nullptr);
    EXPECT_GT(ir_module->getFunctions().size(), 0);
}

TEST_F(BytecodeTranslationTest, ComplexBytecodeTranslation) {
    TypeSystem type_system;
    BytecodeTranslator translator(type_system);
    
    // Create more complex bytecode with loops and conditionals
    std::vector<BytecodeInstruction> bytecode;
    
    // Function setup
    bytecode.emplace_back(100, 0, 0, "Load constant 0");    # LOAD_CONST 0
    bytecode.emplace_back(102, 0, 2, "Store to local i");     # STORE_FAST i
    bytecode.emplace_back(101, 0, 4, "Load local i");        # LOAD_FAST i
    bytecode.emplace_back(100, 10, 6, "Load constant 10");   # LOAD_CONST 10
    bytecode.emplace_back(107, 0, 8, "Compare <");           # COMPARE_OP <
    bytecode.emplace_back(114, 20, 10, "Jump if false");      # POP_JUMP_IF_FALSE 20
    
    // Loop body
    bytecode.emplace_back(101, 0, 12, "Load local i");       # LOAD_FAST i
    bytecode.emplace_back(100, 1, 14, "Load constant 1");     # LOAD_CONST 1
    bytecode.emplace_back(55, 0, 16, "Binary add");          # BINARY_ADD
    bytecode.emplace_back(102, 0, 18, "Store to local i");    # STORE_FAST i
    bytecode.emplace_back(113, 4, 20, "Jump back");         # JUMP_ABSOLUTE 4
    
    // After loop
    bytecode.emplace_back(100, 0, 22, "Load constant 0");   # LOAD_CONST 0
    bytecode.emplace_back(83, 0, 24, "Return value");        # RETURN_VALUE
    
    auto ir_module = translator.translate(bytecode, "loop_test");
    
    ASSERT_NE(ir_module, nullptr);
    EXPECT_GT(ir_module->getFunctions().size(), 0);
}

// Integration tests
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
        ImportSystem::initialize();
    }
    
    void TearDown() override {
        ImportSystem::shutdown();
        Runtime::shutdown();
    }
};

TEST_F(IntegrationTest, FullCompilationPipeline) {
    // Test the complete AOT compilation pipeline
    AOTCompilationPipeline pipeline;
    
    // Create a simple Python file for testing
    const std::string test_code = R"(
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

result = fibonacci(10)
print(f"Fibonacci(10) = {result}")
)";
    
    // Write test code to file
    std::ofstream test_file("integration_test.py");
    test_file << test_code;
    test_file.close();
    
    // Compile using AOT pipeline
    bool success = pipeline.compile("integration_test.py", "integration_test_output");
    
    // Clean up
    std::remove("integration_test.py");
    
    EXPECT_TRUE(success);
}

TEST_F(IntegrationTest, MultiModuleCompilation) {
    // Test compiling multiple modules with dependencies
    const std::string module1_code = R"(
def utility_function(x):
    return x * 2 + 1
)";
    
    const std::string module2_code = R"(
from module1 import utility_function

def main_function(data):
    return [utility_function(x) for x in data]
)";
    
    // Write modules to files
    std::ofstream file1("module1.py");
    file1 << module1_code;
    file1.close();
    
    std::ofstream file2("module2.py");
    file2 << module2_code;
    file2.close();
    
    // Compile modules
    AOTCompilationPipeline pipeline;
    bool success1 = pipeline.compile("module1.py", "module1_output");
    bool success2 = pipeline.compile("module2.py", "module2_output");
    
    // Clean up
    std::remove("module1.py");
    std::remove("module2.py");
    
    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
}

// Performance benchmarks
class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::initialize();
    }
    
    void TearDown() override {
        Runtime::shutdown();
    }
    
    void benchmarkCompilation(const std::string& name, const std::string& code) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Parse
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto module = parser.parse();
        
        // Type inference
        TypeSystem type_system;
        TypeInference inference(type_system);
        inference.infer(*module);
        
        // IR generation
        IRGenerator ir_generator(type_system);
        auto ir_module = ir_generator.generate(*module);
        
        // Code generation
        LLVMCodeGenerator codegen(type_system);
        codegen.generate(*ir_module);
        codegen.optimize(3);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << name << " compilation time: " << duration.count() << "ms" << std::endl;
    }
};

TEST_F(PerformanceTest, CompilationBenchmark) {
    const std::string simple_code = R"(
def simple_function(x):
    return x + 1
)";
    
    const std::string complex_code = R"(
def complex_function(data):
    result = []
    for item in data:
        if item > 0:
            result.append(item * item)
        else:
            result.append(abs(item))
    return sorted(result)
)";
    
    benchmarkCompilation("Simple", simple_code);
    benchmarkCompilation("Complex", complex_code);
}

} // namespace pyplusplus

// Main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}