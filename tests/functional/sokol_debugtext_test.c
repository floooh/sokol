//------------------------------------------------------------------------------
//  sokol-debugtext-test.c
//  For best results, run with ASAN and UBSAN.
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)
#define TFLT(f0,f1) {T(fabs((f0)-(f1))<=(0.000001));}

static void init(void) {
    sg_setup(&(sg_desc){0});
    sdtx_setup(&(sdtx_desc_t){0});
}

static void init_with(const sdtx_desc_t* desc) {
    sg_setup(&(sg_desc){0});
    sdtx_setup(desc);
}

static void shutdown(void) {
    sdtx_shutdown();
    sg_shutdown();
}

UTEST(sokol_debugtext, default_init_shutdown) {
    init();
    T(_sdtx.init_cookie == _SDTX_INIT_COOKIE);
    T(_sdtx.desc.context_pool_size == _SDTX_DEFAULT_CONTEXT_POOL_SIZE);
    T(_sdtx.desc.printf_buf_size == _SDTX_DEFAULT_PRINTF_BUF_SIZE);
    T(_sdtx.desc.context.char_buf_size == _SDTX_DEFAULT_CHAR_BUF_SIZE);
    T(_sdtx.desc.context.canvas_width == _SDTX_DEFAULT_CANVAS_WIDTH);
    T(_sdtx.desc.context.canvas_height == _SDTX_DEFAULT_CANVAS_HEIGHT);
    T(_sdtx.desc.context.tab_width == _SDTX_DEFAULT_TAB_WIDTH);
    T(_sdtx.desc.context.color_format == 0);
    T(_sdtx.desc.context.depth_format == 0);
    T(_sdtx.desc.context.sample_count == 0);
    for (int i = 0; i < SDTX_MAX_FONTS; i++) {
        T(_sdtx.desc.fonts[i].data.ptr == 0);
        T(_sdtx.desc.fonts[i].data.size == 0);
        T(_sdtx.desc.fonts[i].first_char == 0);
        T(_sdtx.desc.fonts[i].last_char == 0);
    }
    T(_sdtx.font_img.id != SG_INVALID_ID);
    T(_sdtx.shader.id != SG_INVALID_ID);
    T(_sdtx.fmt_buf_size == (_SDTX_DEFAULT_CHAR_BUF_SIZE + 1));
    T(_sdtx.fmt_buf);
    T(_sdtx.def_ctx_id.id != 0);
    T(_sdtx.def_ctx_id.id == _sdtx.cur_ctx_id.id);
    T(_sdtx.cur_ctx == _sdtx_lookup_context(_sdtx.cur_ctx_id.id));
    T(_sdtx.cur_ctx->desc.char_buf_size == _sdtx.desc.context.char_buf_size);
    T(_sdtx.cur_ctx->desc.canvas_width == _sdtx.desc.context.canvas_width);
    T(_sdtx.cur_ctx->desc.canvas_height == _sdtx.desc.context.canvas_height);
    T(_sdtx.cur_ctx->desc.tab_width == _sdtx.desc.context.tab_width);
    T(_sdtx.cur_ctx->desc.color_format == 0);
    T(_sdtx.cur_ctx->desc.depth_format == 0);
    T(_sdtx.cur_ctx->desc.sample_count == 0);
    T(_sdtx.cur_ctx->vertices.cap == _SDTX_DEFAULT_CHAR_BUF_SIZE * 6);
    T(_sdtx.cur_ctx->vertices.next == 0);
    T(_sdtx.cur_ctx->vertices.ptr);
    T(_sdtx.cur_ctx->commands.cap == _SDTX_DEFAULT_MAX_COMMANDS);
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr);
    T(_sdtx.cur_ctx->vbuf.id != 0);
    T(_sdtx.cur_ctx->pip.id != 0);
    TFLT(_sdtx.cur_ctx->canvas_size.x, 640.0f);
    TFLT(_sdtx.cur_ctx->canvas_size.y, 480.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.x, 8.0f / 640.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.y, 8.0f / 480.0f);
    TFLT(_sdtx.cur_ctx->origin.x, 0.0f);
    TFLT(_sdtx.cur_ctx->origin.y, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 0.0f);
    TFLT(_sdtx.cur_ctx->tab_width, 4.0f);
    T(_sdtx.cur_ctx->color == _SDTX_DEFAULT_COLOR);
    T(_sdtx.context_pool.contexts);
    T(_sdtx.context_pool.pool.size == (_SDTX_DEFAULT_CONTEXT_POOL_SIZE + 1));
    shutdown();
    T(_sdtx.init_cookie == 0);
}

