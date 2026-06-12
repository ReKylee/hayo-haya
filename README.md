# היה־הייתה Compiler Starter

Starter project for a Hebrew fairy-tale programming language that compiles to C-style C++.

The current compiler implements a **tiny v0.1 subset**:

- opening with `ארץ` and `ממלכה`
- importing `cout` from `הממלכה העתיקה`
- boolean declarations like `השער היה סגור.`
- text declarations like `על השער נכתב "...".`
- count declarations like `בקופה נחו 3 מטבעות.`
- count changes like `נוסף לקופה מטבע אחד.`
- count loops like `שוב ושוב, כל עוד בקופה נחו פחות מ־20 מטבעות:`
- count conditions like `כאשר בקופה נחו פחות מ־4 מטבעות:`
- speech/output like `הכרוז קרא "...".` and `הכרוז קרא את הכתוב שעל השער.`
- selected narrative sentences as no-op story events
- ending with `וכך תם סיפורה של ממלכת ...`

The full design grammar is in [`docs/EBNF-v0.1.md`](docs/EBNF-v0.1.md).

## Build with Dev Container

1. Install Docker Desktop, Podman, or another Docker-compatible runtime.
2. Open this folder in VS Code.
3. Choose **Reopen in Container**.
4. Build:

```bash
cmake --build --preset dev
```

5. Compile the example:

```bash
./build/dev/hyh examples/pea.hyh -o out/pea.cpp
./out/pea
```

## Build with plain Docker

```bash
docker build -t haya-hayta .
docker run --rm -it -v "$PWD":/work haya-hayta bash
```

Inside the container:

```bash
cmake --preset dev
cmake --build --preset dev
./build/dev/hyh examples/pea.hyh -o out/pea.cpp
./out/pea
```

## Local build

You need:

- C++23 compiler
- CMake
- Ninja
- GNU Bison
- re2c

Then:

```bash
cmake --preset dev
cmake --build --preset dev
```

To compile `examples/pea.hyh` through the full pipeline and produce `out/pea`:

```bash
./build/dev/hyh examples/pea.hyh -o out/pea.cpp
```

Then run it:

```bash
./out/pea
```

`hyh` compiles the generated C++ by default. Use `--no-compile` to stop after
writing the generated `.cpp` file, or `--exe <path>` to choose the executable
path.

## Notes

For now the lexer returns Hebrew words as `NAME` tokens and the parser/driver normalize prefixes where grammar expects them. For example:

- `השער` becomes the definite name `שער`
- `בקופה` becomes the in-container name `קופה`

This avoids splitting every word that starts with `ב`, `ל`, `מ`, or `ה`, which would incorrectly break normal words like `מטבעות`.
