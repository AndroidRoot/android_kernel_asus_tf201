/*
 * arch/arm/mach-tegra/board-tf700-misc.c
 *
 * Copyright (C) 2011-2012 ASUSTek Computer Incorporation
 * Author: Paris Yeh <paris_yeh@asus.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <mach/board-tf700-misc.h>
#include <mach/pinmux.h>
#include <mach/pinmux-t3.h>
#include "gpio-names.h"
#include "fuse.h"

#define CARDHU_MISC_ATTR(module) \
static struct kobj_attribute module##_attr = { \
	.attr = { \
		.name = __stringify(module), \
		.mode = 0444, \
	}, \
	.show = module##_show, \
}


/* PCBID is composed of ten GPIO pins */
static unsigned int tf700_pcbid;

static const struct pins tf700_pcbid_pins[] = {
	{TEGRA_GPIO_PR4, TEGRA_PINGROUP_KB_ROW4, "PCB_ID0", false},
	{TEGRA_GPIO_PR5, TEGRA_PINGROUP_KB_ROW5, "PCB_ID1", false},
	{TEGRA_GPIO_PQ4, TEGRA_PINGROUP_KB_COL4, "PCB_ID2", false},
	{TEGRA_GPIO_PQ7, TEGRA_PINGROUP_KB_COL7, "PCB_ID3", false},
	{TEGRA_GPIO_PR2, TEGRA_PINGROUP_KB_ROW2, "PCB_ID4", false},
	{TEGRA_GPIO_PQ5, TEGRA_PINGROUP_KB_COL5, "PCB_ID5", false},
	{TEGRA_GPIO_PJ0, TEGRA_PINGROUP_GMI_CS0_N, "PCB_ID6", false},
	{TEGRA_GPIO_PJ2, TEGRA_PINGROUP_GMI_CS1_N, "PCB_ID7", false},
	{TEGRA_GPIO_PK3, TEGRA_PINGROUP_GMI_CS2_N, "PCB_ID8", false},
	{TEGRA_GPIO_PC7, TEGRA_PINGROUP_GMI_WP_N, "PCB_ID9", true},
};

/* PROJECTID is composed of four GPIO pins */
static unsigned int tf700_extended_projectid;

static const struct pins tf700_extended_projectid_pins[] = {
	{TEGRA_GPIO_PK2, TEGRA_PINGROUP_GMI_CS4_N, "PROJECT_ID0", true},
	{TEGRA_GPIO_PI3, TEGRA_PINGROUP_GMI_CS6_N, "PROJECT_ID1", true},
	{TEGRA_GPIO_PI7, TEGRA_PINGROUP_GMI_WAIT, "PROJECT_ID2", true},
	{TEGRA_GPIO_PK4, TEGRA_PINGROUP_GMI_CS3_N, "PROJECT_ID3", true},
};

static const char *tegra3_project_name[TEGRA3_PROJECT_MAX] = {
	[TEGRA3_PROJECT_TF201] = "TF201",
	[TEGRA3_PROJECT_P1801] = "P1801",
	[TEGRA3_PROJECT_TF300T] = "TF300T",
	[TEGRA3_PROJECT_TF300TG] = "TF300TG",
	[TEGRA3_PROJECT_TF700T] = "TF700T",
	[TEGRA3_PROJECT_TF300TL] = "TF300TL",
	[TEGRA3_PROJECT_EXTENSION] = "Extension",
	[TEGRA3_PROJECT_TF500T] = "TF500T",
	[TEGRA3_PROJECT_ME301T] = "ME301T",
	[TEGRA3_PROJECT_ME301TL] = "ME301TL",
	[TEGRA3_PROJECT_ME371T] = "ME371T",
};

static unsigned int tegra3_project_name_index = TEGRA3_PROJECT_INVALID;
static bool tegra3_misc_enabled = false;

static unsigned long long tegra3_chip_uid = 0;

static int __init tegra3_productid_setup(char *id)
{
	unsigned int index;

	if (!id) return 1;

	index = (unsigned int) simple_strtoul(id, NULL, 0);

	pr_info("[MISC]: Found androidboot.productid = %s\n", id);

	tegra3_project_name_index = (index < TEGRA3_PROJECT_MAX)
					? index : TEGRA3_PROJECT_INVALID;
	return 0;
}

early_param("androidboot.productid", tegra3_productid_setup);

/* Deprecated function */
const char *tegra3_get_project_name(void)
{
	unsigned int project_id = tegra3_project_name_index;

	if (tegra3_misc_enabled) {
		project_id = HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW,

						PROJECT, tf700_pcbid);
		if (project_id == TEGRA3_PROJECT_EXTENSION)
			project_id = 8 + HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW,
				EXTENDED_PROJECT, tf700_extended_projectid);

		/* WARN if project id was not matched with PCBID */
		WARN_ONCE(project_id != tegra3_project_name_index,
			"[MISC]: project ID in kernel cmdline was not matched"
			"with PCBID\n");
	}
	else {
		pr_info("[MISC]: adopt kernel cmdline prior to %s ready.\n",
				__func__);
	}

	return (project_id < TEGRA3_PROJECT_MAX) ?
		tegra3_project_name[project_id] : "unknown";
}
EXPORT_SYMBOL(tegra3_get_project_name);

