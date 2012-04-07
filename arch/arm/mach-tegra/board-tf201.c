/*
 * arch/arm/mach-tegra/board-tf201.c
 *
 * Copyright (c) 2011, NVIDIA Corporation.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/i2c/panjit_ts.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/spi/spi.h>
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT)
#include <linux/i2c/atmel_mxt_ts.h>
#endif
#if defined (CONFIG_TOUCHSCREEN_ATMEL_MT_T9)
#include <linux/i2c/atmel_maxtouch.h>
#endif
#include <linux/tegra_uart.h>
#include <linux/memblock.h>
#include <linux/spi-tegra.h>
#include <linux/nfc/pn544.h>

#include <sound/wm8903.h>

#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/i2s.h>
#include <mach/tegra_wm8903_pdata.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <mach/board-tf201-misc.h>
#include <mach/thermal.h>
#include <mach/pci.h>

#include "board.h"
#include "clock.h"
#include "board-tf201.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"
#include "baseband-xmm-power.h"
#include "wdt-recovery.h"

/* All units are in millicelsius */
static struct tegra_thermal_data thermal_data = {
	.temp_throttle = 85000,
	.temp_shutdown = 90000,
	.temp_offset = TDIODE_OFFSET, /* temps based on tdiode */
#ifdef CONFIG_TEGRA_EDP_LIMITS
	.edp_offset = TDIODE_OFFSET,  /* edp based on tdiode */
	.hysteresis_edp = 3000,
#endif
#ifdef CONFIG_TEGRA_THERMAL_SYSFS
	.tc1 = 0,
	.tc2 = 1,
	.passive_delay = 2000,
#else
	.hysteresis_throttle = 1000,
#endif
};

/* !!!TODO: Change for tf201 (Taken from Ventana) */
static struct tegra_utmip_config utmi_phy_config[] = {
	[0] = {
			.hssync_start_delay = 0,
			.idle_wait_delay = 17,
			.elastic_limit = 16,
			.term_range_adj = 6,
			.xcvr_setup = 15,
			.xcvr_setup_offset = 0,
			.xcvr_use_fuses = 1,
			.xcvr_lsfslew = 2,
			.xcvr_lsrslew = 2,
	},
	[1] = {
			.hssync_start_delay = 0,
			.idle_wait_delay = 17,
			.elastic_limit = 16,
			.term_range_adj = 6,
			.xcvr_setup = 15,
			.xcvr_setup_offset = 0,
			.xcvr_use_fuses = 1,
			.xcvr_lsfslew = 2,
			.xcvr_lsrslew = 2,
	},
	[2] = {
			.hssync_start_delay = 0,
			.idle_wait_delay = 17,
			.elastic_limit = 16,
			.term_range_adj = 6,
			.xcvr_setup = 8,
			.xcvr_setup_offset = 0,
			.xcvr_use_fuses = 1,
			.xcvr_lsfslew = 2,
			.xcvr_lsrslew = 2,
	},
};

static struct resource tf201_bcm4329_rfkill_resources[] = {
	{
		.name   = "bcm4329_nshutdown_gpio",
		.start  = TEGRA_GPIO_PU0,
		.end    = TEGRA_GPIO_PU0,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device tf201_bcm4329_rfkill_device = {
	.name = "bcm4329_rfkill",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(tf201_bcm4329_rfkill_resources),
	.resource       = tf201_bcm4329_rfkill_resources,
};

static struct resource tf201_bluesleep_resources[] = {
	[0] = {
		.name = "gpio_host_wake",
			.start  = TEGRA_GPIO_PU6,
			.end    = TEGRA_GPIO_PU6,
			.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "gpio_ext_wake",
			.start  = TEGRA_GPIO_PU1,
			.end    = TEGRA_GPIO_PU1,
			.flags  = IORESOURCE_IO,
	},
	[2] = {
		.name = "host_wake",
			.start  = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PU6),
			.end    = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PU6),
			.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct platform_device tf201_bluesleep_device = {
	.name           = "bluesleep",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(tf201_bluesleep_resources),
	.resource       = tf201_bluesleep_resources,
};

static noinline void __init tf201_setup_bluesleep(void)
{
	platform_device_register(&tf201_bluesleep_device);
	tegra_gpio_enable(TEGRA_GPIO_PU6);
	tegra_gpio_enable(TEGRA_GPIO_PU1);
	return;
}

static __initdata struct tegra_clk_init_table tf201_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x","pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	3187500,	false},
	{ "blink",	"clk_32k",	32768,		true},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"pll_a_out0",	0,		false},
	{ "dam0",	"pll_a_out0",	0,		false},
	{ "dam1",	"pll_a_out0",	0,		false},
	{ "dam2",	"pll_a_out0",	0,		false},
	{ "vi_sensor",	"pll_m",	150000000,	false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	{ "i2c1",	"pll_p",	3200000,	false},
	{ "i2c2",	"pll_p",	3200000,	false},
	{ "i2c3",	"pll_p",	3200000,	false},
	{ "i2c4",	"pll_p",	3200000,	false},
	{ "i2c5",	"pll_p",	3200000,	false},
	{ NULL,		NULL,		0,		0},
};

