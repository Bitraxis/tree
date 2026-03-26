# Tree Language Documentation

## Overview

Tree is a statically-typed, indentation-sensitive programming language inspired by TCL, TOML, and functional programming paradigms. It emphasizes clarity through naming conventions, explicit type signatures, and clean syntax.

## Core Features

- **Static Typing via Naming Conventions**: Type is inferred from function/variable naming patterns
- **Indentation-Sensitive**: Python-like block structure using indentation
- **Function Calls Require Parentheses**: `print()` not `print`
- **Multiple Container Types**: Lists `[]`, Dictionaries `{}`
- **Semicolon-Terminated Statements**: All statements end with `;`
- **Configuration-Aware**: Built-in config syntax `key=value` in containers
- **Extensible**: Easy to add custom functions and type families

---

## Syntax Reference

### Statements

**Assignment:**
```tree
variable = expression;
```

```tree
## Packages & Modules
function_name(arg1, arg2, arg3);
Tree supports loading packages to access additional functionality. Packages provide namespaced functions that can be called using the `namespace::function()` syntax.
```
### Package Import Syntax

```tree
pkg(){
  "package_name";
  "another_package";
} [as alias];
```
**All statements must end with `;`**
- **Without alias**: Functions are called as `package_name::function()`
- **With alias**: Functions are called as `alias::function()`

### Built-in Packages
---
#### Math Package

Provides mathematical functions and constants for floating-point operations.
## Naming Conventions & Type Families
**Import:**
```tree
pkg(){
  "math";
};
```

**Available Functions:**
- `math::sqrt(x)` - Square root
- `math::pow(base, exp)` - Power function
- `math::abs(x)` - Absolute value
- `math::sin(x)` - Sine (radians)
- `math::cos(x)` - Cosine (radians)
- `math::floor(x)` - Floor function
- `math::ceil(x)` - Ceiling function
Tree uses naming patterns to encode type information:
**Example:**
```tree
pkg(){
  "math";
};

x = math::sqrt(25.0);
print("sqrt(25) = ", x);  # Output: sqrt(25) = 5
### 1. **camelCase with dots (`.`)** → String Family
y = math::pow(2.0, 10.0);
print("2^10 = ", y);  # Output: 2^10 = 1024
```
- Used for string operations and conversions
#### Random Package
- Examples: `to.String()`, `to.Upper()`, `to.Lower()`
Provides pseudo-random number generation functions.
- Pattern: `identifier.identifier()`
**Import:**
```tree
pkg(){
  "random";
};
```

**Available Functions:**
- `random::rand()` - Random float between 0.0 and 1.0
- `random::rand-int(max)` - Random integer from 0 to max-1
- `random::rand-seed(seed)` - Set random seed
```tree
**Example:**
```tree
pkg(){
  "random";
};
x = "hello";
random::rand-seed(42);  # Set seed for reproducibility
r = random::rand();
print("Random: ", r);
result = to.String(x);
dice = random::rand-int(6);  # Roll a 6-sided die
print("Dice roll: ", To,Add(dice, 1));  # 1-6
```
```
### Using Aliases

```tree
pkg(){
  "math";
  "random";
} as utils;
### 2. **PascalCase with commas (`,`)** → Integer Family
x = utils::sqrt(16.0);  # math::sqrt now called as utils::sqrt
y = utils::rand();      # random::rand now called as utils::rand
```
- Used for integer operations and conversions
- Pattern: `Identifier,Identifier()`

