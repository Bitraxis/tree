# Tree + PkgTree

Tree is a small C-based language runtime with a package manager runtime (`pkgtree`) that installs Tree package files into `.tree/packages`.

## Build

### WSL (recommended on Windows)

```bash
make wsl
```

### Linux/native

```bash
make linux
```

### Direct gcc build

With pkgtree runtime:

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c pkgtree_runtime.c -lm -o tree
```

Without pkgtree runtime:

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -lm -o tree
```

## Quick usage

Run a script:

```bash
./tree test_packages.tree
```

Run package manager script:

```bash
./tree pkgtree.tree
```

## Package manager model

`pkgtree_runtime.c` supports two repo modes:

- Local repo directory (default): `repo`
- Remote repo URL (fallback): `https://...`

### `tree.pkg` format (v2)

`tree.pkg` stores project dependencies and repo settings.

```text
# tree.pkg v2
repo=repo
format=2
core,1.0
json,1.0
```

Rules:

- Lines starting with `#` are comments.
- `repo=` selects local directory path or remote URL.
- Dependency entries are `name,version` (or `name@version`).

### Local repository layout

```text
repo/
  MANIFEST.tree
  pkg/
    core@1.0.tree
    text@1.0.tree
    json@1.0.tree
    toml@1.0.tree
    fs@1.0.tree
    cli@1.0.tree
    data@1.0.tree
    project@1.0.tree
    net@1.0.tree
    mathx@1.0.tree
    build@1.0.tree
```

### `repo/MANIFEST.tree` format (v2)

Tab-separated columns:

```text
name<TAB>version<TAB>description<TAB>dependencies
```

- `description`: human-readable package summary.
- `dependencies`: `;`-separated list of `package@version` values.

Example:

```text
data	1.0	Data glue package that combines JSON and TOML	json@1.0;toml@1.0
```

## Included showcase packages

- `core`: math/random helpers.
- `text`: string transform helpers.
- `json`: JSON wrapper helpers.
- `toml`: TOML wrapper helpers.
- `fs`: workspace path helpers.
- `cli`: `argc/argv` helpers.
- `data`: combines JSON + TOML handling.
- `project`: project metadata bootstrap.
- `net`: endpoint/url dictionary helper.
- `mathx`: extra math constants/helpers.
- `build`: build profile metadata helpers.

## Notes

- `pkgtree_install()` validates local packages against `repo/MANIFEST.tree`.
- Installed packages are copied to `.tree/packages/<name>/<version>/<name>@<version>.tree`.
- `tree.lock` is rewritten after install with an installed package count.
