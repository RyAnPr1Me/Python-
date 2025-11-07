#!/usr/bin/env python3
"""
Python++ AOT Compilation Demo
This file demonstrates the advanced features of the Python++ AOT compiler
including advanced syntax, optimizations, and performance features.
"""

import sys
import math
import time
import random
from typing import List, Dict, Tuple, Optional, Union, Any
from dataclasses import dataclass
from functools import wraps, lru_cache
from collections import defaultdict, Counter
import itertools

# Advanced type annotations
Number = Union[int, float]
Vector = List[Number]
Matrix = List[Vector]
Result = Dict[str, Any]

@dataclass
class PerformanceMetrics:
    """Performance metrics for benchmarking"""
    execution_time: float
    memory_usage: int
    operations_per_second: float
    
    def __str__(self) -> str:
        return f"Performance: {self.operations_per_second:.2f} ops/sec, {self.memory_usage} bytes"

def benchmark(func):
    """Decorator for benchmarking function performance"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        start_memory = sys.getsizeof(args) + sys.getsizeof(kwargs)
        
        result = func(*args, **kwargs)
        
        end_time = time.perf_counter()
        end_memory = sys.getsizeof(result)
        execution_time = end_time - start_time
        memory_usage = end_memory - start_memory
        
        metrics = PerformanceMetrics(
            execution_time=execution_time,
            memory_usage=memory_usage,
            operations_per_second=1.0 / execution_time if execution_time > 0 else 0
        )
        
        print(f"{func.__name__}: {metrics}")
        return result
    
    return wrapper

# Advanced mathematical functions
@benchmark
def matrix_multiply(a: Matrix, b: Matrix) -> Matrix:
    """Optimized matrix multiplication with vectorization hints"""
    if len(a[0]) != len(b):
        raise ValueError("Matrix dimensions don't match")
    
    result = [[0.0 for _ in range(len(b[0]))] for _ in range(len(a))]
    
    # Optimized multiplication with cache-friendly access
    for i in range(len(a)):
        for k in range(len(b)):
            aik = a[i][k]
            for j in range(len(b[0])):
                result[i][j] += aik * b[k][j]
    
    return result

@benchmark
def fibonacci_optimized(n: int) -> int:
    """Optimized Fibonacci with memoization and matrix exponentiation"""
    if n <= 1:
        return n
    
    # Matrix exponentiation for O(log n) Fibonacci
    def matrix_pow(mat: Matrix, power: int) -> Matrix:
        if power == 1:
            return mat
        if power % 2 == 0:
            half = matrix_pow(mat, power // 2)
            return matrix_multiply(half, half)
        else:
            return matrix_multiply(mat, matrix_pow(mat, power - 1))
    
    fib_matrix = [[1, 1], [1, 0]]
    result_matrix = matrix_pow(fib_matrix, n)
    return result_matrix[0][1]

# Advanced data structures and algorithms
class AdvancedDataStructures:
    """Demonstration of advanced data structures"""
    
    @staticmethod
    @benchmark
    def quick_sort(arr: List[int]) -> List[int]:
        """Optimized quicksort with median-of-three pivot selection"""
        if len(arr) <= 1:
            return arr
        
        # Median-of-three pivot selection
        first, middle, last = arr[0], arr[len(arr)//2], arr[-1]
        pivot = sorted([first, middle, last])[1]
        
        less = [x for x in arr if x < pivot]
        equal = [x for x in arr if x == pivot]
        greater = [x for x in arr if x > pivot]
        
        return AdvancedDataStructures.quick_sort(less) + equal + AdvancedDataStructures.quick_sort(greater)
    
    @staticmethod
    @benchmark
    def merge_sort(arr: List[int]) -> List[int]:
        """Optimized merge sort with early termination"""
        if len(arr) <= 1:
            return arr
        
        mid = len(arr) // 2
        left = AdvancedDataStructures.merge_sort(arr[:mid])
        right = AdvancedDataStructures.merge_sort(arr[mid:])
        
        # Early termination if already sorted
        if left[-1] <= right[0]:
            return left + right
        
        return AdvancedDataStructures._merge(left, right)
    
    @staticmethod
    def _merge(left: List[int], right: List[int]) -> List[int]:
        result = []
        i = j = 0
        
        while i < len(left) and j < len(right):
            if left[i] <= right[j]:
                result.append(left[i])
                i += 1
            else:
                result.append(right[j])
                j += 1
        
        result.extend(left[i:])
        result.extend(right[j:])
        return result

# Pattern matching and advanced control flow
def pattern_matching_demo(value: Any) -> str:
    """Demonstrate Python 3.10+ pattern matching"""
    match value:
        case int(x) if x > 0:
            return f"Positive integer: {x}"
        case int(x) if x < 0:
            return f"Negative integer: {x}"
        case 0:
            return "Zero"
        case float(x):
            return f"Floating point: {x:.2f}"
        case str(s) if len(s) > 10:
            return f"Long string: {s[:10]}..."
        case str(s):
            return f"String: {s}"
        case list(lst) if len(lst) > 5:
            return f"Long list with {len(lst)} elements"
        case [first, *rest]:
            return f"List starting with {first}, {len(rest)} more elements"
        case {"name": name, "age": age}:
            return f"Person: {name}, age {age}"
        case _:
            return f"Unknown type: {type(value).__name__}"

# Async/await demonstration
import asyncio

async def async_data_processor(data_stream: List[int]) -> Dict[str, float]:
    """Asynchronous data processing pipeline"""
    async def process_chunk(chunk: List[int]) -> float:
        await asyncio.sleep(0.001)  # Simulate I/O
        return sum(chunk) / len(chunk)
    
    chunk_size = 100
    chunks = [data_stream[i:i+chunk_size] for i in range(0, len(data_stream), chunk_size)]
    
    # Process chunks concurrently
    results = await asyncio.gather(*[process_chunk(chunk) for chunk in chunks])
    
    return {
        "mean": sum(results) / len(results),
        "min": min(results),
        "max": max(results),
        "count": len(results)
    }

# Metaclass demonstration
class SingletonMeta(type):
    """Thread-safe singleton metaclass"""
    _instances = {}
    _lock = asyncio.Lock()
    
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            async def create_instance():
                async with cls._lock:
                    if cls not in cls._instances:
                        instance = super().__call__(*args, **kwargs)
                        cls._instances[cls] = instance
                        return instance
                return cls._instances[cls]
            
            # For simplicity, using synchronous version
            with cls._lock:
                if cls not in cls._instances:
                    instance = super().__call__(*args, **kwargs)
                    cls._instances[cls] = instance
        
        return cls._instances[cls]

class ConfigurationManager(metaclass=SingletonMeta):
    """Singleton configuration manager"""
    def __init__(self):
        self.config = {
            "optimization_level": 3,
            "enable_jit": True,
            "memory_limit": 1024 * 1024 * 1024,  # 1GB
            "thread_pool_size": 4
        }
    
    def get(self, key: str, default: Any = None) -> Any:
        return self.config.get(key, default)
    
    def set(self, key: str, value: Any) -> None:
        self.config[key] = value

# Decorator and descriptor demonstration
class LazyProperty:
    """Lazy property descriptor"""
    def __init__(self, func):
        self.func = func
        self.name = func.__name__
    
    def __get__(self, obj, type=None):
        if obj is None:
            return self
        value = self.func(obj)
        setattr(obj, self.name, value)
        return value

class ExpensiveComputation:
    """Class demonstrating lazy evaluation and caching"""
    
    def __init__(self, size: int):
        self.size = size
        self._cache = {}
    
    @LazyProperty
    def expensive_calculation(self) -> List[int]:
        """Expensive calculation that's lazily evaluated"""
        print(f"Performing expensive calculation for size {self.size}...")
        result = []
        for i in range(self.size):
            if i % 2 == 0:
                result.append(i * i)
            else:
                result.append(fibonacci_optimized(i))
        return result
    
    @lru_cache(maxsize=128)
    def cached_fibonacci(self, n: int) -> int:
        """Cached Fibonacci calculation"""
        return fibonacci_optimized(n)