unsigned int tegra3_get_project_id(void)
{
	unsigned int project_id = tegra3_project_name_index;

	if (tegra3_misc_enabled) {
		project_id = HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW,
						PROJECT, tf700_pcbid);

		if (project_id == TEGRA3_PROJECT_EXTENSION)
			project_id = 8 + HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW,
				EXTENDED_PROJECT, tf700_extended_projectid);

		/* WARN if project id was not matched with PCBID */
		WARN_ONCE(project_id != tegra3_project_name_index,
			"[MISC]: project ID in kernel cmdline was not matched"
			"with PCBID\n");
	}
	else {
		pr_info("[MISC]: adopt kernel cmdline prior to %s ready.\n",
				__func__);
	}
	return (project_id < TEGRA3_PROJECT_MAX) ?
		project_id : TEGRA3_PROJECT_INVALID;

}
EXPORT_SYMBOL(tegra3_get_project_id);

unsigned int tegra3_query_touch_module_pcbid(void)
{
	unsigned int touch_pcbid = 0;
	unsigned int project = tegra3_get_project_id();
	unsigned int ret = -1;

	/* Check if running target platform */
	if ((project == TEGRA3_PROJECT_TF300T) ||
		(project == TEGRA3_PROJECT_TF300TG) ||
		(project == TEGRA3_PROJECT_TF300TL)) {
		pr_err("[MISC]: %s is not supported on %02x.\n", __func__,
			tegra3_get_project_id());
		return ret;
	}

	/* Fetch PCB_ID[2] and PCB_ID[6] and recompose it */
	touch_pcbid = (HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW, TOUCHL, tf700_pcbid)) +
		(HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW, TOUCHH, tf700_pcbid) << 1);

	switch (project) {
	case TEGRA3_PROJECT_TF201:
		ret = touch_pcbid;
		break;
	case TEGRA3_PROJECT_TF700T:
		/* Reserve PCB_ID[2] for touch panel identification */
		ret = touch_pcbid & 0x1;
		break;
	default:
		ret = -1;
	}

	return ret;
}
EXPORT_SYMBOL(tegra3_query_touch_module_pcbid);

unsigned int tegra3_query_audio_codec_pcbid(void)
{
	unsigned int codec_pcbid = 0;
	unsigned int project = tegra3_get_project_id();
	unsigned int ret = -1;

	/* Check if running target platform */
	if ((project == TEGRA3_PROJECT_TF201) ||
		(project == TEGRA3_PROJECT_TF700T)) {
		pr_err("[MISC]: %s is not supported on %02x.\n", __func__,
			tegra3_get_project_id());
		return ret;
	}

	codec_pcbid = HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW, ACODEC, tf700_pcbid);

	if ((project == TEGRA3_PROJECT_TF300T) ||
		(project == TEGRA3_PROJECT_TF300TG) ||
		(project == TEGRA3_PROJECT_TF300TL)) {
		ret = codec_pcbid;
	}

	return ret;
}
EXPORT_SYMBOL(tegra3_query_audio_codec_pcbid);

unsigned int tegra3_query_pcba_revision_pcbid(void)
{
	unsigned int project = tegra3_get_project_id();
	unsigned int ret = -1;

	/* Check if running target platform */
	if (project != TEGRA3_PROJECT_TF700T) {
		pr_err("[MISC]: %s is not supported on %02x.\n", __func__,
			tegra3_get_project_id());
		return ret;
	}

	ret = HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW, REVISION, tf700_pcbid);

	return ret;
}
EXPORT_SYMBOL(tegra3_query_pcba_revision_pcbid);

unsigned int tegra3_query_wifi_module_pcbid(void)
{
	unsigned int wifi_pcbid = 0;
	unsigned int project = tegra3_get_project_id();
	unsigned int ret = -1;

	/* Check if running target platform is valid */
	if (project == TEGRA3_PROJECT_INVALID) {
		pr_err("[MISC]: %s is not supported on %02x.\n", __func__,
			tegra3_get_project_id());
		return ret;
	}

	wifi_pcbid = HW_DRF_VAL(TEGRA3_DEVKIT, MISC_HW, WIFI, tf700_pcbid);
	ret = wifi_pcbid;

	return ret;
}
EXPORT_SYMBOL(tegra3_query_wifi_module_pcbid);

static ssize_t tf700_chipid_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;

	if (!tegra3_chip_uid)
		s += sprintf(s, "%016llx\n", tegra_chip_uid());
	else
		s += sprintf(s, "%016llx\n", tegra3_chip_uid);

	return (s - buf);
}

static ssize_t tf700_pcbid_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	int i;

	for (i = ARRAY_SIZE(tf700_extended_projectid_pins); i > 0; i--)
		s += sprintf(s, "%c",
			tf700_extended_projectid & (1 << (i - 1)) ? '1' : '0');

	for (i = ARRAY_SIZE(tf700_pcbid_pins); i > 0; i--)
		s += sprintf(s, "%c",
			tf700_pcbid & (1 << (i - 1)) ? '1' : '0');

	s += sprintf(s, "b\n");
	return (s - buf);
}