```tree
x = 42;
result = To,Int(3.14);
- Used for floating-point operations and conversions
- Examples: `to_float()`, `to_sqrt()`
- Pattern: `identifier_identifier()`

```tree
x = 3.14;
result = to_float(42);
```

### 4. **kebab-case with dashes (`-`)** → Container Operations
- Used for list and dictionary manipulations
- Examples: `append-item()`, `to-list[]`, `to-dict{}`
- Pattern: `identifier-identifier[]` or `identifier-identifier{}`

```tree
list = [1, 2, 3];
result = append-item(list, 4);
```

### 5. **namespace:: (double colon)** → External/Namespaced Functions
- Used for external or scoped functions
- Examples: `https::Link()`, `file::Read()`
- Pattern: `namespace::function()`

```tree
url = "https://example.com";
response = https::Link(url);
```

---

## Built-in Functions

### Output
**`print(...)`**
- Prints all arguments separated by no delimiter, followed by newline
- Returns: `nil`
- Example:
  ```tree
  print("Hello", " ", "World");  # Output: Hello World
  ```

### Type Conversion

**`to.String(value)`**
- Converts any value to string representation
- Returns: string
- Example:
  ```tree
  x = 42;
  s = to.String(x);  # s = "42"
  ```

**`To,Int(value)`**
- Converts string, float, or int to integer
- Returns: integer
- Example:
  ```tree
  x = "123";
  i = To,Int(x);  # i = 123
  ```

**`to_float(value)`**
- Converts string, int, or float to floating-point
- Returns: float
- Example:
  ```tree
  x = 42;
  f = to_float(x);  # f = 42.0
  ```

### Math Functions (Float Family - `snake_case`)

**`to_sqrt(value)`** - Square root
```tree
result = to_sqrt(16.0);  # 4.0
```

**`to_pow(base, exponent)`** - Power function
```tree
result = to_pow(2.0, 3.0);  # 8.0
```

**`to_abs(value)`** - Absolute value
```tree
result = to_abs(-42);  # 42
```

**`to_sin(radians)`** - Sine
```tree
result = to_sin(0.0);  # 0.0
```

**`to_cos(radians)`** - Cosine
```tree
result = to_cos(0.0);  # 1.0
```

**`to_floor(value)`** - Floor (round down)
```tree
result = to_floor(3.7);  # 3.0
```

**`to_ceil(value)`** - Ceiling (round up)
```tree
result = to_ceil(3.2);  # 4.0
```

### Integer Math Functions (Integer Family - `Pascal,Case`)

**`To,Add(a, b)`** - Addition
```tree
result = To,Add(10, 5);  # 15
```

**`To,Sub(a, b)`** - Subtraction
```tree
result = To,Sub(10, 3);  # 7
```

**`To,Mul(a, b)`** - Multiplication
```tree
result = To,Mul(4, 5);  # 20
```

**`To,Div(a, b)`** - Integer division
```tree
result = To,Div(20, 3);  # 6
```

### String Functions (String Family - `dot.case`)

**`to.Length(value)`** - Get length (works on strings, lists, dicts)
- Returns: integer
- Example:
  ```tree
  len = to.Length("hello");  # 5
  list_len = to.Length([1, 2, 3]);  # 3
  ```

**`to.Upper(string)`** - Convert to uppercase
- Returns: string
- Example:
  ```tree
  result = to.Upper("hello");  # "HELLO"
  ```

**`to.Lower(string)`** - Convert to lowercase
- Returns: string
- Example:
  ```tree
  result = to.Lower("HELLO");  # "hello"
  ```

### List & Container Operations (Kebab Family - `kebab-case`)

**`get-length(container)`** - Get number of items
- Returns: integer
- Example:
  ```tree
  mylist = [1, 2, 3];
  count = get-length(mylist);  # 3
  ```

**`get-first(list)`** - Get first item
- Returns: any type (or nil if empty)
- Example:
  ```tree
  mylist = [10, 20, 30];
  first = get-first(mylist);  # 10
  ```

**`get-last(list)`** - Get last item
- Returns: any type (or nil if empty)
- Example:
  ```tree
  mylist = [10, 20, 30];
  last = get-last(mylist);  # 30
  ```

**`get-type(value)`** - Get type of value
- Returns: string ("nil", "int", "float", "string", "list", or "dict")
- Example:
  ```tree
  type_name = get-type(42);     # "int"
  type_name = get-type("hi");   # "string"
  type_name = get-type([1,2]);  # "list"
  ```

---

## Data Types & Literals

### Integers
```tree
x = 42;
y = -10;
z = 0;
```

### Floating-Point Numbers
```tree
pi = 3.14159;
neg = -2.5;
small = 0.001;
```

### Strings
- Enclosed in double quotes `"`
- Supports escape sequences: `\n`, `\t`, `\\`, `\"`