static struct pn544_i2c_platform_data nfc_pdata = {
	.irq_gpio = TEGRA_GPIO_PX0,
	.ven_gpio = TEGRA_GPIO_PP3,
	.firm_gpio = TEGRA_GPIO_PO7,
	};

static struct i2c_board_info __initdata tf201_i2c_bus3_board_info[] = {
	{
		I2C_BOARD_INFO("pn544", 0x28),
		.platform_data = &nfc_pdata,
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PX0),
	},
};
static struct tegra_i2c_platform_data tf201_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PC4, 0},
	.sda_gpio		= {TEGRA_GPIO_PC5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data tf201_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.is_clkon_always = true,
	.scl_gpio		= {TEGRA_GPIO_PT5, 0},
	.sda_gpio		= {TEGRA_GPIO_PT6, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data tf201_i2c3_platform_data = {
	.adapter_nr	= 2,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PBB1, 0},
	.sda_gpio		= {TEGRA_GPIO_PBB2, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data tf201_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 93750, 0 },
	.scl_gpio		= {TEGRA_GPIO_PV4, 0},
	.sda_gpio		= {TEGRA_GPIO_PV5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data tf201_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PZ6, 0},
	.sda_gpio		= {TEGRA_GPIO_PZ7, 0},
	.arb_recovery = arb_lost_recovery,
};


#if 0
struct tegra_wired_jack_conf audio_wr_jack_conf = {
	.hp_det_n = TEGRA_GPIO_PW2,
	.en_mic_ext = TEGRA_GPIO_PX1,
	.en_mic_int = TEGRA_GPIO_PX0,
};
#endif

#ifdef CONFIG_DSP_FM34
static const struct i2c_board_info tf201_dsp_board_info[] = {
	{
		I2C_BOARD_INFO("dsp_fm34", 0x60),
	},
};
#endif

static struct i2c_board_info __initdata rt5631_board_info = {
	I2C_BOARD_INFO("rt5631", 0x1a),
};

static struct i2c_board_info __initdata tf201_i2c_asuspec_info[] = {
	{
		I2C_BOARD_INFO("asuspec", 0x15),
	},
	{
		I2C_BOARD_INFO("asusdec", 0x19),
	},
};




static void tf201_i2c_init(void)
{
	tegra_i2c_device1.dev.platform_data = &tf201_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &tf201_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &tf201_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &tf201_i2c4_platform_data;
	tegra_i2c_device5.dev.platform_data = &tf201_i2c5_platform_data;

	platform_device_register(&tegra_i2c_device5);
	platform_device_register(&tegra_i2c_device4);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device1);

	i2c_register_board_info(2, tf201_i2c_bus3_board_info, 1);
	i2c_register_board_info(1, tf201_i2c_asuspec_info, ARRAY_SIZE(tf201_i2c_asuspec_info));
	i2c_register_board_info(4, &rt5631_board_info, 1);
#ifdef CONFIG_DSP_FM34
	i2c_register_board_info(0, tf201_dsp_board_info, 1);
#endif
}

static struct platform_device *tf201_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
	&tegra_uarte_device,
};
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "clk_m"},
	[1] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[2] = {.name = "pll_m"},
#endif
};

static struct tegra_uart_platform_data tf201_uart_pdata;

