# Control flow examples
# If-elif-else statements
def grade_classifier(score):
    if score >= 90:
        return "A"
    elif score >= 80:
        return "B"
    elif score >= 70:
        return "C"
    elif score >= 60:
        return "D"
    else:
        return "F"

# Test grade classifier
scores = [95, 85, 75, 65, 55]
for score in scores:
    grade = grade_classifier(score)
    print(f"Score {score}: Grade {grade}")

# While loop
def factorial(n):
    result = 1
    i = 1
    while i <= n:
        result *= i
        i += 1
    return result

print(f"5! = {factorial(5)}")
print(f"10! = {factorial(10)}")

# For loop with break and continue
def find_prime_numbers(limit):
    primes = []
    for num in range(2, limit + 1):
        is_prime = True
        for i in range(2, int(num ** 0.5) + 1):
            if num % i == 0:
                is_prime = False
                break
        if is_prime:
            primes.append(num)
    return primes

primes = find_prime_numbers(30)
print(f"Prime numbers up to 30: {primes}")

# Nested loops
def multiplication_table(n):
    for i in range(1, n + 1):
        row = ""
        for j in range(1, n + 1):
            row += f"{i * j:3d} "
        print(row)

print("Multiplication table (5x5):")
multiplication_table(5)

# Exception handling
def safe_divide(a, b):
    try:
        return a / b
    except ZeroDivisionError:
        return "Cannot divide by zero"
    except TypeError:
        return "Invalid types for division"

print(safe_divide(10, 2))
print(safe_divide(10, 0))
print(safe_divide("10", 2))