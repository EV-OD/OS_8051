#include "firmware.h"
#include "gpu_controller.h"
#include "keyboard_controller.h"


#define SCREEN_W 240u
#define SCREEN_H 128u

#define RECT_W 6u
#define RECT_H 6u
#define STEP   5u

/* Maze player is a hollow rectangle with a thicker stroke.
 * Thickness is implemented by drawing nested outline rects.
 */
#define PLAYER_STROKE 2u

/* UI text overlay rows (T6963C text plane) */
#define UI_ROW_TITLE  6u
#define UI_ROW_BTN    8u
#define UI_ROW_HINT   10u

/* 8x8 font -> 30 columns; blank row used for clearing UI lines */
static const __code char UI_BLANK_ROW[] = "                              ";

static void ui_puts(unsigned char col, unsigned char row, const __code char *s) __reentrant
{
    unsigned char ch;

    gpu_write(CMD_TEXT);
    gpu_write(col);
    gpu_write(row);

    if (s)
    {
        while (1)
        {
            ch = (unsigned char)(*s++);
            if (ch == 0x00u) break;
            gpu_write(ch);
        }
    }

    gpu_write(0x00u);
}

static void ui_clear_line(unsigned char row) __reentrant
{
    ui_puts(0u, row, UI_BLANK_ROW);
}

static void ui_clear_overlay(void)
{
    ui_clear_line(UI_ROW_TITLE);
    ui_clear_line(UI_ROW_BTN);
    ui_clear_line(UI_ROW_HINT);
}

static void ui_clear_all_text(void)
{
    unsigned char row;
    for (row = 0u; row < 16u; row++)
        ui_puts(0u, row, UI_BLANK_ROW);
}

static void gfx_clear_both_pages(void)
{
    gpu_set_draw_page(0u);
    gpu_clear_draw_page();
    gpu_set_draw_page(1u);
    gpu_clear_draw_page();
}

static void ui_show_menu(void)
{
    ui_clear_all_text();
    ui_puts(7u, 1u, "GAME SELECT");
    ui_puts(0u, 4u, "UP    MAZE RUNNER");
    ui_puts(0u, 6u, "LEFT  ROGUE CRAWLER");
    ui_puts(0u, 8u, "DOWN  GAME OF LIFE");
    ui_puts(0u, 10u, "RIGHT VIRTUAL PET");
}

/* ─────────────────────────────────────────────────────────────
 * Rogue-like (text-only) map stored in code memory.
 * Map size: 20x12, tiles: '#' wall, '.' floor.
 * ───────────────────────────────────────────────────────────── */
#define ROGUE_W 20u
#define ROGUE_H 12u
static const __code char ROGUE_MAP[] =
    "####################"
    "#..............#...#"
    "#..####.........#..#"
    "#..#..#..####...#..#"
    "#..#..#.........#..#"
    "#..####.........#..#"
    "#...............#..#"
    "#..######..........#"
    "#.............######"
    "#..................#"
    "#..##############..#"
    "####################";

/* ─────────────────────────────────────────────────────────────
 * Conway's Game of Life (tiny 8x8) using graphics pages.
 * Uses key combos:
 *   UP+DOWN: toggle cell under cursor
 *   LEFT+RIGHT: start/pause simulation
 * ───────────────────────────────────────────────────────────── */
#define LIFE_W 8u
#define LIFE_H 8u
#define LIFE_CELL 8u
#define LIFE_X0 88u
#define LIFE_Y0 16u

/* ─────────────────────────────────────────────────────────────
 * Virtual Pet (slow animation via page toggling)
 * Buttons:
 *   LEFT/RIGHT: cycle menu item
 *   UP: action
 *   DOWN: status/back
 * ───────────────────────────────────────────────────────────── */
