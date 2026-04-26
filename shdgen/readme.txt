Helper script to regenerate embedded shaders.

Requires `deno` in the path and a logged-in `gh` (needed for
generating HLSL binaries remotely via a Github Actions Windows VM).

On macOS, clone https://github.com/floooh/sokol-tools-bin next
to the sokol directory, then run

```
deno run --allow-all shdgen.ts
```

...then review and test the header before git commit and push.