UTEST(sokol_debugtext, init_with_params) {
    init_with(&(sdtx_desc_t){
        .context_pool_size = 2,
        .printf_buf_size = 128,
        .context = {
            .char_buf_size = 256,
            .canvas_width = 320,
            .canvas_height = 200,
            .tab_width = 8,
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .sample_count = 4,
        }
    });
    T(_sdtx.init_cookie == _SDTX_INIT_COOKIE);
    T(_sdtx.desc.context_pool_size == 2);
    T(_sdtx.desc.printf_buf_size == 128);
    T(_sdtx.desc.context.char_buf_size == 256);
    T(_sdtx.desc.context.canvas_width == 320);
    T(_sdtx.desc.context.canvas_height == 200);
    T(_sdtx.desc.context.tab_width == 8);
    T(_sdtx.desc.context.color_format == SG_PIXELFORMAT_RGBA8);
    T(_sdtx.desc.context.depth_format == SG_PIXELFORMAT_DEPTH_STENCIL);
    T(_sdtx.desc.context.sample_count == 4);
    T(_sdtx.fmt_buf_size == 129);
    T(_sdtx.cur_ctx->desc.char_buf_size == _sdtx.desc.context.char_buf_size);
    T(_sdtx.cur_ctx->desc.canvas_width == _sdtx.desc.context.canvas_width);
    T(_sdtx.cur_ctx->desc.canvas_height == _sdtx.desc.context.canvas_height);
    T(_sdtx.cur_ctx->desc.tab_width == _sdtx.desc.context.tab_width);
    T(_sdtx.cur_ctx->desc.color_format == SG_PIXELFORMAT_RGBA8);
    T(_sdtx.cur_ctx->desc.depth_format == SG_PIXELFORMAT_DEPTH_STENCIL);
    T(_sdtx.cur_ctx->desc.sample_count == 4);
    T(_sdtx.cur_ctx->vertices.cap == (256 * 6));
    TFLT(_sdtx.cur_ctx->canvas_size.x, 320.0f);
    TFLT(_sdtx.cur_ctx->canvas_size.y, 200.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.x, 8.0f / 320.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.y, 8.0f / 200.0f);
    TFLT(_sdtx.cur_ctx->tab_width, 8.0f);
    T(_sdtx.context_pool.pool.size == 3);
    shutdown();
}

UTEST(sokol_debugtext, make_destroy_context) {
    init();
    sdtx_context ctx_id = sdtx_make_context(&(sdtx_context_desc_t){
        .char_buf_size = 64,
        .canvas_width = 1024,
        .canvas_height = 768,
        .tab_width = 3,
        .color_format = SG_PIXELFORMAT_RGBA32F,
        .sample_count = 2
    });
    T(ctx_id.id != 0);
    T(ctx_id.id != _sdtx.cur_ctx_id.id);
    _sdtx_context_t* ctx = _sdtx_lookup_context(ctx_id.id);
    T(ctx);
    T(ctx != _sdtx.cur_ctx);
    T(ctx->desc.char_buf_size == 64);
    T(ctx->desc.canvas_width == 1024);
    T(ctx->desc.canvas_height == 768);
    T(ctx->desc.tab_width == 3);
    T(ctx->desc.color_format == SG_PIXELFORMAT_RGBA32F);
    T(ctx->desc.depth_format == 0);
    T(ctx->desc.sample_count == 2);
    T(ctx->vertices.ptr);
    T(ctx->vertices.next == 0);
    T(ctx->vertices.cap == (64 * 6));
    TFLT(ctx->canvas_size.x, 1024.0f);
    TFLT(ctx->canvas_size.y, 768.0f);
    TFLT(ctx->glyph_size.x, 8.0f / 1024.0f);
    TFLT(ctx->glyph_size.y, 8.0f / 768.0f);
    TFLT(ctx->tab_width, 3.0f);
    sdtx_destroy_context(ctx_id);
    T(0 == _sdtx_lookup_context(ctx_id.id));
    T(ctx->desc.char_buf_size == 0);
    T(ctx->vertices.ptr == 0);
    shutdown();
}

