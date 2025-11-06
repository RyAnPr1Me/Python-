# Complete Python++ Feature Demonstration
# Shows all implemented Python syntax, functions, and features

from typing import List, Dict, Tuple, Set, Optional, Union, Callable, Any, TypeVar, Generic, Protocol, TypedDict, Literal
import sys
import os
import math
import random
import time
import json
import re
from collections import namedtuple, deque, Counter, OrderedDict
from itertools import chain, cycle, islice, count, product, permutations, combinations
from functools import reduce, partial, lru_cache, wraps
from operator import itemgetter, attrgetter, methodcaller

# Type variables
T = TypeVar('T')
U = TypeVar('U')
K = TypeVar('K')
V = TypeVar('V')

# Protocol definitions
class Drawable(Protocol):
    def draw(self) -> None: ...
    def area(self) -> float: ...

class Comparable(Protocol):
    def __lt__(self, other: Any) -> bool: ...
    def __eq__(self, other: Any) -> bool: ...

# TypedDict definitions
class Person(TypedDict):
    name: str
    age: int
    email: Optional[str]

class Address(TypedDict):
    street: str
    city: str
    state: str
    zip_code: str

# Generic classes
class Stack(Generic[T]):
    """Generic stack implementation"""
    
    def __init__(self) -> None:
        self._items: List[T] = []
    
    def push(self, item: T) -> None:
        self._items.append(item)
    
    def pop(self) -> Optional[T]:
        return self._items.pop() if self._items else None
    
    def is_empty(self) -> bool:
        return not self._items
    
    def __len__(self) -> int:
        return len(self._items)
    
    def __repr__(self) -> str:
        return f"Stack({self._items})"

class Queue(Generic[T]):
    """Generic queue implementation"""
    
    def __init__(self) -> None:
        self._items: List[T] = []
    
    def enqueue(self, item: T) -> None:
        self._items.append(item)
    
    def dequeue(self) -> Optional[T]:
        return self._items.pop(0) if self._items else None
    
    def is_empty(self) -> bool:
        return not self._items
    
    def __len__(self) -> int:
        return len(self._items)

# Advanced decorators
def timer(func: Callable[..., Any]) -> Callable[..., Any]:
    """Decorator to time function execution"""
    @wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        start = time.perf_counter()
        result = func(*args, **kwargs)
        end = time.perf_counter()
        print(f"{func.__name__} took {end - start:.6f} seconds")
        return result
    return wrapper

def memoize_with_size(max_size: int = 128) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
    """Memoization decorator with size limit"""
    def decorator(func: Callable[..., Any]) -> Callable[..., Any]:
        cache = {}
        
        @wraps(func)
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            key = str(args) + str(sorted(kwargs.items()))
            if key in cache:
                return cache[key]
            
            if len(cache) >= max_size:
                # Remove oldest entry (simple FIFO)
                oldest_key = next(iter(cache))
                del cache[oldest_key]
            
            result = func(*args, **kwargs)
            cache[key] = result
            return result
        
        wrapper.cache = cache  # For debugging
        return wrapper
    return decorator

def validate_types(**type_hints: type) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
    """Decorator to validate argument types"""
    def decorator(func: Callable[..., Any]) -> Callable[..., Any]:
        @wraps(func)
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            # Validate positional arguments
            for i, (arg_name, expected_type) in enumerate(type_hints.items()):
                if i < len(args):
                    if not isinstance(args[i], expected_type):
                        raise TypeError(f"Argument {arg_name} should be {expected_type.__name__}, got {type(args[i]).__name__}")
            
            # Validate keyword arguments
            for arg_name, expected_type in type_hints.items():
                if arg_name in kwargs:
                    if not isinstance(kwargs[arg_name], expected_type):
                        raise TypeError(f"Argument {arg_name} should be {expected_type.__name__}, got {type(kwargs[arg_name]).__name__}")
            
            return func(*args, **kwargs)
        return wrapper
    return decorator

