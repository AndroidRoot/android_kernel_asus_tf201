/*
 * arch/arm/mach-tegra/board-tf201-panel.c
 *
 * Copyright (c) 2010-2011, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/pwm_backlight.h>
#include <asm/atomic.h>
#include <linux/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>
#include <mach/board-tf201-misc.h>

#include "board.h"
#include "board-tf201.h"
#include "devices.h"
#include "gpio-names.h"

/* Select DSI panel to be used. */
#define DSI_PANEL_219 0
#define DSI_PANEL_218 1
#define AVDD_LCD PMU_TCA6416_GPIO_PORT17
#define DSI_PANEL_RESET 0

/* Select LVDS panel resolution. 13X7 is default */
#define PM313_LVDS_PANEL_19X12			1
#define PM313_LVDS_PANEL_BPP			1 /* 0:24bpp, 1:18bpp */

/* PM313 display board specific pins */
#define pm313_R_FDE			TEGRA_GPIO_PW0
#define pm313_R_FB			TEGRA_GPIO_PN4
#define pm313_MODE0			TEGRA_GPIO_PZ4
#define pm313_MODE1			TEGRA_GPIO_PW1
#define pm313_BPP			TEGRA_GPIO_PN6 /* 0:24bpp, 1:18bpp */
#define pm313_lvds_shutdown		TEGRA_GPIO_PH1

/* E1247 reworked for pm269 pins */
#define e1247_pm269_lvds_shutdown	TEGRA_GPIO_PN6

/* E1247 tf201 default display board pins */
#define tf201_lvds_shutdown		TEGRA_GPIO_PL2

/* common pins( backlight ) for all display boards */
#define tf201_bl_enb			TEGRA_GPIO_PH2
#define tf201_bl_pwm			TEGRA_GPIO_PH0
#define tf201_hdmi_hpd			TEGRA_GPIO_PN7
#define tf201_hdmi_enb         	TEGRA_GPIO_PP2

#if defined(DSI_PANEL_219) || defined(DSI_PANEL_218)
#define tf201_dsia_bl_enb		TEGRA_GPIO_PW1
#define tf201_dsib_bl_enb		TEGRA_GPIO_PW0
#define tf201_dsi_panel_reset		TEGRA_GPIO_PD2
#endif

#ifdef CONFIG_TEGRA_DC
static struct regulator *tf201_hdmi_reg = NULL;
static struct regulator *tf201_hdmi_pll = NULL;
static struct regulator *tf201_hdmi_vddio = NULL;
#endif

static atomic_t sd_brightness = ATOMIC_INIT(255);

#ifdef CONFIG_TEGRA_TF201_DSI
static struct regulator *tf201_dsi_reg = NULL;
#else
static struct regulator *tf201_lvds_reg = NULL;
static struct regulator *tf201_lvds_vdd_bl = NULL;
static struct regulator *tf201_lvds_vdd_panel = NULL;
#endif

static struct board_info board_info;
static struct board_info display_board_info;

static tegra_dc_bl_output tf201_bl_output_measured = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 49, 50, 51, 52, 53, 54,
	55, 56, 57, 58, 59, 60, 61, 62,
	63, 64, 65, 66, 67, 68, 69, 70,
	70, 72, 73, 74, 75, 76, 77, 78,
	79, 80, 81, 82, 83, 84, 85, 86,
	87, 88, 89, 90, 91, 92, 93, 94,
	95, 96, 97, 98, 99, 100, 101, 102,
	103, 104, 105, 106, 107, 108, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 124, 125, 126,
	127, 128, 129, 130, 131, 132, 133, 133,
	134, 135, 136, 137, 138, 139, 140, 141,
	142, 143, 144, 145, 146, 147, 148, 148,
	149, 150, 151, 152, 153, 154, 155, 156,
	157, 158, 159, 160, 161, 162, 163, 164,
	165, 166, 167, 168, 169, 170, 171, 172,
	173, 174, 175, 176, 177, 179, 180, 181,
	182, 184, 185, 186, 187, 188, 189, 190,
	191, 192, 193, 194, 195, 196, 197, 198,
	199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255
};

static p_tegra_dc_bl_output bl_output;