UTEST(sokol_debugtext, get_default_context) {
    // getting the default context must always return SDTX_DEFAULT_CONTEXT
    init();
    T(sdtx_get_context().id == SDTX_DEFAULT_CONTEXT.id);
    shutdown();
}

UTEST(sokol_debugtext, set_get_context) {
    init();
    sdtx_context ctx_id = sdtx_make_context(&(sdtx_context_desc_t){ 0 });
    T(ctx_id.id != 0);
    T(ctx_id.id != _sdtx.cur_ctx_id.id);
    sdtx_set_context(ctx_id);
    T(sdtx_get_context().id == ctx_id.id);
    T(ctx_id.id == _sdtx.cur_ctx_id.id);
    const _sdtx_context_t* ctx = _sdtx_lookup_context(ctx_id.id);
    T(ctx == _sdtx.cur_ctx);
    sdtx_set_context(SDTX_DEFAULT_CONTEXT);
    T(sdtx_get_context().id == SDTX_DEFAULT_CONTEXT.id);
    T(_sdtx.cur_ctx);
    T(ctx != _sdtx.cur_ctx);
    T(_sdtx.cur_ctx == _sdtx_lookup_context(_sdtx.def_ctx_id.id));
    shutdown();
}

UTEST(sokol_debugtext, destroy_default_context) {
    // destroying the default context is not allowed
    init();
    sdtx_context def_ctx_id = _sdtx.def_ctx_id;
    T(def_ctx_id.id == _sdtx.cur_ctx_id.id);
    sdtx_destroy_context(def_ctx_id);
    T(def_ctx_id.id == _sdtx.def_ctx_id.id);
    T(def_ctx_id.id == _sdtx.cur_ctx_id.id);
    T(_sdtx.cur_ctx);
    shutdown();
}

UTEST(sokol_debugtext, destroy_current_context) {
    // destroying the current context has the same effect
    // as setting a current context with an invalid context handle
    init();
    sdtx_context ctx_id = sdtx_make_context(&(sdtx_context_desc_t){ 0 });
    sdtx_set_context(ctx_id);
    T(sdtx_get_context().id == ctx_id.id);
    T(ctx_id.id == _sdtx.cur_ctx_id.id);
    T(_sdtx_lookup_context(ctx_id.id) == _sdtx.cur_ctx);
    sdtx_destroy_context(ctx_id);
    T(_sdtx.cur_ctx_id.id == ctx_id.id);
    T(_sdtx.cur_ctx == 0);
    T(sdtx_get_context().id == ctx_id.id);
    shutdown();
}

