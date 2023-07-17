//------------------------------------------------------------------------------
//  sokol-shape-test.c
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_SHAPE_IMPL
#include "sokol_shape.h"
#include "utest.h"

#define T(b) EXPECT_TRUE(b)
#define TFLT(f0,f1,epsilon) {T(fabs((f0)-(f1))<=(epsilon));}

UTEST(sokol_shape, color4f) {
    T(sshape_color_4f(1.0f, 0.0f, 0.0f, 0.0f) == 0x000000FF);
    T(sshape_color_4f(0.0f, 1.0f, 0.0f, 0.0f) == 0x0000FF00);
    T(sshape_color_4f(0.0f, 0.0f, 1.0f, 0.0f) == 0x00FF0000);
    T(sshape_color_4f(0.0f, 0.0f, 0.0f, 1.0f) == 0xFF000000);
}

UTEST(sokol_shape, color3f) {
    T(sshape_color_3f(1.0f, 0.0f, 0.0f) == 0xFF0000FF);
    T(sshape_color_3f(0.0f, 1.0f, 0.0f) == 0xFF00FF00);
    T(sshape_color_3f(0.0f, 0.0f, 1.0f) == 0xFFFF0000);
}

UTEST(sokol_shape, color4b) {
    T(sshape_color_4b(255, 0, 0, 0) == 0x000000FF);
    T(sshape_color_4b(0, 255, 0, 0) == 0x0000FF00);
    T(sshape_color_4b(0, 0, 255, 0) == 0x00FF0000);
    T(sshape_color_4b(0, 0, 0, 255) == 0xFF000000);
}

UTEST(sokol_shape, color3b) {
    T(sshape_color_3b(255, 0, 0) == 0xFF0000FF);
    T(sshape_color_3b(0, 255, 0) == 0xFF00FF00);
    T(sshape_color_3b(0, 0, 255) == 0xFFFF0000);
}

UTEST(sokol_shape, mat4) {
    float values[16] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };
    sshape_mat4_t m = sshape_mat4(values);
    T(m.m[0][0] == 1.0f);
    T(m.m[0][1] == 2.0f);
    T(m.m[0][2] == 3.0f);
    T(m.m[0][3] == 4.0f);
    T(m.m[1][0] == 5.0f);
    T(m.m[1][1] == 6.0f);
    T(m.m[1][2] == 7.0f);
    T(m.m[1][3] == 8.0f);
    T(m.m[2][0] == 9.0f);
    T(m.m[2][1] == 10.0f);
    T(m.m[2][2] == 11.0f);
    T(m.m[2][3] == 12.0f);
    T(m.m[3][0] == 13.0f);
    T(m.m[3][1] == 14.0f);
    T(m.m[3][2] == 15.0f);
    T(m.m[3][3] == 16.0f);
}

UTEST(sokol_shape, mat4_transpose) {
    float values[16] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };
    sshape_mat4_t m = sshape_mat4_transpose(values);
    T(m.m[0][0] == 1.0f);
    T(m.m[1][0] == 2.0f);
    T(m.m[2][0] == 3.0f);
    T(m.m[3][0] == 4.0f);
    T(m.m[0][1] == 5.0f);
    T(m.m[1][1] == 6.0f);
    T(m.m[2][1] == 7.0f);
    T(m.m[3][1] == 8.0f);
    T(m.m[0][2] == 9.0f);
    T(m.m[1][2] == 10.0f);
    T(m.m[2][2] == 11.0f);
    T(m.m[3][2] == 12.0f);
    T(m.m[0][3] == 13.0f);
    T(m.m[1][3] == 14.0f);
    T(m.m[2][3] == 15.0f);
    T(m.m[3][3] == 16.0f);
}