static int tf201_backlight_init(struct device *dev) {
	int ret;

	bl_output = tf201_bl_output_measured;

	if (WARN_ON(ARRAY_SIZE(tf201_bl_output_measured) != 256))
		pr_err("bl_output array does not have 256 elements\n");

#ifndef CONFIG_TEGRA_TF201_DSI
	tegra_gpio_disable(tf201_bl_pwm);

	ret = gpio_request(tf201_bl_enb, "backlight_enb");
	if (ret < 0)
		return ret;

	ret = gpio_direction_output(tf201_bl_enb, 1);
	if (ret < 0)
		gpio_free(tf201_bl_enb);
	else
		tegra_gpio_enable(tf201_bl_enb);

	return ret;
#endif

#if DSI_PANEL_219 || DSI_PANEL_218
	/* Enable back light for DSIa panel */
	printk("tf201_dsi_backlight_init\n");
	ret = gpio_request(tf201_dsia_bl_enb, "dsia_bl_enable");
	if (ret < 0)
		return ret;

	ret = gpio_direction_output(tf201_dsia_bl_enb, 1);
	if (ret < 0)
		gpio_free(tf201_dsia_bl_enb);
	else
		tegra_gpio_enable(tf201_dsia_bl_enb);

	/* Enable back light for DSIb panel */
	ret = gpio_request(tf201_dsib_bl_enb, "dsib_bl_enable");
	if (ret < 0)
		return ret;

	ret = gpio_direction_output(tf201_dsib_bl_enb, 1);
	if (ret < 0)
		gpio_free(tf201_dsib_bl_enb);
	else
		tegra_gpio_enable(tf201_dsib_bl_enb);
#endif

	return ret;
};

static void tf201_backlight_exit(struct device *dev) {
#ifndef CONFIG_TEGRA_TF201_DSI
	/* int ret; */
	/*ret = gpio_request(tf201_bl_enb, "backlight_enb");*/
	gpio_set_value(tf201_bl_enb, 0);
	gpio_free(tf201_bl_enb);
	tegra_gpio_disable(tf201_bl_enb);
	return;
#endif
#if DSI_PANEL_219 || DSI_PANEL_218
	/* Disable back light for DSIa panel */
	gpio_set_value(tf201_dsia_bl_enb, 0);
	gpio_free(tf201_dsia_bl_enb);
	tegra_gpio_disable(tf201_dsia_bl_enb);

	/* Disable back light for DSIb panel */
	gpio_set_value(tf201_dsib_bl_enb, 0);
	gpio_free(tf201_dsib_bl_enb);
	tegra_gpio_disable(tf201_dsib_bl_enb);

	gpio_set_value(tf201_lvds_shutdown, 1);
	mdelay(20);
#endif
}

static int tf201_backlight_notify(struct device *unused, int brightness)
{
	int cur_sd_brightness = atomic_read(&sd_brightness);

#ifndef CONFIG_TEGRA_TF201_DSI
	/* Set the backlight GPIO pin mode to 'backlight_enable' */
	gpio_set_value(tf201_bl_enb, !!brightness);
#elif DSI_PANEL_219 || DSI_PANEL_218
	/* DSIa */
	gpio_set_value(tf201_dsia_bl_enb, !!brightness);

	/* DSIb */
	gpio_set_value(tf201_dsib_bl_enb, !!brightness);
#endif

	/* SD brightness is a percentage, 8-bit value. */
	brightness = (brightness * cur_sd_brightness) / 255;

	/* Apply any backlight response curve */
	if (brightness > 255) {
		pr_info("Error: Brightness > 255!\n");
	} else {
		brightness = bl_output[brightness];
	}

	return brightness;
}

static int tf201_disp1_check_fb(struct device *dev, struct fb_info *info);

static struct platform_pwm_backlight_data tf201_backlight_data = {
	.pwm_id		= 0,
	.max_brightness	= 255,
	.dft_brightness	= 224,
	.pwm_period_ns	= 4000000,
	.init		= tf201_backlight_init,
	.exit		= tf201_backlight_exit,
	.notify		= tf201_backlight_notify,
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= tf201_disp1_check_fb,
};