# Advanced data structures
class BinaryTree(Generic[T]):
    """Binary tree implementation"""
    
    class Node:
        def __init__(self, value: T, left: Optional['BinaryTree.Node'] = None, right: Optional['BinaryTree.Node'] = None):
            self.value = value
            self.left = left
            self.right = right
    
    def __init__(self) -> None:
        self.root: Optional[BinaryTree.Node] = None
        self.size = 0
    
    def insert(self, value: T) -> None:
        if not self.root:
            self.root = BinaryTree.Node(value)
        else:
            self._insert_recursive(self.root, value)
        self.size += 1
    
    def _insert_recursive(self, node: 'BinaryTree.Node', value: T) -> None:
        if value < node.value:
            if not node.left:
                node.left = BinaryTree.Node(value)
            else:
                self._insert_recursive(node.left, value)
        else:
            if not node.right:
                node.right = BinaryTree.Node(value)
            else:
                self._insert_recursive(node.right, value)
    
    def inorder_traversal(self) -> List[T]:
        result = []
        self._inorder_recursive(self.root, result)
        return result
    
    def _inorder_recursive(self, node: Optional['BinaryTree.Node'], result: List[T]) -> None:
        if node:
            self._inorder_recursive(node.left, result)
            result.append(node.value)
            self._inorder_recursive(node.right, result)
    
    def __len__(self) -> int:
        return self.size
    
    def __contains__(self, value: T) -> bool:
        return self._contains_recursive(self.root, value)
    
    def _contains_recursive(self, node: Optional['BinaryTree.Node'], value: T) -> bool:
        if not node:
            return False
        if value == node.value:
            return True
        if value < node.value:
            return self._contains_recursive(node.left, value)
        else:
            return self._contains_recursive(node.right, value)

class Graph(Generic[T]):
    """Graph implementation with adjacency list"""
    
    def __init__(self) -> None:
        self.vertices: Dict[T, List[T]] = {}
        self.edges: List[Tuple[T, T]] = []
    
    def add_vertex(self, vertex: T) -> None:
        if vertex not in self.vertices:
            self.vertices[vertex] = []
    
    def add_edge(self, from_vertex: T, to_vertex: T) -> None:
        self.add_vertex(from_vertex)
        self.add_vertex(to_vertex)
        self.vertices[from_vertex].append(to_vertex)
        self.edges.append((from_vertex, to_vertex))
    
    def get_neighbors(self, vertex: T) -> List[T]:
        return self.vertices.get(vertex, [])
    
    def bfs(self, start: T) -> List[T]:
        """Breadth-first search"""
        visited = set()
        queue = [start]
        result = []
        
        while queue:
            vertex = queue.pop(0)
            if vertex not in visited:
                visited.add(vertex)
                result.append(vertex)
                queue.extend(self.get_neighbors(vertex))
        
        return result
    
    def dfs(self, start: T) -> List[T]:
        """Depth-first search"""
        visited = set()
        result = []
        
        def dfs_recursive(vertex: T) -> None:
            if vertex not in visited:
                visited.add(vertex)
                result.append(vertex)
                for neighbor in self.get_neighbors(vertex):
                    dfs_recursive(neighbor)
        
        dfs_recursive(start)
        return result

# Mathematical functions
@lru_cache(maxsize=128)
def fibonacci(n: int) -> int:
    """Memoized Fibonacci function"""
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

def is_prime(n: int) -> bool:
    """Check if a number is prime"""
    if n <= 1:
        return False
    if n <= 3:
        return True
    if n % 2 == 0 or n % 3 == 0:
        return False
    
    i = 5
    w = 2
    while i * i <= n:
        if n % i == 0:
            return False
        i += w
        w = 6 - w
    
    return True

def sieve_of_eratosthenes(n: int) -> List[int]:
    """Generate primes up to n using Sieve of Eratosthenes"""
    if n <= 2:
        return []
    
    sieve = [True] * (n + 1)
    sieve[0] = sieve[1] = False
    
    for i in range(2, int(math.sqrt(n)) + 1):
        if sieve[i]:
            sieve[i*i : n+1 : i] = [False] * len(sieve[i*i : n+1 : i])
    
    return [i for i, is_prime in enumerate(sieve) if is_prime]

def gcd(a: int, b: int) -> int:
    """Greatest common divisor using Euclidean algorithm"""
    while b:
        a, b = b, a % b
    return abs(a)

