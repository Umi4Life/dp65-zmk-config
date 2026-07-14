/*
 * Copyright (c) 2026 DP65 Contributors
 * SPDX-License-Identifier: MIT
 *
 * nice!view VCC is switched by the nice!nano ext-power MOSFET. settings_load()
 * in main() can disable that rail before zmk_display_init() runs, and the
 * display driver does not recover when power is toggled later. Schedule a late
 * re-enable + display init after the rest of startup has finished.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_ZMK_DISPLAY)

#include <drivers/ext_power.h>
#include <zmk/display.h>

static void dp65_display_boot_work(struct k_work *work) {
    ARG_UNUSED(work);

    const struct device *ext_power = device_get_binding("EXT_POWER");

    if (ext_power != NULL && device_is_ready(ext_power)) {
        ext_power_enable(ext_power);
        k_msleep(500);
    }

    zmk_display_init();
}

static K_WORK_DELAYABLE_DEFINE(dp65_display_boot_work_item, dp65_display_boot_work);

static int dp65_display_boot_schedule(void) {
    /* Run after main()'s settings_load() + zmk_display_init() */
    k_work_schedule(&dp65_display_boot_work_item, K_MSEC(100));
    return 0;
}

SYS_INIT(dp65_display_boot_schedule, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_DISPLAY */
