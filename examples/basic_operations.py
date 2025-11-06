# List operations demo
numbers = [1, 2, 3, 4, 5]
print("Original list:", numbers)

# Append and extend
numbers.append(6)
print("After append(6):", numbers)

numbers.extend([7, 8, 9])
print("After extend([7, 8, 9]):", numbers)

# List comprehension
squares = [x*x for x in range(10)]
print("Squares:", squares)

# String operations
text = "Hello, World!"
print("Original:", text)
print("Upper:", text.upper())
print("Lower:", text.lower())
print("Length:", len(text))

# Dictionary operations
person = {
    "name": "Alice",
    "age": 30,
    "city": "New York"
}

print("Dictionary:", person)
print("Keys:", list(person.keys()))
print("Values:", list(person.values()))
print("Has 'name':", "name" in person)

# Math operations
print("abs(-5):", abs(-5))
print("round(3.14159, 2):", round(3.14159, 2))
print("min([1, 5, 3]):", min([1, 5, 3]))
print("max([1, 5, 3]):", max([1, 5, 3]))
print("sum([1, 2, 3, 4]):", sum([1, 2, 3, 4]))