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

static lv_color_t canvas_buffer[50 * 32];
static lv_color_t back_buffer[50 * 32];

static uint8_t x = 25;
static uint8_t y = 16;
static uint8_t dir = 0;

static lv_anim_t a;

void anim_reset(lv_obj_t *canvas) {
    x = 15;
    y = 15;
    dir = 0;
    canvas_reset(canvas);
}

void canvas_reset(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_white();

    lv_canvas_set_buffer(canvas, canvas_buffer, 50, 32, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, 1, 1, 48, 30, &rect_dsc);
}

void anim_exec(void *canvas, int32_t val) {
    // if (x == 0 || x == 49 || y == 0 || y == 31) anim_reset(canvas);

    if (lv_canvas_get_px(canvas, x, y).full == lv_color_black().full) {
        dir++;
        lv_canvas_set_px(canvas, x, y, lv_color_white());
    } else {
        dir--;
        lv_canvas_set_px(canvas, x, y, lv_color_black());
    }
    dir %= 4;

    switch (dir) {
        case 0:
            y++;
            break;
        case 1:
            x++;
            break;
        case 2:
            y--;
            break;
        case 3:
            x--;
            break;
    }

    if (x == 0) x = 48;
    if (x == 49) x = 1;
    if (y == 0) y = 30;
    if (y == 31) y = 1;
}

static void set_symbol(lv_obj_t *widget, struct key_press_state state) {
    lv_anim_del_all();
    lv_anim_start(&a);
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

    lv_anim_del_all();
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t) anim_exec);
    lv_anim_set_var(&a, widget->obj);
    lv_anim_set_time(&a, 15000);
    lv_anim_set_values(&a, 1, 300);

    // lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    lv_anim_start(&a);

    sys_slist_append(&widgets, &widget->node);

    widget_key_press_init();

    return 0;
}

lv_obj_t *zmk_widget_key_press_obj(struct zmk_widget_key_press *widget) {
    return widget->obj;
}
