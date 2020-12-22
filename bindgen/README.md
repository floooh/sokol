TODO: proper readme!

First create AST dumps in JSON format for each sokol header (when on Windows, be sure
to not run this in Powershell, or you'll get the output as UTF-16, which
in turn confuses all the other tools)

```
> clang -Xclang -ast-dump=json ../sokol_gfx.h >sokol_gfx.ast.json
> clang -Xclang -ast-dump=json ../sokol_app.h >sokol_app.ast.json
...
```

Next, convert the raw AST dumps into a simplified JSON API description:
```
> python3 gen_json.py
```

Finally generate the language bindings:
```
> python3 gen_zig.py
...
```
