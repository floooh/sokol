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

    res = sshape_plane_sizes(1, SSHAPE_MAX_VERTEX_SIZE);
    T(4 == res.vertices.num);
    T(6 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_plane_sizes(2, SSHAPE_MAX_VERTEX_SIZE);
    T(9 == res.vertices.num);
    T(24 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE== res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, box_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_box_sizes(1, SSHAPE_MAX_VERTEX_SIZE);
    T(24 == res.vertices.num);
    T(36 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_box_sizes(2, SSHAPE_MAX_VERTEX_SIZE);
    T(54 == res.vertices.num);
    T(144 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, sphere_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_sphere_sizes(3, 2, SSHAPE_MAX_VERTEX_SIZE);
    T(12 == res.vertices.num);
    T(18 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_sphere_sizes(36, 12, SSHAPE_MAX_VERTEX_SIZE);
    T(481 ==  res.vertices.num);
    T(2376 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE== res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, cylinder_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_cylinder_sizes(3, 1, SSHAPE_MAX_VERTEX_SIZE);
    T(24 == res.vertices.num);
    T(36 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_cylinder_sizes(5, 2, SSHAPE_MAX_VERTEX_SIZE);
    T(42 == res.vertices.num);
    T(90 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, torus_buffer_sizes) {
    sshape_sizes_t res;

    res = sshape_torus_sizes(3, 3, SSHAPE_MAX_VERTEX_SIZE);
    T(16 == res.vertices.num);
    T(54 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);

    res = sshape_torus_sizes(4, 5, SSHAPE_MAX_VERTEX_SIZE);
    T(30 == res.vertices.num);
    T(120 == res.indices.num);
    T(res.vertices.num * SSHAPE_MAX_VERTEX_SIZE == res.vertices.size);
    T(res.indices.num * sizeof(uint16_t) == res.indices.size);
}

UTEST(sokol_shape, buffer_layout_desc) {
    sshape_state_t shp = { .valid = true };
    const sg_vertex_buffer_layout_state l_state = sshape_vertex_buffer_layout_state(&shp);
    T(SSHAPE_MAX_VERTEX_SIZE == l_state.stride);
    T(0 == l_state.step_func);
    T(0 == l_state.step_rate);
}

UTEST(sokol_shape, attr_descs) {
    sshape_state_t shp = { .valid = true };
    {
        const sg_vertex_attr_state a_state = sshape_position_vertex_attr_state(&shp);
        T(0 == a_state.offset);
        T(SG_VERTEXFORMAT_FLOAT3 == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_normal_vertex_attr_state(&shp);
        T(12 == a_state.offset);
        T(SG_VERTEXFORMAT_BYTE4N == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_texcoord_vertex_attr_state(&shp);
        T(16 == a_state.offset);
        T(SG_VERTEXFORMAT_USHORT2N == a_state.format);
        T(0 == a_state.buffer_index);
    }
    {
        const sg_vertex_attr_state a_state = sshape_color_vertex_attr_state(&shp);
        T(20 == a_state.offset);
        T(SG_VERTEXFORMAT_UBYTE4N == a_state.format);
        T(0 == a_state.buffer_index);
    }
}

UTEST(sokol_shape, buffer_descs_elm_range) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 128] = { 0 };
    uint16_t ix[128] = { 0 };
    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer = SSHAPE_RANGE(ix),
    };

    // build a box...
    {
        sshape_build_box(&shp, &(sshape_box_t) {0});
        const sg_buffer_desc vbuf_desc = sshape_vertex_buffer_desc(&shp);
        const sg_buffer_desc ibuf_desc = sshape_index_buffer_desc(&shp);
        const sshape_element_range_t elm_range = sshape_element_range(&shp);
        T(vbuf_desc.size == 0);
        T(vbuf_desc.usage.vertex_buffer);
        T(vbuf_desc.usage.immutable);
        T(vbuf_desc.data.ptr == vx);
        T(vbuf_desc.data.size == 24 * SSHAPE_MAX_VERTEX_SIZE);
        T(ibuf_desc.size == 0);
        T(ibuf_desc.usage.index_buffer);
        T(ibuf_desc.usage.immutable);
        T(ibuf_desc.data.ptr == ix);
        T(ibuf_desc.data.size == 36 * sizeof(uint16_t));
        T(elm_range.base_element == 0);
        T(elm_range.num_elements == 36);
    }

    // append a plane...
    {
        sshape_build_plane(&shp, &(sshape_plane_t) {0});
        const sg_buffer_desc vbuf_desc = sshape_vertex_buffer_desc(&shp);
        const sg_buffer_desc ibuf_desc = sshape_index_buffer_desc(&shp);
        const sshape_element_range_t elm_range = sshape_element_range(&shp);
        T(vbuf_desc.size == 0);
        T(vbuf_desc.usage.vertex_buffer);
        T(vbuf_desc.usage.immutable);
        T(vbuf_desc.data.ptr == vx);
        T(vbuf_desc.data.size == 28 * SSHAPE_MAX_VERTEX_SIZE);
        T(ibuf_desc.size == 0);
        T(ibuf_desc.usage.index_buffer);
        T(ibuf_desc.usage.immutable);
        T(ibuf_desc.data.ptr == ix);
        T(ibuf_desc.data.size == 42 * sizeof(uint16_t));
        T(elm_range.base_element == 36);
        T(elm_range.num_elements == 6);
    }
}

UTEST(sokol_shape, build_plane_defaults) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 64] = { 0 };
    uint16_t ix[64] = { 0 };

    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    sshape_build_plane(&shp, &(sshape_plane_t) { 0 });

    T(shp.valid);
    T(0 == shp.vertices.shape_offset);
    T(4 * SSHAPE_MAX_VERTEX_SIZE == shp.vertices.data_size);
    T(0 == shp.indices.shape_offset);
    T(6 * sizeof(uint16_t) == shp.indices.data_size);
    for (int i = 0; i < 4; i++) {
        uint32_t* c_ptr = (uint32_t*)&(vx[i * SSHAPE_MAX_VERTEX_SIZE + 20]);
        const uint32_t c = *c_ptr;
        T(c == 0xFFFFFFFF);
    }
    T(ix[0] == 0);
    T(ix[1] == 1);
    T(ix[2] == 3);
    T(ix[3] == 0);
    T(ix[4] == 3);
    T(ix[5] == 2);
}

UTEST(sokol_shape, build_plane_validate) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 64] = { 0 };
    uint16_t ix[64] = { 0 };
    const sshape_plane_t params = { 0 };

    // vertex buffer too small
    {
        sshape_state_t shp = {
            .vertices.buffer = { .ptr = vx, .size = 3 * SSHAPE_MAX_VERTEX_SIZE },
            .indices.buffer  = SSHAPE_RANGE(ix),
        };
        sshape_build_plane(&shp, &params);
        T(!shp.valid);
    }

    // index buffer too small
    {
        sshape_state_t shp = {
            .vertices.buffer = SSHAPE_RANGE(vx),
            .indices.buffer  = { .ptr = ix, .size = 5 * sizeof(uint16_t) }
        };
        sshape_build_plane(&shp, &params);
        T(!shp.valid);
    }
    // just the right size
    {
        sshape_state_t shp = {
            .vertices.buffer = { .ptr = vx, .size = 4 * SSHAPE_MAX_VERTEX_SIZE },
            .indices.buffer  = { .ptr = ix, .size = 6 * sizeof(uint16_t) }
        };
        sshape_build_plane(&shp, &params);
        T(shp.valid);
    }

    // too small for two planes
    {
        sshape_state_t shp = {
            .vertices.buffer = { .ptr = vx, .size = 5 * SSHAPE_MAX_VERTEX_SIZE },
            .indices.buffer  = { .ptr = ix, .size = 7 * sizeof(uint16_t) }
        };
        sshape_build_plane(&shp, &params);
        T(shp.valid);
        sshape_build_plane(&shp, &params);
        T(!shp.valid);
    }

    // just the right size for two planes
    {
        sshape_state_t shp = {
            .vertices.buffer = { .ptr = vx, .size = 8 * SSHAPE_MAX_VERTEX_SIZE },
            .indices.buffer  = { .ptr = ix, .size = 12 * sizeof(uint16_t) }
        };
        sshape_build_plane(&shp, &params);
        T(shp.valid);
        T(shp.vertices.shape_offset == 0);
        T(shp.vertices.data_size == 4 * SSHAPE_MAX_VERTEX_SIZE);
        T(shp.indices.shape_offset == 0);
        T(shp.indices.data_size == 6 * sizeof(uint16_t));
        sshape_build_plane(&shp, &params);
        T(shp.valid);
        T(shp.vertices.shape_offset == 4 * SSHAPE_MAX_VERTEX_SIZE);
        T(shp.vertices.data_size == 8 * SSHAPE_MAX_VERTEX_SIZE);
        T(shp.indices.shape_offset == 6 * sizeof(uint16_t));
        T(shp.indices.data_size == 12 * sizeof(uint16_t));
    }
}

UTEST(sokol_shape, build_box_defaults) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    sshape_build_box(&shp, &(sshape_box_t) { .color = 0xFF0000FF });
    T(shp.valid);
    T(shp.vertices.buffer.ptr == vx);
    T(shp.vertices.buffer.size == sizeof(vx));
    T(shp.indices.buffer.ptr == ix);
    T(shp.indices.buffer.size == sizeof(ix));
    T(shp.vertices.shape_offset == 0);
    T(shp.vertices.data_size == 24 * SSHAPE_MAX_VERTEX_SIZE);
    T(shp.indices.shape_offset == 0);
    T(shp.indices.data_size == 36 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_sphere_defaults) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    sshape_build_sphere(&shp, &(sshape_sphere_t) { .color = 0xFF0000FF });
    T(shp.valid);
    T(shp.vertices.buffer.ptr == vx);
    T(shp.vertices.buffer.size == sizeof(vx));
    T(shp.indices.buffer.ptr == ix);
    T(shp.indices.buffer.size == sizeof(ix));
    T(shp.vertices.shape_offset == 0);
    T(shp.vertices.data_size == 30 * SSHAPE_MAX_VERTEX_SIZE);
    T(shp.indices.shape_offset == 0);
    T(shp.indices.data_size == 90 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_cylinder_defaults) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 128] = { 0 };
    uint16_t ix[128] = { 0 };

    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix)
    };
    sshape_build_cylinder(&shp, &(sshape_cylinder_t) { .color = 0xFF0000FF });
    T(shp.valid);
    T(shp.vertices.buffer.ptr == vx);
    T(shp.vertices.buffer.size == sizeof(vx));
    T(shp.indices.buffer.ptr == ix);
    T(shp.indices.buffer.size == sizeof(ix));
    T(shp.vertices.shape_offset == 0);
    T(shp.vertices.data_size == 36 * SSHAPE_MAX_VERTEX_SIZE);
    T(shp.indices.shape_offset == 0);
    T(shp.indices.data_size == 60 * sizeof(uint16_t));
}

UTEST(sokol_shape, build_torus_defaults) {
    uint8_t vx[SSHAPE_MAX_VERTEX_SIZE * 128] = { 0 };
    uint16_t ix[256] = { 0 };

    sshape_state_t shp = {
        .vertices.buffer = SSHAPE_RANGE(vx),
        .indices.buffer  = SSHAPE_RANGE(ix),
    };
    sshape_build_torus(&shp, &(sshape_torus_t) { .color = 0xFF0000FF });
    T(shp.valid);
    T(shp.vertices.buffer.ptr == vx);
    T(shp.vertices.buffer.size == sizeof(vx));
    T(shp.indices.buffer.ptr == ix);
    T(shp.indices.buffer.size == sizeof(ix));
    T(shp.vertices.shape_offset == 0);
    T(shp.vertices.data_size == 36 * SSHAPE_MAX_VERTEX_SIZE);
    T(shp.indices.shape_offset == 0);
    T(shp.indices.data_size == 150 * sizeof(uint16_t));
}
