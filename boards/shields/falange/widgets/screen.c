/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include "screen.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct screen_state {
    uint8_t level;
};

static lv_color_t battery_image_buffer[5 * 8];

static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    lv_draw_rect_dsc_t rect_fill_dsc;
    lv_draw_rect_dsc_init(&rect_fill_dsc);
    rect_fill_dsc.bg_color = lv_color_white();

    lv_canvas_set_px(canvas, 0, 0, lv_color_white());
    lv_canvas_set_px(canvas, 4, 0, lv_color_white());

    if (level > 90) {
        // full
    } else if (level > 70) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 1, &rect_fill_dsc);
    } else if (level > 50) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 2, &rect_fill_dsc);
    } else if (level > 30) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 3, &rect_fill_dsc);
    } else if (level > 10) {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 4, &rect_fill_dsc);
    } else {
        lv_canvas_draw_rect(canvas, 1, 2, 3, 5, &rect_fill_dsc);
    }
}

static void set_battery_symbol(lv_obj_t *widget, struct screen_state state) {
    lv_obj_t *symbol = lv_obj_get_child(widget, 0);
    lv_obj_t *label = lv_obj_get_child(widget, 1);

    draw_battery(symbol, state.level);
    lv_label_set_text_fmt(label, "%3u%%", state.level);
}

void screen_update_cb(struct screen_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct screen_state screen_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct screen_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_screen, struct screen_state,
                            screen_update_cb, screen_get_state)

ZMK_SUBSCRIPTION(widget_screen, zmk_battery_state_changed);

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);

    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_t *image_canvas = lv_canvas_create(widget->obj);
    lv_obj_t *battery_label = lv_label_create(widget->obj);

    lv_canvas_set_buffer(image_canvas, battery_image_buffer, 5, 8, LV_IMG_CF_TRUE_COLOR);

    lv_obj_align(image_canvas, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -7, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_screen_init();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) {
    return widget->obj;
}
