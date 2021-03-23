## Language Binding Generation Scripts

### Zig

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
