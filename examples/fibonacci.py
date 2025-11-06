# Fibonacci calculation
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

# Test the function
result = fibonacci(10)
print(f"fibonacci(10) = {result}")

# Test with different values
for i in range(6):
    print(f"fibonacci({i}) = {fibonacci(i)}")