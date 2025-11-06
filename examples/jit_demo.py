# JIT Compilation Example
# Demonstrates just-in-time compilation for hot code paths

import time
from typing import List, Callable

# Simple function that will be hot
def fibonacci(n: int) -> int:
    """Calculate Fibonacci number - will be JIT compiled when hot"""
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

# Mathematical operations
def matrix_multiply(a: List[List[int]], b: List[List[int]]) -> List[List[int]]:
    """Matrix multiplication - computationally intensive"""
    rows_a, cols_a = len(a), len(a[0])
    rows_b, cols_b = len(b), len(b[0])
    
    if cols_a != rows_b:
        raise ValueError("Matrix dimensions don't match")
    
    result = [[0 for _ in range(cols_b)] for _ in range(rows_a)]
    
    for i in range(rows_a):
        for j in range(cols_b):
            for k in range(cols_a):
                result[i][j] += a[i][k] * b[k][j]
    
    return result

def bubble_sort(arr: List[int]) -> List[int]:
    """Bubble sort - will be optimized by JIT"""
    n = len(arr)
    result = arr.copy()
    
    for i in range(n):
        for j in range(0, n - i - 1):
            if result[j] > result[j + 1]:
                result[j], result[j + 1] = result[j + 1], result[j]
    
    return result

def string_processing(text: str) -> dict:
    """String processing function"""
    words = text.split()
    word_count = len(words)
    char_count = len(text)
    
    # Count word frequencies
    frequencies = {}
    for word in words:
        word_lower = word.lower().strip('.,!?')
        frequencies[word_lower] = frequencies.get(word_lower, 0) + 1
    
    return {
        'word_count': word_count,
        'char_count': char_count,
        'frequencies': frequencies,
        'unique_words': len(frequencies)
    }

def mathematical_operations(n: int) -> dict:
    """Various mathematical operations"""
    result = {}
    
    # Prime numbers
    primes = []
    for num in range(2, n + 1):
        is_prime = True
        for i in range(2, int(num ** 0.5) + 1):
            if num % i == 0:
                is_prime = False
                break
        if is_prime:
            primes.append(num)
    
    result['primes'] = primes
    result['prime_count'] = len(primes)
    
    # Factorials
    def factorial(x: int) -> int:
        if x <= 1:
            return 1
        return x * factorial(x - 1)
    
    result['factorials'] = [factorial(i) for i in range(1, min(n, 10) + 1)]
    
    # Powers of 2
    result['powers_of_2'] = [2 ** i for i in range(n)]
    
    return result

# Hot loop function
def hot_loop(iterations: int) -> int:
    """Function that will become hot and trigger JIT compilation"""
    total = 0
    for i in range(iterations):
        total += i * i + i + 1
    return total

# Recursive function with memoization
def memoized_fibonacci() -> Callable[[int], int]:
    """Factory for memoized Fibonacci function"""
    cache = {}
    
    def fib(n: int) -> int:
        if n in cache:
            return cache[n]
        if n <= 1:
            cache[n] = n
        else:
            cache[n] = fib(n - 1) + fib(n - 2)
        return cache[n]
    
    return fib

# Data processing pipeline
def data_pipeline(data: List[dict]) -> List[dict]:
    """Data processing pipeline that will benefit from JIT"""
    processed = []
    
    for item in data:
        # Transform data
        transformed = {
            'id': item.get('id', 0),
            'value': item.get('value', 0) * 2,
            'category': item.get('category', 'unknown').upper(),
            'processed': True
        }
        
        # Filter
        if transformed['value'] > 10:
            processed.append(transformed)
    
    # Sort
    processed.sort(key=lambda x: x['value'], reverse=True)
    
    return processed

