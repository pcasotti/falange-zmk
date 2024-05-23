/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen_right.h"
#include "widgets/right/battery_status.h"
#include "widgets/right/key_press.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_key_press key_press_widget;

lv_style_t global_style;

static lv_color_t canvas_buffer[50 * 30];

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;

    screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_text_font(&global_style, &lv_font_unscii_8);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

    zmk_widget_key_press_init(&key_press_widget, screen);
    lv_obj_align(zmk_widget_key_press_obj(&key_press_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    return screen;
}