static struct platform_device tf201_backlight_device = {
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.platform_data = &tf201_backlight_data,
	},
};

#ifndef CONFIG_TEGRA_TF201_DSI
static int tf201_panel_enable(void)
{
	if (tf201_lvds_reg == NULL) {
		tf201_lvds_reg = regulator_get(NULL, "vdd_lvds");
		if (WARN_ON(IS_ERR(tf201_lvds_reg)))
			pr_err("%s: couldn't get regulator vdd_lvds: %ld\n",
			       __func__, PTR_ERR(tf201_lvds_reg));
		else
			regulator_enable(tf201_lvds_reg);
	}

	if (tf201_lvds_vdd_panel == NULL) {
		tf201_lvds_vdd_panel = regulator_get(NULL, "vdd_lcd_panel");
		if (WARN_ON(IS_ERR(tf201_lvds_vdd_panel)))
			pr_err("%s: couldn't get regulator vdd_lcd_panel: %ld\n",
			       __func__, PTR_ERR(tf201_lvds_vdd_panel));
		else
			regulator_enable(tf201_lvds_vdd_panel);
	}
	msleep(20);

	gpio_set_value(e1247_pm269_lvds_shutdown, 1);

	msleep(210);

	if (tf201_lvds_vdd_bl == NULL) {
		tf201_lvds_vdd_bl = regulator_get(NULL, "vdd_backlight");
		if (WARN_ON(IS_ERR(tf201_lvds_vdd_bl)))
			pr_err("%s: couldn't get regulator vdd_backlight: %ld\n",
			       __func__, PTR_ERR(tf201_lvds_vdd_bl));
		else
			regulator_enable(tf201_lvds_vdd_bl);
	}
	msleep(10);

	return 0;
}

static int tf201_panel_disable(void)
{
	regulator_disable(tf201_lvds_reg);
	regulator_put(tf201_lvds_reg);
	tf201_lvds_reg = NULL;

	msleep(10);
	regulator_disable(tf201_lvds_vdd_bl);
	regulator_put(tf201_lvds_vdd_bl);
	tf201_lvds_vdd_bl = NULL;
	msleep(250);


	gpio_set_value(e1247_pm269_lvds_shutdown, 0);

	msleep(20);

	regulator_disable(tf201_lvds_vdd_panel);
	regulator_put(tf201_lvds_vdd_panel);
	tf201_lvds_vdd_panel= NULL;

	return 0;
}
#endif

#ifdef CONFIG_TEGRA_DC
static int tf201_hdmi_vddio_enable(void)
{
	int ret;
	if (!tf201_hdmi_vddio) {
		tf201_hdmi_vddio = regulator_get(NULL, "vdd_hdmi_con");
		if (IS_ERR_OR_NULL(tf201_hdmi_vddio)) {
			ret = PTR_ERR(tf201_hdmi_vddio);
			pr_err("hdmi: couldn't get regulator vdd_hdmi_con\n");
			tf201_hdmi_vddio = NULL;
			return ret;
		}
	}
	ret = regulator_enable(tf201_hdmi_vddio);
	if (ret < 0) {
		pr_err("hdmi: couldn't enable regulator vdd_hdmi_con\n");
		regulator_put(tf201_hdmi_vddio);
		tf201_hdmi_vddio = NULL;
		return ret;
	}
	return ret;
}

static int tf201_hdmi_vddio_disable(void)
{
	if (tf201_hdmi_vddio) {
		regulator_disable(tf201_hdmi_vddio);
		regulator_put(tf201_hdmi_vddio);
		tf201_hdmi_vddio = NULL;
	}
	return 0;
}