static void pet_draw_frames(void)
{
    /* page0 */
    gpu_set_draw_page(0u);
    gpu_clear_draw_page();
    gpu_fill_rect(100u, 40u, 40u, 40u); /* body */
    gpu_clear_rect(110u, 55u, 6u, 6u);  /* eye */
    gpu_clear_rect(124u, 55u, 6u, 6u);  /* eye */
    gpu_clear_rect(118u, 70u, 4u, 2u);  /* mouth */

    /* page1 (slightly different mouth -> "walk"/blink illusion) */
    gpu_set_draw_page(1u);
    gpu_clear_draw_page();
    gpu_fill_rect(100u, 40u, 40u, 40u);
    gpu_clear_rect(110u, 55u, 6u, 6u);
    gpu_clear_rect(124u, 55u, 6u, 6u);
    gpu_clear_rect(117u, 71u, 6u, 2u);
}

static void ui_show_game_over(void)
{
    ui_clear_all_text();
    ui_puts(7u, 6u, "GAME OVER");
    ui_puts(4u, 8u, "UP TO MENU");
}

static void ui_pet_menu(unsigned char sel) __reentrant
{
    /* One of 4 pre-baked lines (keeps RAM small). */
    switch (sel & 3u)
    {
        case 0u: ui_puts(0u, 2u, "[FEED] CLEAN  PLAY  STAT"); break;
        case 1u: ui_puts(0u, 2u, " FEED [CLEAN] PLAY  STAT"); break;
        case 2u: ui_puts(0u, 2u, " FEED  CLEAN [PLAY] STAT"); break;
        default: ui_puts(0u, 2u, " FEED  CLEAN  PLAY [STAT]"); break;
    }
}

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

MENU:
        saved_key = 0x00u;
        gfx_clear_both_pages();
        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);
        ui_show_menu();

        for (;;)
        {
            type = (unsigned char)isKeyPressed();
            RNG_STEP();
            if (type == (unsigned char)UP)    goto RUN_MAZE;
            if (type == (unsigned char)LEFT)  goto RUN_ROGUE;
            if (type == (unsigned char)DOWN)  goto RUN_LIFE;
            if (type == (unsigned char)RIGHT) goto RUN_PET;
        }

RUN_MAZE:
        /* Maze runner (graphics + page flip) */
        ui_clear_all_text();
        gfx_clear_both_pages();
        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);

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

            /* Player: thick outline, non-filled */
            gpu_draw_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);
#if (PLAYER_STROKE >= 2u)
            gpu_draw_rect((unsigned char)(x + 1u), (unsigned char)(y + 1u),
                          (unsigned char)(RECT_W - 2u), (unsigned char)(RECT_H - 2u));
#endif
        }

        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);

        for (;;)
        {
            __bit moved = 0;
            unsigned char old_x = page_x[draw_page];
            unsigned char old_y = page_y[draw_page];
            type = (unsigned char)isKeyPressed();

            /* keep RNG moving; timing between key presses changes layout */
            RNG_STEP();

            switch (type) {
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

                    /* Game over screen clears graphics and returns to menu. */
                    gfx_clear_both_pages();
                    ui_show_game_over();
                    for (;;)
                    {
                        type = (unsigned char)isKeyPressed();
                        RNG_STEP();
                        if (type == (unsigned char)UP)
                        {
                            saved_key = 0x00u;
                            break;
                        }
                    }
                    goto MENU;
                }

                gpu_clear_rect(old_x, old_y, (unsigned char)RECT_W, (unsigned char)RECT_H);
                /* Player: thick outline, non-filled */
                gpu_draw_rect(x, y, (unsigned char)RECT_W, (unsigned char)RECT_H);
#if (PLAYER_STROKE >= 2u)
                gpu_draw_rect((unsigned char)(x + 1u), (unsigned char)(y + 1u),
                              (unsigned char)(RECT_W - 2u), (unsigned char)(RECT_H - 2u));
#endif

                page_x[draw_page] = x;
                page_y[draw_page] = y;

                gpu_swap_pages();
                draw_page ^= 1u;
            }
        }

RUN_ROGUE:
        /* Rogue-like Dungeon Crawler (text-only). */
        gfx_clear_both_pages();
        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);
        ui_clear_all_text();

        /* Reuse x,y as tile coordinates (0..19,0..11) */
        x = 1u;
        y = 1u;
        ox = 15u;  /* enemy x */
        oy = 6u;   /* enemy y */
        hit = 1;   /* enemy_alive */

