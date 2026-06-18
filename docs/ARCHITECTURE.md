# Hayo-Haya Compiler Architecture

Hayo-Haya is a controlled Hebrew fairy-tale compiler. Its goal is not to solve arbitrary Hebrew natural-language understanding, but to compile a deliberately constrained narrative dialect into executable C++.

The compiler is built around one central design rule: low-level tooling handles structure, while project-owned analysis handles Hebrew and story meaning. `re2c` and Bison are used only for deterministic scanning and raw document structure. Hebrew morphology, sentence interpretation, discourse resolution, fairy-tale semantics, and code generation remain inside the compiler codebase.

## Pipeline Overview

```text
source .hyh file
  -> re2c raw lexer
  -> Bison raw line/block parser
  -> Hebrew orthography and morphology analysis
  -> Hebrew sentence IR
  -> fairy-tale semantic lowering
  -> semantic resolution passes
       -> discourse and salience resolution
       -> mirror answer type resolution
  -> generic story AST
  -> C++ code generation planning
  -> C++ code generation
  -> native executable
```

Each stage has a narrow responsibility. Earlier stages preserve information rather than committing too early. Later stages make semantic decisions only after the relevant linguistic and discourse context is available.

## Layer Responsibilities

The codebase is organized around the data each layer owns. A change should land
in the earliest layer that has enough information to make the decision safely,
and no earlier.

| Layer | Main files | Owns | Must not own |
|---|---|---|---|
| Raw lexer/parser | `lexer.re`, `parser.yy`, `raw/raw_syntax.hpp` | UTF-8 spans, punctuation, statement/block shape, source positions | Hebrew morphology, story roles, output/input semantics |
| Hebrew analysis | `hebrew/*` | orthography, morphology candidates, sentence templates, raw property expressions, grammatical diagnostics | fairy-tale concepts such as magic mirrors, imported callables, story entities |
| Fairy semantic lowering | `fairy/semantic_analyzer.*`, `fairy/story_ast.hpp` | lowering Hebrew sentence IR into story statements, entities, roles, properties, calls, speech, input slots | code-generation details, C++ variable names, lexer/parser token tricks |
| Fairy semantic resolvers | `fairy/discourse_resolver.*`, `fairy/mirror_type_resolver.*` | whole-program semantic decisions that need context after lowering | surface recognition, C++ emission |
| Domain recognizers | `fairy/mirror_questions.*` | fairy-tale-specific surface forms over Hebrew IR, such as magic-mirror questions | generic Hebrew question grammar, generated input code |
| Debug dumps | `hebrew/sentence_analyzer.cpp`, `fairy/semantic_dump.*` | layer-owned textual IR views | semantic decisions for later stages |
| C++ codegen | `codegen/*` | concrete C++ structs, variable names, includes, emitted control flow and input conversion | Hebrew parsing, discourse resolution, answer-type inference |
| Driver/CLI | `driver.*`, `main.cpp` | pipeline orchestration, file/CLI handling, selecting dump views | layer internals beyond invoking layer-owned APIs |

Two practical rules follow from this table:

- If a decision needs the whole story, it belongs in a semantic resolver pass,
  not in the Hebrew sentence analyzer or code generator.
- If a feature names a fairy-tale object or ritual, such as `מראת הקסם`, it
  belongs in the fairy layer, even when it reuses Hebrew grammatical facts.

### Change Placement Guide

Use this checklist when adding or changing behavior:

- New punctuation, statement terminators, or indentation behavior: raw
  lexer/parser.
- New Hebrew word analysis, agreement signal, construct phrase, possessive form,
  or sentence-template recognition: Hebrew analysis.
- New story-level concept such as a role, import, speech act, magic object, or
  fairy-tale ritual: fairy semantic lowering or a fairy domain recognizer.
- New behavior that needs earlier/later statements to be known first: semantic
  resolver pass.
- New emitted C++ shape for an already-resolved AST concept: codegen.
- New debug text for a layer-owned IR: that layer's dump module.

Avoid duplicating a rule across layers. For example, magic-mirror question
surface recognition lives in `fairy/mirror_questions.*`; answer type inference
from later usage lives in `fairy/mirror_type_resolver.*`; generated reads and
casts live in `codegen/*`.

## Design Goals

The architecture is designed to support:

- controlled Hebrew narrative syntax;
- niqqud-aware but niqqud-optional analysis;
- generic fairy-tale roles, entities, properties, loops, conditions, and speech;
- fairy-tale input syntax through magic-mirror questions;
- compiler diagnostics for ambiguity and agreement errors;
- generated C++ that does not depend on Cinderella-specific objects or fields;
- a clear separation between raw text structure, Hebrew analysis, story semantics, and code generation.

