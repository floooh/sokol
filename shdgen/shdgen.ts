import { parseArgs } from 'jsr:@std/cli@^1/parse-args';

const denoArgs = parseArgs(Deno.args, {
    boolean: ['hlsl', 'local'],
    string: ['shdcroot', 'branch', 'only'],
    default: {
        hlsl: false,
        local: false,
        shdcroot: '../../sokol-tools-bin',
        branch: 'master',
        only: null,
    },
});

type Item = { header: string, shader: string, prefix: string, progs: string[] };

const items: Item[] = [
    { header: '../util/sokol_gl.h', shader: 'sokol_gl.glsl', prefix: '_sgl', progs: ['shd'] },
    { header: '../util/sokol_debugtext.h', shader: 'sokol_debugtext.glsl', prefix: '_sdtx', progs: ['shd'] },
    { header: '../util/sokol_fontstash.h', shader: 'sokol_fontstash.glsl', prefix: '_sfons', progs: ['shd'] },
    { header: '../util/sokol_imgui.h', shader: 'sokol_imgui.glsl', prefix: '_simgui', progs: ['shd'] },
    { header: '../util/sokol_nuklear.h', shader: 'sokol_nuklear.glsl', prefix: '_snk', progs: ['shd'] },
    { header: '../util/sokol_spine.h', shader: 'sokol_spine.glsl', prefix: '_sspine', progs: ['shd'] },
    {
        header: '../util/sokol_framebuffer.h',
        shader: 'sokol_framebuffer.glsl',
        prefix:  '_sfb',
        progs: ['rgba8', 'palette8', 'render'],
    }
];

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
        for (const item of items) {
            await compile(item);
        }
    } else {
        // compile non-HLSL shader locally, and HLSL by triggering a remote Github CI run
        if (!denoArgs.local) {
            await compileHlslRemote();
        }
        for (const item of items) {
            await compile(item);
            const outp: string[] = []
            for (const prog of item.progs) {
                outp.push(...gatherShader(item, prog));
            }
            if (denoArgs.only && !item.header.includes(denoArgs.only)) {
                continue;
            }
            inject(item.header, outp);
        }
    }
}

async function compile(item: Item): Promise<void> {
    let slangs: string;
    if (denoArgs.hlsl) {
        slangs = 'hlsl4';
    } else {
        slangs = 'glsl410:glsl300es:metal_macos:metal_ios:metal_sim:wgsl:spirv_vk';
        if (denoArgs.local) {
            slangs += ':hlsl4';
        }
    }
    const args = [
        '-i', item.shader,
        '-t', 'out/tmpdir',
        '-l', slangs,
        '-b',
    ];
    // compile once as regular C header and once as bare_yaml
    await shdc([...args, '-o', `out/${item.shader}.h`]);
    await shdc([...args, '-o', `out/${item.shader}`, '-f', 'bare_yaml']);
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

function gatherSlang(item: Item, prog: string, slang: string, ext: string, isBinary: boolean): string[] {
    const res: string[] = [];
    for (const stage of ['vertex', 'fragment']) {
        const path = `out/${item.shader}_${prog}_${slang}_${stage}${ext}`;
        let bytes: Uint8Array = Deno.readFileSync(path);
        if (!isBinary) {
            // for source code, append a terminating zero
            bytes = new Uint8Array([...bytes, 0]);
        }
        const cArrayName = `${item.prefix}_${prog}_${stage==='vertex'?'vs':'fs'}_${isBinary?'bytecode':'source'}_${slang}`;
        const cArray = bytesToCArray(cArrayName, bytes);
        res.push(...cArray);
    }
    return res;
}

function gatherShader(item: Item, prog: string): string[] {
    const res: string[] = [];
    res.push('#if defined(SOKOL_GLCORE)');
    res.push(...gatherSlang(item, prog, 'glsl410', '.glsl', false));
    res.push('#elif defined(SOKOL_GLES3)');
    res.push(...gatherSlang(item, prog, 'glsl300es', '.glsl', false));
    res.push('#elif defined(SOKOL_METAL)');
    res.push(...gatherSlang(item, prog, 'metal_macos', '.metallib', true));
    res.push(...gatherSlang(item, prog, 'metal_ios', '.metallib', true));
    res.push(...gatherSlang(item, prog, 'metal_sim', '.metal', false));
    res.push('#elif defined(SOKOL_D3D11)');
    if (denoArgs.local) {
        res.push(...gatherSlang(item, prog, 'hlsl4', '.hlsl', false));
    } else {
        res.push(...gatherSlang(item, prog, 'hlsl4', '.fxc', true));
    }
    res.push('#elif defined(SOKOL_WGPU)');
    res.push(...gatherSlang(item, prog, 'wgsl', '.wgsl', false));
    res.push('#elif defined(SOKOL_VULKAN)');
    res.push(...gatherSlang(item, prog, 'spirv_vk', '', true));
    res.push('#elif defined(SOKOL_DUMMY_BACKEND)');
    res.push(`static const char* ${item.prefix}_${prog}_vs_source_dummy = "";`);
    res.push(`static const char* ${item.prefix}_${prog}_fs_source_dummy = "";`);
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
    const ref = denoArgs.branch;
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