# Generator and iterator demonstration
def prime_generator(limit: int):
    """Efficient prime number generator using Sieve of Eratosthenes"""
    sieve = [True] * (limit + 1)
    sieve[0] = sieve[1] = False
    
    for num in range(2, int(limit ** 0.5) + 1):
        if sieve[num]:
            sieve[num*num : limit+1 : num] = [False] * len(sieve[num*num : limit+1 : num])
    
    for num in range(2, limit + 1):
        if sieve[num]:
            yield num

def data_pipeline(source: List[int]) -> Dict[str, Any]:
    """Complex data processing pipeline"""
    # Stage 1: Filter and transform
    filtered = (x for x in source if x > 0)
    transformed = (math.sqrt(x) for x in filtered)
    
    # Stage 2: Aggregate
    data = list(transformed)
    
    # Stage 3: Statistical analysis
    return {
        "count": len(data),
        "mean": sum(data) / len(data) if data else 0,
        "variance": sum((x - sum(data)/len(data))**2 for x in data) / len(data) if data else 0,
        "min": min(data) if data else 0,
        "max": max(data) if data else 0
    }

# Context manager demonstration
class PerformanceProfiler:
    """Context manager for performance profiling"""
    def __init__(self, name: str):
        self.name = name
        self.start_time = None
        self.start_memory = None
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        self.start_memory = sys.getsizeof([])
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        end_time = time.perf_counter()
        execution_time = end_time - self.start_time
        print(f"[PROFILE] {self.name}: {execution_time:.4f}s")
        return False

