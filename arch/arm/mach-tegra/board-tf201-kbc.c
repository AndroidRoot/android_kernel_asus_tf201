/*
 * arch/arm/mach-tegra/board-tf201-kbc.c
 * Keys configuration for Nvidia tegra3 tf201 platform.
 *
 * Copyright (C) 2011 NVIDIA, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mfd/tps6591x.h>
#include <linux/mfd/max77663-core.h>
#include <linux/interrupt_keys.h>
#include <linux/gpio_scrollwheel.h>

#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include <mach/board-tf201-misc.h>
#include "board.h"
#include "board-tf201.h"

#include "gpio-names.h"
#include "devices.h"

#define TF201_PM269_ROW_COUNT	1
#define TF201_PM269_COL_COUNT	4

static const u32 kbd_keymap[] = {
	KEY(0, 0, KEY_RESERVED),
	KEY(0, 1, KEY_RESERVED),
	KEY(0, 2, KEY_VOLUMEUP),
	KEY(0, 3, KEY_VOLUMEDOWN),
};

static const struct matrix_keymap_data keymap_data = {
	.keymap	 = kbd_keymap,
	.keymap_size    = ARRAY_SIZE(kbd_keymap),
};

static struct tegra_kbc_platform_data tf201_kbc_platform_data = {
	.debounce_cnt = 20,
	.repeat_cnt = 1,
	.scan_count = 30,
	.wakeup = false,
	.keymap_data = &keymap_data,
	.wake_cnt = 0,
};

int __init tf201_kbc_init(void)
{
	struct tegra_kbc_platform_data *data = &tf201_kbc_platform_data;
	int i;

	pr_info("Registering tegra-kbc\n");
	tegra_kbc_device.dev.platform_data = &tf201_kbc_platform_data;

	for (i = 0; i < TF201_PM269_ROW_COUNT; i++) {
		data->pin_cfg[i].num = i;
		data->pin_cfg[i].is_row = true;
		data->pin_cfg[i].en = true;
	}
	for (i = 0; i < TF201_PM269_COL_COUNT; i++) {
		/*
		 * Avoid keypad scan (ROW0,COL0) and (ROW0, COL1)
		 * KBC-COL1 (GPIO-Q-01) is used for Pad EC request#
		 * KBC-COL0(GPIO-Q-00) and AP_ONKEY#(GPIO-P-00) are
		 * both wired with power button, but we configure
		 * AP_ONKEY pin for power button instead.
		 */
		if (i <= 1) continue;
		data->pin_cfg[i + KBC_PIN_GPIO_16].num = i;
		data->pin_cfg[i + KBC_PIN_GPIO_16].en = true;
	}

	platform_device_register(&tegra_kbc_device);
	return 0;
}

int __init tf201_scroll_init(void)
{
	return 0;
}

#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}

static struct gpio_keys_button tf201_keys_tf201[] = {
	[0] = GPIO_KEY(KEY_POWER, PV0, 1),
};

static struct gpio_keys_platform_data tf201_keys_tf201_platform_data = {
	.buttons	= tf201_keys_tf201,
	.nbuttons	= ARRAY_SIZE(tf201_keys_tf201),
};

static struct platform_device tf201_keys_tf201_device = {
	.name   = "gpio-keys",
	.id     = 0,
	.dev    = {
		.platform_data  = &tf201_keys_tf201_platform_data,
	},
};
#define INT_KEY(_id, _irq, _iswake, _deb_int)	\
	{					\
		.code = _id,			\
		.irq = _irq,			\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = _deb_int,	\
	}

int __init tf201_keys_init(void)
{
	int i;

	pr_info("Registering gpio keys\n");

	for (i = 0; i < ARRAY_SIZE(tf201_keys_tf201); i++)
		tegra_gpio_enable(tf201_keys_tf201[i].gpio);

	platform_device_register(&tf201_keys_tf201_device);

	return 0;
}
