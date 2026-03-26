# Tree Language - Built-in Functions Reference

## Quick Command Reference

| Name | Family | Signature | Returns | Example |
|------|--------|-----------|---------|---------|
| `print` | Output | `print(...args)` | nil | `print("x = ", 42);` |
| `to.String` | String | `to.String(value)` | string | `to.String(42)` → `"42"` |
| `To,Int` | Integer | `To,Int(value)` | int | `To,Int("42")` → `42` |
| `to_float` | Float | `to_float(value)` | float | `to_float(42)` → `42.0` |
| `to_sqrt` | Math/Float | `to_sqrt(x)` | float | `to_sqrt(16.0)` → `4.0` |
| `to_pow` | Math/Float | `to_pow(base, exp)` | float | `to_pow(2.0, 3.0)` → `8.0` |
| `to_abs` | Math/Float | `to_abs(x)` | float/int | `to_abs(-5)` → `5` |
| `to_sin` | Math/Float | `to_sin(radians)` | float | `to_sin(0.0)` → `0.0` |
| `to_cos` | Math/Float | `to_cos(radians)` | float | `to_cos(0.0)` → `1.0` |
| `to_floor` | Math/Float | `to_floor(x)` | float | `to_floor(3.7)` → `3.0` |
| `to_ceil` | Math/Float | `to_ceil(x)` | float | `to_ceil(3.2)` → `4.0` |
| `To,Add` | Math/Integer | `To,Add(a, b)` | int | `To,Add(5, 3)` → `8` |
| `To,Sub` | Math/Integer | `To,Sub(a, b)` | int | `To,Sub(10, 3)` → `7` |
| `To,Mul` | Math/Integer | `To,Mul(a, b)` | int | `To,Mul(4, 5)` → `20` |
| `To,Div` | Math/Integer | `To,Div(a, b)` | int | `To,Div(20, 4)` → `5` |
| `to.Length` | String | `to.Length(value)` | int | `to.Length("hi")` → `2` |
| `to.Upper` | String | `to.Upper(string)` | string | `to.Upper("hi")` → `"HI"` |
| `to.Lower` | String | `to.Lower(string)` | string | `to.Lower("HI")` → `"hi"` |
| `get-length` | Container | `get-length(container)` | int | `get-length([1,2])` → `2` |
| `get-first` | Container | `get-first(list)` | any | `get-first([10,20])` → `10` |
| `get-last` | Container | `get-last(list)` | any | `get-last([10,20])` → `20` |
| `get-type` | Container | `get-type(value)` | string | `get-type(42)` → `"int"` |

---

## Functions by Category

### 1. Output Functions

#### `print(...)`
Prints all arguments concatenated (no spaces) followed by newline.

**Family**: Output  
**Returns**: `nil`  
**Signature**: `print(arg1, arg2, ...)`  

**Examples**:
```tree
print("Hello");                           # Output: Hello
print("x = ", 42);                        # Output: x = 42
print("a=", 1, ", b=", 2, ", c=", 3);   # Output: a=1, b=2, c=3
```

---

### 2. Type Conversion Functions

#### `to.String(value)` - String Family (dot.case)
Converts any value to its string representation.

**Returns**: `string`  
**Types Handled**: int, float, string, list, dict, nil  

**Examples**:
```tree
s1 = to.String(42);           # "42"
s2 = to.String(3.14);         # "3.14"
s3 = to.String("hello");      # "hello"
s4 = to.String([1, 2]);       # "[...]"
s5 = to.String({x=1});        # "{...}"
print("Value: ", to.String(99));
```

#### `To,Int(value)` - Integer Family (Pascal,Case)
Converts string, float, or int to integer.

**Returns**: `int`  
**Types Accepted**: string (parsed as base-10), float (truncated), int (identity)  

**Examples**:
```tree
i1 = To,Int("123");           # 123
i2 = To,Int(3.99);            # 3 (truncated)
i3 = To,Int(50);              # 50
port = To,Int("8080");
count = To,Int(args);
```

#### `to_float(value)` - Float Family (snake_case)
Converts string, int, or float to floating-point number.

