#include <azur/gint/render.h>
#include <math.h>
#include <stdio.h>

uint8_t AZRP_SHADER_TRIANGLE2 = -1;

static void configure(void)
{
    azrp_set_uniforms(AZRP_SHADER_TRIANGLE2, (void *)azrp_width);
}

__attribute__((constructor))
static void register_shader(void)
{
    extern azrp_shader_t azrp_shader_triangle2;
    AZRP_SHADER_TRIANGLE2 = azrp_register_shader(azrp_shader_triangle2,
        configure);
    configure();
}

static int min(int x, int y)
{
    return (x < y) ? x : y;
}
static int max(int x, int y)
{
    return (x > y) ? x : y;
}

//---

struct command {
   uint8_t shader_id;
   /* Local y coordinate of the first line in the fragment */
   uint8_t y;
   /* Number of lines to render total, including this fragment */
   uint8_t height_total;
   /* Number of lines to render on the current fragment */
   uint8_t height_frag;
   /* Rectangle along the x coordinates (x_max included) */
   uint16_t x_min, x_max;
   /* Color */
   uint16_t color;
   uint16_t _;

   /* Initial barycentric coordinates */
   int u0, v0, w0;
   /* Variation of each coordinate for a movement in x */
   int du_x, dv_x, dw_x;
   /* Variation of each coordinate for a movement in y while canceling rows'
      movements in x */
   int du_row, dv_row, dw_row;

   uint8_t* texture;
   color_t* palette;
};

static int edge_start(int x1, int y1, int x2, int y2, int px, int py)
{
    return (y2 - y1) * (px - x1) - (x2 - x1) * (py - y1);
}
void azrp_triangle2(int x1, int y1, int x2, int y2, int x3, int y3, int color, uint8_t* texture, color_t* palette)
{
    prof_enter(azrp_perf_cmdgen);

    /* Find a rectangle containing the triangle */
    int min_x = max(0, min(x1, min(x2, x3)));
    int max_x = min(azrp_width-1, max(x1, max(x2, x3)));
    int min_y = max(0, min(y1, min(y2, y3)));
    int max_y = min(azrp_height-1, max(y1, max(y2, y3)));

    if(min_x > max_x || min_y > max_y) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    int frag_first, first_offset, frag_count;
    azrp_config_get_lines(min_y, max_y - min_y + 1,
        &frag_first, &first_offset, &frag_count);

    struct command *cmd =
        azrp_new_command(sizeof *cmd, frag_first, frag_count);
    if(!cmd) {
        prof_leave(azrp_perf_cmdgen);
        return;
    }

    cmd->shader_id = AZRP_SHADER_TRIANGLE2;
    cmd->y = first_offset;
    cmd->height_total = max_y - min_y + 1;
    cmd->height_frag = min(cmd->height_total, azrp_frag_height - cmd->y);
    cmd->x_min = min_x;
    cmd->x_max = max_x;
    cmd->color = color;

    /* Swap points 1 and 2 if the order of points is not left-handed */
    if(edge_start(x1, y1, x2, y2, x3, y3) < 0) {
        int xt = x1;
        x1 = x2;
        x2 = xt;
        int yt = y1;
        y1 = y2;
        y2 = yt;
    }

    int norm_factor;
    if (edge_start(x1, y1, x2, y2, x3, y3) == 0) {
        norm_factor = 1;
    } else {
        norm_factor = 65536 * 16 / edge_start(x1, y1, x2, y2, x3, y3);
    }

    /* Vector products for barycentric coordinates */
    /* We only need to normalize u and w, as v is not used for texture mapping */
    cmd->u0 = edge_start(x2, y2, x3, y3, min_x, min_y) * norm_factor;
    cmd->du_x = (y3 - y2) * norm_factor;
    int du_y = (x2 - x3) * norm_factor;

    cmd->v0 = edge_start(x3, y3, x1, y1, min_x, min_y);
    cmd->dv_x = y1 - y3;
    int dv_y = x3 - x1;

    cmd->w0 = edge_start(x1, y1, x2, y2, min_x, min_y) * norm_factor;
    cmd->dw_x = (y2 - y1) * norm_factor;
    int dw_y = (x1 - x2) * norm_factor;

    int columns = max_x - min_x + 1;
    cmd->du_row = du_y - columns * cmd->du_x;
    cmd->dv_row = dv_y - columns * cmd->dv_x;
    cmd->dw_row = dw_y - columns * cmd->dw_x;

    cmd->texture = texture;
    cmd->palette = palette + 128;

    // if (color) {
    //     // cmd->u0 = 65536 * 16 - cmd->u0;
    //     // cmd->w0 = 65536 * 16 - cmd->u0;
    // }

    // printf("(%d, %d, %d), (%d, %d, %d), (%d, %d, %d)\n", cmd->u0, cmd->v0,cmd-> w0, cmd->du_x, cmd->dv_x, cmd->dw_x, cmd->du_row, cmd->dv_row, cmd->dw_row);

    prof_leave(azrp_perf_cmdgen);
}
