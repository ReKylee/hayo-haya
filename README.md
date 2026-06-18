# hayo-haya / hyh

`hyh` is a prototype compiler for controlled Hebrew fairy-tale programs.

This version keeps the researched architecture: re2c scans raw UTF-8 tokens, Bison groups the stream into raw sentences/blocks, the custom Hebrew layer builds morphology lattices and Hebrew sentence IR, and the fairy-tale semantic layer lowers that into a generic story AST.

## Build

```bash
cmake --preset dev
cmake --build --preset dev
```

The compiler executable is written to:

```text
bin/hyh.exe   # Windows
bin/hyh       # Linux/devcontainer
```

## Run

```bash
./bin/hyh examples/cinderella.hyh
./bin/cinderella
```

On Windows PowerShell:

```powershell
.\bin\hyh.exe .\examples\cinderella.hyh
.\bin\cinderella.exe
```

By default `hyh` compiles the generated C++ using:

```text
--cxx <compiler> > HYH_CXX > CXX > clang++
```

Generate C++ only:

```bash
./bin/hyh examples/cinderella.hyh --no-compile
```

Debug the Hebrew layer:

```bash
./bin/hyh examples/cinderella.hyh --no-compile --dump-lattice
./bin/hyh examples/cinderella.hyh --no-compile --dump-ir
./bin/hyh examples/raven.hyh --no-compile --dump-ir
```

## Examples

- `examples/cinderella.hyh`: validates that Cinderella is data, not compiler logic.
- `examples/raven.hyh`: validates that conditions are generic property comparisons, not `מידת`-specific.
- `examples/generic-condition.hyh`: minimal non-Cinderella test of `כנפו == גובה השער`.
- `examples/shoe-size-property.hyh`: validates that `מידת נעל` is one role property, not a shoe entity lookup.
- `examples/import-output.hyh`: validates imported output aliases lowering to generated calls.
- `examples/magic-mirror-input.hyh`: validates magic-mirror questions and typed answer-slot inference.
- `examples/bad-agreement.hyh`: demonstrates the `HN004` agreement diagnostic.
- `examples/bad-import-alias-call.hyh`: demonstrates the `HN013` imported-alias call diagnostic.
- `examples/bad-quantity.hyh`: demonstrates the `HN005` Hebrew quantity phrase diagnostic.

## Important design point

The compiler does not contain story-specific fields like `princesses`, `shoeSize`, or `footSize`, and the condition parser is not tied to `מידת`.

The Hebrew layer recognizes word/phrase structure and the semantic layer emits generic facts such as:

```text
Entity("דריזלה") has role "נסיכה"
Entity("שער").numbers["גובה"] = 12
currentActor.numbers["כנף"] == Entity("שער").numbers["גובה"]
roles["שליח"] is iterated
```

The compiler may use generic maps internally while resolving Hebrew discourse,
but generated C++ uses concrete role structs and role collections. Each
princess, messenger, or object is an instance of the relevant generated role
type, and loops over `כל <role>` iterate the collection of those instances.
