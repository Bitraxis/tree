# Tree Language

## Overview

Tree is an indentation-aware scripting language with a C runtime.

Core characteristics:

- Statements end with `;`
- Functions are called with parentheses
- `pkg(){ ... };` imports package namespaces
- Runtime values include `int`, `float`, `string`, `list`, `dict`, and `nil`

## Basic syntax

Assignment:

```tree
x = 42;
name = "tree";
```

Function call:

```tree
print("hello");
```

Container literals:

```tree
numbers = [1, 2, 3];
meta = {name="tree", version="1.0"};
```

## Packages

Import package namespaces:

```tree
pkg(){
    "math";
    "random";
};
```

Call namespaced functions:

```tree
x = math::sqrt(16.0);
r = random::rand();
```

Optional alias:

```tree
pkg(){
    "math";
} as m;

x = m::sqrt(25.0);
```

## Runtime arguments

When executing scripts via the C runtime, command-line args are exposed as:

- `argc`: argument count
- `argv`: argument list-like runtime value

Example usage in package scripts:

```tree
count = argc;
first = argv[0];
print("argc=", to.String(count));
```

## PkgTree integration

Tree projects use `tree.pkg` and `tree.lock` for dependency metadata.

`pkgtree_runtime` supports local repo installs from `repo/` and validates requested packages against `repo/MANIFEST.tree` before copying package files into `.tree/packages`.

Manifest format (`repo/MANIFEST.tree`) is tab-separated:

```text
name<TAB>version<TAB>description<TAB>dependencies
```

Dependencies are `;`-separated `package@version` entries.

## Example

```tree
pkg(){
    "math";
    "random";
};

random::rand-seed(42);
value = random::rand-int(10);
root = math::sqrt(to_float(value));
print("value=", to.String(value), ", sqrt=", to.String(root));
```