# Performance benchmark
def benchmark_functions() -> dict:
    """Benchmark various functions to trigger JIT compilation"""
    results = {}
    
    # Benchmark Fibonacci
    start = time.time()
    fib_result = fibonacci(30)
    end = time.time()
    results['fibonacci'] = {
        'result': fib_result,
        'time': end - start
    }
    
    # Benchmark matrix multiplication
    matrix_a = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
    matrix_b = [[9, 8, 7], [6, 5, 4], [3, 2, 1]]
    
    start = time.time()
    matrix_result = matrix_multiply(matrix_a, matrix_b)
    end = time.time()
    results['matrix_multiply'] = {
        'result': matrix_result,
        'time': end - start
    }
    
    # Benchmark bubble sort
    test_array = [64, 34, 25, 12, 22, 11, 90, 88, 76, 50, 42]
    start = time.time()
    sorted_array = bubble_sort(test_array)
    end = time.time()
    results['bubble_sort'] = {
        'result': sorted_array,
        'time': end - start
    }
    
    # Benchmark hot loop
    start = time.time()
    loop_result = hot_loop(100000)
    end = time.time()
    results['hot_loop'] = {
        'result': loop_result,
        'time': end - start
    }
    
    # Benchmark memoized Fibonacci
    memo_fib = memoized_fibonacci()
    start = time.time()
    memo_result = memo_fib(100)
    end = time.time()
    results['memoized_fibonacci'] = {
        'result': memo_result,
        'time': end - start
    }
    
    return results

# Stress test for JIT
def stress_test_jit() -> None:
    """Stress test to trigger JIT compilation on multiple functions"""
    print("Running JIT stress test...")
    
    # Multiple calls to make functions hot
    for i in range(100):
        # Hot function
        hot_loop(1000)
        
        # Mathematical operations
        if i % 10 == 0:
            mathematical_operations(50)
    
    # String processing
    text = "This is a sample text for processing. It contains multiple sentences. " * 10
    for i in range(50):
        string_processing(text)
    
    # Data processing
    sample_data = [
        {'id': i, 'value': i * 3, 'category': f'cat_{i % 5}'}
        for i in range(100)
    ]
    
    for i in range(20):
        data_pipeline(sample_data)
    
    print("Stress test completed - JIT compilation should have been triggered")

# Demonstrate JIT features
def demonstrate_jit_features() -> None:
    """Demonstrate various JIT compilation features"""
    
    print("=== JIT Compilation Demonstration ===\n")
    
    # Initial benchmark
    print("Initial benchmark (cold functions):")
    initial_results = benchmark_functions()
    for name, result in initial_results.items():
        print(f"  {name}: {result['time']:.6f}s")
    
    print("\nRunning functions multiple times to trigger JIT compilation...")
    
    # Make functions hot
    for i in range(10):
        fibonacci(20)
        hot_loop(10000)
    
    # Second benchmark (should be faster due to JIT)
    print("\nSecond benchmark (functions should be JIT compiled):")
    final_results = benchmark_functions()
    for name, result in final_results.items():
        initial_time = initial_results[name]['time']
        final_time = result['time']
        speedup = initial_time / final_time if final_time > 0 else 1.0
        print(f"  {name}: {final_time:.6f}s (speedup: {speedup:.2f}x)")
    
    # Stress test
    print("\n" + "="*50)
    stress_test_jit()
    
    # Demonstrate different optimization levels
    print("\n" + "="*50)
    print("Demonstrating different optimization scenarios:")
    
    # Small function (O1 optimization)
    def small_function(x: int) -> int:
        return x * x + x * 2 + 1
    
    # Medium function (O2 optimization)
    def medium_function(data: List[int]) -> int:
        total = 0
        for i, val in enumerate(data):
            if val % 2 == 0:
                total += val * i
            else:
                total += val
        return total
    
    # Large function (O3 optimization)
    def large_function(n: int) -> List[int]:
        result = []
        for i in range(n):
            for j in range(n):
                if (i + j) % 2 == 0:
                    result.append(i * j)
        return result
    
    # Test functions
    print("Testing small function...")
    for i in range(1000):
        small_function(i)
    
    print("Testing medium function...")
    test_data = list(range(100))
    for i in range(100):
        medium_function(test_data)
    
    print("Testing large function...")
    for i in range(10):
        large_function(50)
    
    print("All optimization levels tested!")

if __name__ == "__main__":
    demonstrate_jit_features()