ROGUE_REDRAW:
        ui_clear_all_text();
        ui_puts(0u, 0u, "Rogue-like Dungeon");
        ui_puts(0u, 1u, "@ move  E battle");
        ui_puts(0u, 2u, "Screen updates on step");

        for (i = 0u; i < (unsigned char)ROGUE_H; i++)
        {
            gpu_write(CMD_TEXT);
            gpu_write(0u);
            gpu_write((unsigned char)(3u + i));

            for (j = 0u; j < (unsigned char)ROGUE_W; j++)
            {
                if (j == x && i == y)
                    r = (unsigned char)'@';
                else if (hit && j == ox && i == oy)
                    r = (unsigned char)'E';
                else
                {
                    /* idx = i*20 + j = (i<<4)+(i<<2)+j */
                    r = (unsigned char)((i << 4) + (i << 2) + j);
                    r = (unsigned char)ROGUE_MAP[r];
                }

                gpu_write(r);
            }
            gpu_write(0x00u);
        }

        for (;;)
        {
            type = (unsigned char)isKeyPressed();
            RNG_STEP();
            if (type == (unsigned char)KEY_NONE) continue;

            w = x;
            h = y;
            if (type == (unsigned char)UP) {
                if (h > 0u) h--;
            } else if (type == (unsigned char)DOWN) {
                if (h < (unsigned char)(ROGUE_H - 1u)) h++;
            } else if (type == (unsigned char)LEFT) {
                if (w > 0u) w--;
            } else if (type == (unsigned char)RIGHT) {
                if (w < (unsigned char)(ROGUE_W - 1u)) w++;
            }

            if (w == x && h == y) continue;

            /* wall check */
            r = (unsigned char)((h << 4) + (h << 2) + w);
            type = (unsigned char)ROGUE_MAP[r];
            if (type == (unsigned char)'#')
                continue;

            /* battle */
            if (hit && w == ox && h == oy)
            {
                ui_clear_all_text();
                ui_puts(0u, 4u, "Goblin appears!");
                ui_puts(0u, 6u, "UP   Fight");
                ui_puts(0u, 7u, "DOWN Run");

                for (;;)
                {
                    type = (unsigned char)isKeyPressed();
                    RNG_STEP();
                    if (type == (unsigned char)UP)
                    {
                        hit = 0;   /* enemy defeated */
                        x = w; y = h;
                        break;
                    }
                    if (type == (unsigned char)DOWN)
                    {
                        /* run away: stay in place */
                        break;
                    }
                }

                goto ROGUE_REDRAW;
            }

            x = w;
            y = h;
            goto ROGUE_REDRAW;
        }