```tree
greeting = "Hello, World!";
multiline = "Line 1\nLine 2";
```

### Lists
- Ordered collections enclosed in `[]`
- Supports mixed types and configuration entries

```tree
simple_list = [1, 2, 3];
mixed_list = ["hello", 42, 3.14];
config_list = [e="\n", mode="fast", 100];
```

**Accessing List Items:**
```tree
items = [1, 2, 3];
# Note: Direct indexing not yet implemented
```

### Dictionaries
- Key-value collections enclosed in `{}`
- Keys must be identifiers, values are expressions
- All key-value pairs use `key=value` syntax

```tree
person = {name="Alice", age=To,Int(30), active=1};
config = {host="localhost", port=To,Int(8080)};
```

**Accessing Dictionary Values:**
```tree
person = {name="Alice", age=To,Int(30)};
# Note: Direct access not yet implemented
```

---

## Variables & Assignment

**Declaration and Assignment:**
```tree
name = "John";
age = To,Int(25);
height = to_float(180.5);
```

**Reassignment:**
```tree
x = 10;
x = To,Int(20);
```

**Type Consistency:**
Tree enforces static typing. Once a variable holds a type, reassignment to incompatible types may produce warnings (depends on implementation).

```tree
x = 42;
x = to_float(3.14);  # Type change: int → float
name = "Alice";
# name = To,Int(123);  # ✗ Would violate type consistency
```

---

## Comments

Currently, comments are **not implemented** in this version.

Future syntax may use:
- `#` for line comments
- `/* */` for block comments

---

## Program Structure

Every Tree program is a sequence of statements:

```tree
print("Starting Tree program");

x = To,Int(10);
y = to_float(3.14);

name = to.String(x);
print("x as string: ", name);

numbers = [1, 2, 3, 4, 5];
config = {timeout=To,Int(30), retries=To,Int(3)};

print("Done");
```

---

## Examples

### Example 1: Simple Output
```tree
print("Welcome to Tree!");
name = "Alice";
print("Hello, ", name);
```

**Output:**
```
Welcome to Tree!
Hello, Alice
```

### Example 2: Type Conversions
```tree
string_num = "42";
int_value = To,Int(string_num);
float_value = to_float(int_value);
back_to_string = to.String(float_value);

print("Original: ", string_num);
print("As int: ", to.String(int_value));
print("As float: ", to.String(float_value));
print("Back to string: ", back_to_string);
```

### Example 3: Collections
```tree
ids = [1, 2, 3];
prefs = [mode="debug", level=To,Int(5)];

user = {name="Bob", age=To,Int(30), active=1};

print("User: ", to.String(user));
```

### Example 4: Math Operations
```tree
x = to_float(10.5);
y = to_sqrt(x);
z = to_pow(2.0, 3.0);
sum = To,Add(10, 5);
product = To,Mul(4, 7);

print("Square root of 10.5: ", to.String(y));
print("2^3 = ", to.String(z));
print("10 + 5 = ", to.String(sum));
print("4 * 7 = ", to.String(product));
```

### Example 5: String Operations
```tree
text = "Hello World";
upper = to.Upper(text);
lower = to.Lower(text);
length = to.Length(text);

print("Original: ", text);
print("Uppercase: ", upper);
print("Lowercase: ", lower);
print("Length: ", to.String(length));
```

### Example 6: List Operations
```tree
numbers = [10, 20, 30, 40, 50];
list_len = get-length(numbers);
first = get-first(numbers);
last = get-last(numbers);
num_type = get-type(first);

print("List length: ", to.String(list_len));
print("First item: ", to.String(first));
print("Last item: ", to.String(last));
print("Type of first: ", num_type);
```

