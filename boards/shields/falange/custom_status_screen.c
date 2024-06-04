/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"
#include "widgets/screen.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_widget_screen screen_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

    zmk_widget_screen_init(&screen_widget, screen);
    lv_obj_align(zmk_widget_screen_obj(&screen_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}
