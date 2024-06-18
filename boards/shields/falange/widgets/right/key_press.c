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

void canvas_reset(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_white();

    lv_canvas_set_buffer(canvas, canvas_buffer, 50, 32, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, 1, 1, 48, 30, &rect_dsc);

    lv_color_t c;
    c.full = 3;
    for (uint8_t i = 0; i < 9; i++) {
        lv_canvas_set_px(canvas, 20, i+10, c);
    }
    for (uint8_t i = 0; i < 8; i++) {
        lv_canvas_set_px(canvas, 35, i+1, c);
    }
}

void anim_exec(void *widget, int32_t val) {
    for (uint8_t i = 1; i < 31; i++) {
        if (lv_canvas_get_px(widget, 1, i).full == lv_color_black().full
                && lv_canvas_get_px(widget, 2, i).full == lv_color_black().full
                && lv_canvas_get_px(widget, 3, i).full == lv_color_black().full) {
            canvas_reset(widget);
            return;
        }
    }

    for (uint8_t i = 48; i > 0; i--) {
        for (uint8_t j = 30; j > 0; j--) {
            if (lv_canvas_get_px(widget, i, j).full == lv_color_black().full) {
                if (i+1 <= 48) {
                    if (lv_canvas_get_px(widget, i+1, j).full == lv_color_white().full) {
                        lv_canvas_set_px(widget, i, j, lv_color_white());
                        lv_canvas_set_px(widget, i+1, j, lv_color_black());
                    } else if (j+1 <= 30 && lv_canvas_get_px(widget, i+1, j+1).full == lv_color_white().full) {
                        lv_canvas_set_px(widget, i, j, lv_color_white());
                        lv_canvas_set_px(widget, i+1, j+1, lv_color_black());
                    } else if (j-1 >= 1 && lv_canvas_get_px(widget, i+1, j-1).full == lv_color_white().full) {
                        lv_canvas_set_px(widget, i, j, lv_color_white());
                        lv_canvas_set_px(widget, i+1, j-1, lv_color_black());
                    }
                }
            }
        }
    }
}

static void set_symbol(lv_obj_t *widget, struct key_press_state state) {
    // canvas_reset(widget);
    // lv_anim_del_all();

    uint32_t pos = state.position%12+1;
    uint32_t spawn = pos*2;
    if (state.state) spawn += 1;
    lv_canvas_set_px(widget, 1, spawn, lv_color_black());
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

    lv_anim_t a;
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
