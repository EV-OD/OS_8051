#include "firmware.h"
#include "gpu_controller.h"
#include "keyboard_controller.h"


#define SCREEN_W 240u
#define SCREEN_H 128u

#define RECT_W 6u
#define RECT_H 6u
#define STEP   5u

/* Static maze obstacles (bigger shapes than the player block) */
#define MAX_OBS      8u
#define OBS_THICK    6u

/* Obstacles use size indices -> flash lookup tables (saves RAM). */
static const unsigned char __code OBS_W_TBL[] = { 14u, 20u, 28u, 36u, 44u, 52u, 60u, 68u };
static const unsigned char __code OBS_H_TBL[] = { 10u, 14u, 18u, 22u, 26u, 30u, 34u, 38u };

/* xorshift8 step (no div/mod helpers) */
#define RNG_STEP() do { \
    rng ^= (unsigned char)(rng << 3); \
    rng ^= (unsigned char)(rng >> 5); \
    rng ^= (unsigned char)(rng << 1); \
} while (0)

void main(void)
{
    m_entry();
    STB = 1;
    keyboard_init();

    {
        unsigned char x = 50u;
        unsigned char y = 50u;

        /* Obstacles are stored compactly:
         * obs_p bits:
         *   [2:0]  w_idx (0..7) -> OBS_W_TBL
         *   [5:3]  h_idx (0..7) -> OBS_H_TBL
         *   [7:6]  type  (0=block,1=L,2=T)
         */
        unsigned char obs_x[MAX_OBS];
        unsigned char obs_y[MAX_OBS];
        unsigned char obs_p[MAX_OBS];

        /* Pseudo RNG (xorshift8). Timing between keypresses changes output. */
        unsigned char rng = 0xA5u;

        unsigned char draw_page = 1u;
        unsigned char page_x[2] = { 50u, 50u };
        unsigned char page_y[2] = { 50u, 50u };

        /* Scratch (keep local count small to fit DSEG) */
        unsigned char i;
        unsigned char tries;
        unsigned char w;
        unsigned char h;
        unsigned char ox;
        unsigned char oy;
        unsigned char r;
        unsigned char j;
        unsigned char type;
        unsigned char w_idx;
        unsigned char h_idx;
        unsigned char maxv;
        unsigned char px2;
        unsigned char py2;
        unsigned char ox2;
        unsigned char oy2;
        __bit overlap;
        __bit hit;

        const unsigned char max_x = (unsigned char)(SCREEN_W - (unsigned char)RECT_W);
        const unsigned char max_y = (unsigned char)(SCREEN_H - (unsigned char)RECT_H);

RESET_GAME:
        /* Player start */
        x = 50u;
        y = 50u;
        if (x > max_x) x = max_x;
        if (y > max_y) y = max_y;

        page_x[0] = x; page_x[1] = x;
        page_y[0] = y; page_y[1] = y;
        draw_page = 1u;

        saved_key = 0x00u;

        /* Stir RNG a bit so each reset differs */
        rng ^= x;
        rng ^= (unsigned char)(y << 1);
        rng ^= (unsigned char)(page_x[0] + 0x3Du);
        rng ^= (unsigned char)(page_y[0] + 0x7Bu);

        /* Generate obstacles (non-overlapping with player start and each other). */
        px2 = (unsigned char)(x + (unsigned char)RECT_W - 1u);
        py2 = (unsigned char)(y + (unsigned char)RECT_H - 1u);
        for (i = 0u; i < (unsigned char)MAX_OBS; i++)
        {
            obs_p[i] = 0u;

            for (tries = 0u; tries < 40u; tries++)
            {
                /* type */
                RNG_STEP();
                type = (unsigned char)(rng & 0x03u);
                if (type > 2u) type = 2u;

                /* w/h indices */
                RNG_STEP();
                w_idx = (unsigned char)(rng & 0x07u);
                RNG_STEP();
                h_idx = (unsigned char)(rng & 0x07u);

                w = OBS_W_TBL[w_idx];
                h = OBS_H_TBL[h_idx];

                /* Keep within screen and ensure thickness fits */
                if (w > (unsigned char)SCREEN_W) w = (unsigned char)SCREEN_W;
                if (h > (unsigned char)SCREEN_H) h = (unsigned char)SCREEN_H;
                if (w < (unsigned char)(OBS_THICK + 4u)) w = (unsigned char)(OBS_THICK + 4u);
                if (h < (unsigned char)(OBS_THICK + 4u)) h = (unsigned char)(OBS_THICK + 4u);

                /* ox in [0..(SCREEN_W-w)] via rejection (cheap + RAM friendly) */
                maxv = (unsigned char)(SCREEN_W - w);
                do {
                    RNG_STEP();
                    r = rng;
                } while (r > maxv);
                ox = r;

                /* oy in [0..(SCREEN_H-h)] via rejection */
                maxv = (unsigned char)(SCREEN_H - h);
                do {
                    RNG_STEP();
                    r = rng;
                } while (r > maxv);
                oy = r;

                /* Overlap check vs player start (touch counts as hit). */
                ox2 = (unsigned char)(ox + w - 1u);
                oy2 = (unsigned char)(oy + h - 1u);
                overlap = !((px2 < ox) || (ox2 < x) || (py2 < oy) || (oy2 < y));
                if (overlap) continue;

                /* Overlap check vs previous obstacles (bounding box). */
                for (j = 0u; j < i; j++)
                {
                    if (obs_p[j] == 0u) continue;

                    {
                        unsigned char bw_idx = (unsigned char)(obs_p[j] & 0x07u);
                        unsigned char bh_idx = (unsigned char)((obs_p[j] >> 3) & 0x07u);
                        unsigned char bw = OBS_W_TBL[bw_idx];
                        unsigned char bh = OBS_H_TBL[bh_idx];

                        unsigned char bx1 = obs_x[j];
                        unsigned char by1 = obs_y[j];
                        unsigned char bx2 = (unsigned char)(bx1 + bw - 1u);
                        unsigned char by2 = (unsigned char)(by1 + bh - 1u);

                        overlap = !((ox2 < bx1) || (bx2 < ox) || (oy2 < by1) || (by2 < oy));
                    }

                    if (overlap) break;
                }
                if (overlap) continue;

                obs_x[i] = ox;
                obs_y[i] = oy;
                obs_p[i] = (unsigned char)((type << 6) | (h_idx << 3) | (w_idx));
                break;
            }
        }

        /* Draw obstacles + player on BOTH pages */
        for (tries = 0u; tries < 2u; tries++)
        {
            gpu_set_draw_page(tries);
            gpu_clear_draw_page();

            for (i = 0u; i < (unsigned char)MAX_OBS; i++)
            {
                if (obs_p[i] == 0u) continue;

                ox = obs_x[i];
                oy = obs_y[i];
                w_idx = (unsigned char)(obs_p[i] & 0x07u);
                h_idx = (unsigned char)((obs_p[i] >> 3) & 0x07u);
                type = (unsigned char)(obs_p[i] >> 6);
                w = OBS_W_TBL[w_idx];
                h = OBS_H_TBL[h_idx];

                /* Draw obstacle shape */
                if (type == 0u)
                {
                    /* solid block */
                    gpu_fill_rect(ox, oy, w, h);
                }
                else if (type == 1u)
                {
                    /* L-shape */
                    gpu_fill_rect(ox, oy, w, (unsigned char)OBS_THICK);
                    gpu_fill_rect(ox, oy, (unsigned char)OBS_THICK, h);
                }
                else
                {
                    /* T-shape */
                    unsigned char stem_x;
                    unsigned char half;
                    half = (unsigned char)(w >> 1);
                    stem_x = (unsigned char)(ox + half);
                    if (stem_x >= (unsigned char)(OBS_THICK >> 1)) stem_x = (unsigned char)(stem_x - (unsigned char)(OBS_THICK >> 1));
                    else stem_x = ox;

                    gpu_fill_rect(ox, oy, w, (unsigned char)OBS_THICK);
                    gpu_fill_rect(stem_x, oy, (unsigned char)OBS_THICK, h);
                }
            }

            gpu_fill_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);
        }

        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);

        for (;;)
        {
            __bit moved = 0;
            unsigned char old_x = page_x[draw_page];
            unsigned char old_y = page_y[draw_page];
            KEY_STATE key = isKeyPressed();

            /* keep RNG moving; timing between key presses changes layout */
            RNG_STEP();

            switch (key) {
                case UP:
                    if (y >= (unsigned char)STEP) y = (unsigned char)(y - (unsigned char)STEP);
                    else y = 0u;
                    moved = 1;
                    break;
                case DOWN:
                    if (y <= (unsigned char)(max_y - (unsigned char)STEP)) y = (unsigned char)(y + (unsigned char)STEP);
                    else y = max_y;
                    moved = 1;
                    break;
                case LEFT:
                    if (x >= (unsigned char)STEP) x = (unsigned char)(x - (unsigned char)STEP);
                    else x = 0u;
                    moved = 1;
                    break;
                case RIGHT:
                    if (x <= (unsigned char)(max_x - (unsigned char)STEP)) x = (unsigned char)(x + (unsigned char)STEP);
                    else x = max_x;
                    moved = 1;
                    break;
                default:
                    break;
            }

            if (moved) {
                /* Collision: touching a wall fails → reset game */
                hit = 0;
                px2 = (unsigned char)(x + (unsigned char)RECT_W - 1u);
                py2 = (unsigned char)(y + (unsigned char)RECT_H - 1u);
                for (i = 0u; i < (unsigned char)MAX_OBS; i++)
                {
                    if (obs_p[i] == 0u) continue;

                    ox = obs_x[i];
                    oy = obs_y[i];
                    w_idx = (unsigned char)(obs_p[i] & 0x07u);
                    h_idx = (unsigned char)((obs_p[i] >> 3) & 0x07u);
                    type = (unsigned char)(obs_p[i] >> 6);
                    w = OBS_W_TBL[w_idx];
                    h = OBS_H_TBL[h_idx];

                    /* collision against actual shape (components) */
                    if (type == 0u)
                    {
                        ox2 = (unsigned char)(ox + w - 1u);
                        oy2 = (unsigned char)(oy + h - 1u);
                        hit = !((px2 < ox) || (ox2 < x) || (py2 < oy) || (oy2 < y));
                    }
                    else if (type == 1u)
                    {
                        /* L-shape: top bar + left bar */
                        unsigned char ax2;
                        unsigned char ay2;

                        /* top bar */
                        ax2 = (unsigned char)(ox + w - 1u);
                        ay2 = (unsigned char)(oy + (unsigned char)OBS_THICK - 1u);
                        hit = !((px2 < ox) || (ax2 < x) || (py2 < oy) || (ay2 < y));

                        if (!hit)
                        {
                            /* left bar */
                            ax2 = (unsigned char)(ox + (unsigned char)OBS_THICK - 1u);
                            ay2 = (unsigned char)(oy + h - 1u);
                            hit = !((px2 < ox) || (ax2 < x) || (py2 < oy) || (ay2 < y));
                        }
                    }
                    else
                    {
                        /* T-shape: top bar + stem */
                        unsigned char stem_x;
                        unsigned char half;
                        unsigned char ax2;
                        unsigned char ay2;

                        /* top bar */
                        ax2 = (unsigned char)(ox + w - 1u);
                        ay2 = (unsigned char)(oy + (unsigned char)OBS_THICK - 1u);
                        hit = !((px2 < ox) || (ax2 < x) || (py2 < oy) || (ay2 < y));

                        if (!hit)
                        {
                            half = (unsigned char)(w >> 1);
                            stem_x = (unsigned char)(ox + half);
                            if (stem_x >= (unsigned char)(OBS_THICK >> 1)) stem_x = (unsigned char)(stem_x - (unsigned char)(OBS_THICK >> 1));
                            else stem_x = ox;

                            ax2 = (unsigned char)(stem_x + (unsigned char)OBS_THICK - 1u);
                            ay2 = (unsigned char)(oy + h - 1u);
                            hit = !((px2 < stem_x) || (ax2 < x) || (py2 < oy) || (ay2 < y));
                        }
                    }

                    if (hit) break;
                }

                if (hit) {
                    saved_key = 0x00u;
                    goto RESET_GAME;
                }

                gpu_clear_rect(old_x, old_y, (unsigned char)RECT_W, (unsigned char)RECT_H);
                gpu_fill_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);

                page_x[draw_page] = x;
                page_y[draw_page] = y;

                gpu_swap_pages();
                draw_page ^= 1u;
            }
        }
    }
}
