# Implementation Notes

## Current toolchain

- `re2c` generates the lexer from `src/lexer.re`.
- GNU Bison generates the LR-family parser from `src/parser.yy`.
- The compiler currently generates one C++ source file.

## Why prefix handling is not purely lexical yet

Hebrew prefixes are attached to words:

- `השער`
- `בקופה`
- `לקופה`
- `מקופה`

A naive lexer rule that splits every word starting with `מ` would break ordinary words like `מטבעות` and `מזרנים`.

So the current starter takes a safer approach:

1. The lexer recognizes fixed keywords first.
2. Other Hebrew words become `NAME` tokens.
3. Grammar helper nonterminals call driver normalization methods when a prefix is expected:
   - `definite_name` strips optional `ה`
   - `in_container_name` requires and strips `ב`

This follows the v0.1 EBNF idea while avoiding premature Hebrew morphology work.

## Current partial story support

- Imports use the external/original symbol first, then the Hebrew story name:
  `מן הממלכה העתיקה הגיע cout ושמו כרוז.`
- Count changes, count loops, and count conditions are implemented for the
  current sample story.
- Pure narrative sentences are accepted as no-op story events where the compiler
  does not yet model their meaning.
- Utterance conditions currently compile as unconditional blocks until the
  language has event/input tracking.

## Next milestones

1. Add semantic checks:
   - imported alias must exist before speech
   - text reference must refer to an object with text
   - count changes must refer to known counted containers
2. Replace no-op narrative parsing with real story events where useful.
3. Give utterance conditions a real runtime model.
4. Improve generated C++ name mangling.