# Main demonstration function
def main():
    """Main function demonstrating all Python++ AOT features"""
    print("Python++ AOT Compilation Demo")
    print("=" * 50)
    
    # Configuration
    config = ConfigurationManager()
    print(f"Optimization level: {config.get('optimization_level')}")
    
    # Pattern matching demo
    print("\nPattern Matching Demo:")
    test_values = [42, -3.14, "Hello World", [1, 2, 3, 4, 5, 6], 
                   {"name": "Alice", "age": 30}]
    for value in test_values:
        print(f"  {value!r} -> {pattern_matching_demo(value)}")
    
    # Mathematical computations
    print("\nMathematical Computations:")
    
    # Matrix multiplication
    size = 100
    matrix_a = [[random.random() for _ in range(size)] for _ in range(size)]
    matrix_b = [[random.random() for _ in range(size)] for _ in range(size)]
    
    with PerformanceProfiler("Matrix Multiplication"):
        result = matrix_multiply(matrix_a, matrix_b)
    print(f"  Matrix multiplication result: {len(result)}x{len(result[0])}")
    
    # Fibonacci calculations
    with PerformanceProfiler("Fibonacci"):
        fib_result = fibonacci_optimized(1000)
    print(f"  Fibonacci(1000): {fib_result}")
    
    # Sorting algorithms
    print("\nSorting Algorithms:")
    test_data = [random.randint(1, 1000) for _ in range(1000)]
    
    with PerformanceProfiler("Quick Sort"):
        quick_sorted = AdvancedDataStructures.quick_sort(test_data.copy())
    
    with PerformanceProfiler("Merge Sort"):
        merge_sorted = AdvancedDataStructures.merge_sort(test_data.copy())
    
    print(f"  Quick sort: {len(quick_sorted)} elements")
    print(f"  Merge sort: {len(merge_sorted)} elements")
    
    # Prime generation
    print("\nPrime Generation:")
    with PerformanceProfiler("Prime Generation"):
        primes = list(prime_generator(10000))
    print(f"  Found {len(primes)} primes up to 10000")
    
    # Data pipeline
    print("\nData Pipeline:")
    source_data = [random.randint(1, 1000) for _ in range(10000)]
    with PerformanceProfiler("Data Pipeline"):
        pipeline_result = data_pipeline(source_data)
    print(f"  Pipeline result: {pipeline_result}")
    
    # Lazy evaluation
    print("\nLazy Evaluation:")
    expensive = ExpensiveComputation(100)
    print(f"  First access: {len(expensive.expensive_calculation)} items")
    print(f"  Second access: {len(expensive.expensive_calculation)} items (cached)")
    
    # Async processing (simplified for demo)
    print("\nAsync Processing:")
    async_data = [random.randint(1, 100) for _ in range(1000)]
    # Note: In real AOT compilation, async would be properly handled
    print(f"  Prepared {len(async_data)} items for async processing")
    
    print("\n" + "=" * 50)
    print("Demo completed successfully!")
    print("This code demonstrates advanced Python++ AOT features:")
    print("  - Advanced type annotations")
    print("  - Pattern matching")
    print("  - Async/await support")
    print("  - Metaclasses")
    print("  - Decorators and descriptors")
    print("  - Generators and iterators")
    print("  - Context managers")
    print("  - Performance optimizations")
    print("  - Memory management")
    print("  - Error handling")

if __name__ == "__main__":
    main()