static void __init tf201_uart_init(void)
{
	struct clk *c;
	int i;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	tf201_uart_pdata.parent_clk_list = uart_parent_clk;
	tf201_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	tegra_uarta_device.dev.platform_data = &tf201_uart_pdata;
	tegra_uartb_device.dev.platform_data = &tf201_uart_pdata;
	tegra_uartc_device.dev.platform_data = &tf201_uart_pdata;
	tegra_uartd_device.dev.platform_data = &tf201_uart_pdata;
	tegra_uarte_device.dev.platform_data = &tf201_uart_pdata;

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs()) {
		/* Clock enable for the debug channel */
		if (!IS_ERR_OR_NULL(debug_uart_clk)) {
			pr_info("The debug console clock name is %s\n",
						debug_uart_clk->name);
			c = tegra_get_clock_by_name("pll_p");
			if (IS_ERR_OR_NULL(c))
				pr_err("Not getting the parent clock pll_p\n");
			else
				clk_set_parent(debug_uart_clk, c);

			clk_enable(debug_uart_clk);
			clk_set_rate(debug_uart_clk, clk_get_rate(c));
		} else {
			pr_err("Not getting the clock %s for debug console\n",
					debug_uart_clk->name);
		}
	}

	platform_add_devices(tf201_uart_devices,
				ARRAY_SIZE(tf201_uart_devices));
}

static struct platform_device tegra_camera = {
	.name = "tegra_camera",
	.id = -1,
};

static struct platform_device *tf201_spi_devices[] __initdata = {
	&tegra_spi_device4,
};

struct spi_clk_parent spi_parent_clk[] = {
	[0] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[1] = {.name = "pll_m"},
	[2] = {.name = "clk_m"},
#else
	[1] = {.name = "clk_m"},
#endif
};

static struct tegra_spi_platform_data tf201_spi_pdata = {
	.is_dma_based		= true,
	.max_dma_buffer		= (16 * 1024),
	.is_clkon_always	= false,
	.max_rate		= 100000000,
};

static void __init tf201_spi_init(void)
{
	int i;
	struct clk *c;

	for (i = 0; i < ARRAY_SIZE(spi_parent_clk); ++i) {
		c = tegra_get_clock_by_name(spi_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						spi_parent_clk[i].name);
			continue;
		}
		spi_parent_clk[i].parent_clk = c;
		spi_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	tf201_spi_pdata.parent_clk_list = spi_parent_clk;
	tf201_spi_pdata.parent_clk_count = ARRAY_SIZE(spi_parent_clk);
	tegra_spi_device4.dev.platform_data = &tf201_spi_pdata;
	platform_add_devices(tf201_spi_devices,
				ARRAY_SIZE(tf201_spi_devices));
}

static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};

static struct tegra_wm8903_platform_data tf201_audio_pdata = {
	.gpio_spkr_en		= TEGRA_GPIO_SPKR_EN,
	.gpio_hp_det		= TEGRA_GPIO_HP_DET,
	.gpio_hp_mute		= -1,
	.gpio_int_mic_en	= -1,
	.gpio_ext_mic_en	= -1,
};

static struct platform_device tf201_audio_device = {
	.name	= "tegra-snd-rt5631",
	.id	= 0,
	.dev	= {
		.platform_data  = &tf201_audio_pdata,
	},
};

static struct resource ram_console_resources[] = {
	{
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name 		= "ram_console",
	.id 		= -1,
	.num_resources	= ARRAY_SIZE(ram_console_resources),
	.resource	= ram_console_resources,
};

static struct platform_device *tf201_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_IOVMM_SMMU)
	&tegra_smmu_device,
#endif
	&tegra_wdt_device,
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
	&tegra_camera,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device1,
	&tegra_i2s_device3,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tf201_bcm4329_rfkill_device,
	&tegra_pcm_device,
	&tf201_audio_device,
	&tegra_hda_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
	&ram_console_device,
};
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT)
#define MXT_CONFIG_CRC  0xD62DE8
static const u8 config[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0x32, 0x0A, 0x00, 0x14, 0x14, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x00,
	0x1B, 0x2A, 0x00, 0x20, 0x3C, 0x04, 0x05, 0x00,
	0x02, 0x01, 0x00, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF,
	0x02, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x64, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
	0x00, 0x00, 0x00, 0x05, 0x0A, 0x15, 0x1E, 0x00,
	0x00, 0x04, 0xFF, 0x03, 0x3F, 0x64, 0x64, 0x01,
	0x0A, 0x14, 0x28, 0x4B, 0x00, 0x02, 0x00, 0x64,
	0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x10, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define MXT_CONFIG_CRC_SKU2000  0xA24D9A
static const u8 config_sku2000[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0x32, 0x0A, 0x00, 0x14, 0x14, 0x19,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x00,
	0x1B, 0x2A, 0x00, 0x20, 0x3A, 0x04, 0x05, 0x00,  //23=thr  2 di
	0x04, 0x04, 0x41, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF,
	0x02, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00,  //0A=limit
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
	0x00, 0x00, 0x00, 0x05, 0x0A, 0x15, 0x1E, 0x00,
	0x00, 0x04, 0x00, 0x03, 0x3F, 0x64, 0x64, 0x01,
	0x0A, 0x14, 0x28, 0x4B, 0x00, 0x02, 0x00, 0x64,
	0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x10, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static struct mxt_platform_data atmel_mxt_info = {
	.x_line         = 27,
	.y_line         = 42,
	.x_size         = 768,
	.y_size         = 1366,
	.blen           = 0x20,
	.threshold      = 0x3C,
	.voltage        = 3300000,              /* 3.3V */
	.orient         = 5,
	.config         = config,
	.config_length  = 157,
	.config_crc     = MXT_CONFIG_CRC,
	.irqflags       = IRQF_TRIGGER_FALLING,
/*	.read_chg       = &read_chg, */
	.read_chg       = NULL,
};

static struct i2c_board_info __initdata atmel_i2c_info[] = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x5A),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PH4),
		.platform_data = &atmel_mxt_info,
	}
};