UTEST(sokol_debugtext, ignore_invalid_context_handle) {
    // trying to render with an invalid context handle must not crash,
    // instead ignore all operations
    init();
    sdtx_context ctx_id = sdtx_make_context(&(sdtx_context_desc_t){ 0 });
    sdtx_set_context(ctx_id);
    sdtx_destroy_context(ctx_id);
    T(0 == _sdtx.cur_ctx);
    T(sdtx_get_context().id == ctx_id.id);
    sdtx_font(0);
    sdtx_canvas(100.0f, 200.0f);
    sdtx_origin(10.0f, 10.0f);
    sdtx_home();
    sdtx_pos(1.0f, 2.0f);
    sdtx_pos_x(1.0f);
    sdtx_pos_y(2.0f);
    sdtx_move(2.0f, 3.0f);
    sdtx_move_x(2.0f);
    sdtx_move_y(3.0f);
    sdtx_crlf();
    sdtx_color3b(255, 255, 255);
    sdtx_color3f(1.0f, 1.0f, 1.0f);
    sdtx_color4b(255, 255, 255, 255);
    sdtx_color4f(1.0f, 1.0f, 1.0f, 1.0f);
    sdtx_color1i(0xFFFFFFFF);
    sdtx_putc('A');
    sdtx_puts("Hello World!");
    sdtx_putr("Hello World!", 5);
    sdtx_printf("Hello World %d %d %d", 1, 2, 3);
    shutdown();
}

UTEST(sokol_debugtext, set_font) {
    init();
    T(0 == _sdtx.cur_ctx->cur_font);
    sdtx_font(1);
    T(1 == _sdtx.cur_ctx->cur_font);
    sdtx_font(2);
    T(2 == _sdtx.cur_ctx->cur_font);
    shutdown();
}

UTEST(sokol_debugtext, set_canvas) {
    init();
    sdtx_origin(10.0f, 11.0f);
    sdtx_pos(1.0f, 2.0f);
    sdtx_canvas(320.0f, 200.0f);
    TFLT(_sdtx.cur_ctx->canvas_size.x, 320.0f);
    TFLT(_sdtx.cur_ctx->canvas_size.y, 200.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.x, 8.0f / 320.0f);
    TFLT(_sdtx.cur_ctx->glyph_size.y, 8.0f / 200.0f);
    // origin and pos must be reset to 0 when canvas is set
    TFLT(_sdtx.cur_ctx->origin.x, 0.0f);
    TFLT(_sdtx.cur_ctx->origin.y, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 0.0f);
    shutdown();
}

UTEST(sokol_debugtext, set_origin) {
    init();
    sdtx_origin(10.0f, 20.0f);
    TFLT(_sdtx.cur_ctx->origin.x, 10.0f);
    TFLT(_sdtx.cur_ctx->origin.y, 20.0f);
    shutdown();
}

UTEST(sokol_debugtext, cursor_movement) {
    init();
    sdtx_pos(1.0f, 2.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 1.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 2.0f);
    sdtx_pos_x(5.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 5.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 2.0f);
    sdtx_pos_y(6.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 5.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 6.0f);
    sdtx_move(-1.0f, -3.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 4.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 3.0f);
    sdtx_move_x(+1.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 5.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 3.0f);
    sdtx_move_y(+3.0f);
    TFLT(_sdtx.cur_ctx->pos.x, 5.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 6.0f);
    sdtx_crlf();
    TFLT(_sdtx.cur_ctx->pos.x, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 7.0f);
    sdtx_pos(20.0f, 30.0f);
    sdtx_home();
    TFLT(_sdtx.cur_ctx->pos.x, 0.0f);
    TFLT(_sdtx.cur_ctx->pos.y, 0.0f);
    shutdown();
}

UTEST(sokol_debugtext, set_color) {
    init();
    T(_sdtx.cur_ctx->color == _SDTX_DEFAULT_COLOR);
    sdtx_color3b(255, 127, 0);
    T(_sdtx.cur_ctx->color == 0xFF007FFF);
    sdtx_color4b(0, 127, 255, 255);
    T(_sdtx.cur_ctx->color == 0xFFFF7F00);
    sdtx_color3f(1.0f, 0.5f, 0.0f);
    T(_sdtx.cur_ctx->color == 0xFF007FFF);
    sdtx_color4f(0.0f, 0.5f, 1.0f, 1.0f);
    T(_sdtx.cur_ctx->color == 0xFFFF7F00);
    sdtx_color1i(0xFF000000);
    T(_sdtx.cur_ctx->color == 0xFF000000);
    shutdown();
}