The compiler intentionally hardcodes grammar templates and closed-class language markers, but not story-specific nouns, characters, objects, or plot concepts.

For example, words such as `כאשר`, `כל`, `היה`, `הייתה`, `אמר`, `קרא`, and `בזו אחר זו` may be part of the controlled language grammar. Words such as `נסיכה`, `נעל`, `שער`, `עורב`, `זכוכית`, or `כרוז` must be analyzed as story data rather than built into the compiler logic.

## Non-Goals

Hayo-Haya is not intended to be:

- a general-purpose Hebrew parser;
- a statistical or neural NLP system;
- a compiler that understands every valid Hebrew sentence;
- a Cinderella-specific translator;
- a runtime engine with hardcoded story objects.

The language succeeds by being controlled. Source programs should use recognizable fairy-tale sentence patterns that the compiler can analyze deterministically and explain through diagnostics.

## Lexical Analysis with re2c

The lexer is intentionally shallow. It scans the input into raw lexical spans while preserving the original text needed by later Hebrew analysis.

The lexer emits tokens such as:

- Hebrew word spans;
- ASCII words and identifiers;
- numbers;
- strings;
- commas;
- dots;
- question marks;
- colons;
- newlines;
- end-of-file.

The lexer preserves surface spelling, niqqud, maqaf, and UTF-8 text. It does not split Hebrew prefixes, identify roots, infer possessive suffixes, decide gender or number, or assign story meaning.

This keeps the lexer stable and predictable. Hebrew is analyzed later, where ambiguity can be represented explicitly.

## Raw Parsing with Bison

The Bison parser is also intentionally shallow. It groups raw tokens into raw lines and nested block structure.

A dot or question mark marks a completed statement. A colon marks a statement with a child block. Newlines separate unpunctuated lines and support readable source layout.

The parser does not model Hebrew morphology. It does not decide whether a word is a noun, verb, role, property, speaker, or entity. Its job is only to produce deterministic raw syntax that later compiler stages can consume.

This separation avoids forcing Bison to solve problems that belong to Hebrew analysis.

## Hebrew Analysis Layer

The Hebrew layer is the compiler's linguistic front end. It converts raw token sequences into Hebrew sentence IR while preserving ambiguity where needed.

This layer is responsible for:

- orthographic normalization;
- consonantal form extraction;
- niqqud-aware candidate scoring;
- prefix analysis for forms such as `ב`, `ל`, `מ`, `ה`, `ו`, and `כ`;
- lexical noun, name, and out-of-vocabulary candidates;
- root and binyan features where known;
- gender, number, and person features;
- possessive suffix analysis, such as `רגלה`, `כנפו`, and `שמלתה`;
- maqaf compounds and construct-like expressions;
- generic property-expression analysis.

The Hebrew layer should not collapse ambiguity too early. A surface form may produce multiple candidate analyses. Niqqud, syntax, sentence templates, and discourse context can then select the best candidate or produce a diagnostic if the source remains ambiguous.

### Property Expressions

Property expressions are represented generically. The compiler should not special-case one story property such as shoe size, wing height, or gate height.

The Hebrew layer distinguishes between three related forms:

- explicit measured properties, such as `גובה השער` or `מידת הנעל`;
- possessed parts, such as `כנפו`, `רגלה`, or `שמלתה`;
- measured possessed parts, such as `כנפו היה בגובה 12` or `רגלה הייתה במידה 39`.

An explicit measured property has the form:

```hebrew
גובה השער היה 12.
```

and normalizes to:

```text
entity("שער")["גובה"] = 12
```

A measured possessed part expresses the same kind of property assignment, but the owner is carried by the possessive suffix and the measurement is introduced by a prefixed measurement phrase:

```hebrew
כנפו היה בגובה 12.
```

This normalizes to:

```text
resolvedOwner["גובה־כנף"] = 12
```

The compiler should therefore treat these as equivalent measured-property assignments:

```text
גובה השער היה 12
  -> owner=<resolved gate entity>, property=גובה, value=12

כנפו היה בגובה 12
  -> owner=<resolved possessive owner>, property=גובה־כנף, value=12
```

The normalized property name is built from the measurement and the possessed part:

```text
<measurement> + "־" + <part>
```

Examples:

```text
כנפו היה בגובה 12
  -> property=גובה־כנף

רגלה הייתה במידה 39
  -> property=מידה־רגל

שמלתה הייתה באורך 20
  -> property=אורך־שמלה
```

This rule is also used for comparisons. For example:

```hebrew
כאשר כנפו היה כגובה השער:
```

