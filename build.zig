const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const native = b.option(bool, "native", "Do not use dependencies, use system files instead") orelse true;
    const dynamic_linkage = b.option(bool, "shared", "Build sokol_turbo as a shared library") orelse false;
    // const include_src = b.option(bool, "include_src", "Add the root directory as an include directory") orelse false;
    const use_x11 = b.option(bool, "x11", "Build with X11. Only useful on Linux") orelse true;
    const use_wayland = b.option(bool, "wayland", "Force Wayland (default: false, Linux only, not supported in main-line headers)") orelse false;
    const use_opengl = b.option(bool, "opengl", "Force OpenGL (default: false); deprecated on MacOS") orelse false;
    const use_sokol_imgui = b.option(bool, "sokol_imgui", "Add support for sokol_imgui.h bindings") orelse false;
    const sokol_backend: SokolBackend = if (use_opengl) .gl else .auto;

    // if (shared) lib.root_module.addCMacro("SOKOL_DLL", "1");

    // This creates a module, which represents a collection of source files alongside
    // some compilation options, such as optimization mode and linked system libraries.
    // Zig modules are the preferred way of making Zig code available to consumers.
    // addModule defines a module that we intend to make available for importing
    // to our consumers. We must give it a name because a Zig package can expose
    // multiple modules and consumers will need to be able to specify which
    // module they want to access.
    const mod_sokol_turbo = b.addModule("sokol_turbo", .{
        // The root source file is the "entry point" of this module. Users of
        // this module will only be able to access public declarations contained
        // in this file, which means that if you have declarations that you
        // intend to expose to consumers that were defined in other files part
        // of this module, you will have to make sure to re-export them from
        // the root file.
        .root_source_file = b.path("src/root.zig"),
        // Later on we'll use this module as the root module of a test executable
        // which requires us to specify a target.
        .target = target,
    });

    const lib_sokol_turbo = try buildLibSokol(b, .{
        .target = target,
        .optimize = optimize,
        .backend = sokol_backend,
        .dynamic_linkage = dynamic_linkage,
        .use_x11 = use_x11,
        .use_wayland = use_wayland,
        .with_sokol_imgui = use_sokol_imgui,
        .dont_link_system_libs = !native,
    });
    mod_sokol_turbo.linkLibrary(lib_sokol_turbo);

    // Here we define an executable. An executable needs to have a root module
    // which needs to expose a `main` function. While we could add a main function
    // to the module defined above, it's sometimes preferable to split business
    // logic and the CLI into two separate modules.
    //
    // If your goal is to create a Zig library for others to use, consider if
    // it might benefit from also exposing a CLI tool. A parser library for a
    // data serialization format could also bundle a CLI syntax checker, for example.
    //
    // If instead your goal is to create an executable, consider if users might
    // be interested in also being able to embed the core functionality of your
    // program in their own executable in order to avoid the overhead involved in
    // subprocessing your CLI tool.
    //
    // If neither case applies to you, feel free to delete the declaration you
    // don't need and to put everything under a single module.
    const exe = b.addExecutable(.{
        .name = "sokol_turbo_test",
        .root_module = b.createModule(.{
            // b.createModule defines a new module just like b.addModule but,
            // unlike b.addModule, it does not expose the module to consumers of
            // this package, which is why in this case we don't have to give it a name.
            .root_source_file = b.path("src/main.zig"),
            // Target and optimization levels must be explicitly wired in when
            // defining an executable or library (in the root module), and you
            // can also hardcode a specific target for an executable or library
            // definition if desireable (e.g. firmware for embedded devices).
            .target = target,
            .optimize = optimize,
            // List of modules available for import in source files part of the
            // root module.
            .imports = &.{
                // Here "sokol_turbo" is the name you will use in your source code to
                // import this module (e.g. `@import("sokol_turbo")`). The name is
                // repeated because you are allowed to rename your imports, which
                // can be extremely useful in case of collisions (which can happen
                // importing modules from different packages).
                .{ .name = "sokol_turbo", .module = mod_sokol_turbo },
            },
        }),
    });

    // This declares intent for the executable to be installed into the
    // install prefix when running `zig build` (i.e. when executing the default
    // step). By default the install prefix is `zig-out/` but can be overridden
    // by passing `--prefix` or `-p`.
    b.installArtifact(exe);

    // This creates a top level step. Top level steps have a name and can be
    // invoked by name when running `zig build` (e.g. `zig build run`).
    // This will evaluate the `run` step rather than the default step.
    // For a top level step to actually do something, it must depend on other
    // steps (e.g. a Run step, as we will see in a moment).
    const run_step = b.step("run", "Run the app");

    // This creates a RunArtifact step in the build graph. A RunArtifact step
    // invokes an executable compiled by Zig. Steps will only be executed by the
    // runner if invoked directly by the user (in the case of top level steps)
    // or if another step depends on it, so it's up to you to define when and
    // how this Run step will be executed. In our case we want to run it when
    // the user runs `zig build run`, so we create a dependency link.
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);

    // By making the run step depend on the default step, it will be run from the
    // installation directory rather than directly from within the cache directory.
    run_cmd.step.dependOn(b.getInstallStep());

    // This allows the user to pass arguments to the application in the build
    // command itself, like this: `zig build run -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // Creates an executable that will run `test` blocks from the provided module.
    // Here `mod` needs to define a target, which is why earlier we made sure to
    // set the releative field.
    const mod_tests = b.addTest(.{
        .root_module = mod_sokol_turbo,
    });

    // A run step that will run the test executable.
    const run_mod_tests = b.addRunArtifact(mod_tests);

    // Creates an executable that will run `test` blocks from the executable's
    // root module. Note that test executables only test one module at a time,
    // hence why we have to create two separate ones.
    const exe_tests = b.addTest(.{
        .root_module = exe.root_module,
    });

    // A run step that will run the second test executable.
    const run_exe_tests = b.addRunArtifact(exe_tests);

    // A top level step for running all tests. dependOn can be called multiple
    // times and since the two run steps do not depend on one another, this will
    // make the two of them run in parallel.
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_mod_tests.step);
    test_step.dependOn(&run_exe_tests.step);

    // Just like flags, top level steps are also listed in the `--help` menu.
    //
    // The Zig build system is entirely implemented in userland, which means
    // that it cannot hook into private compiler APIs. All compilation work
    // orchestrated by the build system will result in other Zig compiler
    // subcommands being invoked with the right flags defined. You can observe
    // these invocations when one fails (or you pass a flag to increase
    // verbosity) to validate assumptions and diagnose problems.
    //
    // Lastly, the Zig build system is relatively simple and self-contained,
    // and reading its source code will allow you to master it.
}