UTEST(sokol_debugtext, vertex_overflow) {
    // overflowing the vertex buffer must not crash
    init_with(&(sdtx_desc_t){
        .context.char_buf_size = 8,
    });
    for (int i = 0; i < 32; i++) {
        sdtx_putc('A');
    }
    sdtx_puts("1234567890");
    sdtx_putr("1234567890", 5);
    sdtx_printf("Hello World %d!\n", 12);
    T(_sdtx.cur_ctx->vertices.next == _sdtx.cur_ctx->vertices.cap);
    shutdown();
}

UTEST(sokol_debugtext, context_overflow) {
    // creating too many contexts should not crash
    init_with(&(sdtx_desc_t){
        .context_pool_size = 4,
    });
    T(_sdtx.context_pool.pool.size == 5);
    // one slot is taken by the default context
    sdtx_context ctx[4];
    for (int i = 0; i < 4; i++) {
        ctx[i] = sdtx_make_context(&(sdtx_context_desc_t){ 0 });
        if (i < 3) {
            T(ctx[i].id != 0);
        }
        else {
            T(ctx[i].id == 0);
        }
    }
    // destroying an invalid context should not crash
    for (int i = 0; i < 4; i++) {
        sdtx_destroy_context(ctx[i]);
    }
    shutdown();
}

UTEST(sokol_debugtext, printf_overflow) {
    // overflowing the printf formatting buffer should not crash
    init_with(&(sdtx_desc_t){
        .printf_buf_size = 8
    });
    T(9 == _sdtx.fmt_buf_size);
    T(16 == sdtx_printf("Hello %d\n", 123456789));
    T('H' == _sdtx.fmt_buf[0])
    T('e' == _sdtx.fmt_buf[1])
    T('l' == _sdtx.fmt_buf[2])
    T('l' == _sdtx.fmt_buf[3])
    T('o' == _sdtx.fmt_buf[4])
    T(' ' == _sdtx.fmt_buf[5])
    T('1' == _sdtx.fmt_buf[6])
    T('2' == _sdtx.fmt_buf[7])
    T(0   == _sdtx.fmt_buf[8])
    shutdown();
}

UTEST(sokol_debugtext, rewind_after_draw) {
    // calling sdtx_draw() must rewind the cursor position, font and
    // vertex pointer, to keep canvas size and origin as is
    init();
    sdtx_canvas(256, 128);
    TFLT(_sdtx.cur_ctx->canvas_size.x, 256);
    TFLT(_sdtx.cur_ctx->canvas_size.y, 128);
    sdtx_origin(5, 5);
    TFLT(_sdtx.cur_ctx->origin.x, 5);
    TFLT(_sdtx.cur_ctx->origin.y, 5);
    sdtx_pos(10, 20);
    TFLT(_sdtx.cur_ctx->pos.x, 10);
    TFLT(_sdtx.cur_ctx->pos.y, 20);
    sdtx_font(3);
    T(_sdtx.cur_ctx->cur_font == 3);
    sdtx_printf("Hello World!\n");
    T(_sdtx.cur_ctx->vertices.next != 0);
    sg_begin_default_pass(&(sg_pass_action){ 0 }, 256, 256);
    sdtx_draw();
    sg_end_pass();
    sg_commit();
    TFLT(_sdtx.cur_ctx->canvas_size.x, 256);
    TFLT(_sdtx.cur_ctx->canvas_size.y, 128);
    TFLT(_sdtx.cur_ctx->origin.x, 5);
    TFLT(_sdtx.cur_ctx->origin.y, 5);
    TFLT(_sdtx.cur_ctx->pos.x, 0);
    TFLT(_sdtx.cur_ctx->pos.x, 0);
    T(_sdtx.cur_ctx->cur_font == 0);
    T(_sdtx.cur_ctx->vertices.next == 0);
    shutdown();
}

