# Garal

A mathematical programming language supporting scalars, sets, and matrices.

## Building

Requires `gcc`, `gfortran`, and `make`.

```
make
```

## Usage

REPL:
```
./garal
> 2 + 2
4
> {1..5} ∩ {3..7}
{3, 4, 5}
```

Script mode:
```
./garal script.grl
```

## Language

### Types

- Scalar: integer (`42`), rational (`1/3` via division), real (`3.14`)
- Set: `{1..10}`, `{1, 2, 3}`
- Matrix: `[[1, 2], [3, 4]]`

### Operators

| Operator | Scalars | Sets | Matrices |
|---|---|---|---|
| `+` | addition | union | elementwise |
| `-` | subtraction | difference | elementwise |
| `*` | multiply | scalar * set | matrix multiply |
| `/` | divide | - | - |
| `^` | power | - | matrix power |
| `×` | - | Cartesian product | - |
| `∩` | - | intersection | - |

### Built-in functions

`abs`, `floor`, `ceil`, `sqrt`, `gcd`, `lcm`, `len`, `det`, `transpose`, `rank`

### User-defined functions

```
fn hypotenuse(a, b) =>
    c2 = a^2 + b^2
    sqrt(c2)
```

### Let-in blocks

```
let
    A = 2 + 2
    B = {1..A}
in
    B × B
```

### Comments

```
// this is a comment
```

## Running tests

```
make test
```
