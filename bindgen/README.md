## Language Binding Generation Scripts

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