UTEST(sokol_debugtext, putr) {
    // test if sdtx_putr() draws the right amount of characters
    init();
    int start_index = _sdtx.cur_ctx->vertices.next;
    sdtx_putr("Hello World!", 5);
    T((5 * 6) == (_sdtx.cur_ctx->vertices.next - start_index));

    start_index = _sdtx.cur_ctx->vertices.next;
    sdtx_putr("Hello!\n\n\n\n\n\n\n\n\n\n\n", 10);
    // NOTE: the \n's don't result in rendered vertices
    T((6 * 6) == (_sdtx.cur_ctx->vertices.next - start_index));
    shutdown();
}

UTEST(sokol_debugtext, default_context) {
    init();
    T(sdtx_default_context().id == SDTX_DEFAULT_CONTEXT.id);
    shutdown();
}

// switching layers without any text inbetween should not advance the current draw command
UTEST(sokol_debug_text, empty_layers) {
    init();
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 0);
    sdtx_layer(1);
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 1);
    sdtx_layer(2);
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 2);
    sdtx_layer(0);
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 0);
    shutdown();
}

// switching layers with text inbetween should advance the current draw command
UTEST(sokol_debug_text, non_empty_layers) {
    init();
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 0);
    T(_sdtx.cur_ctx->commands.ptr[0].first_vertex == 0);
    T(_sdtx.cur_ctx->commands.ptr[0].num_vertices == 0);
    sdtx_puts("123");
    T(_sdtx.cur_ctx->commands.next == 1);
    T(_sdtx.cur_ctx->commands.ptr[0].layer_id == 0);
    T(_sdtx.cur_ctx->commands.ptr[0].first_vertex == 0);
    T(_sdtx.cur_ctx->commands.ptr[0].num_vertices == (3 * 6));
    sdtx_layer(1);
    sdtx_puts("1234");
    T(_sdtx.cur_ctx->commands.next == 2);
    T(_sdtx.cur_ctx->commands.ptr[1].layer_id == 1);
    T(_sdtx.cur_ctx->commands.ptr[1].first_vertex == (3 * 6));
    T(_sdtx.cur_ctx->commands.ptr[1].num_vertices == (4 * 6));
    // switching to same layer should not start a new draw commands
    sdtx_layer(1);
    sdtx_puts("12345");
    T(_sdtx.cur_ctx->commands.next == 2);
    T(_sdtx.cur_ctx->commands.ptr[1].layer_id == 1);
    T(_sdtx.cur_ctx->commands.ptr[1].first_vertex == (3 * 6));
    T(_sdtx.cur_ctx->commands.ptr[1].num_vertices == (9 * 6));
    sdtx_layer(0);
    sdtx_puts("123456");
    T(_sdtx.cur_ctx->commands.next == 3);
    T(_sdtx.cur_ctx->commands.ptr[2].layer_id == 0);
    T(_sdtx.cur_ctx->commands.ptr[2].first_vertex == (12 * 6));
    T(_sdtx.cur_ctx->commands.ptr[2].num_vertices == (6 * 6));
    shutdown();
}

UTEST(sokol_debug_text, command_buffer_overflow) {
    init_with(&(sdtx_desc_t){
        .context = {
            .max_commands = 4
        }
    });
    sdtx_puts("0");
    T(_sdtx.cur_ctx->commands.next == 1);
    sdtx_layer(1);
    sdtx_puts("1");
    T(_sdtx.cur_ctx->commands.next == 2);
    sdtx_layer(2);
    sdtx_puts("2");
    T(_sdtx.cur_ctx->commands.next == 3);
    sdtx_layer(3);
    sdtx_puts("3");
    T(_sdtx.cur_ctx->commands.next == 4);
    // from here on should fail
    sdtx_layer(4);
    sdtx_puts("4");
    T(_sdtx.cur_ctx->commands.next == 4);
}