def lcm(a: int, b: int) -> int:
    """Least common multiple"""
    return abs(a * b) // gcd(a, b) if a and b else 0

# String processing functions
def palindrome_check(s: str) -> bool:
    """Check if string is a palindrome"""
    s = s.lower().replace(' ', '').replace(',', '').replace('.', '')
    return s == s[::-1]

def anagram_check(s1: str, s2: str) -> bool:
    """Check if two strings are anagrams"""
    return sorted(s1.lower()) == sorted(s2.lower())

def word_frequency(text: str) -> Dict[str, int]:
    """Count word frequency in text"""
    words = re.findall(r'\b\w+\b', text.lower())
    return dict(Counter(words))

def compress_string(s: str) -> str:
    """Run-length encoding compression"""
    if not s:
        return ""
    
    result = []
    count = 1
    current_char = s[0]
    
    for char in s[1:]:
        if char == current_char:
            count += 1
        else:
            result.append(f"{current_char}{count}")
            current_char = char
            count = 1
    
    result.append(f"{current_char}{count}")
    return ''.join(result)

# File I/O functions
def read_file_lines(filename: str) -> List[str]:
    """Read file and return lines"""
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            return file.readlines()
    except FileNotFoundError:
        raise FileNotFoundError(f"File {filename} not found")
    except IOError as e:
        raise IOError(f"Error reading file {filename}: {e}")

def write_file_lines(filename: str, lines: List[str]) -> None:
    """Write lines to file"""
    try:
        with open(filename, 'w', encoding='utf-8') as file:
            file.writelines(lines)
    except IOError as e:
        raise IOError(f"Error writing file {filename}: {e}")

def process_csv(filename: str) -> List[Dict[str, str]]:
    """Simple CSV processor"""
    lines = read_file_lines(filename)
    if not lines:
        return []
    
    headers = lines[0].strip().split(',')
    data = []
    
    for line in lines[1:]:
        values = line.strip().split(',')
        if len(values) == len(headers):
            data.append(dict(zip(headers, values)))
    
    return data

