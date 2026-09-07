# Adding a new sokol header to the language bindings

Applies whenever a new `sokol_*.h` / `util/sokol_*.h` header is added to the
main repo and needs to reach the language bindings under
`./bindgen/sokol-<lang>/`. Regeneration alone is not enough for most bindings;
each language's C build has to learn about the new file too.

## 0. Precondition -- verify each bindings repo is ready

Before running the generator or touching any build script, check every
`bindgen/sokol-<lang>/` repo:

1. Working tree is clean -- `git -C bindgen/sokol-<lang> status --short`
   returns no output.
2. The current branch is a feature branch, **never `master` or `main`**.
   Check with `git -C bindgen/sokol-<lang> branch --show-current`. Binding
   regenerations always land on a feature branch (typically the same
   branch name as the sokol PR that added the header); a repo sitting on
   `master`/`main` means no feature branch has been created yet.

If any repo is dirty, sitting on `master`/`main`, or on a branch that
doesn't match what the user asked for, **stop and notify the user** --
list which repo(s) failed the check and what state they're in. Do not
attempt to clean, switch, or create branches on your own; those are
destructive actions the user must direct.

Only proceed to step 1 once every repo passes both checks.

Also verify each repo's `master`/`main` branch is up to date with its
remote before the user creates the feature branch from it -- an outdated
base would make the eventual PR contain unrelated commits or fall behind
upstream. From within the repo, run:

```
git fetch origin
git rev-list --count master..origin/master   # 0 == up to date; non-zero == behind
```

(replace `master` with `main` where that's the default branch). If the
count is non-zero, stop and tell the user which repo(s) are behind so
they can fast-forward before branching.

## 1. Register the header with the generator

Add an entry to the `tasks` list in `gen_all.py`:

```python
[ '../util/sokol_newmodule.h', 'snm_', ['sg_'] ],   # c_path, prefix, deps
```

Add the prefix → module name mapping in the `module_names` dict in the same
file (a short lowercase name, matching sokol convention).

Then update each per-language generator's own `module_names` dict when it has
one (see below). Missing entries cause `gen_all.py` to skip that language for
the new header and emit `>> warning: skipping generation for {c_prefix} …`.

Per-generator module maps to update:
- `gen_c3.py`
- `gen_d.py`
- `gen_jai.py`
- `gen_nim.py`
- `gen_odin.py`
- `gen_rust.py`
- `gen_zig.py`

Run `python3 gen_all.py` after the changes and confirm the target `.<ext>` file
appears in each binding repo. Also confirm the corresponding
`sokol_<newmodule>.c` and `sokol_<newmodule>.h` land in each binding's
`c/`-style directory (paths differ per repo -- see below).

## 2. Wire the new `.c` file into each binding's C build

Each repo has its own C build. The header alone is not enough -- the `.c`
shim must be picked up by the build.

### sokol-zig -- `build.zig`

Add `"sokol_newmodule.c"` to the C source list (search for existing entries
like `sokol_gfx.c`).

### sokol-nim -- generated `.nim` module

Nothing to do. The generator emits `{.compile: "c/sokol_newmodule.c".}` into
the produced `newmodule.nim`, and Nim compiles the C source automatically
when the module is imported.

### sokol-rust -- `build.rs`

Add `"sokol_newmodule.c"` to the `files` array (around the block starting
`let files = [`).

After running `gen_all.py`, `cd bindgen/sokol-rust && cargo fmt` to
re-apply rustfmt to the generated `.rs` files. The generator emits code
that doesn't match rustfmt output, so without this the diff shows large
formatting-only changes on every regeneration.

### sokol-d -- `build.d`

Add `"sokol_newmodule.c"` to the `sokolSources` array.

### sokol-jai -- 5 build scripts + unified `sokol.c`

Static-lib builds (`sokol/build_clibs_*.sh` / `.cmd`) use one entry per
config × backend. Insert `sokol_newmodule` lines after the last existing
module in each block, mirroring the existing pattern. Files to touch:

- `sokol/build_clibs_macos.sh`  -- 8 lines (arm64/x64 × release/debug × metal/gl)
- `sokol/build_clibs_linux.sh`  -- 2 lines (x64 × release/debug × gl)
- `sokol/build_clibs_wasm.sh`   -- 2 lines (release/debug × gles3)
- `sokol/build_clibs_windows.cmd` -- add `newmodule` to the `set sources=…` list
- `sokol/build_clibs_macos_dylib.sh` -- **no per-module change**; it builds a
  single unified dylib from `sokol/c/sokol.c` (see next bullet)

Unified compilation unit `sokol/c/sokol.c` (used by the macOS dylib and the
Windows DLL builds): add `#include "sokol_newmodule.h"`.

### sokol-odin -- 5 build scripts + unified `sokol.c`

Same shape as sokol-jai. Files to touch:

- `sokol/build_clibs_macos.sh`  -- 8 lines
- `sokol/build_clibs_linux.sh`  -- 2 lines
- `sokol/build_clibs_wasm.sh`   -- add `"newmodule"` to the bash `libs=(…)` array
- `sokol/build_clibs_wasm.cmd`  -- add `newmodule` to `set sources=…` (kept in sync with `wasm.sh` even though it may be stale for prior modules)
- `sokol/build_clibs_windows.cmd` -- add `newmodule` to `set sources=…`
- `sokol/build_clibs_macos_dylib.sh` -- **no per-module change**
- `sokol/c/sokol.c` -- add `#include "sokol_newmodule.h"`

### sokol-c3 -- unified `sokol.c`

`sokol.c3l/c/sokol.c` is the single compilation unit driven by
`sokol.c3l/manifest.json`. Add `#include "sokol_newmodule.h"` there. The
`.c3` binding itself is emitted by `gen_c3.py` once the generator's
`module_names` dict knows the prefix (step 1).

## 3. Verify

For each repo, ensure `git status` shows the expected new/modified files:

- generated language binding (`.zig`, `.nim`, `.rs`, `.d`, `.jai`, `.odin`, `.c3`)
- `c/sokol_newmodule.h` and `c/sokol_newmodule.c` (path varies per repo)
- build-script edits from step 2

Then rebuild each binding's example (or the smallest one, usually `clear`)
to prove the wiring works end to end. Before running examples, rebuild the
pre-generated C static libs where the repo relies on them -- otherwise the
example still links the stale `.a` from a previous run. Commands from
each repo's `README.md`:

| repo | rebuild C libs first | build/run example |
|---|---|---|
| sokol-zig | not needed (build.zig compiles C from source) | `cd sokol-zig && zig build examples` |
| sokol-rust | not needed (build.rs compiles C from source) | `cd sokol-rust && cargo build --all-targets` |
| sokol-nim | not needed (`.compile` pragma in each `.nim` module) | `cd sokol-nim && nimble clear` |
| sokol-c3 | not needed (`sokol.c3l` C sources built by c3c) | `cd sokol-c3 && bash build-examples.sh` |
| sokol-d | `cd sokol-d && rm -f build/*.a && dub build -c sokol-static --compiler=ldc2 --force` | `cd sokol-d/examples/clear && dub build --compiler=ldc2` |
| sokol-jai | `cd sokol-jai/sokol && ./build_clibs_macos.sh` (or `_linux.sh` / `_windows.cmd`) | `cd sokol-jai && jai-macos examples/first.jai - clear` (binary is `jai-macos` on macOS; `jai-linux` / `jai.exe` elsewhere) |
| sokol-odin | `cd sokol-odin/sokol && ./build_clibs_macos.sh` (or the equivalent) | `cd sokol-odin && odin run examples/clear -strict-style -debug` |

If a rebuild fails because the toolchain isn't installed (e.g. no `jai`
compiler), skip that repo and note it in the summary rather than trying
to install anything.

