//------------------------------------------------------------------------------
//  1-4-2-quad
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "transparency.glsl.h"

#define N_CUBES 2

typedef struct cube_desc {
    hmm_vec3 position;
    hmm_vec3 color;
} cube_desc;

cube_desc Make_cube(const hmm_vec3 position, const hmm_vec3 color)
{
    cube_desc res;
    res.position = position;
    res.color = color;

    return res;
}

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    cube_desc cube_positions[N_CUBES];
    lopgl_camera_state camera_state;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });

    /* initialize sokol_time */
    stm_setup();

    // hide mouse cursor
    sapp_show_mouse(false);

    // set default camera configuration
    lopgl_camera_state camera_default = { .camera_pos = HMM_Vec3(0.0f, 0.0f,  3.0f),
        .camera_front = HMM_Vec3(0.0f, 0.0f, -1.0f),
        .camera_up = HMM_Vec3(0.0f, 1.0f,  0.0f),
        .first_mouse = true,
        .fov = 45.0f,
        .yaw = -90.0f
    };
    state.camera_state = camera_default;

    state.cube_positions[0] = (cube_desc){ HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(1.0f, 0.0f, 0.0f) };
    state.cube_positions[1] = Make_cube(HMM_Vec3(0.5f, 0.5f, 1.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc());

    /* a vertex buffer with 4 vertices */
    float vertices[] = {
        // positions
        0.5f,  0.5f, 0.0f,      // top right
        0.5f, -0.5f, 0.0f,      // bottom right
        -0.5f, -0.5f, 0.0f,     // bottom left
        -0.5f,  0.5f, 0.0f      // top left
    };
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "quad-vertices"
    });

    /* an index buffer with 2 triangles */
    uint16_t indices[] = {
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
    };
    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .content = indices,
        .label = "quad-indices"
    });

    /* a pipeline state object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_rgb = SG_BLENDOP_ADD,
            .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .op_alpha = SG_BLENDOP_ADD
        },
        .label = "quad-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.0f, 0.0f, 0.0f, 1.0f} }
    };
}

void frame(void) {
    state.camera_state.delta_time = stm_laptime(&state.camera_state.last_time);
    hmm_mat4 view = lopgl_camera_view(&state.camera_state);
    hmm_mat4 projection = lopgl_camera_perspective(&state.camera_state, (float)sapp_width() / (float)sapp_height());

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    vs_params_t vs_params = {
    .view = view,
    .projection = projection
    };

    for (size_t i = 0; i < N_CUBES; i++)
    {
        vs_params.model = HMM_Translate(state.cube_positions[i].position);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
        fs_params_t fs_params = { .color = state.cube_positions[i].color };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));
        sg_draw(0, 6, 1);
    }

    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }

        if (e->key_code == SAPP_KEYCODE_SPACE) {
            bool mouse_shown = sapp_mouse_shown();
            sapp_show_mouse(!mouse_shown);
        }

        const float time = (float)stm_sec(state.camera_state.delta_time);
        const lopgl_camera_buttons button = sg_to_lopgl_button(e->key_code);
        lopgl_camera_button(&state.camera_state, time, button);
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        lopgl_camera_mouse_move(&state.camera_state, e->mouse_x, e->mouse_y);
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        lopgl_camera_scroll(&state.camera_state, e->scroll_y);
    }
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .gl_force_gles2 = true,
        .window_title = "Quad - LearnOpenGL",
    };
}
