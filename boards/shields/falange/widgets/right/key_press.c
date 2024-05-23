/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#include "key_press.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct key_press_state {};

static lv_color_t canvas_buffer[50 * 32];
static bool toggle = true;

static void set_symbol(lv_obj_t *widget, struct key_press_state state) {
    if (toggle) {
        lv_canvas_set_px(widget, 0, 0, lv_color_white());
        lv_canvas_set_px(widget, 4, 0, lv_color_white());
    } else {
        lv_canvas_set_px(widget, 0, 0, lv_color_black());
        lv_canvas_set_px(widget, 4, 0, lv_color_black());
    }
    toggle != toggle;
}

void key_press_update_cb(struct key_press_state state) {
    struct zmk_widget_key_press *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_symbol(widget->obj, state); }
}

static struct key_press_state key_press_get_state(const zmk_event_t *eh) {
    const struct zmk_state_changed *ev = as_zmk_position_state_changed(eh);
    return (struct key_press_state){};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_key_press, struct key_press_state,
                            key_press_update_cb, key_press_get_state)

ZMK_SUBSCRIPTION(widget_key_press, zmk_position_state_changed);

int zmk_widget_key_press_init(struct zmk_widget_key_press *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);

    lv_canvas_set_buffer(widget->obj, canvas_buffer, 50, 32, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(widget->obj, lv_color_black(), LV_OPA_COVER);
    lv_obj_align(widget->obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_key_press_init();

    return 0;
}

lv_obj_t *zmk_widget_key_press_obj(struct zmk_widget_key_press *widget) {
    return widget->obj;
}
