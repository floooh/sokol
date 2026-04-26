#!/usr/bin/env -S deno --allow-all
import { parseArgs } from 'jsr:@std/cli@^1/parse-args';

type Shader = { header: string, shader: string, prefix: string };

const shaders: Shader[] = [
    { header: '../util/sokol_gl.h', shader: 'sokol_gl.glsl', prefix: '_sgl' }
];

const args = parseArgs(Deno.args, {
    boolean: ['hlsl'],
    string: ['shdcroot'],
    default: { hlsl: false, shdcroot: '../../sokol-tools-bin' }
});

const shdcRootDir = args.shdcroot;

// only compile hlsl shaders into binaries (runs on CI)
if (args.hlsl) {
    for (const shd of shaders) {
        compile(shd, true);
    }
}

function compile(shd: Shader, hlsl: boolean): void {
    Deno.mkdirSync('out', { recursive: true });
    shdc([
        '-i', shd.shader,
        '-o', `out/${shd.shader}`,
        '-l', hlsl ? 'hlsl4' : 'glsl410:glsl300es:metal_macos:metal_ios:metal_sim:wgsl:spirv_vk',
        '-f', 'bare_yaml',
        '-b',
    ]);
}

function inject(path: string, prefix: string, content: string[]): void {
    const startMarker = `//>#shdgen ${prefix}`
    const endMarker = '//>#shdgen';
    let insideMarker = false;
    const inp = Deno.readTextFileSync(path).split(/\r?\n/);
    const outp = [];
    for (const line of inp) {
        if (insideMarker) {
            if (line.startsWith(startMarker)) {
                outp.push(...content);
                outp.push(line);
                insideMarker = false;
            }
        } else {
            outp.push(line);
            if (line.startsWith(endMarker)) {
                insideMarker = true;
            }
        }
    }
    const eol = inp[0].includes('\r\n') ? '\r\n' : '\n';
    Deno.writeTextFileSync(path, outp.join(eol));
}

function genShader(shd: Shader) {

    //
}

function getShdcPath(): string {
    const os = Deno.build.os;
    const arch = Deno.build.arch;
    let dir;
    switch (os) {
        case 'darwin': dir = arch === 'aarch64' ? 'osx_arm64' : 'osx'; break;
        case 'windows': dir = 'win32'; break;
        default: dir = arch === 'aarch64' ? 'linux_arm64' : 'linux'; break;
    }
    return `${shdcRootDir}/bin/${dir}/sokol-shdc`;
}

function shdc(args: string[]): void {
    const cmd = new Deno.Command(getShdcPath(), { args, stdout: 'inherit', stderr: 'inherit' });
    const { code } = cmd.outputSync();
    if (code !== 0) {
        throw new Error(`sokol-shdc failed with exit code ${code}`);
    }
}