static int tf201_hdmi_enable(void)
{
	int ret;
	if (!tf201_hdmi_reg) {
		tf201_hdmi_reg = regulator_get(NULL, "avdd_hdmi");
		if (IS_ERR_OR_NULL(tf201_hdmi_reg)) {
			pr_err("hdmi: couldn't get regulator avdd_hdmi\n");
			tf201_hdmi_reg = NULL;
			return PTR_ERR(tf201_hdmi_reg);
		}
	}
	ret = regulator_enable(tf201_hdmi_reg);
	if (ret < 0) {
		pr_err("hdmi: couldn't enable regulator avdd_hdmi\n");
		return ret;
	}
	if (!tf201_hdmi_pll) {
		tf201_hdmi_pll = regulator_get(NULL, "avdd_hdmi_pll");
		if (IS_ERR_OR_NULL(tf201_hdmi_pll)) {
			pr_err("hdmi: couldn't get regulator avdd_hdmi_pll\n");
			tf201_hdmi_pll = NULL;
			regulator_put(tf201_hdmi_reg);
			tf201_hdmi_reg = NULL;
			return PTR_ERR(tf201_hdmi_pll);
		}
	}
	ret = regulator_enable(tf201_hdmi_pll);
	if (ret < 0) {
		pr_err("hdmi: couldn't enable regulator avdd_hdmi_pll\n");
		return ret;
	}
	return 0;
}

static int tf201_hdmi_disable(void)
{
	regulator_disable(tf201_hdmi_reg);
	regulator_put(tf201_hdmi_reg);
	tf201_hdmi_reg = NULL;

	regulator_disable(tf201_hdmi_pll);
	regulator_put(tf201_hdmi_pll);
	tf201_hdmi_pll = NULL;
	return 0;
}

static struct resource tf201_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= 0,	/* Filled in by tf201_panel_init() */
		.end	= 0,	/* Filled in by tf201_panel_init() */
		.flags	= IORESOURCE_MEM,
	},
#ifdef CONFIG_TEGRA_DSI_INSTANCE_1
	{
		.name	= "dsi_regs",
		.start	= TEGRA_DSIB_BASE,
		.end	= TEGRA_DSIB_BASE + TEGRA_DSIB_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
#else
	{
		.name	= "dsi_regs",
		.start	= TEGRA_DSI_BASE,
		.end	= TEGRA_DSI_BASE + TEGRA_DSI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
#endif
};

static struct resource tf201_disp2_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_B_GENERAL,
		.end	= INT_DISPLAY_B_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY2_BASE,
		.end	= TEGRA_DISPLAY2_BASE + TEGRA_DISPLAY2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.flags	= IORESOURCE_MEM,
		.start	= 0,
		.end	= 0,
	},
	{
		.name	= "hdmi_regs",
		.start	= TEGRA_HDMI_BASE,
		.end	= TEGRA_HDMI_BASE + TEGRA_HDMI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};
#endif

#ifndef CONFIG_TEGRA_TF201_DSI

static struct tegra_dc_mode tf201_panel_modes[] = {
  {
    /* 1366x768@59Hz */
    .pclk = 68000000,
    .h_ref_to_sync = 4,
    .v_ref_to_sync = 2,
    .h_sync_width = 30,
    .v_sync_width = 5,
    .h_back_porch = 18,
    .v_back_porch = 12,
    .h_active = 1280,
    .v_active = 800,
    .h_front_porch = 48,
    .v_front_porch = 3,
  },
};
#endif

static struct tegra_dc_sd_settings tf201_sd_settings = {
	.enable = 1, /* enabled by default. */
	.use_auto_pwm = false,
	.hw_update_delay = 0,
	.bin_width = -1,
	.aggressiveness = 1,
	.phase_in_adjustments = true,
	.use_vid_luma = false,
	/* Default video coefficients */
	.coeff = {5, 9, 2},
	.fc = {0, 0},
	/* Immediate backlight changes */
	.blp = {1024, 255},
	/* Gammas: R: 2.2 G: 2.2 B: 2.2 */
	/* Default BL TF */
	.bltf = {
			{
				{57, 65, 74, 83},
				{93, 103, 114, 126},
				{138, 151, 165, 179},
				{194, 209, 225, 242},
			},
			{
				{58, 66, 75, 84},
				{94, 105, 116, 127},
				{140, 153, 166, 181},
				{196, 211, 227, 244},
			},
			{
				{60, 68, 77, 87},
				{97, 107, 119, 130},
				{143, 156, 170, 184},
				{199, 215, 231, 248},
			},
			{
				{64, 73, 82, 91},
				{102, 113, 124, 137},
				{149, 163, 177, 192},
				{207, 223, 240, 255},
			},
		},
	/* Default LUT */
	.lut = {
			{
				{250, 250, 250},
				{194, 194, 194},
				{149, 149, 149},
				{113, 113, 113},
				{82, 82, 82},
				{56, 56, 56},
				{34, 34, 34},
				{15, 15, 15},
				{0, 0, 0},
			},
			{
				{246, 246, 246},
				{191, 191, 191},
				{147, 147, 147},
				{111, 111, 111},
				{80, 80, 80},
				{55, 55, 55},
				{33, 33, 33},
				{14, 14, 14},
				{0, 0, 0},
			},
			{
				{239, 239, 239},
				{185, 185, 185},
				{142, 142, 142},
				{107, 107, 107},
				{77, 77, 77},
				{52, 52, 52},
				{30, 30, 30},
				{12, 12, 12},
				{0, 0, 0},
			},
			{
				{224, 224, 224},
				{173, 173, 173},
				{133, 133, 133},
				{99, 99, 99},
				{70, 70, 70},
				{46, 46, 46},
				{25, 25, 25},
				{7, 7, 7},
				{0, 0, 0},
			},
		},
	.sd_brightness = &sd_brightness,
	.bl_device = &tf201_backlight_device,
};