# Advanced algorithms
def quick_sort(arr: List[T]) -> List[T]:
    """Quick sort implementation"""
    if len(arr) <= 1:
        return arr
    
    pivot = arr[len(arr) // 2]
    left = [x for x in arr if x < pivot]
    middle = [x for x in arr if x == pivot]
    right = [x for x in arr if x > pivot]
    
    return quick_sort(left) + middle + quick_sort(right)

def merge_sort(arr: List[T]) -> List[T]:
    """Merge sort implementation"""
    if len(arr) <= 1:
        return arr
    
    mid = len(arr) // 2
    left = merge_sort(arr[:mid])
    right = merge_sort(arr[mid:])
    
    return merge(left, right)

def merge(left: List[T], right: List[T]) -> List[T]:
    """Merge two sorted lists"""
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

def dijkstra(graph: Dict[T, Dict[T, float]], start: T) -> Dict[T, float]:
    """Dijkstra's shortest path algorithm"""
    distances = {vertex: float('inf') for vertex in graph}
    distances[start] = 0
    visited = set()
    unvisited = set(graph.keys())
    
    while unvisited:
        current = min(unvisited, key=lambda vertex: distances[vertex])
        unvisited.remove(current)
        visited.add(current)
        
        for neighbor, weight in graph[current].items():
            if neighbor not in visited:
                distance = distances[current] + weight
                if distance < distances[neighbor]:
                    distances[neighbor] = distance
    
    return distances

# Data analysis functions
def mean(numbers: List[float]) -> float:
    """Calculate arithmetic mean"""
    return sum(numbers) / len(numbers) if numbers else 0.0

def median(numbers: List[float]) -> float:
    """Calculate median"""
    if not numbers:
        return 0.0
    
    sorted_numbers = sorted(numbers)
    n = len(sorted_numbers)
    middle = n // 2
    
    if n % 2 == 0:
        return (sorted_numbers[middle - 1] + sorted_numbers[middle]) / 2
    else:
        return sorted_numbers[middle]

def mode(numbers: List[T]) -> List[T]:
    """Calculate mode(s)"""
    if not numbers:
        return []
    
    counts = Counter(numbers)
    max_count = max(counts.values())
    return [num for num, count in counts.items() if count == max_count]

def standard_deviation(numbers: List[float]) -> float:
    """Calculate standard deviation"""
    if len(numbers) < 2:
        return 0.0
    
    avg = mean(numbers)
    variance = sum((x - avg) ** 2 for x in numbers) / (len(numbers) - 1)
    return math.sqrt(variance)

def correlation(x: List[float], y: List[float]) -> float:
    """Calculate Pearson correlation coefficient"""
    if len(x) != len(y) or len(x) < 2:
        return 0.0
    
    n = len(x)
    sum_x = sum(x)
    sum_y = sum(y)
    sum_xy = sum(xi * yi for xi, yi in zip(x, y))
    sum_x2 = sum(xi ** 2 for xi in x)
    sum_y2 = sum(yi ** 2 for yi in y)
    
    numerator = n * sum_xy - sum_x * sum_y
    denominator = math.sqrt((n * sum_x2 - sum_x ** 2) * (n * sum_y2 - sum_y ** 2))
    
    return numerator / denominator if denominator != 0 else 0.0

# Pattern matching examples (Python 3.10+)
def process_value(value: Any) -> str:
    """Demonstrate pattern matching"""
    match value:
        case int(x) if x > 0:
            return f"Positive integer: {x}"
        case int(x) if x < 0:
            return f"Negative integer: {x}"
        case 0:
            return "Zero"
        case str(s):
            return f"String: {s}"
        case list(items) if len(items) > 0:
            return f"List with {len(items)} items"
        case dict(items):
            return f"Dictionary with {len(items)} items"
        case (x, y):
            return f"Tuple with two elements: {x}, {y}"
        case [first, *rest]:
            return f"List starting with {first}, rest: {rest}"
        case {"name": name, "age": age}:
            return f"Person: {name}, age {age}"
        case _:
            return f"Unknown type: {type(value).__name__}"

def advanced_pattern_matching() -> None:
    """Advanced pattern matching examples"""
    print("\n--- Advanced Pattern Matching ---")
    
    # Structural pattern matching
    data_points = [
        (1, 2),
        [3, 4, 5],
        {"type": "circle", "radius": 5},
        {"type": "rectangle", "width": 10, "height": 20},
        "simple string",
        42
    ]
    
    for point in data_points:
        match point:
            case (x, y):
                print(f"2D point: ({x}, {y})")
            case [x, y, z]:
                print(f"3D point: ({x}, {y}, {z})")
            case {"type": "circle", "radius": r}:
                print(f"Circle with radius {r}")
            case {"type": "rectangle", "width": w, "height": h}:
                print(f"Rectangle: {w}x{h}")
            case str(s):
                print(f"String: {s}")
            case int(n):
                print(f"Integer: {n}")
            case _:
                print(f"Unrecognized pattern: {point}")
    
    # Guard clauses
    numbers = [1, -2, 3, -4, 0, 5]
    for num in numbers:
        match num:
            case n if n > 0:
                print(f"{n} is positive")
            case n if n < 0:
                print(f"{n} is negative")
            case 0:
                print("Zero")
    
    # Nested patterns
    nested_data = [
        {"status": "success", "data": [1, 2, 3]},
        {"status": "error", "error": {"code": 404, "message": "Not found"}},
        {"status": "pending", "data": None}
    ]
    
    for response in nested_data:
        match response:
            case {"status": "success", "data": [first, *rest]}:
                print(f"Success with data starting with {first}")
            case {"status": "error", "error": {"code": code, "message": msg}}:
                print(f"Error {code}: {msg}")
            case {"status": "pending"}:
                print("Request is pending")
            case _:
                print("Unknown response format")

# Async/await examples
import asyncio

async def fetch_data(url: str) -> Dict[str, Any]:
    """Async function to fetch data"""
    await asyncio.sleep(0.1)  # Simulate network delay
    return {"url": url, "data": f"Data from {url}", "timestamp": time.time()}

async def process_multiple_urls(urls: List[str]) -> List[Dict[str, Any]]:
    """Process multiple URLs concurrently"""
    tasks = [fetch_data(url) for url in urls]
    return await asyncio.gather(*tasks)

# Context manager examples
class Timer:
    """Context manager for timing operations"""
    
    def __init__(self, name: str = "Operation"):
        self.name = name
        self.start_time = None
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        print(f"Starting {self.name}")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.start_time:
            elapsed = time.perf_counter() - self.start_time
            print(f"{self.name} completed in {elapsed:.6f} seconds")
        return False  # Don't suppress exceptions

class DatabaseConnection:
    """Context manager for database connections"""
    
    def __init__(self, connection_string: str):
        self.connection_string = connection_string
        self.connection = None
    
    def __enter__(self):
        print(f"Connecting to database: {self.connection_string}")
        self.connection = f"MockConnection({self.connection_string})"
        return self.connection
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.connection:
            print("Closing database connection")
            self.connection = None
        return False  # Don't suppress exceptions

# Generator examples
def fibonacci_generator(n: int):
    """Generate Fibonacci numbers"""
    a, b = 0, 1
    for _ in range(n):
        yield a
        a, b = b, a + b

def prime_generator():
    """Generate infinite prime numbers"""
    primes = []
    candidate = 2
    
    while True:
        is_prime = all(candidate % prime != 0 for prime in primes)
        if is_prime:
            primes.append(candidate)
            yield candidate
        candidate += 1

def file_reader(filename: str):
    """Generator for reading files line by line"""
    with open(filename, 'r') as file:
        for line in file:
            yield line.strip()

# Metaclass examples
class SingletonMeta(type):
    """Metaclass for creating singleton classes"""
    _instances = {}
    
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super().__call__(*args, **kwargs)
        return cls._instances[cls]

class Logger(metaclass=SingletonMeta):
    """Singleton logger class"""
    
    def __init__(self):
        self.logs = []
    
    def log(self, message: str) -> None:
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        self.logs.append(f"[{timestamp}] {message}")
        print(f"[{timestamp}] {message}")

# Descriptor examples
class ValidatedAttribute:
    """Descriptor for validated attributes"""
    
    def __init__(self, name: str, validator: Callable[[Any], bool]):
        self.name = name
        self.validator = validator
        self.value = None
    
    def __get__(self, obj, objtype):
        return self.value
    
    def __set__(self, obj, value):
        if self.validator(value):
            self.value = value
        else:
            raise ValueError(f"Invalid value for {self.name}: {value}")
    
    def __set_name__(self, owner, name):
        self.name = name

class Person:
    """Class using descriptors"""
    name = ValidatedAttribute('name', lambda x: isinstance(x, str) and len(x) > 0)
    age = ValidatedAttribute('age', lambda x: isinstance(x, int) and 0 <= x <= 150)
    email = ValidatedAttribute('email', lambda x: isinstance(x, str) and '@' in x if x else True)

# Property examples
class Circle:
    """Class with properties"""
    
    def __init__(self, radius: float):
        self._radius = radius
    
    @property
    def radius(self) -> float:
        return self._radius
    
    @radius.setter
    def radius(self, value: float) -> None:
        if value < 0:
            raise ValueError("Radius cannot be negative")
        self._radius = value
    
    @property
    def area(self) -> float:
        return math.pi * self._radius ** 2
    
    @property
    def circumference(self) -> float:
        return 2 * math.pi * self._radius
    
    def __repr__(self) -> str:
        return f"Circle(radius={self._radius})"

# Main demonstration function
@timer
def main() -> None:
    """Demonstrate all Python++ features"""
    print("=== Complete Python++ Feature Demonstration ===\n")
    
    # Basic types and operations
    print("--- Basic Types and Operations ---")
    numbers = [1, 2, 3, 4, 5]
    strings = ["hello", "world", "python"]
    mixed = [1, "two", 3.0, True, None]
    
    print(f"Numbers: {numbers}")
    print(f"Strings: {strings}")
    print(f"Mixed: {mixed}")
    print(f"Length of numbers: {len(numbers)}")
    print(f"Sum of numbers: {sum(numbers)}")
    print(f"Max of numbers: {max(numbers)}")
    print(f"Min of numbers: {min(numbers)}")
    
    # Advanced data structures
    print("\n--- Advanced Data Structures ---")
    stack = Stack[int]()
    for i in range(5):
        stack.push(i)
    print(f"Stack after pushes: {stack}")
    
    while not stack.is_empty():
        print(f"Popped: {stack.pop()}")
    
    queue = Queue[str]()
    for item in ["first", "second", "third"]:
        queue.enqueue(item)
    print(f"Queue after enqueues: {queue}")
    
    while not queue.is_empty():
        print(f"Dequeued: {queue.dequeue()}")
    
    # Binary tree
    tree = BinaryTree[int]()
    for num in [5, 3, 7, 2, 4, 6, 8]:
        tree.insert(num)
    print(f"Binary tree inorder traversal: {tree.inorder_traversal()}")
    print(f"Tree contains 4: {4 in tree}")
    print(f"Tree contains 9: {9 in tree}")
    
    # Graph
    graph = Graph[int]()
    for i in range(5):
        graph.add_vertex(i)
    graph.add_edge(0, 1)
    graph.add_edge(0, 2)
    graph.add_edge(1, 3)
    graph.add_edge(2, 4)
    
    print(f"Graph BFS from 0: {graph.bfs(0)}")
    print(f"Graph DFS from 0: {graph.dfs(0)}")
    
    # Mathematical functions
    print("\n--- Mathematical Functions ---")
    print(f"Fibonacci(10): {fibonacci(10)}")
    print(f"Is 17 prime: {is_prime(17)}")
    print(f"Is 18 prime: {is_prime(18)}")
    print(f"Primes up to 30: {sieve_of_eratosthenes(30)}")
    print(f"GCD(48, 18): {gcd(48, 18)}")
    print(f"LCM(48, 18): {lcm(48, 18)}")
    
    # String processing
    print("\n--- String Processing ---")
    test_string = "A man a plan a canal Panama"
    print(f"Is '{test_string}' a palindrome: {palindrome_check(test_string)}")
    print(f"Are 'listen' and 'silent' anagrams: {anagram_check('listen', 'silent')}")
    
    text = "Hello world! This is a test. Hello again!"
    print(f"Word frequency: {word_frequency(text)}")
    print(f"Compressed 'AAABBBCC': {compress_string('AAABBBCC')}")
    
    # Sorting algorithms
    print("\n--- Sorting Algorithms ---")
    unsorted = [64, 34, 25, 12, 22, 11, 90]
    print(f"Original: {unsorted}")
    print(f"Quick sort: {quick_sort(unsorted)}")
    print(f"Merge sort: {merge_sort(unsorted)}")
    
    # Data analysis
    print("\n--- Data Analysis ---")
    data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    print(f"Data: {data}")
    print(f"Mean: {mean(data):.2f}")
    print(f"Median: {median(data)}")
    print(f"Mode: {mode(data)}")
    print(f"Standard deviation: {standard_deviation(data):.2f}")
    
    x_data = [1, 2, 3, 4, 5]
    y_data = [2, 4, 6, 8, 10]
    print(f"Correlation between {x_data} and {y_data}: {correlation(x_data, y_data):.3f}")
    
    # Pattern matching
    print("\n--- Pattern Matching ---")
    test_values = [42, -5, 0, "hello", [1, 2, 3], {"a": 1, "b": 2}]
    for value in test_values:
        print(f"{value} -> {process_value(value)}")
    
    # Advanced pattern matching
    advanced_pattern_matching()
    
    # Generators
    print("\n--- Generators ---")
    print("First 10 Fibonacci numbers:")
    for i, num in enumerate(fibonacci_generator(10)):
        print(f"  {i}: {num}")
    
    print("\nFirst 5 prime numbers:")
    prime_gen = prime_generator()
    for _ in range(5):
        print(f"  {next(prime_gen)}")
    
    # Context managers
    print("\n--- Context Managers ---")
    with Timer("data processing"):
        # Simulate some work
        time.sleep(0.1)
        result = sum(range(1000))
        print(f"Processing result: {result}")
    
    with DatabaseConnection("postgresql://localhost/mydb"):
        print("Executing database query...")
        # Simulate database work
        time.sleep(0.05)
    
    # Decorators
    print("\n--- Decorators ---")
    
    @validate_types(x=int, y=int)
    def add(x, y):
        return x + y
    
    try:
        print(f"add(5, 3): {add(5, 3)}")
        print(f"add(5, '3'): {add(5, '3')}")  # Should raise TypeError
    except TypeError as e:
        print(f"TypeError caught: {e}")
    
    @memoize_with_size(max_size=3)
    def expensive_function(x):
        print(f"Computing expensive_function({x})")
        return x * x
    
    print(f"expensive_function(5): {expensive_function(5)}")
    print(f"expensive_function(5): {expensive_function(5)}")  # From cache
    print(f"expensive_function(3): {expensive_function(3)}")
    print(f"expensive_function(7): {expensive_function(7)}")
    print(f"expensive_function(5): {expensive_function(5)}")  # From cache
    print(f"expensive_function(3): {expensive_function(3)}")  # From cache
    
    # Properties and descriptors
    print("\n--- Properties and Descriptors ---")
    circle = Circle(5)
    print(f"Circle radius: {circle.radius}")
    print(f"Circle area: {circle.area:.2f}")
    print(f"Circle circumference: {circle.circumference:.2f}")
    
    try:
        circle.radius = -1
    except ValueError as e:
        print(f"ValueError caught: {e}")
    
    person = Person()
    person.name = "John Doe"
    person.age = 30
    person.email = "john@example.com"
    
    print(f"Person: name={person.name}, age={person.age}, email={person.email}")
    
    try:
        person.age = -5
    except ValueError as e:
        print(f"ValueError caught: {e}")
    
    # Metaclasses
    print("\n--- Metaclasses ---")
    logger1 = Logger()
    logger2 = Logger()
    
    logger1.log("First message")
    logger2.log("Second message")
    
    print(f"Same instance? {logger1 is logger2}")
    
    # Async/await (would need asyncio.run() in real usage)
    print("\n--- Async/Await ---")
    print("Async functions defined but not executed (requires asyncio.run())")
    print(f"fetch_data function: {fetch_data}")
    print(f"process_multiple_urls function: {process_multiple_urls}")
    
    # Standard library modules
    print("\n--- Standard Library Modules ---")
    print(f"Math pi: {math.pi}")
    print(f"Math e: {math.e}")
    print(f"Math sqrt(16): {math.sqrt(16)}")
    print(f"Random choice: {random.choice([1, 2, 3, 4, 5])}")
    print(f"Time time: {time.time()}")
    print(f"JSON dumps: {json.dumps({'key': 'value', 'number': 42})}")
    
    # Collections
    Point = namedtuple('Point', ['x', 'y'])
    p = Point(3, 4)
    print(f"Named tuple Point: {p}")
    print(f"Point x: {p.x}, y: {p.y}")
    
    d = deque([1, 2, 3])
    d.append(4)
    d.appendleft(0)
    print(f"Deque: {list(d)}")
    
    counter = Counter(['a', 'b', 'a', 'c', 'b', 'a'])
    print(f"Counter: {dict(counter)}")
    
    # Itertools
    print("\n--- Itertools ---")
    print(f"Chain: {list(chain([1, 2], [3, 4]))}")
    print(f"Count: {list(islice(count(10), 5))}")
    print(f"Product: {list(product([1, 2], ['a', 'b']))}")
    print(f"Permutations: {list(permutations([1, 2, 3], 2))}")
    print(f"Combinations: {list(combinations([1, 2, 3, 4], 2))}")
    
    # Functools
    print("\n--- Functools ---")
    numbers_list = [[1, 2], [3, 4], [5, 6]]
    print(f"Reduce with sum: {reduce(lambda x, y: x + y, numbers_list)}")
    
    multiply_by_2 = partial(lambda x, y: x * y, 2)
    print(f"Partial multiply_by_2(5): {multiply_by_2(5)}")
    
    # Operator
    print("\n--- Operator Module ---")
    people = [{'name': 'Alice', 'age': 30}, {'name': 'Bob', 'age': 25}]
    get_name = itemgetter('name')
    print(f"Get names: {list(map(get_name, people))}")
    
    print("\n" + "="*60)
    print("All Python++ features demonstrated successfully!")
    print("This shows comprehensive Python syntax and standard library support.")

if __name__ == "__main__":
    main()