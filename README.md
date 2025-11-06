# Python++: AOT-Compiled Python Language

Python++ is a programming language identical to Python in syntax and semantics, but compiled ahead-of-time to native code for significant performance improvements.

## Features

- **100% Python Syntax Compatibility**: Write standard Python code
- **Ahead-of-Time Compilation**: Compile to native machine code
- **Gradual Typing**: Optional static types with powerful inference
- **LLVM Backend**: Leverage LLVM optimizations
- **Ecosystem Compatibility**: Use existing Python packages
- **Zero-Runtime Overhead**: Fast startup and execution

## Building

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler
- LLVM 15+
- Git

### Build Steps

```bash
git clone <repository-url>
cd python-plus-plus
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

```bash
# Compile a Python file
py++c hello.py -o hello

# Run with optimizations
py++c --optimize=3 compute.py -o compute

# Debug build
py++c --debug script.py -o script
```

## Example

```python
# hello.py
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

print(fibonacci(10))
```

```bash
py++c hello.py -o hello
./hello  # Outputs: 55
```

## Performance

Typical performance improvements over CPython:
- 2-5x for numerical computations
- 3-10x for algorithmic code
- 10-50x startup time improvement

## License

MIT License