#!/usr/bin/env -S deno --allow-all
import { parseArgs } from 'jsr:@std/cli@^1/parse-args';

type Shader = { filename: string, prefix: string, prog: string };
type Header = { path: string, shaders: Shader[] };


const headers: Header[] = [
    {
        path: '../util/sokol_gl.h',
        shaders: [ { filename: 'sokol_gl.glsl', prefix: '_sgl', prog: 'sgl' } ],
    }
];


const denoArgs = parseArgs(Deno.args, {
    boolean: ['hlsl'],
    string: ['shdcroot'],
    default: { hlsl: false, shdcroot: '../../sokol-tools-bin' }
});

function dirExists(path: string): boolean {
    try {
        return Deno.statSync(path).isDirectory;
    } catch {
        return false;
    }
}

async function main(): Promise<void> {
    if (dirExists('out')) {
        Deno.removeSync('out', { recursive: true });
    }
    Deno.mkdirSync('out/tmpdir', { recursive: true });

    if (denoArgs.hlsl) {
        // only compile hlsl shaders into binaries (runs on CI)
        for (const hdr of headers) {
            for (const shd of hdr.shaders) {
                await compile(shd, true);
            }
        }
    } else {
        // compile non-HLSL shader locally, and HLSL by triggering a remote Github CI run
        await compileHlslRemote();
        for (const hdr of headers) {
            const outp: string[] = []
            for (const shd of hdr.shaders) {
                await compile(shd, false);
                outp.push(...gatherShader(shd));
            }
            inject(hdr.path, outp);
        }
    }
}

async function compile(shd: Shader, hlsl: boolean): Promise<void> {
    await shdc([
        '-i', shd.filename,
        '-o', `out/${shd.filename}`,
        '-t', 'out/tmpdir',
        '-l', hlsl ? 'hlsl4' : 'glsl410:glsl300es:metal_macos:metal_ios:metal_sim:wgsl:spirv_vk',
        '-f', 'bare_yaml',
        '-b',
    ]);
}

function bytesToCArray(name: string, bytes: Uint8Array): string[] {
    const lines: string[] = [];
    lines.push(`static const uint8_t ${name}[${bytes.length}] = {`);
    for (let i = 0; i < bytes.length; i += 16) {
        const chunk = bytes.slice(i, Math.min(i + 16, bytes.length));
        const hex = Array.from(chunk).map(b => '0x' + b.toString(16).padStart(2, '0')).join(',');
        lines.push(`    ${hex},`);
    }
    lines.push('};');
    return lines;
}

function gatherSlang(shd: Shader, slang: string, ext: string, isBinary: boolean): string[] {
    const res: string[] = [];
    for (const stage of ['vertex', 'fragment']) {
        const path = `out/${shd.filename}_${shd.prog}_${slang}_${stage}${ext}`;
        let bytes: Uint8Array = Deno.readFileSync(path);
        if (!isBinary) {
            // for source code, append a terminating zero
            bytes = new Uint8Array([...bytes, 0]);
        }
        const cArrayName = `${shd.prefix}_${shd.prog}_${stage==='vertex'?'vs':'fs'}_${isBinary?'bytecode':'source'}_${slang}`;
        const cArray = bytesToCArray(cArrayName, bytes);
        res.push(...cArray);
    }
    return res;
}

function gatherShader(shd: Shader): string[] {
    const res: string[] = [];
    res.push('#if defined(SOKOL_GLCORE)');
    res.push(...gatherSlang(shd, 'glsl410', '.glsl', false));
    res.push('#elif defined(SOKOL_GLES3)');
    res.push(...gatherSlang(shd, 'glsl300es', '.glsl', false));
    res.push('#elif defined(SOKOL_METAL)');
    res.push(...gatherSlang(shd, 'metal_macos', '.metallib', true));
    res.push(...gatherSlang(shd, 'metal_ios', '.metallib', true));
    res.push(...gatherSlang(shd, 'metal_sim', '.metal', false));
    res.push('#elif defined(SOKOL_D3D11)');
    res.push(...gatherSlang(shd, 'hlsl4', '.fxc', true));
    res.push('#elif defined(SOKOL_WGPU');
    res.push(...gatherSlang(shd, 'wgsl', '.wgsl', false));
    res.push('#elif defined(SOKOL_VULKAN');
    res.push(...gatherSlang(shd, 'spirv_vk', '', true));
    res.push('elif defined(SOKOL_DUMMY_BACKEND');
    res.push(`static const char* ${shd.prefix}_vs_source_dummy = "";`);
    res.push(`static const char* ${shd.prefix}_fs_source_dummy = "";`);
    res.push('#else');
    res.push('#error "Please define one of SOKOL_GLCORE, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL, SOKOL_WGPU, SOKOL_VULKAN or SOKOL_DUMMY_BACKEND!"');
    res.push('#endif');
    return res;
}