#ifdef CONFIG_TEGRA_DC
#ifndef CONFIG_TEGRA_TF201_DSI
static struct tegra_fb_data tf201_fb_data = {
	.win		= 0,
	.xres		= 1280,
	.yres		= 800,
	.bits_per_pixel	= 32,
//	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};
#endif

static struct tegra_fb_data tf201_hdmi_fb_data = {
	.win		= 0,
	.xres		= 1280,
	.yres		= 800,
	.bits_per_pixel	= 32,
//	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_out tf201_disp2_out = {
	.type		= TEGRA_DC_OUT_HDMI,
	.flags		= TEGRA_DC_OUT_HOTPLUG_HIGH,

	.dcc_bus	= 3,
	.hotplug_gpio	= tf201_hdmi_hpd,

	.max_pixclock	= KHZ2PICOS(148500),

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.enable		= tf201_hdmi_enable,
	.disable	= tf201_hdmi_disable,

	.postsuspend	= tf201_hdmi_vddio_disable,
	.hotplug_init	= tf201_hdmi_vddio_enable,
};

static struct tegra_dc_platform_data tf201_disp2_pdata = {
	.flags		= 0,
	.default_out	= &tf201_disp2_out,
	.fb		= &tf201_hdmi_fb_data,
	.emc_clk_rate	= 300000000,
};
#endif

#ifdef CONFIG_TEGRA_TF201_DSI
static int tf201_dsi_panel_enable(void)
{
	int ret;

	if (tf201_dsi_reg == NULL) {
		tf201_dsi_reg = regulator_get(NULL, "avdd_dsi_csi");
		if (IS_ERR_OR_NULL(tf201_dsi_reg)) {
		pr_err("dsi: Could not get regulator avdd_dsi_csi\n");
			tf201_dsi_reg = NULL;
			return PTR_ERR(tf201_dsi_reg);
		}
	}
	regulator_enable(tf201_dsi_reg);

	ret = gpio_request(TEGRA_GPIO_PJ1, "DSI TE");
	if (ret < 0)
		return ret;

	ret = gpio_direction_input(TEGRA_GPIO_PJ1);
	if (ret < 0) {
		gpio_free(TEGRA_GPIO_PJ1);
		return ret;
	}
	tegra_gpio_enable(TEGRA_GPIO_PJ1);

#if DSI_PANEL_219

	ret = gpio_request(TEGRA_GPIO_PH0, "ph0");
	if (ret < 0)
		return ret;
	ret = gpio_direction_output(TEGRA_GPIO_PH0, 0);
	if (ret < 0) {
		gpio_free(TEGRA_GPIO_PH0);
		return ret;
	}
	else
		tegra_gpio_enable(TEGRA_GPIO_PH0);

	ret = gpio_request(TEGRA_GPIO_PH2, "ph2");
	if (ret < 0)
		return ret;
	ret = gpio_direction_output(TEGRA_GPIO_PH2, 0);
	if (ret < 0) {
		gpio_free(TEGRA_GPIO_PH2);
		return ret;
	}
	else
		tegra_gpio_enable(TEGRA_GPIO_PH2);

	ret = gpio_request(TEGRA_GPIO_PU2, "pu2");
	if (ret < 0)
		return ret;
	ret = gpio_direction_output(TEGRA_GPIO_PU2, 0);
	if (ret < 0) {
		gpio_free(TEGRA_GPIO_PU2);
		return ret;
	}
	else
		tegra_gpio_enable(TEGRA_GPIO_PU2);

	gpio_set_value(tf201_lvds_shutdown, 1);
	mdelay(20);
	gpio_set_value(TEGRA_GPIO_PH0, 1);
	mdelay(10);
	gpio_set_value(TEGRA_GPIO_PH2, 1);
	mdelay(15);
	gpio_set_value(TEGRA_GPIO_PU2, 0);
	gpio_set_value(TEGRA_GPIO_PU2, 1);
	mdelay(10);
	gpio_set_value(TEGRA_GPIO_PU2, 0);
	mdelay(10);
	gpio_set_value(TEGRA_GPIO_PU2, 1);
	mdelay(15);
#endif

#if DSI_PANEL_218
	printk("DSI_PANEL_218 is enabled\n");
	ret = gpio_request(AVDD_LCD, "avdd_lcd");
	if(ret < 0)
		gpio_free(AVDD_LCD);
	ret = gpio_direction_output(AVDD_LCD, 1);
	if(ret < 0)
		gpio_free(AVDD_LCD);
	else
		tegra_gpio_enable(AVDD_LCD);

#if DSI_PANEL_RESET
	ret = gpio_request(TEGRA_GPIO_PD2, "pd2");
	if (ret < 0){
		return ret;
	}
	ret = gpio_direction_output(TEGRA_GPIO_PD2, 0);
	if (ret < 0) {
		gpio_free(TEGRA_GPIO_PD2);
		return ret;
	}
	else
		tegra_gpio_enable(TEGRA_GPIO_PD2);

	gpio_set_value(TEGRA_GPIO_PD2, 1);
	gpio_set_value(TEGRA_GPIO_PD2, 0);
	mdelay(2);
	gpio_set_value(TEGRA_GPIO_PD2, 1);
	mdelay(2);
#endif
#endif

	return 0;
}

static int tf201_dsi_panel_disable(void)
{
	int err;

	err = 0;
	printk(KERN_INFO "DSI panel disable\n");

#if DSI_PANEL_219
	tegra_gpio_disable(TEGRA_GPIO_PU2);
	gpio_free(TEGRA_GPIO_PU2);
	tegra_gpio_disable(TEGRA_GPIO_PH2);
	gpio_free(TEGRA_GPIO_PH2);
	tegra_gpio_disable(TEGRA_GPIO_PH0);
	gpio_free(TEGRA_GPIO_PH0);
	tegra_gpio_disable(TEGRA_GPIO_PL2);
	gpio_free(TEGRA_GPIO_PL2);
#endif

#if DSI_PANEL_218
	tegra_gpio_disable(TEGRA_GPIO_PD2);
	gpio_free(TEGRA_GPIO_PD2);
#endif

	return err;
}

static int tf201_dsi_panel_postsuspend(void)
{
	int err;

	err = 0;
	printk(KERN_INFO "DSI panel postsuspend\n");

	if (tf201_dsi_reg) {
		err = regulator_disable(tf201_dsi_reg);
		if (err < 0)
			printk(KERN_ERR
			"DSI regulator avdd_dsi_csi disable failed\n");
		regulator_put(tf201_dsi_reg);
		tf201_dsi_reg = NULL;
	}

#if DSI_PANEL_218
	tegra_gpio_disable(AVDD_LCD);
	gpio_free(AVDD_LCD);
#endif

	return err;
}

static struct tegra_dsi_cmd dsi_init_cmd[]= {
	DSI_CMD_SHORT(0x05, 0x11, 0x00),
	DSI_DLY_MS(150),
	DSI_CMD_SHORT(0x05, 0x29, 0x00),
	DSI_DLY_MS(20),
};

static struct tegra_dsi_cmd dsi_suspend_cmd[] = {
	DSI_CMD_SHORT(0x05, 0x28, 0x00),
	DSI_DLY_MS(20),
	DSI_CMD_SHORT(0x05, 0x10, 0x00),
	DSI_DLY_MS(5),
};

struct tegra_dsi_out tf201_dsi = {
	.n_data_lanes = 2,
	.pixel_format = TEGRA_DSI_PIXEL_FORMAT_24BIT_P,
	.refresh_rate = 60,
	.virtual_channel = TEGRA_DSI_VIRTUAL_CHANNEL_0,