RUN_LIFE:
        /* Conway's Game of Life: editor + run/pause.
         * Uses obs_x[0..7] as rows bitset, obs_y[0..7] as next.
         */
        gfx_clear_both_pages();
        ui_clear_all_text();
        ui_puts(0u, 0u, "Conway Life (8x8)");
        ui_puts(0u, 1u, "UP/DN move cursor");
        ui_puts(0u, 2u, "UP+DOWN toggle cell");
        ui_puts(0u, 3u, "LEFT+RIGHT run/pause");

        for (i = 0u; i < 8u; i++) { obs_x[i] = 0u; obs_y[i] = 0u; }
        x = 0u; y = 0u;           /* cursor cell */
        type = 0u;                /* running flag: 0 paused, 1 running */
        draw_page = 0u;           /* display page for cursor blink */
        tries = 0u;
        maxv = 0u;

        /* draw border on both pages */
        gpu_set_draw_page(0u);
        gpu_draw_rect((unsigned char)LIFE_X0, (unsigned char)LIFE_Y0,
                      (unsigned char)(LIFE_W * LIFE_CELL), (unsigned char)(LIFE_H * LIFE_CELL));
        gpu_set_draw_page(1u);
        gpu_draw_rect((unsigned char)LIFE_X0, (unsigned char)LIFE_Y0,
                      (unsigned char)(LIFE_W * LIFE_CELL), (unsigned char)(LIFE_H * LIFE_CELL));

        /* cursor only on page0, implemented by invert so we can remove cleanly */
        gpu_set_draw_page(0u);
        gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                        (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                        (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
        gpu_set_display_page(0u);

        for (;;)
        {
            /* raw mask to detect combos */
            keyboard_read_mask(r);
            RNG_STEP();

            if (r == 0u)
            {
                /* blink cursor by toggling displayed graphics page */
                tries++;
                if (tries == 0u)
                {
                    draw_page ^= 1u;
                    gpu_set_display_page(draw_page);
                }
                continue;
            }

            /* LEFT+RIGHT toggles run/pause */
            if ((r & (unsigned char)LEFT) && (r & (unsigned char)RIGHT))
            {
                if (type == 0u)
                {
                    /* remove cursor (page0), then run showing page1 */
                    gpu_set_draw_page(0u);
                    gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                    (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                    (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                    type = 1u;
                    gpu_set_display_page(1u);
                    draw_page = 1u;
                    maxv = 0u;
                }
                else
                {
                    /* pause: redraw cursor (page0) and show it */
                    type = 0u;
                    draw_page = 0u;
                    gpu_set_draw_page(0u);
                    gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                    (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                    (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                    gpu_set_display_page(0u);
                }

                continue;
            }

            if (type == 0u)
            {
                /* paused editor */
                if ((r & (unsigned char)UP) && (r & (unsigned char)DOWN))
                {
                    /* remove cursor effect, toggle cell, redraw cell on both pages, reapply cursor */
                    gpu_set_draw_page(0u);
                    gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                    (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                    (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);

                    obs_x[y] ^= (unsigned char)(1u << x);

                    /* cell pixels */
                    w = (unsigned char)(LIFE_X0 + (unsigned char)(x << 3));
                    h = (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3));

                    gpu_set_draw_page(0u);
                    if (obs_x[y] & (unsigned char)(1u << x)) gpu_fill_rect(w, h, (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                    else                                      gpu_clear_rect(w, h, (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);

                    gpu_set_draw_page(1u);
                    if (obs_x[y] & (unsigned char)(1u << x)) gpu_fill_rect(w, h, (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                    else                                      gpu_clear_rect(w, h, (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);

                    gpu_set_draw_page(0u);
                    gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                    (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                    (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                    gpu_set_display_page(0u);
                    draw_page = 0u;
                    continue;
                }

                /* move cursor (single direction) */
                gpu_set_draw_page(0u);
                gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);

                if ((r & (unsigned char)UP) && (y > 0u)) y--;
                else if ((r & (unsigned char)DOWN) && (y < 7u)) y++;
                else if ((r & (unsigned char)LEFT) && (x > 0u)) x--;
                else if ((r & (unsigned char)RIGHT) && (x < 7u)) x++;

                gpu_set_draw_page(0u);
                gpu_invert_rect((unsigned char)(LIFE_X0 + (unsigned char)(x << 3)),
                                (unsigned char)(LIFE_Y0 + (unsigned char)(y << 3)),
                                (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                gpu_set_display_page(0u);
                draw_page = 0u;
            }
            else
            {
                /* running simulation: step occasionally */
                maxv++;
                if (maxv != 0u) continue;

                /* next = 0 */
                for (i = 0u; i < 8u; i++) obs_y[i] = 0u;

                for (i = 0u; i < 8u; i++)
                {
                    for (j = 0u; j < 8u; j++)
                    {
                        r = 0u;
                        for (h = (unsigned char)((i == 0u) ? 0u : (i - 1u)); h <= (unsigned char)((i == 7u) ? 7u : (i + 1u)); h++)
                        {
                            for (w = (unsigned char)((j == 0u) ? 0u : (j - 1u)); w <= (unsigned char)((j == 7u) ? 7u : (j + 1u)); w++)
                            {
                                if (w == j && h == i) continue;
                                if (obs_x[h] & (unsigned char)(1u << w)) r++;
                            }
                        }

                        if (obs_x[i] & (unsigned char)(1u << j))
                        {
                            if (r == 2u || r == 3u) obs_y[i] |= (unsigned char)(1u << j);
                        }
                        else
                        {
                            if (r == 3u) obs_y[i] |= (unsigned char)(1u << j);
                        }
                    }
                }

                for (i = 0u; i < 8u; i++) obs_x[i] = obs_y[i];

                /* redraw both pages */
                gfx_clear_both_pages();
                gpu_set_draw_page(0u);
                gpu_draw_rect((unsigned char)LIFE_X0, (unsigned char)LIFE_Y0,
                              (unsigned char)(LIFE_W * LIFE_CELL), (unsigned char)(LIFE_H * LIFE_CELL));
                for (i = 0u; i < 8u; i++)
                {
                    for (j = 0u; j < 8u; j++)
                    {
                        if (obs_x[i] & (unsigned char)(1u << j))
                        {
                            gpu_fill_rect((unsigned char)(LIFE_X0 + (unsigned char)(j << 3)),
                                          (unsigned char)(LIFE_Y0 + (unsigned char)(i << 3)),
                                          (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                        }
                    }
                }

                gpu_set_draw_page(1u);
                gpu_draw_rect((unsigned char)LIFE_X0, (unsigned char)LIFE_Y0,
                              (unsigned char)(LIFE_W * LIFE_CELL), (unsigned char)(LIFE_H * LIFE_CELL));
                for (i = 0u; i < 8u; i++)
                {
                    for (j = 0u; j < 8u; j++)
                    {
                        if (obs_x[i] & (unsigned char)(1u << j))
                        {
                            gpu_fill_rect((unsigned char)(LIFE_X0 + (unsigned char)(j << 3)),
                                          (unsigned char)(LIFE_Y0 + (unsigned char)(i << 3)),
                                          (unsigned char)LIFE_CELL, (unsigned char)LIFE_CELL);
                        }
                    }
                }

                gpu_set_display_page(1u);
            }
        }

RUN_PET:
        /* Virtual Pet (graphics + text menu). */
        ui_clear_all_text();
    gfx_clear_both_pages();
        pet_draw_frames();
        gpu_set_display_page(0u);
        gpu_set_draw_page(1u);

        w_idx = 0u;  /* menu selection */
        ui_puts(0u, 0u, "Retro Tamagotchi");
        ui_pet_menu(w_idx);
        ui_puts(0u, 4u, "LEFT/RIGHT: menu");
        ui_puts(0u, 5u, "UP: action  DOWN: status");
        ui_clear_line(7u);

        draw_page = 0u;
        maxv = 0u;
        for (;;)
        {
            type = (unsigned char)isKeyPressed();
            RNG_STEP();

            /* slow frame toggle using display page (LCD ghosting helps) */
            maxv++;
            if (maxv == 0u)
            {
                draw_page ^= 1u;
                gpu_set_display_page(draw_page);
            }

            if (type == (unsigned char)KEY_NONE) continue;

            if (type == (unsigned char)LEFT)
            {
                w_idx = (unsigned char)((w_idx == 0u) ? 3u : (w_idx - 1u));
                ui_pet_menu(w_idx);
                ui_clear_line(7u);
            }
            else if (type == (unsigned char)RIGHT)
            {
                w_idx = (unsigned char)((w_idx + 1u) & 3u);
                ui_pet_menu(w_idx);
                ui_clear_line(7u);
            }
            else if (type == (unsigned char)UP)
            {
                if (w_idx == 0u) ui_puts(0u, 7u, "Fed! :)");
                else if (w_idx == 1u) ui_puts(0u, 7u, "Cleaned! :)");
                else if (w_idx == 2u) ui_puts(0u, 7u, "Played! :)");
                else ui_puts(0u, 7u, "Status: OK");
            }
            else if (type == (unsigned char)DOWN)
            {
                ui_puts(0u, 7u, "Hunger: OK  Mood: OK");
            }
        }
    }
}
