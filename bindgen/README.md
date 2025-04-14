## Language Binding Generation Scripts

### Updating the bindings

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
> git clone https://github.com/kassane/sokol-d
> git clone https://github.com/colinbellino/sokol-jai
> git clone https://github.com/floooh/sokol-c3
> python3 gen_all.py
```

...and then to test and run Zig samples:

```
> cd sokol/bindgen/sokol-zig
> zig build run-clear
> zig build run-triangle
> zig build run-cube
...
```