#endif

#if defined (CONFIG_TOUCHSCREEN_ATMEL_MT_T9)
static u8 read_chg(void)
{
	return gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9);
}

static u8 valid_interrupt(void)
{
	return !read_chg();
}

static struct mxt_platform_data atmel_mxt_info = {
	/* Maximum number of simultaneous touches to report. */
	.numtouch = 10,
	/* TODO: no need for any hw-specific things at init/exit? */
	.init_platform_hw = NULL,
	.exit_platform_hw = NULL,
	.max_x = 1600,
	.max_y = 1000,
	.valid_interrupt = &valid_interrupt,
	.read_chg = &read_chg,
};

static struct i2c_board_info __initdata atmel_i2c_info[] = {
	{
		I2C_BOARD_INFO("maXTouch", MXT_I2C_ADDRESS),
		.irq = TEGRA_GPIO_TO_IRQ(TOUCH_GPIO_IRQ_ATMEL_T9),
		.platform_data = &atmel_mxt_info,
	}
};
#endif

#if defined(CONFIG_TOUCHSCREEN_ELAN_TF_3K)
// Interrupt pin: TEGRA_GPIO_PH4
// Reset pin: TEGRA_GPIO_PH6
// Power pin:

#include <linux/i2c/ektf3k.h>

struct elan_ktf3k_i2c_platform_data ts_elan_ktf3k_data[] = {
        {
                .version = 0x0001,
		   .abs_x_min = 0,
                .abs_x_max = ELAN_X_MAX,   //LG 9.7" Dpin 2368, Spin 2112
                .abs_y_min = 0,
                .abs_y_max = ELAN_Y_MAX,   //LG 9.7" Dpin 1728, Spin 1600
                .intr_gpio = TEGRA_GPIO_PH4,
        },
};
#endif

static int __init tf201_touch_init(void)
{

	tegra_gpio_enable(TEGRA_GPIO_PH4);
	tegra_gpio_enable(TEGRA_GPIO_PH6);

	gpio_request(TEGRA_GPIO_PH4, "atmel-irq");
	gpio_direction_input(TEGRA_GPIO_PH4);

	gpio_request(TEGRA_GPIO_PH6, "atmel-reset");
	gpio_direction_output(TEGRA_GPIO_PH6, 0);
	msleep(1);
	gpio_set_value(TEGRA_GPIO_PH6, 1);
	msleep(100);

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT)
		atmel_mxt_info.config = config_sku2000;
		atmel_mxt_info.config_crc = MXT_CONFIG_CRC_SKU2000;
#endif
	i2c_register_board_info(TOUCH_BUS_ATMEL_T9, atmel_i2c_info, 1);
	return 0;
}

static struct tegra_ehci_platform_data tegra_ehci_pdata[] = {
	[0] = {
			.phy_config = &utmi_phy_config[0],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
	},
	[1] = {
			.phy_config = &utmi_phy_config[1],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
	},
	[2] = {
			.phy_config = &utmi_phy_config[2],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
			.hotplug = 1,
	},
};

static struct tegra_otg_platform_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci_pdata[0],
};

