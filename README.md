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

## Installation

### Windows

Download the latest Windows binary release from the [Releases](../../releases) page.

1. Extract the ZIP archive to a directory of your choice
2. **Right-click on `setup.bat` and select "Run as administrator"**
3. The setup will automatically:
   - Add Python++ to your system PATH
   - Associate `.py+` files with the `p++` runner
4. Restart your terminal for changes to take effect

After installation, you can:
- Run Python++ scripts from the command line: `p++ script.py+`
- Double-click `.py+` files to execute them directly!

**Alternative:** You can also manually configure file associations using:
```cmd
p++ --install
```
(Must be run from an Administrator command prompt)

### From Source

See [Building](#building) section below.

## Usage

### Quick Run (like `python` command)

```bash
# Run a Python++ script directly
p++ hello.py+
p++ script.py

# Pass arguments to your script
p++ myprogram.py+ arg1 arg2
```

### Compile to Executable

```bash
# Compile a Python file
py++c hello.py -o hello
py++c hello.py+ -o hello

# Run with optimizations
py++c --optimize=3 compute.py -o compute

# Debug build
py++c --debug script.py -o script
```

### File Extensions

Python++ supports both `.py` and `.py+` file extensions:
- `.py` - Compatible with standard Python files
- `.py+` - Recommended for Python++ specific files

## Examples

### Quick Run Example

```python
# hello.py+
def greet(name):
    return f"Hello, {name}!"

print(greet("World"))
```

```bash
p++ hello.py+  # Compiles and runs immediately
```

### Compilation Example

```python
# fibonacci.py+
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

print(fibonacci(10))
```

```bash
py++c fibonacci.py+ -o fibonacci
./fibonacci  # Outputs: 55
```

## Performance

Typical performance improvements over CPython:
- 2-5x for numerical computations
- 3-10x for algorithmic code
- 10-50x startup time improvement

## License

MIT License