	.panel_has_frame_buffer = true,
#ifdef CONFIG_TEGRA_DSI_INSTANCE_1
	.dsi_instance = 1,
#else
	.dsi_instance = 0,
#endif
	.panel_reset = DSI_PANEL_RESET,

	.n_init_cmd = ARRAY_SIZE(dsi_init_cmd),
	.dsi_init_cmd = dsi_init_cmd,

	.n_suspend_cmd = ARRAY_SIZE(dsi_suspend_cmd),
	.dsi_suspend_cmd = dsi_suspend_cmd,

	.video_data_type = TEGRA_DSI_VIDEO_TYPE_COMMAND_MODE,
	.lp_cmd_mode_freq_khz = 430000,
};

static struct tegra_dc_mode tf201_dsi_modes[] = {
#if DSI_PANEL_219
	{
		.pclk = 10000000,
		.h_ref_to_sync = 4,
		.v_ref_to_sync = 1,
		.h_sync_width = 16,
		.v_sync_width = 1,
		.h_back_porch = 32,
		.v_back_porch = 1,
		.h_active = 540,
		.v_active = 960,
		.h_front_porch = 32,
		.v_front_porch = 2,
	},
#endif

#if DSI_PANEL_218
	{
		.pclk = 323000000,
		.h_ref_to_sync = 11,
		.v_ref_to_sync = 1,
		.h_sync_width = 16,
		.v_sync_width = 4,
		.h_back_porch = 16,
		.v_back_porch = 4,
		.h_active = 864,
		.v_active = 480,
		.h_front_porch = 16,
		.v_front_porch = 4,
	},
#endif

};


static struct tegra_fb_data tf201_dsi_fb_data = {
#if DSI_PANEL_219
	.win		= 0,
	.xres		= 540,
	.yres		= 960,
	.bits_per_pixel	= 32,
#endif

#if DSI_PANEL_218
	.win		= 0,
	.xres		= 864,
	.yres		= 480,
	.bits_per_pixel	= 32,
#endif
	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};
#endif

static struct tegra_dc_out tf201_disp1_out = {
	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,
	.sd_settings	= &tf201_sd_settings,
	.parent_clk	= "pll_p",

#ifndef CONFIG_TEGRA_TF201_DSI
	.type		= TEGRA_DC_OUT_RGB,