#ifdef CONFIG_USB_SUPPORT
static struct usb_phy_plat_data tegra_usb_phy_pdata[] = {
	[0] = {
			.instance = 0,
			.vbus_gpio = -1,
// Do not use .vbus_reg_supply (This will use GPIO_PI4.) because unused GMI_RST_N(TEGRA_GPIO_PI4) pin by hardware
//			.vbus_reg_supply = "vdd_vbus_micro_usb",
	},
	[1] = {
			.instance = 1,
			.vbus_gpio = -1,
	},
	[2] = {
			.instance = 2,
			.vbus_gpio = -1,
// Do not use .vbus_reg_supply (This will use GPIO_PH7.) because 5V is controlled by the hardware.
//			.vbus_reg_supply = "vdd_vbus_typea_usb",
	},
};

static void tf201_usb_init(void)
{
	tegra_usb_phy_init(tegra_usb_phy_pdata,
			ARRAY_SIZE(tegra_usb_phy_pdata));

	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	tegra_ehci3_device.dev.platform_data = &tegra_ehci_pdata[2];
	platform_device_register(&tegra_ehci3_device);
}
#else
static void tf201_usb_init(void) { }
#endif

static void tf201_gps_init(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PU2);
	tegra_gpio_enable(TEGRA_GPIO_PU3);
}

static void tf201_nfc_init(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PX0);
	tegra_gpio_enable(TEGRA_GPIO_PP3);
	tegra_gpio_enable(TEGRA_GPIO_PO7);
}

static struct tegra_pci_platform_data tf201_pci_platform_data = {
	.port_status[0]	= 1,
	.port_status[1]	= 1,
	.port_status[2]	= 1,
	.use_dock_detect	= 0,
	.gpio		= 0,
};

static void tf201_pci_init(void)
{
	tegra_pci_device.dev.platform_data = &tf201_pci_platform_data;
	platform_device_register(&tegra_pci_device);
}


#ifdef CONFIG_SATA_AHCI_TEGRA
static void tf201_sata_init(void)
{
	platform_device_register(&tegra_sata_device);
}
#else
static void tf201_sata_init(void) { }
#endif

extern void tegra_booting_info(void );
static void __init tegra_tf201_init(void)
{
	tegra_thermal_init(&thermal_data);
	tegra_clk_init_from_table(tf201_clk_init_table);
	tf201_pinmux_init();
	tf201_misc_init();
       tegra_booting_info( );
	tf201_i2c_init();
	tf201_spi_init();
	tf201_usb_init();
#ifdef CONFIG_TEGRA_EDP_LIMITS
	tf201_edp_init();
#endif
	tf201_uart_init();
	snprintf(tf201_chipid, sizeof(tf201_chipid), "%016llx",
		tegra_chip_uid());
	tf201_tsensor_init();
	platform_add_devices(tf201_devices, ARRAY_SIZE(tf201_devices));
	tf201_sdhci_init();
	tf201_regulator_init();
	tf201_gpio_switch_regulator_init();
	tf201_suspend_init();
	tf201_power_off_init();
	tf201_touch_init();
	tf201_gps_init();
	tf201_kbc_init();
	tf201_scroll_init();
	tf201_keys_init();
	tf201_panel_init();
	tf201_pmon_init();
	tf201_sensors_init();
	tf201_setup_bluesleep();
	tf201_sata_init();
	//audio_wired_jack_init();
	tf201_pins_state_init();
	tf201_emc_init();
	tegra_release_bootloader_fb();
	tf201_nfc_init();
	tf201_pci_init();
#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif
}

static void __init tf201_ramconsole_reserve(unsigned long size)
{
	struct resource *res;
	long ret;

	res = platform_get_resource(&ram_console_device, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Failed to find memory resource for ram console\n");
		return;
	}
	res->start = memblock_end_of_DRAM() - size;
	res->end = res->start + size - 1;
	ret = memblock_remove(res->start, size);
	if (ret) {
		ram_console_device.resource = NULL;
		ram_console_device.num_resources = 0;
		pr_err("Failed to reserve memory block for ram console\n");
	}
}

static void __init tegra_tf201_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	/* support 1920X1200 with 24bpp */
	tegra_reserve(0, SZ_8M + SZ_1M, SZ_8M + SZ_1M);
#else
	tegra_reserve(SZ_128M, SZ_8M, SZ_8M);
#endif
	tf201_ramconsole_reserve(SZ_1M);
}

// Has to be cardhu to match bootloader.
// This board is NOT cardhu, but derived from it.
MACHINE_START(CARDHU, "cardhu")
	.boot_params    = 0x80000100,
	.map_io         = tegra_map_common_io,
	.reserve        = tegra_tf201_reserve,
	.init_early	= tegra_init_early,
	.init_irq       = tegra_init_irq,
	.timer          = &tegra_timer,
	.init_machine   = tegra_tf201_init,
MACHINE_END
