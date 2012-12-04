/*
 * arch/arm/mach-tegra/board-tf700-kbc.c
 * Keys configuration for Nvidia tegra3 tf700 platform.
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

#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include "board.h"
#include "board-tf700.h"

#include "gpio-names.h"
#include "devices.h"

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

static struct gpio_keys_button tf700_int_keys[] = {
	[0] = GPIO_KEY(KEY_POWER, PV0, 1),
	[1] = GPIO_KEY(KEY_VOLUMEUP, PQ2, 0),
	[2] = GPIO_KEY(KEY_VOLUMEDOWN, PQ3, 0),
};

static struct gpio_keys_platform_data tf700_int_keys_pdata = {
	.buttons        = tf700_int_keys,
	.nbuttons       = ARRAY_SIZE(tf700_int_keys),
};

static struct platform_device tf700_int_keys_device = {
	.name   = "gpio-keys",
	.id     = 0,
	.dev    = {
		.platform_data  = &tf700_int_keys_pdata,
	},
};

int __init tf700_keys_init(void)
{
	int i;
	struct board_info board_info;

	tegra_get_board_info(&board_info);
	BUG_ON(board_info.board_id != BOARD_PM269);

	pr_info("Registering gpio keys\n");

	for (i = 0; i < tf700_int_keys_pdata.nbuttons; i++)
		tegra_gpio_enable(tf700_int_keys_pdata.buttons[i].gpio);


	platform_device_register(&tf700_int_keys_device);

	return 0;
}