	.modes	 	= tf201_panel_modes,
	.n_modes 	= ARRAY_SIZE(tf201_panel_modes),

	.enable		= tf201_panel_enable,
	.disable	= tf201_panel_disable,
#else
	.type		= TEGRA_DC_OUT_DSI,

	.modes	 	= tf201_dsi_modes,
	.n_modes 	= ARRAY_SIZE(tf201_dsi_modes),

	.dsi		= &tf201_dsi,

	.enable		= tf201_dsi_panel_enable,
	.disable	= tf201_dsi_panel_disable,
	.postsuspend	= tf201_dsi_panel_postsuspend,
#endif
};

#ifdef CONFIG_TEGRA_DC
static struct tegra_dc_platform_data tf201_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &tf201_disp1_out,
	.emc_clk_rate	= 300000000,
#ifndef CONFIG_TEGRA_TF201_DSI
	.fb		= &tf201_fb_data,
#else
	.fb		= &tf201_dsi_fb_data,
#endif
};

static struct nvhost_device tf201_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= tf201_disp1_resources,
	.num_resources	= ARRAY_SIZE(tf201_disp1_resources),
	.dev = {
		.platform_data = &tf201_disp1_pdata,
	},
};

static int tf201_disp1_check_fb(struct device *dev, struct fb_info *info)
{
	return info->device == &tf201_disp1_device.dev;
}