means:

```text
currentActor["גובה־כנף"] == entity("שער")["גובה"]
```

Similarly:

```hebrew
כאשר רגלה הייתה כמידת הנעל:
```

means:

```text
currentActor["מידה־רגל"] == entity("נעל")["מידה"]
```

At this stage, `שער` and `נעל` are still Hebrew noun heads. They are resolved to concrete story entities later by the semantic discourse resolver.

### Possessive Binding and Measurement Inference

Possessive forms are not always bound the same way. Outside an iteration or explicit actor scope, a possessive suffix is resolved through ordinary discourse salience. Inside a role-group iteration, the same possessive form should prefer the current actor when the grammatical features are compatible.

For example:

```hebrew
כרוז היה שליח נאמן.
כנפו היה בגובה 12.
```

The second sentence resolves `כנפו` through discourse, most likely to the recently introduced masculine singular entity `כרוז`:

```text
entity("כרוז")["גובה־כנף"] = 12
```

Inside a loop, however:

```hebrew
בזה אחר זה ניגש כל שליח אל שער־הארמון:
    כאשר כנפו היה כגובה השער:
```

`כנפו` is interpreted as the current loop actor's wing, not necessarily the last mentioned entity's wing:

```text
currentActor["גובה־כנף"] == entity("שער־הארמון")["גובה"]
```

A bare possessed part is not numeric by itself. It becomes comparable only when a measurement is supplied explicitly or can be inferred from the other side of the comparison. If both sides are bare parts and no measurement dimension is available, the compiler should emit a diagnostic instead of guessing.

## Hebrew Sentence IR

The Hebrew sentence IR records what the Hebrew analyzer understood before story-level salience resolution.

It may contain:

- the original sentence surface;
- the sentence kind;
- subject names;
- role heads;
- gender and number features;
- predicate lemma, root, binyan, and agreement features;
- numeric property expressions;
- current-actor property references;
- raw entity-owner heads;
- speech content.

This IR is useful for debugging morphology and sentence-template recognition. It answers the question: "What did the Hebrew layer see?"

For example, in the phrase:

```hebrew
כמידת הנעל
```

Hebrew sentence IR may show:

```text
property=מידה owner=נעל
```

because the Hebrew layer has identified the owner noun head, but not yet resolved it to a specific discourse entity.

## Fairy-Tale Semantic Analysis

The semantic layer consumes Hebrew sentence IR and lowers it into a generic story AST.

It recognizes controlled story templates such as:

```text
<name> היה/הייתה <role> [attributes...]
```

as role introduction,

```text
<property expression> היה/הייתה <number>
```

as numeric property assignment,

```text
בזה/בזו אחר זה/זו <verb> כל <role> ...
```

as role-group iteration,

```text
כאשר <number expression> היה/הייתה <number expression>
```

as a condition,

```text
<speaker> אמר/קרא/לחשה "..."
```

as speech/output.

Magic-mirror questions are recognized by the fairy-tale domain recognizer on top
of the Hebrew sentence analysis. The accepted input forms are deliberately
story-shaped:

```text
מראה מראה שעל הקיר, <question>
<character> שאלה את מראת הקסם "<question>"
```

The question phrase derives the answer slot. `מה מידת הנעל?` binds the answer to
the source-level property phrase `מידת הנעל`; `מי הכי יפה בעיר?` binds the
answer to the nominalized predicate `היפה בעיר`. The question wording provides
only a type hint: `כמה` suggests a number, yes/no questions suggest a boolean,
and `מה` does not inherently constrain the type. A later mirror type-resolution
pass collects constraints from assignments and conditions, so
`כאשר מידת הנעל הייתה 37:` resolves the answer as numeric while
`כאשר היפה בעיר הייתה אלה:` resolves it as text. Conflicting constraints are
reported as diagnostics before C++ generation.

Sentences that do not match a known semantic template can be retained as narrative events. This allows the compiler to preserve source structure even when a sentence is not currently executable.

## Discourse and Salience Resolution

The semantic layer also maintains a discourse model of known story entities and role groups.

A story entity records information such as:

- display name;
- head lemma;
- roles;
- attributes;
- gender;
- number;
- first mention line;
- last mention line;
- mention count.

This allows later definite references to resolve to earlier story entities.

For example:

```hebrew
נעל־הזכוכית הייתה חפץ עתיק.
מידת הנעל הייתה 37.
```

The first sentence introduces an entity whose display name is `נעל־הזכוכית` and whose head lemma is `נעל`. The second sentence refers to `הנעל`, which the Hebrew layer initially treats as the noun head `נעל`. The semantic resolver then resolves that head to the salient entity `נעל־הזכוכית`.

