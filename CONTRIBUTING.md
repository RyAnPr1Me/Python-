# Contributing to Python++

Thank you for your interest in contributing to Python++! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

Please be respectful and constructive in all interactions with the project and community.

## Getting Started

### Prerequisites

Before you begin, ensure you have the following installed:
- CMake 3.16 or higher
- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- LLVM 15+
- Python 3.10+ with development headers
- Git

### Building from Source

1. Clone the repository:
```bash
git clone <repository-url>
cd Python-
```

2. Create a build directory:
```bash
mkdir build
cd build
```

3. Configure with CMake:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

4. Build:
```bash
make -j$(nproc)
```

5. Run tests (if available):
```bash
make test
```

## Development Workflow

### Making Changes

1. **Create a new branch** for your changes:
```bash
git checkout -b feature/your-feature-name
```

2. **Make your changes** following the coding guidelines below

3. **Test your changes** thoroughly:
   - Build the project without errors
   - Run existing tests
   - Add new tests for your changes if applicable

4. **Commit your changes** with clear, descriptive messages:
```bash
git add .
git commit -m "Brief description of changes"
```

5. **Push your changes** and create a pull request:
```bash
git push origin feature/your-feature-name
```

## Coding Guidelines

### C++ Style

- Use C++20 features where appropriate
- Follow the existing code style in the repository
- Use meaningful variable and function names
- Add comments for complex logic

### Code Quality

- **No magic numbers**: Use named constants or enums
- **Error handling**: Always check return values and handle errors appropriately
  - Use specific exception types, not `catch(...)`
  - Check pointers for null before dereferencing
- **Memory safety**: 
  - Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
  - Use RAII for resource management
  - Avoid manual `new`/`delete` where possible
- **Security**:
  - Always escape shell arguments when using `system()` calls
  - Validate all user input
  - Check array bounds before access

### CMake

- Keep CMakeLists.txt files organized and commented
- Use target-based CMake commands
- Set appropriate include directories and link libraries

### Documentation

- Document public APIs with comments
- Update README.md if adding new features
- Add inline comments for complex algorithms

## Testing

- Write tests for new features
- Ensure existing tests pass
- Test on multiple platforms if possible (Linux, macOS, Windows)

## Security

If you discover a security vulnerability, please email the maintainers directly rather than opening a public issue.

### Security Best Practices

- Always validate input from external sources
- Use safe string operations (avoid buffer overflows)
- Properly escape shell commands
- Check return values from system calls
- Handle exceptions appropriately

## Pull Request Process

1. Ensure your code builds without warnings
2. Update documentation as needed
3. Add tests for new functionality
4. Ensure all tests pass
5. Follow the commit message guidelines
6. Request review from maintainers

### Commit Message Guidelines

- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- First line should be a brief summary (50 chars or less)
- Add detailed description after a blank line if needed

Example:
```
Fix shell injection vulnerability in linker

Added escapeShellArg() function to properly escape file paths
before passing them to system() calls. This prevents command
injection attacks when processing user-provided filenames.
```

## Areas for Contribution

### High Priority

- Bug fixes
- Security improvements
- Performance optimizations
- Test coverage improvements
- Documentation improvements

### Feature Development

- New Python language features
- Optimization passes
- Standard library implementations
- Platform support (additional architectures)

### Good First Issues

Look for issues tagged with "good first issue" in the issue tracker. These are typically:
- Documentation improvements
- Simple bug fixes
- Code cleanup
- Test additions

## Questions?

If you have questions about contributing, feel free to:
- Open an issue with your question
- Check existing documentation
- Review closed pull requests for examples

Thank you for contributing to Python++!