static ssize_t tf700_projectid_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
        char *s = buf;

        s += sprintf(s, "%02x\n", tegra3_get_project_id());
        return (s - buf);
}

static ssize_t tf700_projectname_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
        char *s = buf;

        s += sprintf(s, "%s\n", tegra3_get_project_name());
        return (s - buf);
}

CARDHU_MISC_ATTR(tf700_chipid);
CARDHU_MISC_ATTR(tf700_pcbid);
CARDHU_MISC_ATTR(tf700_projectid);
CARDHU_MISC_ATTR(tf700_projectname);

static struct attribute *attr_list[] = {
	&tf700_chipid_attr.attr,
	&tf700_pcbid_attr.attr,
	&tf700_projectid_attr.attr,
	&tf700_projectname_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attr_list,
};

static struct platform_device *tf700_misc_device;

static int board_pins_init(
	const struct pins *board_pins,
	unsigned int pin_size,
	unsigned int *pin_value)
{
	int ret = 0, i = 0;


	for (i = 0; i < pin_size; i++) {
		ret = gpio_request(board_pins[i].gpio, board_pins[i].label);

		if (ret) {
			while (i >= 0) {
				gpio_free(board_pins[i].gpio);
				i--;
			}
			return ret;
		}
		gpio_direction_input(board_pins[i].gpio);
		*pin_value |= gpio_get_value(board_pins[i].gpio) << i;
	}

	return 0;
}

static void board_pins_reset(
	const struct pins *board_pins,
	unsigned int pin_size)
{
	int i = 0;
	enum tegra3_project project_id = tegra3_get_project_id();

	for (i = 0; i < pin_size; i++) {
		if (board_pins[i].extended_pins) {
			/* set no-pull */
			tegra_pinmux_set_pullupdown(board_pins[i].pingroup,
				TEGRA_PUPD_NORMAL);
			if (project_id < 8)
				/* disable input buffer */
				tegra_pinmux_set_io(board_pins[i].pingroup,
					TEGRA_PIN_OUTPUT);
				/* release GPIO related registration */
				gpio_free(board_pins[i].gpio);
		}
	}
}

int __init tf700_misc_init(unsigned long long uid)
{
	int ret = 0;

	pr_debug("%s: start\n", __func__);

	if (!uid)
		pr_err("[MISC]: chip unique id is unavailable.\n");
	else
		tegra3_chip_uid = uid;

	// create a platform device
	tf700_misc_device = platform_device_alloc("tf700_misc", -1);

        if (!tf700_misc_device) {
		ret = -ENOMEM;
		goto fail_platform_device;
        }

	// add a platform device to device hierarchy
	ret = platform_device_add(tf700_misc_device);
	if (ret) {
		pr_err("[MISC]: cannot add device to platform.\n");
		goto fail_platform_add_device;
	}

	ret = sysfs_create_group(&tf700_misc_device->dev.kobj, &attr_group);
	if (ret) {
		pr_err("[MISC]: cannot create sysfs group.\n");
		goto fail_sysfs;
	}

	/* acquire pcb_id info */
	ret = board_pins_init(tf700_pcbid_pins,
		ARRAY_SIZE(tf700_pcbid_pins), &tf700_pcbid);
	if (ret) {
		pr_err("[MISC]: cannot acquire PCB_ID info.\n");
		goto fail_sysfs;
	}

	/* acquire project_id info */
	ret = board_pins_init(tf700_extended_projectid_pins,
		ARRAY_SIZE(tf700_extended_projectid_pins),
		&tf700_extended_projectid);
	if (ret) {
		pr_err("[MISC]: cannot acquire PROJECT_ID info.\n");
		goto fail_sysfs;
	}

	// indicate misc module well-prepared
	tegra3_misc_enabled = true;

	return ret;

fail_sysfs:
	platform_device_del(tf700_misc_device);

fail_platform_add_device:
	platform_device_put(tf700_misc_device);

fail_platform_device:
	return ret;
}

int __init tf700_misc_reset(void)
{
	enum tegra3_project project_id = tegra3_get_project_id();

	/* reset pcb_id pins */
	board_pins_reset(tf700_pcbid_pins,
		ARRAY_SIZE(tf700_pcbid_pins));

	/* reset project_id pins */
	board_pins_reset(tf700_extended_projectid_pins,
		ARRAY_SIZE(tf700_extended_projectid_pins));

	/* reset GMI_CS4_N to specific state for ELAN touch IC on TF700T */
	if (project_id == TEGRA3_PROJECT_TF700T) {
		/* set pull-up */
		tegra_pinmux_set_pullupdown(TEGRA_PINGROUP_GMI_CS4_N,
			TEGRA_PUPD_PULL_UP);
		/* enable output buffer */
		tegra_pinmux_set_tristate(TEGRA_PINGROUP_GMI_CS4_N,
			TEGRA_TRI_NORMAL);
	}

}