function inject(path: string, content: string[]): void {
    const startMarker = '//>#shdgen';
    const endMarker = '//<#shdgen';
    let insideMarker = false;
    const inp = Deno.readTextFileSync(path).split(/\r?\n/);
    const outp = [];
    for (const line of inp) {
        if (insideMarker) {
            if (line.startsWith(endMarker)) {
                outp.push(...content);
                outp.push(line);
                insideMarker = false;
            }
        } else {
            outp.push(line);
            if (line.startsWith(startMarker)) {
                insideMarker = true;
            }
        }
    }
    const eol = inp[0].includes('\r\n') ? '\r\n' : '\n';
    Deno.writeTextFileSync(path, outp.join(eol));
}

async function shdc(args: string[]): Promise<void> {
    console.log(`> sokol-shdc ${args.join(' ')}`);
    const os = Deno.build.os;
    const arch = Deno.build.arch;
    let dir;
    switch (os) {
        case 'darwin': dir = arch === 'aarch64' ? 'osx_arm64' : 'osx'; break;
        case 'windows': dir = 'win32'; break;
        default: dir = arch === 'aarch64' ? 'linux_arm64' : 'linux'; break;
    }
    const shdcPath = `${denoArgs.shdcroot}/bin/${dir}/sokol-shdc`;
    const cmd = new Deno.Command(shdcPath, { args, stdout: 'inherit', stderr: 'inherit' });
    const { code } = await cmd.output();
    if (code !== 0) {
        throw new Error(`sokol-shdc failed with exit code ${code}`);
    }
}

async function gh(args: string[]): Promise<string> {
    console.log(`> gh ${args.join(' ')}`);
    const cmd = new Deno.Command('gh', { args, stdout: 'piped', stderr: 'inherit' });
    const { code, stdout } = await cmd.output();
    if (code !== 0) {
        throw new Error(`gh failed with exit code ${code}`);
    }
    return new TextDecoder().decode(stdout).trim();
}

function sleep(ms: number): Promise<void> {
    return new Promise(r => setTimeout(r, ms));
}

// special helper function to compile HLSL files to binary via
// Github Actions on a Windows VM
async function compileHlslRemote(): Promise<void> {
    const repo = 'floooh/sokol';
    const workflow = 'compile_hlsl.yml';
    const ref = 'shdgen';
    const startTime = new Date().toISOString();

    // trigger workflow
    const trigger = new Deno.Command('gh', {
        args: ['workflow', 'run', workflow, '-R', repo, '--ref', ref],
        stdout: 'inherit', stderr: 'inherit',
    });
    const { code: triggerCode } = await trigger.output();
    if (triggerCode !== 0) {
        throw new Error('Failed to trigger GH workflow');
    }

    // poll for run id
    console.log('getting gh workflow run-id...');
    let runId = '';
    for (let i = 0; i < 30; i++) {
        await sleep(2000);
        runId = await gh(['run', 'list', '-R', repo, '-w', workflow, '--limit', '1',
            '--json', 'databaseId,createdAt', '--jq',
            `.[0] | select(.createdAt >= "${startTime}") | .databaseId`
        ]);
        if (runId) break;
    }
    if (!runId) {
        throw new Error('Timed out waiting for workflow run');
    }

    // wait for workflow completion
    console.log('waiting for gh workflow to finish...');
    await gh(['run', 'watch', runId, '-R', repo]);

    // download artifact
    console.log('download compiled hlsl shader binaries');
    await gh(['run', 'download', runId, '-n', 'hlsl-shader', '-D', 'out']);
}

await main();