**Returns**: `float`  
**Types Accepted**: string (parsed), int (promoted), float (identity)  

**Examples**:
```tree
f1 = to_float("3.14");        # 3.14
f2 = to_float(42);            # 42.0
f3 = to_float(2.71);          # 2.71
x = to_float(100) / to_float(3);
```

---

### 3. Math Functions (Float Family - snake_case)

All math functions work with integers or floats (integers are auto-promoted).

#### `to_sqrt(x)`
Square root of x.

**Returns**: `float`  
**Domain**: x ≥ 0

**Examples**:
```tree
print("sqrt(16) = ", to.String(to_sqrt(16.0)));  # 4.0
print("sqrt(2) = ", to.String(to_sqrt(2.0)));    # 1.414...
```

#### `to_pow(base, exponent)`
Raise base to the power of exponent.

**Returns**: `float`  
**Signature**: `to_pow(b, e)`  

**Examples**:
```tree
print(to.String(to_pow(2.0, 3.0)));   # 8.0
print(to.String(to_pow(10.0, 2.0)));  # 100.0
result = to_pow(to_float(5), to_float(2));  # 25.0
```

#### `to_abs(x)`
Absolute value of x.

**Returns**: `float` (if input is float) or `int` (if input is int)  

**Examples**:
```tree
print(to.String(to_abs(-42)));      # 42
print(to.String(to_abs(-3.14)));    # 3.14
value = to_abs(x);
```

#### `to_sin(radians)`
Sine of angle in radians.

**Returns**: `float`  

**Examples**:
```tree
print(to.String(to_sin(0.0)));      # 0 (sin(0))
print(to.String(to_sin(1.5708)));   # ~1.0 (sin(π/2))
```

#### `to_cos(radians)`
Cosine of angle in radians.

**Returns**: `float`  

**Examples**:
```tree
print(to.String(to_cos(0.0)));      # 1.0 (cos(0))
print(to.String(to_cos(3.14159)));  # ~-1.0 (cos(π))
```

#### `to_floor(x)`
Round x down to nearest integer.

**Returns**: `float`  

**Examples**:
```tree
print(to.String(to_floor(3.7)));    # 3.0
print(to.String(to_floor(-2.3)));   # -3.0 (rounds down)
rounded = to_floor(measurement);
```

#### `to_ceil(x)`
Round x up to nearest integer.

**Returns**: `float`  

**Examples**:
```tree
print(to.String(to_ceil(3.2)));     # 4.0
print(to.String(to_ceil(2.0)));     # 2.0
rounded = to_ceil(pages);
```

---

### 4. Integer Math Functions (Integer Family - Pascal,Case)

These functions perform integer arithmetic and return integers.

#### `To,Add(a, b)`
Addition: a + b

**Returns**: `int`  

**Examples**:
```tree
sum = To,Add(5, 3);               # 8
total = To,Add(x, y);
product_sum = To,Add(To,Mul(2, 3), To,Mul(4, 5));  # 2*3 + 4*5 = 26
```

#### `To,Sub(a, b)`
Subtraction: a - b

**Returns**: `int`  

**Examples**:
```tree
diff = To,Sub(10, 3);             # 7
remaining = To,Sub(count, used);
```

#### `To,Mul(a, b)`
Multiplication: a × b

**Returns**: `int`  

**Examples**:
```tree
product = To,Mul(4, 5);           # 20
area = To,Mul(width, height);
```

#### `To,Div(a, b)`
Integer division: a ÷ b (truncates remainder)

**Returns**: `int`  
**Error**: Division by zero returns `nil` with error message  

**Examples**:
```tree
quotient = To,Div(20, 4);         # 5
quotient = To,Div(23, 5);         # 4 (not 4.6)
batches = To,Div(items, batch_size);
```

---

### 5. String Functions (String Family - dot.case)

#### `to.Length(value)`
Get length of string, list, or dictionary.

**Returns**: `int`  
**Works on**: strings, lists, dictionaries  

**Examples**:
```tree
len = to.Length("hello");         # 5
len = to.Length([1, 2, 3]);       # 3
len = to.Length({x=1, y=2});      # 2
print("String length: ", to.String(to.Length(text)));
```

