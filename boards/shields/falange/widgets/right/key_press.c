/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#include "key_press.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct key_press_state {
    uint32_t position;
    bool state;
};

struct pos {
    uint8_t x;
    uint8_t y;
};

struct piece {
    struct pos pos;
    uint8_t rotation;
};

static struct piece starting_pieces[7] = {
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
};

static struct piece pieces[7] = {
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
    {{1,16}, 0},
};

static struct pos rotations[7][4][4] = {
    {
        {{2,0},{2,1},{2,2},{2,3}},
        {{0,1},{1,1},{2,1},{3,1}},
        {{2,0},{2,1},{2,2},{2,3}},
        {{0,1},{1,1},{2,1},{3,1}},
    },
    {
        {{1,1},{1,2},{2,1},{2,2}},
        {{1,1},{1,2},{2,1},{2,2}},
        {{1,1},{1,2},{2,1},{2,2}},
        {{1,1},{1,2},{2,1},{2,2}},
    },
    {
        {{1,0},{1,1},{0,2},{1,2}},
        {{0,0},{0,1},{1,1},{2,1}},
        {{1,0},{2,0},{1,1},{1,2}},
        {{0,1},{1,1},{2,1},{2,2}},
    },
    {
        {{1,0},{1,1},{0,0},{1,2}},
        {{2,0},{0,1},{1,1},{2,1}},
        {{1,0},{2,2},{1,1},{1,2}},
        {{0,1},{1,1},{2,1},{0,2}},
    },
    {
        {{1,0},{1,1},{2,1},{2,2}},
        {{1,0},{2,0},{0,1},{1,1}},
        {{1,0},{1,1},{2,1},{2,2}},
        {{1,0},{2,0},{0,1},{1,1}},
    },
    {
        {{2,0},{1,1},{2,1},{1,2}},
        {{0,0},{1,0},{1,1},{2,1}},
        {{2,0},{1,1},{2,1},{1,2}},
        {{0,0},{1,0},{1,1},{2,1}},
    },
    {
        {{1,0},{1,1},{1,2},{2,1}},
        {{0,1},{1,1},{2,1},{1,2}},
        {{1,0},{1,1},{1,2},{0,1}},
        {{0,1},{1,1},{2,1},{1,0}},
    },
};

static uint8_t current_piece = 2;
static bool input_pending = false;

static bool pressed[4] = { false, false, false, false };

static int8_t inputs[4] = { 0, 0, 0, 0 };
static int8_t current_input = -1;

static lv_color_t canvas_buffer[50 * 32];
static lv_color_t back_buffer[50 * 32];

static lv_anim_t a;
static uint8_t frame = 0;
static uint8_t del = 1;

void canvas_reset(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_white();

    lv_canvas_set_buffer(canvas, canvas_buffer, 50, 32, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, 1, 1, 48, 30, &rect_dsc);
}

void draw_piece(uint8_t piece, void *canvas, lv_color_t color) {
    for (uint8_t i = 0; i < 4; ++i) {
        uint8_t x = pieces[piece].pos.x + (rotations[piece][pieces[piece].rotation][i].x*2);
        uint8_t y = pieces[piece].pos.y + (rotations[piece][pieces[piece].rotation][i].y*2);
        lv_canvas_set_px(canvas, x, y, color);
        lv_canvas_set_px(canvas, x+1, y, color);
        lv_canvas_set_px(canvas, x, y+1, color);
        lv_canvas_set_px(canvas, x+1, y+1, color);
    }
}