The resolver uses weighted signals such as:

- exact name match;
- head-lemma match;
- role-lemma match;
- definiteness;
- current actor compatibility;
- gender compatibility;
- number compatibility;
- recency;
- mention frequency.

If one candidate clearly wins, the reference is resolved. If multiple candidates are too close, the compiler should report an ambiguity diagnostic instead of silently guessing.

This distinction is important:

```text
Hebrew sentence IR:
  owner=נעל

Resolved semantic IR:
  owner=נעל־הזכוכית
```

The first line shows the linguistic noun head. The second line shows the resolved story entity.

## Generic Story AST

The story AST is independent of any particular fairy tale. It represents generic compiler concepts such as:

- entities;
- role groups;
- role introductions;
- property assignments;
- role-group iteration;
- conditions;
- speech statements;
- input statements;
- narrative events.

The AST should not contain fields such as `shoeSize`, `footSize`, `princesses`, or other Cinderella-specific concepts. Instead, properties and roles are represented as strings derived from Hebrew analysis.

For example:

```text
entity("נעל־הזכוכית").properties["מידה"] = 37
currentActor.properties["מידה־רגל"] == entity("נעל־הזכוכית").properties["מידה"]
```

This keeps the generated program generic and allows the same compiler mechanisms to work for other fairy-tale domains.

## C++ Code Generation

The code generator lowers the generic story AST into C++.

Generated C++ should use resolved, concrete story instances rather than runtime
maps. The semantic AST can stay generic, but by code generation time entity
names, role groups, and property owners should already be resolved.

```cpp
struct hyh_role_1_t {
    int hyh_property_1 = 0; // מידה־נעל
    std::string_view name;
};

std::array<hyh_role_1_t, 2> hyh_role_group_1{{
    {.name = "דריזלה"},
    {.name = "אנסטסיה"},
}};

auto& hyh_entity_1 = hyh_role_group_1[0];
auto& hyh_entity_2 = hyh_role_group_1[1];
```

Entity properties are keyed by Hebrew-derived property names such as:

```text
גובה
גובה־כנף
מידה
מידה־רגל
אורך־שמלה
```

Role groups are also generic in the semantic AST. A loop over `כל נסיכה` should
resolve through the role group for `נסיכה`, not through Cinderella-specific
compiler logic. In the final C++ this is emitted as a normal loop over the
resolved role collection:

```cpp
for (auto& hyh_actor_1 : hyh_role_group_1) {
    if (hyh_actor_1.hyh_property_1 == 40) {
        // ...
    }
}
```

Indefinite construct phrases such as `מידת נעל` name one property kind on the
current role instance. Definite references such as `מידת הנעל` can still resolve
through discourse to a concrete story entity when the source intentionally refers
to a known shoe.

## Diagnostics

Diagnostics are part of the language design. The compiler should explain when a source sentence cannot be interpreted safely.

Examples include:

- malformed raw syntax;
- unknown or unsupported sentence template;
- niqqud contradiction;
- gender agreement mismatch;
- number agreement mismatch;
- unresolved definite reference;
- ambiguous discourse reference;
- unsupported property expression;
- incomparable bare possessed parts without a measurement dimension.

The compiler should prefer explicit diagnostics over silent guesses, especially when two story entities are plausible referents for the same definite noun phrase.

## Debugging Views

The compiler exposes separate debug views for different layers.

`--dump-hebrew-ir` prints the Hebrew sentence IR. This is useful for checking morphology, roots, binyan, agreement, raw property expressions, and sentence-template recognition.

`--dump-semantic-ir` prints the resolved semantic IR. This is useful for checking story-level lowering, salience resolution, role groups, property ownership, conditions, and speech.

`--dump-ir` prints both views. This is useful when debugging the full pipeline because it shows exactly where a meaning changed from raw Hebrew analysis to resolved story semantics.

A typical successful resolution looks like this:

```text
== Hebrew sentence IR ==
right-number: property=מידה owner=נעל raw=כמידת הנעל

== Resolved semantic IR ==
right-number: property=מידה owner=נעל־הזכוכית
```

This output confirms that the Hebrew layer recognized the noun head and the semantic layer resolved it to the correct discourse entity.

## Architectural Principle

The main architectural principle is delayed commitment.

The lexer does not understand Hebrew. The parser does not understand morphology. The Hebrew layer does not hardcode plot objects. The semantic layer does not guess when references are ambiguous. The code generator does not know about Cinderella.

Each layer receives structured information from the previous layer, adds one kind of understanding, and passes a more precise representation forward.