pub const SokolBackend = enum {
    auto, // macOS/iOS: Metal, otherwise: GL
    metal,
    gl,
};

pub const TargetPlatform = enum {
    linux,
    darwin, // macos and ios
    macos,
    windows,
};

pub fn isPlatform(target: std.Target, platform: TargetPlatform) bool {
    return switch (platform) {
        .linux => target.os.tag == .linux,
        .darwin => target.os.tag.isDarwin(),
        .macos => target.os.tag == .macos,
        .windows => target.os.tag == .windows,
    };
}

// helper function to resolve .auto backend based on target platform
pub fn resolveSokolBackend(backend: SokolBackend, target: std.Target) SokolBackend {
    if (backend != .auto) {
        return backend;
    } else if (isPlatform(target, .darwin)) {
        return .metal;
    } else {
        return .gl;
    }
}

// build the sokol C headers into a static library
pub const LibSokolOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    backend: SokolBackend = .auto,
    use_x11: bool = true,
    use_wayland: bool = false,
    dynamic_linkage: bool = false,
    with_sokol_imgui: bool = false,
    cimgui_header_path: ?[]const u8 = null,
    dont_link_system_libs: bool = false,
};

pub fn buildLibSokol(b: *std.Build, options: LibSokolOptions) !*std.Build.Step.Compile {
    const mod_target = options.target.result;

    const csrc_root = "./";
    _ = csrc_root;

    const csources = if (mod_target.os.tag == .macos) [_][]const u8{
        "sokol-turbo.m",
    } else [_][]const u8{
        "sokol-turbo.c",
    };
    const mod = b.addModule("mod_sokol_turbo_clib", .{
        .target = options.target,
        .optimize = options.optimize,
        .link_libc = true,
    });

    const backend = resolveSokolBackend(options.backend, mod_target);
    const lib = b.addLibrary(.{
        .name = "sokol_turbo_clib",
        .linkage = if (options.dynamic_linkage) .dynamic else .static,
        .root_module = mod,
    });

    // resolve .auto backend into specific backend by platform
    var cflags_buf: [64][]const u8 = undefined;
    var cflags = std.ArrayListUnmanaged([]const u8).initBuffer(&cflags_buf);

    try cflags.appendBounded("-DIMPL");
    try cflags.appendBounded("-DSOKOL_IMPL");
    if (options.optimize != .Debug) {
        try cflags.appendBounded("-DNDEBUG");
    }
    // Question: should we use addCMacro, or .cflags like above?!
    if (options.dynamic_linkage) lib.root_module.addCMacro("SOKOL_DLL", "1");
    switch (backend) {
        .metal => try cflags.appendBounded("-DSOKOL_METAL"),
        .gl => try cflags.appendBounded("-DSOKOL_GLCORE"),
        else => @panic("unknown sokol backend"),
    }

    // platform specific compile and link options
    const link_system_libs = !options.dont_link_system_libs;
    if (isPlatform(mod_target, .darwin)) {
        try cflags.appendBounded("-ObjC");
        if (link_system_libs) {
            mod.linkFramework("Foundation", .{});
            mod.linkFramework("AudioToolbox", .{});
            if (.metal == backend) {
                mod.linkFramework("MetalKit", .{});
                mod.linkFramework("Metal", .{});
            }
        } else if (mod_target.os.tag == .macos) {
            mod.linkFramework("Cocoa", .{});
            mod.linkFramework("QuartzCore", .{});
            if (.gl == backend) {
                mod.linkFramework("OpenGL", .{});
            }
        }
    } else if (isPlatform(mod_target, .linux)) {
        if (!options.use_x11) try cflags.appendBounded("-DSOKOL_DISABLE_X11");
        if (!options.use_wayland) try cflags.appendBounded("-DSOKOL_DISABLE_WAYLAND");
        const link_egl = options.use_wayland;
        if (link_system_libs) {
            // mod.linkSystemLibrary("asound", .{});
            mod.linkSystemLibrary("GL", .{});
            if (options.use_x11) {
                mod.linkSystemLibrary("X11", .{});
                mod.linkSystemLibrary("Xi", .{});
                mod.linkSystemLibrary("Xcursor", .{});
            }
            if (options.use_wayland) {
                mod.linkSystemLibrary("wayland-client", .{});
                mod.linkSystemLibrary("wayland-cursor", .{});
                mod.linkSystemLibrary("wayland-egl", .{});
                mod.linkSystemLibrary("xkbcommon", .{});
            }
            if (link_egl) {
                mod.linkSystemLibrary("EGL", .{});
            }
        }
    } else if (isPlatform(mod_target, .windows)) {
        if (link_system_libs) {
            mod.linkSystemLibrary("kernel32", .{});
            mod.linkSystemLibrary("user32", .{});
            mod.linkSystemLibrary("gdi32", .{});
            mod.linkSystemLibrary("ole32", .{});
            // if (.d3d11 == backend) {
            //     mod.linkSystemLibrary("d3d11", .{});
            //     mod.linkSystemLibrary("dxgi", .{});
            // }
        }
    }

    // finally add the C source files
    inline for (csources) |csrc| {
        mod.addCSourceFile(.{
            // .file = b.path(csrc_root ++ csrc),
            .file = b.path(csrc),
            .flags = cflags.items,
        });
    }

    // optional Dear ImGui support, the called is required to also
    // add the cimgui include path to the returned compile step
    if (options.with_sokol_imgui) {
        // if (options.sokol_imgui_cprefix) |cprefix| {
        //     try cflags.appendBounded(b.fmt("-DSOKOL_IMGUI_CPREFIX={s}", .{cprefix}));
        // }
        // if (options.cimgui_header_path) |cimgui_header_path| {
        //     try cflags.appendBounded(b.fmt("-DCIMGUI_HEADER_PATH=\"{s}\"", .{cimgui_header_path}));
        // }
        mod.addCSourceFile(.{
            // .file = b.path(csrc_root ++ "sokol_imgui.c"),
            .file = b.path("util/sokol_imgui.c"),
            .flags = cflags.items,
        });
    }

    // make sokol headers available to users of `sokol_turbo_clib` via `#include "sokol_turbo/sokol_gfx.h"
    // lib.installHeadersDirectory(b.path("."), "sokol_turbo", .{});
    // TODO: loop on array of headers and build dest_rel_path, call lib.installHeader
    lib.installHeader(b.path("sokol_app_turbo.h"), "sokol_turbo/sokol_app_turbo.h");
    lib.installHeader(b.path("sokol_app.h"), "sokol_turbo/sokol_app.h");
    lib.installHeader(b.path("sokol_log.h"), "sokol_turbo/sokol_log.h");
    lib.installHeader(b.path("sokol_time.h"), "sokol_turbo/sokol_time.h");
    lib.installHeader(b.path("sokol_gfx.h"), "sokol_turbo/sokol_gfx.h");
    lib.installHeader(b.path("sokol_glue.h"), "sokol_turbo/sokol_glue.h");

    // installArtifact allows us to find the lib_sokol compile step when
    // sokol is used as package manager dependency via 'dep_sokol.artifact("sokol_turbo_clib")'
    b.installArtifact(lib);

    return lib;
}