UTEST(sokol_shape, plane_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_plane_sizes(1);
    T(4 == res.vertices.num);
    T(6 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_plane_sizes(2);
    T(9 == res.vertices.num);
    T(24 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, box_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_box_sizes(1);
    T(24 == res.vertices.num);
    T(36 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_box_sizes(2);
    T(54 == res.vertices.num);
    T(144 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, sphere_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_sphere_sizes(3, 2);
    T(12 == res.vertices.num);
    T(18 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_sphere_sizes(36, 12);
    T(481 ==  res.vertices.num);
    T(2376 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, cylinder_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_cylinder_sizes(3, 1);
    T(24 == res.vertices.num);
    T(36 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_cylinder_sizes(5, 2);
    T(42 == res.vertices.num);
    T(90 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, torus_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_torus_sizes(3, 3);
    T(16 == res.vertices.num);
    T(54 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_torus_sizes(4, 5);
    T(30 == res.vertices.num);
    T(120 == res.indices.num);
    T(res.vertices.num * sizeof(sshape_vertex_t) == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, buffer_layout_desc) {
    const sg_vertex_buffer_layout_state l_state = sshape_vertex_buffer_layout_state();
    T(sizeof(sshape_vertex_t) == l_state.stride);
    T(0 == l_state.step_func);
    T(0 == l_state.step_rate);
}

UTEST(sokol_shape, attr_descs) {
    {
        const sg_vertex_attr_state a_state = sshape_position_vertex_attr_state();
        T(offsetof(sshape_vertex_t, x) == a_state.offset);
        T(SG_VERTEXFORMAT_FLOAT3 == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_normal_vertex_attr_state();
        T(offsetof(sshape_vertex_t, normal) == a_state.offset);
        T(SG_VERTEXFORMAT_BYTE4N == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_texcoord_vertex_attr_state();
        T(offsetof(sshape_vertex_t, u) == a_state.offset);
        T(SG_VERTEXFORMAT_USHORT2N == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_color_vertex_attr_state();
        T(offsetof(sshape_vertex_t, color) == a_state.offset);
        T(SG_VERTEXFORMAT_UBYTE4N == a_state.format);
        T(0 == a_state.buffer_index);
    }
}

UTEST(sokol_shape, buffer_descs_elm_range) {
    sshape_vertex_t vx[128] = { 0 };
    uint16_t ix[128] = { 0 };
    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer = SSHAPE_RANGE(ix),
    };

    // build a box...
    {
        buf = sshape_build_box(&buf, &(sshape_box_t) {0});
        const sg_buffer_desc vbuf_desc = sshape_vertex_buffer_desc(&buf);
        const sg_buffer_desc ibuf_desc = sshape_index_buffer_desc(&buf);
        const sshape_element_range_t elm_range = sshape_element_range(&buf);
        T(vbuf_desc.size == 0);
        T(vbuf_desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
        T(vbuf_desc.usage == SG_USAGE_IMMUTABLE);
        T(vbuf_desc.data.ptr == vx);
        T(vbuf_desc.data.size == 24 * sizeof(sshape_vertex_t));
        T(ibuf_desc.size == 0);
        T(ibuf_desc.type == SG_BUFFERTYPE_INDEXBUFFER);
        T(ibuf_desc.usage == SG_USAGE_IMMUTABLE);
        T(ibuf_desc.data.ptr == ix);
        T(ibuf_desc.data.size == 36 * sizeof(uint16_t));
        T(elm_range.base_element == 0);
        T(elm_range.num_elements == 36);
    }

    // append a plane...
    {
        buf = sshape_build_plane(&buf, &(sshape_plane_t) {0});
        const sg_buffer_desc vbuf_desc = sshape_vertex_buffer_desc(&buf);
        const sg_buffer_desc ibuf_desc = sshape_index_buffer_desc(&buf);
        const sshape_element_range_t elm_range = sshape_element_range(&buf);
        T(vbuf_desc.size == 0);
        T(vbuf_desc.type == SG_BUFFERTYPE_VERTEXBUFFER);
        T(vbuf_desc.usage == SG_USAGE_IMMUTABLE);
        T(vbuf_desc.data.ptr == vx);
        T(vbuf_desc.data.size == 28 * sizeof(sshape_vertex_t));
        T(ibuf_desc.size == 0);
        T(ibuf_desc.type == SG_BUFFERTYPE_INDEXBUFFER);
        T(ibuf_desc.usage == SG_USAGE_IMMUTABLE);
        T(ibuf_desc.data.ptr == ix);
        T(ibuf_desc.data.size == 42 * sizeof(uint16_t));
        T(elm_range.base_element == 36);
        T(elm_range.num_elements == 6);
    }
}

UTEST(sokol_shape, build_plane_defaults) {
    sshape_vertex_t vx[64] = { 0 };
    uint16_t ix[64] = { 0 };

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    buf = sshape_build_plane(&buf, &(sshape_plane_t) { 0 });

    T(buf.valid);
    T(0 == buf.vertices.shape_offset);
    T(4 * sizeof(sshape_vertex_t) == buf.vertices.data_size);
    T(0 == buf.indices.shape_offset);
    T(6 * sizeof(uint16_t) == buf.indices.data_size);
    for (int i = 0; i < 4; i++) {
        T(vx[i].color == 0xFFFFFFFF);
    }
    T(ix[0] == 0);
    T(ix[1] == 1);
    T(ix[2] == 3);
    T(ix[3] == 0);
    T(ix[4] == 3);
    T(ix[5] == 2);
}

UTEST(sokol_shape, build_plane_validate) {
    sshape_vertex_t vx[64] = { 0 };
    uint16_t ix[64] = { 0 };
    const sshape_plane_t params = { 0 };

    // vertex buffer too small
    {
        sshape_buffer_t buf = {
            .vertices.buffer = { .ptr = vx, .size = 3 * sizeof(sshape_vertex_t) },
            .indices.buffer  = SSHAPE_RANGE(ix),
        };
        T(!sshape_build_plane(&buf, &params).valid);
    }

    // index buffer too small
    {
        sshape_buffer_t buf = {
            .vertices.buffer = SSHAPE_RANGE(vx),
            .indices.buffer  = { .ptr = ix, .size = 5 * sizeof(uint16_t) }
        };
        T(!sshape_build_plane(&buf, &params).valid);
    }
    // just the right size
    {
        sshape_buffer_t buf = {
            .vertices.buffer = { .ptr = vx, .size = 4 * sizeof(sshape_vertex_t) },
            .indices.buffer  = { .ptr = ix, .size = 6 * sizeof(uint16_t) }
        };
        T(sshape_build_plane(&buf, &params).valid);
    }

    // too small for two planes
    {
        sshape_buffer_t buf = {
            .vertices.buffer = { .ptr = vx, .size = 5 * sizeof(sshape_vertex_t) },
            .indices.buffer  = { .ptr = ix, .size = 7 * sizeof(uint16_t) }
        };
        buf = sshape_build_plane(&buf, &params);
        T(buf.valid);
        buf = sshape_build_plane(&buf, &params);
        T(!buf.valid);
    }

    // just the right size for two planes
    {
        sshape_buffer_t buf = {
            .vertices.buffer = { .ptr = vx, .size = 8 * sizeof(sshape_vertex_t) },
            .indices.buffer  = { .ptr = ix, .size = 12 * sizeof(uint16_t) }
        };
        buf = sshape_build_plane(&buf, &params);
        T(buf.valid);
        T(buf.vertices.shape_offset == 0);
        T(buf.vertices.data_size == 4 * sizeof(sshape_vertex_t));
        T(buf.indices.shape_offset == 0);
        T(buf.indices.data_size == 6 * sizeof(uint16_t));
        buf = sshape_build_plane(&buf, &params);
        T(buf.valid);
        T(buf.vertices.shape_offset == 4 * sizeof(sshape_vertex_t));
        T(buf.vertices.data_size == 8 * sizeof(sshape_vertex_t));
        T(buf.indices.shape_offset == 6 * sizeof(uint16_t));
        T(buf.indices.data_size == 12 * sizeof(uint16_t));
    }
}

UTEST(sokol_shape, build_box_defaults) {
    sshape_vertex_t vx[128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    buf = sshape_build_box(&buf, &(sshape_box_t) { .color = 0xFF0000FF });
    T(buf.valid);
    T(buf.vertices.buffer.ptr == vx);
    T(buf.vertices.buffer.size == sizeof(vx));
    T(buf.indices.buffer.ptr == ix);
    T(buf.indices.buffer.size == sizeof(ix));
    T(buf.vertices.shape_offset == 0);
    T(buf.vertices.data_size == 24 * sizeof(sshape_vertex_t));
    T(buf.indices.shape_offset == 0);
    T(buf.indices.data_size == 36 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_sphere_defaults) {
    sshape_vertex_t vx[128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    buf = sshape_build_sphere(&buf, &(sshape_sphere_t) { .color = 0xFF0000FF });
    T(buf.valid);
    T(buf.vertices.buffer.ptr == vx);
    T(buf.vertices.buffer.size == sizeof(vx));
    T(buf.indices.buffer.ptr == ix);
    T(buf.indices.buffer.size == sizeof(ix));
    T(buf.vertices.shape_offset == 0);
    T(buf.vertices.data_size == 30 * sizeof(sshape_vertex_t));
    T(buf.indices.shape_offset == 0);
    T(buf.indices.data_size == 90 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_cylinder_defaults) {
    sshape_vertex_t vx[128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix)
    };
    buf = sshape_build_cylinder(&buf, &(sshape_cylinder_t) { .color = 0xFF0000FF });
    T(buf.valid);
    T(buf.vertices.buffer.ptr == vx);
    T(buf.vertices.buffer.size == sizeof(vx));
    T(buf.indices.buffer.ptr == ix);
    T(buf.indices.buffer.size == sizeof(ix));
    T(buf.vertices.shape_offset == 0);
    T(buf.vertices.data_size == 36 * sizeof(sshape_vertex_t));
    T(buf.indices.shape_offset == 0);
    T(buf.indices.data_size == 60 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_torus_defaults) {
    sshape_vertex_t vx[128] = { 0 };
    uint16_t ix[256] = { 0 };

    sshape_buffer_t buf = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    buf = sshape_build_torus(&buf, &(sshape_torus_t) { .color = 0xFF0000FF });
    T(buf.valid);
    T(buf.vertices.buffer.ptr == vx);
    T(buf.vertices.buffer.size == sizeof(vx));
    T(buf.indices.buffer.ptr == ix);
    T(buf.indices.buffer.size == sizeof(ix));
    T(buf.vertices.shape_offset == 0);
    T(buf.vertices.data_size == 36 * sizeof(sshape_vertex_t));
    T(buf.indices.shape_offset == 0);
    T(buf.indices.data_size == 150 * sizeof(uint16_t));
}