---

## Execution

### Running a Tree Script

```bash
./tree_parser script.tree
```

The interpreter will:
1. **Lex** the input (tokenization)
2. **Parse** the tokens into an AST
3. **Evaluate** the AST and execute statements
4. Output results to stdout

### Error Handling

- **Lexer errors**: Invalid characters, unterminated strings
- **Parser errors**: Unexpected tokens, missing semicolons
- **Runtime errors**: Undefined variables, wrong function arguments

Examples:
```
Lexer error at 2:15: unterminated string
Parse error at 1:8: expected ;, got 'x'
Runtime error: Undefined variable: foo
```

---

## Type System Summary

| Pattern | Family | Examples | Usage |
|---------|--------|----------|-------|
| `dot.case` | String | `to.String()`, `to.Upper()` | String transformation |
| `Pascal,Case` | Integer | `To,Int()`, `To,Add()` | Integer conversion/ops |
| `snake_case` | Float | `to_float()`, `to_sqrt()` | Float conversion/ops |
| `kebab-case[]` | List Ops | `append-item()`, `to-list[]` | Container manipulation |
| `namespace::` | External | `https::Link()`, `file::Read()` | Scoped/external functions |

---

## Language Philosophy

1. **Clarity through Convention**: Types are visible in naming, not annotations
2. **Explicit Over Implicit**: All function calls require `()`, all statements end with `;`
3. **Simplicity**: Minimal keywords, maximum expressiveness
4. **Extensibility**: Easy plugin system for new functions and types
5. **Familiar Yet Unique**: Combines Python indentation, TCL simplicity, TOML data structures

---

## Future Extensions

Planned features:
- Conditional statements (`if`/`else`)
- Loops (`for`, `while`)
- Custom functions (`def`)
- List indexing `[n]` and dictionary access `[key]`
- Operators: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`
- Comments `#` and `/* */`
- Modules and imports
- Error handling (`try`/`catch`)
- Pattern matching

---

## License & Attribution

Tree is an educational programming language combining ideas from:
- **TCL**: Command-based syntax
- **TOML**: Clean configuration format
- **Python**: Indentation-based structure
- **Scheme/Lisp**: Functional paradigms

---

## Quick Reference

```tree
# Assignment
variable = expression;

# Function calls (all need parentheses)
print("Hello");
result = To,Int("42");
value = to.String(x);

# Conversions
s = to.String(42);        # int → string
i = To,Int("100");        # string → int
f = to_float(50);         # int → float

# Math
sqrt_val = to_sqrt(16.0); # 4.0
power = to_pow(2.0, 3.0); # 8.0
abs_val = to_abs(-10);    # 10

# Integer arithmetic
sum = To,Add(5, 3);       # 8
diff = To,Sub(10, 4);     # 6
prod = To,Mul(3, 4);      # 12
quot = To,Div(20, 4);     # 5

# Strings
upper = to.Upper("hi");   # "HI"
lower = to.Lower("HELLO"); # "hello"
len = to.Length("abc");   # 3

# Lists
list = [1, 2, 3, mode="fast"];
first = get-first(list);
last = get-last(list);
len = get-length(list);
type = get-type(value);

# Dictionaries
dict = {name="Alice", age=To,Int(30)};
```

# Packages
pkg(){
  "math";
  "random";
};

# Math package functions
sqrt_25 = math::sqrt(25.0);      # 5.0
power = math::pow(2.0, 8.0);     # 256.0
abs_val = math::abs(-42);        # 42
---
# Random package functions
random::rand-seed(99);
random_float = random::rand();   # 0.0 to 1.0
random_int = random::rand-int(100); # 0 to 99

# Package alias
pkg(){
  "math";
} as m;
result = m::sqrt(16.0);          # 4.0
**Last Updated**: March 2026
