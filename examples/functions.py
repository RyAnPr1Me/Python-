# Function definitions and calls
def greet(name, greeting="Hello"):
    return f"{greeting}, {name}!"

def calculate_area(length, width):
    return length * width

def is_even(n):
    return n % 2 == 0

# Test functions
print(greet("Alice"))
print(greet("Bob", "Hi"))

area = calculate_area(5, 3)
print(f"Area of 5x3 rectangle: {area}")

for i in range(10):
    if is_even(i):
        print(f"{i} is even")
    else:
        print(f"{i} is odd")

# Function with variable arguments
def sum_all(*args):
    total = 0
    for num in args:
        total += num
    return total

print("Sum of 1, 2, 3, 4, 5:", sum_all(1, 2, 3, 4, 5))

# Function with keyword arguments
def create_person(**kwargs):
    person = {}
    for key, value in kwargs.items():
        person[key] = value
    return person

person = create_person(name="Charlie", age=25, city="London")
print("Created person:", person)