## Language Binding Generation Scripts

> REMINDER: we can pass `-fparse-all-comments` to the clang ast-dump command line which adds the following node types to the ast-dump.json: FullComment, ParagraphComment, TextComment. This might allow us to preserve comments in the language bindings (might be useful as part of a bigger change to make sokol header comments autodoc and Intellisense-friendly)

### Zig

First make sure that clang and python3 are in the path:

```
> clang --version
> python3 --version
```

...on Windows I simply install those with scoop:

```
> scoop install llvm
> scoop install python
```

To update the Zig bindings:

```
> cd sokol/bindgen
> git clone https://github.com/floooh/sokol-zig
> git clone https://github.com/floooh/sokol-nim
> git clone https://github.com/floooh/sokol-odin
> git clone https://github.com/floooh/sokol-rust
> python3 gen_all.py
```

Test and run samples:

```
> cd sokol/bindgen/sokol-zig
> zig build run-clear
> zig build run-triangle
> zig build run-cube
...
```
