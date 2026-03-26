# :tree:emoji:goes:here: - Tree and PkgTree repository

## Build and usage
---

- Needs CMake and Make

---
With pkgtree runtime (default):

```bash
make
./tree ../pkgtree/pkgtree.tree
```

Without pkgtree runtime (stubs only):

```bash
mkdir -p build
cd build
cmake -DWITH_PKGTREE=OFF ..
cmake --build . --config Release
./tree ../pkgtree/pkgtree.tree
```

### You can also compile directly with `gcc`:

With pkgtree:

```bash
gcc -o tree main.c pkgtree_runtime.c -lm
```

Without pkgtree:

```bash
gcc -o tree main.c -lm
```

## Quick Start

### Run

```bash
./tree_parser script.tree
```

### Example Script

Create `hello.tree`:
```tree
print("Hello from Tree!");
x = To,Int(42);
y = to_float(3.14);
name = to.String(x);
print("Answer: ", name);
```

Run it:
```bash
./tree_parser hello.tree
```

### Program 5: Math Package
```tree
pkg(){
    "math";
};

x = 16.0;
result = math::sqrt(x);
print("sqrt(16) = ", to.String(result));

base = 2.0;
exp = 10.0;
power = math::pow(base, exp);
print("2^10 = ", to.String(power));

angle = 0.0;
sine = math::sin(angle);
cosine = math::cos(angle);
print("sin(0) = ", to.String(sine));
print("cos(0) = ", to.String(cosine));
```
Output:
### Program 6: Random Package
```tree
pkg(){
    "random";
};

random::rand-seed(12345);

r1 = random::rand();
r2 = random::rand();
print("Random 1: ", to.String(r1));
print("Random 2: ", to.String(r2));

dice = random::rand-int(6);
print("Dice roll (0-5): ", to.String(dice));

coin = random::rand-int(2);
result = coin;
print("Coin (0=heads, 1=tails): ", to.String(result));
```
```
Answer: 42
```

---

## Features

✅ **Lexer**: Tokenizes Tree source with proper indentation tracking  
✅ **Parser**: Recursive descent parser building an AST  
✅ **Interpreter**: Direct AST evaluation with runtime values  
✅ **Static Typing**: Type inference via naming conventions  
✅ **Built-in Functions**: `print()`, `To,Int()`, `to_float()`, `to.String()`  
✅ **Collections**: Lists and dictionaries with config syntax  
✅ **Variables**: Symbol table with proper scoping  
✅ **Packages**: Built-in `math` and `random` packages with namespaced functions  


## Architecture

```
Source Code (.tree)
    ↓
Lexer (tokenization + indentation handling)
    ↓
Parser (AST construction)
    ↓
Interpreter (evaluation + execution)
    ↓
Output
```

### Core Components

- **`main.c`**: Complete implementation (~800 lines)
  - Lexer: `lex_all()`, token types
  - Parser: `parse_program()`, `parse_expr()`, `parse_stmt()`
  - Interpreter: `eval_program()`, `eval_expr()`, builtin functions
  - Value system: runtime type representation
  - Symbol table: variable storage

---

## Language Syntax

### Basics

```tree
# Variables and assignment
x = 42;
name = "Alice";

# Function calls require parentheses
print("Hello", " ", "World");

# Type conversion by naming convention
int_val = To,Int("100");           # PascalCase,WithComma → int
float_val = to_float(42);          # snake_case → float
str_val = to.String(int_val);      # dot.case → string

# Math operations
sqrt_16 = to_sqrt(16.0);           # 4.0
power = to_pow(2.0, 3.0);          # 8.0
sum = To,Add(5, 3);                # 8
product = To,Mul(4, 5);            # 20

# String operations
upper = to.Upper("hello");         # "HELLO"
len = to.Length("hello");          # 5
```

### Collections

```tree
# Lists (with optional config entries)
numbers = [1, 2, 3];
configured = [1, 2, 3, separator="\n", mode="fast"];

# Dictionaries
person = {name="Bob", age=To,Int(30), active=1};
```

### Functions

### Functions

| Category | Function | Returns | Notes |
|----------|----------|---------|-------|
| **Output** | `print(...)` | nil | Prints all args + newline |
| **Conversion** | `To,Int(x)` | int | String/float to int |
| | `to_float(x)` | float | String/int to float |
| | `to.String(x)` | string | Any to string |
| **Math (Float)** | `to_sqrt(x)` | float | Square root |
| | `to_pow(b,e)` | float | Power function |
| | `to_abs(x)` | float/int | Absolute value |
| | `to_sin(x)` | float | Sine (radians) |
| | `to_cos(x)` | float | Cosine (radians) |
| | `to_floor(x)` | float | Round down |
| | `to_ceil(x)` | float | Round up |
| **Integer Math** | `To,Add(a,b)` | int | Addition |
| | `To,Sub(a,b)` | int | Subtraction |
| | `To,Mul(a,b)` | int | Multiplication |
| | `To,Div(a,b)` | int | Integer division |
| **Strings** | `to.Length(s)` | int | String/list/dict length |
| | `to.Upper(s)` | string | Uppercase |
| | `to.Lower(s)` | string | Lowercase |
| **Containers** | `get-length(c)` | int | Number of items |
| | `get-first(l)` | any | First item |
| | `get-last(l)` | any | Last item |
| | `get-type(x)` | string | Type name (int/float/string/list/dict/nil) |

