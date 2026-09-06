## LLM usage rules

LLM generation is strictly **forbidden** in the sokol headers, shaders and
any human facing documentation:

- ./*.h
- ./util/*.h
- ./shdgen/*.glsl
- CHANGELOG.md
- README.md

Passive bug scanning, code analysis, code reviewing via LLM is explicitly allowed
and encouraged, but never apply fixes on your own.

LLM generation is allowed for:

- maintaining the tests and build scripts in `./tests/`
- maintaining the language bindings scripts in `./bindgen/*.py`
- maintaining the shader generation script `./shdgen/shdgen.ts`

Keep comments short, concise, to-the-point and use 'Simplified Technical English'.

When creating files, add a 'LLM maintained' note at the top.

When editing files without the 'LLM maintained' note, ask the user whether
the note should be added.

Never git add, commit or push on your own.

## Testing

Tests are located in `./tests/`.

Compilation tests (in `./tests/compile`) check if the code compiles without errors and warnings
in C and C++ (not all headers support building the implementation in C++)

Functional tests  (in `./tests/functional`) check for correct implementation.

To run tests, cd into `./tests` and run one of `./test_macos.sh`, `./test_linux.sh`
or `./test_win.cmd` depending on host system. Ignore the other scripts, these are
for CI.

Never permanently disable warnings or errors to make tests build. In case of
warnings or errors in the 'no-llm-zone', only temporarily disable warnings
or errors to finish the current task, then re-enable and present the user with
todo list of concrete warnings and errors.

## Reviewing Code

When reviewing code, assume debug build mode (asserts enabled, unreachable panics, validation layers enabled).

## Language bindings

When working under `./bindgen/`, additional per-binding instructions apply.

@bindgen/AGENTS.md