static struct nvhost_device tf201_disp2_device = {
	.name		= "tegradc",
	.id		= 1,
	.resource	= tf201_disp2_resources,
	.num_resources	= ARRAY_SIZE(tf201_disp2_resources),
	.dev = {
		.platform_data = &tf201_disp2_pdata,
	},
};
#else
static int tf201_disp1_check_fb(struct device *dev, struct fb_info *info)
{
	return 0;
}
#endif

static struct nvmap_platform_carveout tf201_carveouts[] = {
	[0] = NVMAP_HEAP_CARVEOUT_IRAM_INIT,
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0,	/* Filled in by tf201_panel_init() */
		.size		= 0,	/* Filled in by tf201_panel_init() */
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data tf201_nvmap_data = {
	.carveouts	= tf201_carveouts,
	.nr_carveouts	= ARRAY_SIZE(tf201_carveouts),
};

static struct platform_device tf201_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &tf201_nvmap_data,
	},
};


static struct platform_device *tf201_gfx_devices[] __initdata = {
	&tf201_nvmap_device,
#ifdef CONFIG_TEGRA_GRHOST
	&tegra_grhost_device,
#endif
	&tegra_pwfm0_device,
	&tf201_backlight_device,
};


#ifdef CONFIG_HAS_EARLYSUSPEND
/* put early_suspend/late_resume handlers here for the display in order
 * to keep the code out of the display driver, keeping it closer to upstream
 */
struct early_suspend tf201_panel_early_suspender;

static void tf201_panel_early_suspend(struct early_suspend *h)
{

	/* power down LCD, add use a black screen for HDMI */
	if (num_registered_fb > 0)
		fb_blank(registered_fb[0], FB_BLANK_POWERDOWN);
	if (num_registered_fb > 1)
		fb_blank(registered_fb[1], FB_BLANK_NORMAL);
	printk(KERN_INFO "%sed\n", __func__);
}

static void tf201_panel_late_resume(struct early_suspend *h)
{
	unsigned i;
	for (i = 0; i < num_registered_fb; i++)
		fb_blank(registered_fb[i], FB_BLANK_UNBLANK);
	printk(KERN_INFO "%sd\n", __func__);
}
#endif

int __init tf201_panel_init(void)
{
	int err;
	struct resource __maybe_unused *res;

	tegra_get_board_info(&board_info);
	tegra_get_display_board_info(&display_board_info);

	tf201_carveouts[1].base = tegra_carveout_start;
	tf201_carveouts[1].size = tegra_carveout_size;

	tegra_gpio_enable(tf201_hdmi_enb);
	gpio_request(tf201_hdmi_enb, "hdmi_5v_en");
	gpio_direction_output(tf201_hdmi_enb, 1);

	tegra_gpio_enable(tf201_hdmi_hpd);
	gpio_request(tf201_hdmi_hpd, "hdmi_hpd");
	gpio_direction_input(tf201_hdmi_hpd);

#ifdef CONFIG_HAS_EARLYSUSPEND
	tf201_panel_early_suspender.suspend = tf201_panel_early_suspend;
	tf201_panel_early_suspender.resume = tf201_panel_late_resume;
	tf201_panel_early_suspender.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&tf201_panel_early_suspender);
#endif

	err = platform_add_devices(tf201_gfx_devices,
				ARRAY_SIZE(tf201_gfx_devices));

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	res = nvhost_get_resource_byname(&tf201_disp1_device,
					 IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb_start;
	res->end = tegra_fb_start + tegra_fb_size - 1;
#endif

	/* Copy the bootloader fb to the fb. */
	tegra_move_framebuffer(tegra_fb_start, tegra_bootloader_fb_start,
				min(tegra_fb_size, tegra_bootloader_fb_size));

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	if (!err)
		err = nvhost_device_register(&tf201_disp1_device);

	res = nvhost_get_resource_byname(&tf201_disp2_device,
					 IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb2_start;
	res->end = tegra_fb2_start + tegra_fb2_size - 1;
	if (!err)
		err = nvhost_device_register(&tf201_disp2_device);
#endif

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_NVAVP)
	if (!err)
		err = nvhost_device_register(&nvavp_device);
#endif
	return err;
}