---

## File Structure

```
Domco/
├── main.c                    # Parser + interpreter implementation
├── TREE_LANGUAGE.md         # Full language documentation
└── README.md                # This file
```

---

## Example Programs

### Program 1: Variables and Conversions
```tree
x = To,Int(10);
y = to_float(x);
z = to.String(y);
print("Result: ", z);
```

### Program 2: Math Operations
```tree
x = to_float(16.0);
sqrt_x = to_sqrt(x);
print("sqrt(16) = ", to.String(sqrt_x));

a = 10;
b = 3;
sum = To,Add(a, b);
product = To,Mul(a, b);
print(to.String(a), " + ", to.String(b), " = ", to.String(sum));
print(to.String(a), " * ", to.String(b), " = ", to.String(product));
```

### Program 3: String Manipulation
```tree
text = "tree language";
upper_text = to.Upper(text);
lower_text = to.Lower(text);
length = to.String(to.Length(text));

print("Original: ", text);
print("Uppercase: ", upper_text);
print("Lowercase: ", lower_text);
print("Length: ", length);
```

### Program 4: List Operations
```tree
numbers = [5, 10, 15, 20, 25];
first = get-first(numbers);
last = get-last(numbers);
len = to.String(get-length(numbers));
item_type = get-type(first);

print("First: ", to.String(first));
print("Last: ", to.String(last));
print("Count: ", len);
print("Item type: ", item_type);
```

---

## Development Notes

### Adding New Built-in Functions

1. Create a `builtin_function_name()` function:
   ```c
   static Value builtin_my_func(AstNode **args, int argc, SymTable *table) {
       if (argc != expected_argc) {
           fprintf(stderr, "my_func expects %d argument(s)\n", expected_argc);
           return val_nil();
       }
       // Evaluate arguments and perform operation
       Value result = ...;
       return result;
   }
   ```

2. Register it in `eval_call()`:
   ```c
   if (strcmp(name, "my.Func") == 0) return builtin_my_func(args, argc, table);
   ```

### Extending the Type System

Add new naming patterns:
- Pattern: `lowercase_with_underscore()` for float ops (existing)
- Pattern: `identifier_identifier()` for new custom family
- Update `eval_call()` to recognize and dispatch

### Testing

Create `.tree` files and run:
```bash
./tree_parser test_file.tree
```

Check for:
- Correct output
- No memory leaks (use `valgrind` if available)
- Proper error messages on invalid syntax

---

## Limitations & TODOs

### Current Limitations

### Completed Features
✅ Package system with `pkg()` syntax
✅ Built-in `math` package (sqrt, pow, abs, sin, cos, floor, ceil)
✅ Built-in `random` package (rand, rand-int, rand-seed)
✅ Namespace resolution with `::` syntax
✅ Package aliases with `as` keyword
### Future Enhancements
1. **Operators**: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `and`, `or`
1. **Tree Package Loading**: Load `.tree` files as packages
2. **C Extension Loading**: Load compiled `.so`/`.dll` C extensions
3. **Package Versioning**: Support version checking (`pkg,version` syntax)
4. **Operators**: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `and`, `or`
5. **Indexing**: `list[n]`, `dict[key]`
6. **Control Flow**: `if`/`elsif`/`else`, `for`, `while`, `break`, `continue`
7. **Custom Functions**: `def` keyword for user-defined functions
8. **Comments**: `#` for line comments
9. **Error Handling**: `try`/`catch`, graceful error recovery
10. **Pattern Matching**: Destructuring for lists/dicts
---

## Building on Different Platforms

### Linux/macOS
---

```bash
gcc -o tree_parser main.c -lm
```

---

## Performance

- **Lexing**: O(n) where n = source code length
- **Parsing**: O(m) where m = number of tokens
- **Evaluation**: O(p × q) where p = program statements, q = avg expression depth

For typical scripts (<1000 LOC), execution is sub-millisecond.

---

## Contributing

To extend Tree:
1. Understand the lexer, parser, and interpreter flow
2. Add new tokens to `TokenKind` enum if needed
3. Extend the parser (`parse_expr`, `parse_stmt`)
4. Add evaluation logic (`eval_expr`, `eval_call`)
5. Test thoroughly with `.tree` examples

---

## License

- LGPL-v3
- ! AI was slightly used - *only* for help with errors that didn't *even exist on stackoverflow* = It was *lobotomized* so it **couldn't** write the actual full code !

---

## References

- **Language Design**: https://en.wikipedia.org/wiki/Programming_language
- **Interpretation**: https://craftinginterpreters.com/
- **C Memory**: Standard C library (`malloc`, `free`)

---

**Version**: 1.0  
**Created**: March 2026  
**Language**: C (C99 compatible)