void anim_exec(void *widget, int32_t val) {
    lv_canvas_set_buffer(widget, canvas_buffer, 50, 32, LV_IMG_CF_TRUE_COLOR);
    draw_piece(current_piece, widget, lv_color_white());

    if (current_input != -1) {
        if (inputs[current_input] == 0) {
            pieces[current_piece].rotation = (pieces[current_piece].rotation+1)%4;
        } else {
            int8_t dir = 0;
            if (inputs[current_input] == 2) dir = 2;
            else if (inputs[current_input] == 3) dir = -2;

            bool collide = false;
            for (uint8_t i = 0; i < 4; ++i) {
                uint8_t x = pieces[current_piece].pos.x + (rotations[current_piece][pieces[current_piece].rotation][i].x*2);
                uint8_t y = pieces[current_piece].pos.y + (rotations[current_piece][pieces[current_piece].rotation][i].y*2);
                if (lv_canvas_get_px(widget, x, y+dir).full == lv_color_black().full) {
                    collide = true;
                    break;
                } else if (lv_canvas_get_px(widget, x+1, y+dir).full == lv_color_black().full) {
                    collide = true;
                    break;
                } else if (lv_canvas_get_px(widget, x, y+dir+1).full == lv_color_black().full) {
                    collide = true;
                    break;
                } else if (lv_canvas_get_px(widget, x+1, y+dir+1).full == lv_color_black().full) {
                    collide = true;
                    break;
                }
            }

            if (!collide) pieces[current_piece].pos.y += dir;
        }

        --current_input;
    }

    if (frame == 1) {
        frame = 0;

        bool collide = false;
        for (uint8_t i = 0; i < 4; ++i) {
            uint8_t x = pieces[current_piece].pos.x + (rotations[current_piece][pieces[current_piece].rotation][i].x*2);
            uint8_t y = pieces[current_piece].pos.y + (rotations[current_piece][pieces[current_piece].rotation][i].y*2);
            if (lv_canvas_get_px(widget, x+1, y).full == lv_color_black().full) {
                collide = true;
                break;
            } else if (lv_canvas_get_px(widget, x+2, y).full == lv_color_black().full) {
                collide = true;
                break;
            } else if (lv_canvas_get_px(widget, x, y+1).full == lv_color_black().full) {
                collide = true;
                break;
            } else if (lv_canvas_get_px(widget, x+2, y+1).full == lv_color_black().full) {
                collide = true;
                break;
            }
        }

        if (!collide) pieces[current_piece].pos.x += 1;

        draw_piece(current_piece, widget, lv_color_black());

        if (collide) {
            if (pieces[current_piece].pos.x == 1) {
                canvas_reset(widget);
                return;
            }

            draw_piece(current_piece, widget, lv_color_black());
            current_piece = (current_piece+1)%7;

            pieces[current_piece] = starting_pieces[current_piece];

            for (uint8_t i = 0; i < 4; ++i) {
                bool line = true;
                uint8_t x = pieces[current_piece].pos.x + (rotations[current_piece][pieces[current_piece].rotation][i].x*2);
                for (uint8_t j = 2; j < 30; ++j) {
                    if (lv_canvas_get_px(widget, x, j).full != lv_color_black().full) {
                        line = false;
                        break;
                    }
                }
                if (line) {
                    for (uint8_t j = 2; j < 30; ++j) {
                        lv_canvas_set_px(widget, x, j, lv_color_white());
                        lv_canvas_set_px(widget, x+1, j, lv_color_white());
                    }
                    break;
                }
            }
        }
    } else {
        draw_piece(current_piece, widget, lv_color_black());
        ++frame;
    }
}

static void set_symbol(lv_obj_t *widget, struct key_press_state state) {
    if (current_input == 3) return;

    int8_t i = -1;
    if (state.position%4 == 3 && state.state) i = 0;
    else if (state.position%4 == 0 && state.state) i = 2;
    else if (state.position%4 == 1 && state.state) i = 1;
    else if (state.position%4 == 2 && state.state) i = 3;

    ++current_input;
    inputs[current_input] = i;
}

void key_press_update_cb(struct key_press_state state) {
    struct zmk_widget_key_press *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_symbol(widget->obj, state); }
}

static struct key_press_state key_press_get_state(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    return (struct key_press_state){
        .position = ev->position,
        .state = ev->state,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_key_press, struct key_press_state,
                            key_press_update_cb, key_press_get_state)

ZMK_SUBSCRIPTION(widget_key_press, zmk_position_state_changed);

int zmk_widget_key_press_init(struct zmk_widget_key_press *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);

    lv_obj_align(widget->obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    canvas_reset(widget->obj);

    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t) anim_exec);
    lv_anim_set_var(&a, widget->obj);
    lv_anim_set_time(&a, 10000);
    lv_anim_set_values(&a, 1, 200);

    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    lv_anim_start(&a);

    sys_slist_append(&widgets, &widget->node);

    widget_key_press_init();

    return 0;
}

lv_obj_t *zmk_widget_key_press_obj(struct zmk_widget_key_press *widget) {
    return widget->obj;
}
