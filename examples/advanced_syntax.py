# Advanced Python Syntax Examples
# Demonstrates decorators, generators, async/await, comprehensions, and more

from typing import List, Dict, Optional, Union, Callable, Any, TypeVar, Generic, Protocol, TypedDict, Literal

# Type variables
T = TypeVar('T')
U = TypeVar('U')

# Protocol definition
class Drawable(Protocol):
    def draw(self) -> None: ...
    def area(self) -> float: ...

# TypedDict definition
class Person(TypedDict):
    name: str
    age: int
    email: Optional[str]

# Decorator examples
def timer(func: Callable[..., Any]) -> Callable[..., Any]:
    """Decorator to time function execution"""
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        import time
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()
        print(f"{func.__name__} took {end - start:.4f} seconds")
        return result
    return wrapper

def validate_types(**type_hints: type) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
    """Decorator to validate argument types"""
    def decorator(func: Callable[..., Any]) -> Callable[..., Any]:
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            # Type validation logic here
            return func(*args, **kwargs)
        return wrapper
    return decorator

# Class with decorators
@timer
@validate_types(x=int, y=int)
class Calculator:
    """A calculator class with type hints and decorators"""
    
    def __init__(self, precision: int = 2) -> None:
        self.precision: int = precision
        self._history: List[str] = []
    
    @property
    def history(self) -> List[str]:
        return self._history.copy()
    
    def add(self, x: float, y: float) -> float:
        result = round(x + y, self.precision)
        self._history.append(f"{x} + {y} = {result}")
        return result
    
    def multiply(self, x: float, y: float) -> float:
        result = round(x * y, self.precision)
        self._history.append(f"{x} * {y} = {result}")
        return result

# Generator function
def fibonacci_generator(n: int) -> Generator[int, None, None]:
    """Generate Fibonacci numbers up to n"""
    a, b = 0, 1
    count = 0
    while count < n:
        yield a
        a, b = b, a + b
        count += 1

# Async function examples
import asyncio
from typing import AsyncGenerator

async def fetch_data(url: str) -> Dict[str, Any]:
    """Async function to fetch data from URL"""
    await asyncio.sleep(0.1)  # Simulate network delay
    return {"url": url, "data": f"Data from {url}"}

async def process_data_stream() -> AsyncGenerator[Dict[str, Any], None]:
    """Async generator for processing data streams"""
    for i in range(5):
        data = await fetch_data(f"http://example.com/{i}")
        yield data

# Comprehensions
def demonstrate_comprehensions() -> None:
    """Demonstrate various comprehension types"""
    
    # List comprehension
    squares: List[int] = [x**2 for x in range(10) if x % 2 == 0]
    print(f"Squares of even numbers: {squares}")
    
    # Dict comprehension
    square_dict: Dict[int, int] = {x: x**2 for x in range(5)}
    print(f"Square dictionary: {square_dict}")
    
    # Set comprehension
    unique_squares: set[int] = {x**2 for x in range(10) if x % 3 == 0}
    print(f"Unique squares divisible by 3: {unique_squares}")
    
    # Generator expression
    sum_squares: int = sum(x**2 for x in range(100) if x % 7 == 0)
    print(f"Sum of squares divisible by 7: {sum_squares}")
    
    # Nested comprehensions
    matrix: List[List[int]] = [[i*j for j in range(3)] for i in range(3)]
    print(f"Multiplication table: {matrix}")

# Pattern matching (Python 3.10+)
def pattern_matching_example(value: Any) -> str:
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
        case {"name": name, "age": age}:
            return f"Person: {name}, age {age}"
        case _:
            return f"Unknown type: {type(value).__name__}"

# Generic class
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

# Context manager
class TimerContext:
    """Context manager for timing operations"""
    
    def __enter__(self) -> 'TimerContext':
        import time
        self.start_time = time.time()
        return self
    
    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        import time
        end_time = time.time()
        print(f"Operation took {end_time - self.start_time:.4f} seconds")

# Exception handling with type hints
class CustomError(Exception):
    """Custom exception with additional data"""
    
    def __init__(self, message: str, error_code: int = 0) -> None:
        super().__init__(message)
        self.error_code: int = error_code

def demonstrate_error_handling() -> None:
    """Demonstrate advanced error handling"""
    try:
        raise CustomError("Something went wrong", 404)
    except CustomError as e:
        print(f"Custom error: {e}, code: {e.error_code}")
    except Exception as e:
        print(f"Generic error: {e}")
    finally:
        print("Cleanup completed")

# Main demonstration
async def main() -> None:
    """Main function demonstrating all features"""
    
    print("=== Advanced Python Syntax Demonstration ===\n")
    
    # Class with decorators
    calc = Calculator(precision=3)
    result1 = calc.add(2.5, 3.7)
    result2 = calc.multiply(4.2, 1.8)
    print(f"Calculator results: {result1}, {result2}")
    print(f"History: {calc.history}\n")
    
    # Generator
    print("Fibonacci sequence:")
    fib_gen = fibonacci_generator(10)
    for num in fib_gen:
        print(num, end=" ")
    print("\n")
    
    # Async operations
    print("Async data fetching:")
    async for data in process_data_stream():
        print(f"Received: {data}")
    print()
    
    # Comprehensions
    print("Comprehensions:")
    demonstrate_comprehensions()
    print()
    
    # Pattern matching
    print("Pattern matching:")
    test_values = [42, -5, 0, "hello", [1, 2, 3], {"name": "Alice", "age": 30}]
    for value in test_values:
        result = pattern_matching_example(value)
        print(f"{value} -> {result}")
    print()
    
    # Generic class
    print("Generic stack:")
    int_stack = Stack[int]()
    int_stack.push(1)
    int_stack.push(2)
    int_stack.push(3)
    while not int_stack.is_empty():
        print(f"Popped: {int_stack.pop()}")
    print()
    
    # Context manager
    print("Context manager:")
    with TimerContext():
        import time
        time.sleep(0.1)
    print()
    
    # Error handling
    print("Error handling:")
    demonstrate_error_handling()
    print()
    
    # Type annotations with complex types
    def complex_function(
        data: Dict[str, Union[List[int], Dict[str, float]]],
        callback: Optional[Callable[[str], None]] = None
    ) -> Literal["success", "error"]:
        """Function with complex type annotations"""
        if callback:
            callback("Processing data")
        return "success"
    
    result = complex_function({"numbers": [1, 2, 3], "values": {"a": 1.5, "b": 2.7}})
    print(f"Complex function result: {result}")

if __name__ == "__main__":
    asyncio.run(main())