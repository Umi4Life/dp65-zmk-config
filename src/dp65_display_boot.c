/*
 * Copyright (c) 2026 DP65 Contributors
 * SPDX-License-Identifier: MIT
 *
 * nice!view VCC is on the nice!nano switched 3V3 rail (P0.13). settings_load()
 * in main() can disable that rail before the display stack runs. The LS0xx
 * driver does not recover when power returns unless we force the rail and
 * re-blank the panel.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if IS_ENABLED(CONFIG_ZMK_DISPLAY)

#include <drivers/ext_power.h>
#include <lvgl.h>
#include <zmk/display.h>

LOG_MODULE_REGISTER(dp65_display_boot, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
#define EXT_POWER_NODE DT_INST(0, zmk_ext_power_generic)
static const struct gpio_dt_spec ext_pwr_gpio =
    GPIO_DT_SPEC_GET_BY_IDX(EXT_POWER_NODE, control_gpios, 0);
static const struct device *ext_power_dev = DEVICE_DT_GET(EXT_POWER_NODE);
#endif

static int boot_attempt;

static void dp65_force_ext_power_on(void) {
#if DT_HAS_COMPAT_STATUS_OKAY(zmk_ext_power_generic)
    if (gpio_is_ready_dt(&ext_pwr_gpio)) {
        (void)gpio_pin_configure_dt(&ext_pwr_gpio, GPIO_OUTPUT_ACTIVE);
        (void)gpio_pin_set_dt(&ext_pwr_gpio, 1);
    }

    if (device_is_ready(ext_power_dev)) {
        (void)ext_power_enable(ext_power_dev);
    }
#else
    const struct device *ext_power = device_get_binding("EXT_POWER");

    if (ext_power != NULL && device_is_ready(ext_power)) {
        (void)ext_power_enable(ext_power);
    }
#endif
}

static void dp65_redraw_status_screen(void) {
    extern lv_obj_t *zmk_display_status_screen(void);
    lv_obj_t *screen = zmk_display_status_screen();

    if (screen == NULL) {
        LOG_ERR("No status screen from shield");
        return;
    }

    (void)display_blanking_off(display_dev);
    lv_scr_load(screen);
}

static void dp65_display_boot_work(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(dp65_display_boot_work_item, dp65_display_boot_work);

static void dp65_display_boot_work(struct k_work *work) {
    ARG_UNUSED(work);

    boot_attempt++;
    LOG_INF("nice!view boot recovery attempt %d", boot_attempt);

    dp65_force_ext_power_on();
    k_msleep(500);

    if (!device_is_ready(display_dev)) {
        int err = device_init(display_dev);

        if (err < 0 && err != -EALREADY) {
            LOG_WRN("display device_init returned %d", err);
        }
        k_msleep(100);
    }

    if (device_is_ready(display_dev)) {
        (void)display_clear(display_dev);
        (void)display_blanking_off(display_dev);
    }

    (void)zmk_display_init();
    k_msleep(250);
    dp65_redraw_status_screen();

    if (boot_attempt < 5) {
        k_work_reschedule(&dp65_display_boot_work_item, K_SECONDS(2));
    }
}

static int dp65_display_boot_schedule(void) {
    k_work_schedule(&dp65_display_boot_work_item, K_MSEC(500));
    return 0;
}

SYS_INIT(dp65_display_boot_schedule, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_DISPLAY */