#### `to.Upper(string)`
Convert string to uppercase.

**Returns**: `string`  
**Input**: string  

**Examples**:
```tree
upper = to.Upper("hello");        # "HELLO"
upper = to.Upper("HeLLo");        # "HELLO"
username = to.Upper(input);
```

#### `to.Lower(string)`
Convert string to lowercase.

**Returns**: `string`  
**Input**: string  

**Examples**:
```tree
lower = to.Lower("HELLO");        # "hello"
lower = to.Lower("HeLLo");        # "hello"
normalized = to.Lower(name);
```

---

### 6. Container/List Operations (Kebab Family - kebab-case)

#### `get-length(container)`
Get number of items in list or dictionary.

**Returns**: `int`  
**Works on**: lists, dictionaries  

**Examples**:
```tree
mylist = [10, 20, 30];
count = get-length(mylist);       # 3
mydict = {x=1, y=2};
count = get-length(mydict);       # 2
```

#### `get-first(list)`
Get first item from list.

**Returns**: any (or `nil` if empty)  
**Works on**: lists  

**Examples**:
```tree
mylist = [10, 20, 30];
first = get-first(mylist);        # 10
head = get-first([1, 2, 3]);      # 1
```

#### `get-last(list)`
Get last item from list.

**Returns**: any (or `nil` if empty)  
**Works on**: lists  

**Examples**:
```tree
mylist = [10, 20, 30];
last = get-last(mylist);          # 30
tail = get-last([1, 2, 3]);       # 3
```

#### `get-type(value)`
Get type name of value as string.

**Returns**: `string`  
**Returns one of**: `"nil"`, `"int"`, `"float"`, `"string"`, `"list"`, `"dict"`  

**Examples**:
```tree
type1 = get-type(42);             # "int"
type2 = get-type(3.14);           # "float"
type3 = get-type("hello");        # "string"
type4 = get-type([1, 2]);         # "list"
type5 = get-type({x=1});          # "dict"
type6 = get-type(nil_val);        # "nil"

typ = get-type(x);
print("Type of x: ", typ);
```

---

## Type Conversion Matrix

```
                FROM →
TO ↓         int    float   string  list   dict
int          -      trunc   parse   nil    nil
float        cast   -       parse   nil    nil
string       str    str     -       [...]  {...}
list         nil    nil     nil     -      nil
dict         nil    nil     nil     nil    -
```

**Legend**:
- `-` : Identity operation
- `trunc` : Truncate decimal
- `cast` : Promote to floating point
- `parse` : Parse string to number
- `str` : Convert to string representation
- `[...]` / `{...}` : Representation used in strings
- `nil` : Operation not supported, returns nil

---

## Error Handling

Functions handle edge cases gracefully:

```tree
# Division by zero
result = To,Div(10, 0);          # Returns nil, prints error

# Type mismatches
result = to.Upper(42);            # Returns nil (expects string)

# Empty containers
first = get-first([]);            # Returns nil

# Undefined variables
type = get-type(undefined_var);   # Returns "nil"
```

---

## Performance Notes

- All math functions are implemented with C `math.h` library
- String operations are O(n) where n = string length
- Container access (`get-first`, `get-last`, `get-length`) are O(1)
- Type conversions are O(1)

---

## Future Functions (Planned)

```tree
# Planned additions:
to_round(x);                      # Round to nearest integer
To,Mod(a, b);                     # Modulo (remainder)
to.Trim(s);                       # Remove leading/trailing whitespace
to.Split(s, delimiter);           # Split string into list
to.Join(list, delimiter);         # Join list into string
To,Max(a, b);                     # Integer maximum
To,Min(a, b);                     # Integer minimum
get-item(list, index);            # Get item by index
set-item(list, index, value);     # Set item by index
append-item(list, item);          # Add to end of list
```

---

**Last Updated**: March 2026  
**Total Functions**: 23  
**Categorized into**: 6 groups (Output, Conversion, Math/Float, Math/Integer, String, Container)