Then leave every repo's changes uncommitted for the user to review.
**Do not `git add`, `git commit`, or `git push` in any bindings repo** --
verification and commit are the user's job. See the top-level
`bindgen/README.md` for the branch-naming convention (typically the same
branch name as the sokol PR that added the header) when the user asks you
to create a feature branch.

## Special case -- headers with external C/C++ dependencies

Some headers wrap third-party C/C++ libraries and cannot be auto-compiled
by every binding's C build. Current examples:

- `sokol_imgui.h`, `sokol_gfx_imgui.h`, `sokol_app_imgui.h` -- require
  Dear ImGui (C++). Users are expected to vendor
  [dcimgui](https://github.com/floooh/dcimgui) into their project.
- `sokol_nuklear.h` -- requires nuklear.h. Currently D-only because each
  generator needs opaque-type declarations for the foreign `nk_*` types
  (see `gen_d.py`'s `gen_nuklear_types()`).

For headers like these, the generator still emits the language binding
module **and** the C stub (`c/sokol_<name>.c`), but the per-repo C build
must NOT reference them -- consumers compile the stub against their own
vendored dependency. Concretely:

- sokol-zig / sokol-d: their build scripts (`build.zig`, `build.d`)
  already have opt-in flags / configurations for imgui + nuklear; users
  bring dcimgui / nuklear.h through those.
- sokol-nim: `gen_nim.py` skips the `{.compile: ...}` pragma for these
  prefixes. Users add their own compile pragma against a vendored
  dcimgui.
- sokol-rust: `gen_rust.py` gates these modules behind a cargo feature
  (`imgui`), and `build.rs` intentionally omits their `.c` stubs.
  Users enable the feature and compile the stub in their own `build.rs`.
- sokol-jai / sokol-odin / sokol-c3: their `build_clibs_*.sh` (or
  `sokol.c` in c3) do not compile these stubs. Users build them manually
  and drop the archives where the module's `#library` / `foreign import`
  block expects them.

If you add another header of this shape, put its task in a helper list
(`imgui_tasks`) so it's shared across bindings, but do NOT extend any
build script or `sokol.c` unified include. Also add a "Dear ImGui
integration"-style subsection to each binding's `README.md` documenting
how to wire up the external dependency.

### nuklear.h staging

`sokol_nuklear.h`'s public API uses foreign `nk_*` types, so the
generator's `clang` parse needs `nuklear.h` alongside `sokol_nuklear.h`
in the C dir. `gen_d.prepare()` copies `../tests/ext/nuklear.h` into
`sokol-d/src/sokol/c/nuklear.h` before generation and `gen_d.cleanup()`
removes it after -- so `nuklear.h` is never shipped in the D repo.
A generator that adds nuklear support should follow the same
prepare/cleanup pattern.

