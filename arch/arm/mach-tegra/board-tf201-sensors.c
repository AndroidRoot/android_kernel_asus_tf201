/*
 * arch/arm/mach-tegra/board-tf201-sensors.c
 *
 * Copyright (c) 2010-2011, NVIDIA CORPORATION, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of NVIDIA CORPORATION nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_I2C_MUX_PCA954x
#include <linux/i2c/pca954x.h>
#endif
#include <linux/i2c/pca953x.h>
#include <linux/nct1008.h>
#include <mach/fb.h>
#include <mach/gpio.h>
#ifdef CONFIG_VIDEO_OV5650
#include <media/ov5650.h>
#include <media/ov14810.h>
#endif
#ifdef CONFIG_VIDEO_OV2710
#include <media/ov2710.h>
#endif
#ifdef CONFIG_VIDEO_YUV
#include <media/yuv_sensor.h>
#endif /* CONFIG_VIDEO_YUV */
#include <media/tps61050.h>
#include <generated/mach-types.h>
#include "gpio-names.h"
#include "board.h"
#include <linux/mpu.h>
#ifdef CONFIG_VIDEO_SH532U
#include <media/sh532u.h>
#endif
#include <linux/bq27x00.h>
#include <mach/gpio.h>
#include <mach/edp.h>
#include <mach/thermal.h>

#include "gpio-names.h"
#include "board-tf201.h"
#include "cpu-tegra.h"

#include <mach/board-tf201-misc.h>

#if 0 //WK: Disable NV's camera code
static struct regulator *tf201_1v8_cam1 = NULL;
static struct regulator *tf201_1v8_cam2 = NULL;
static struct regulator *tf201_1v8_cam3 = NULL;
static struct regulator *tf201_vdd_2v8_cam1 = NULL;
static struct regulator *tf201_vdd_2v8_cam2 = NULL;
static struct regulator *tf201_vdd_cam3 = NULL;
#endif

static struct board_info board_info;
static struct regulator *reg_tf201_cam;	/* LDO6 */
static struct regulator *reg_tf201_1v8_cam;	/* VDDIO_CAM 1.8V PBB4 */
static struct regulator *reg_tf201_2v85_cam;	/* Front Camera 2.85V power */
static bool camera_busy = false;

#ifdef CONFIG_I2C_MUX_PCA954x
static struct pca954x_platform_mode tf201_pca954x_modes[] = {
	{ .adap_id = PCA954x_I2C_BUS0, .deselect_on_exit = true, },
	{ .adap_id = PCA954x_I2C_BUS1, .deselect_on_exit = true, },
	{ .adap_id = PCA954x_I2C_BUS2, .deselect_on_exit = true, },
	{ .adap_id = PCA954x_I2C_BUS3, .deselect_on_exit = true, },
};

static struct pca954x_platform_data tf201_pca954x_data = {
	.modes    = tf201_pca954x_modes,
	.num_modes      = ARRAY_SIZE(tf201_pca954x_modes),
};
#endif

static int tf201_camera_init(void)
{
    return 0;
}

#ifdef CONFIG_VIDEO_YUV
static int IsTF200XorTF200XG(void)
{
        return 0;
}

static int yuv_sensor_power_on(void)
{
    int ret;

    printk("yuv_sensor_power_on+\n");

    if(camera_busy){
        printk("yuv_sensor busy\n");
        return -EBUSY;
    }
    camera_busy = true;
    tegra_gpio_enable(143);
    gpio_request(143, "gpio_pr7");
    gpio_direction_output(143, 1);
    pr_info("gpio 2.85V %d set to %d\n",143, gpio_get_value(143));
    gpio_free(143);

    pr_info("gpio %d set to %d\n",ISP_POWER_1V2_EN_GPIO, gpio_get_value(ISP_POWER_1V2_EN_GPIO));
    tegra_gpio_enable(ISP_POWER_1V2_EN_GPIO);
    ret = gpio_request(ISP_POWER_1V2_EN_GPIO, "isp_power_1v2_en");
    if (ret < 0)
        pr_err("%s: gpio_request failed for gpio %s\n",
        __func__, "ISP_POWER_1V2_EN_GPIO");
    gpio_direction_output(ISP_POWER_1V2_EN_GPIO, 1);
    pr_info("gpio %d set to %d\n",ISP_POWER_1V2_EN_GPIO, gpio_get_value(ISP_POWER_1V2_EN_GPIO));

    msleep(5);

    if (!reg_tf201_1v8_cam) {
        reg_tf201_1v8_cam = regulator_get(NULL, "vdd_1v8_cam1");
        if (IS_ERR_OR_NULL(reg_tf201_1v8_cam)) {
            pr_err("TF201_m6mo_power_on PBB4: vdd_1v8_cam1 failed\n");
            reg_tf201_1v8_cam = NULL;
            return PTR_ERR(reg_tf201_1v8_cam);
        }
        regulator_set_voltage(reg_tf201_1v8_cam, 1800000, 1800000);
        regulator_enable(reg_tf201_1v8_cam);
    }

    if (!reg_tf201_cam) {
        reg_tf201_cam = regulator_get(NULL, "avdd_dsi_csi");
        if (IS_ERR_OR_NULL(reg_tf201_cam)) {
            pr_err("TF201_m6mo_power_on LDO6: p_tegra_cam failed\n");
            reg_tf201_cam = NULL;
            return PTR_ERR(reg_tf201_cam);
        }
        regulator_set_voltage(reg_tf201_cam, 1200000, 1200000);
        regulator_enable(reg_tf201_cam);
    }

    return 0;
}
int yuv_sensor_power_on_reset_pin(void)
{
    int ret;

    pr_info("gpio %d set to %d\n",ISP_POWER_RESET_GPIO, gpio_get_value(ISP_POWER_RESET_GPIO));
    tegra_gpio_enable(ISP_POWER_RESET_GPIO);
    ret = gpio_request(ISP_POWER_RESET_GPIO, "isp_power_rstx");
    if (ret < 0)
        pr_err("%s: gpio_request failed for gpio %s\n",
            __func__, "ISP_POWER_RESET_GPIO");
    gpio_direction_output(ISP_POWER_RESET_GPIO, 1);
    pr_info("gpio %d set to %d\n",ISP_POWER_RESET_GPIO, gpio_get_value(ISP_POWER_RESET_GPIO));

    printk("yuv_sensor_power_on -\n");
    return ret;
}

static int yuv_sensor_power_off(void)
{
    if(reg_tf201_cam){
        regulator_disable(reg_tf201_cam);
        regulator_put(reg_tf201_cam);
        reg_tf201_cam = NULL;
    }

    if(reg_tf201_1v8_cam){
        regulator_disable(reg_tf201_1v8_cam);
        regulator_put(reg_tf201_1v8_cam);
        reg_tf201_1v8_cam = NULL;
    }

    gpio_direction_output(ISP_POWER_1V2_EN_GPIO, 0);

    pr_info("gpio %d set to %d\n",ISP_POWER_1V2_EN_GPIO, gpio_get_value(ISP_POWER_1V2_EN_GPIO));
    gpio_free(ISP_POWER_1V2_EN_GPIO);
    printk("yuv_sensor_power_off-\n");
    return 0;
}

int yuv_sensor_power_off_reset_pin(void)
{
    printk("yuv_sensor_power_off+\n");
    camera_busy = false;
    gpio_direction_output(ISP_POWER_RESET_GPIO, 0);
    pr_info("gpio %d set to %d\n",ISP_POWER_RESET_GPIO, gpio_get_value(ISP_POWER_RESET_GPIO));
    gpio_free(ISP_POWER_RESET_GPIO);
    return 0;
}

struct yuv_sensor_platform_data yuv_rear_sensor_data = {
    .power_on = yuv_sensor_power_on,
    .power_off = yuv_sensor_power_off,
};

/*1.2M Camera Reset */
#define FRONT_YUV_SENSOR_RST_GPIO		TEGRA_GPIO_PO0

static int yuv_front_sensor_power_on(void)
{
	int ret;
	printk("yuv_front_sensor_power_on+\n");

	if(camera_busy){
		printk("yuv_sensor busy\n");
		return -EBUSY;
	}
	camera_busy = true;
	/* 1.8V VDDIO_CAM controlled by "EN_1V8_CAM(GPIO_PBB4)" */
	if (!reg_tf201_1v8_cam) {
		reg_tf201_1v8_cam = regulator_get(NULL, "vdd_1v8_cam1"); /*cam2/3?*/
		if (IS_ERR_OR_NULL(reg_tf201_1v8_cam)) {
			reg_tf201_1v8_cam = NULL;
			pr_err("Can't get reg_tf201_1v8_cam.\n");
			goto fail_to_get_reg;
		}
		regulator_set_voltage(reg_tf201_1v8_cam, 1800000, 1800000);
		regulator_enable(reg_tf201_1v8_cam);
	}

  if (IsTF200XorTF200XG())
  {
    //VDD_CAM1_ldo
    pr_info("gpio %d read as %d\n",CAM1_LDO_EN_GPIO, gpio_get_value(CAM1_LDO_EN_GPIO));
    tegra_gpio_enable(CAM1_LDO_EN_GPIO);
    ret = gpio_request(CAM1_LDO_EN_GPIO, "cam1_ldo_en");
    if (ret < 0)
        pr_err("%s: gpio_request failed for gpio %s\n",
        __func__, "CAM1_LDO_EN_GPIO");
    gpio_direction_output(CAM1_LDO_EN_GPIO, 1);
    pr_info("gpio %d set to %d\n",CAM1_LDO_EN_GPIO, gpio_get_value(CAM1_LDO_EN_GPIO));
  }
  else
  {
  	/* 2.85V VDD_CAM2 controlled by CAM2/3_LDO_EN(GPIO_PS0)*/
  	if (!reg_tf201_2v85_cam) {
  		reg_tf201_2v85_cam = regulator_get(NULL, "vdd_cam3");
  		if (IS_ERR_OR_NULL(reg_tf201_2v85_cam)) {
  			reg_tf201_2v85_cam = NULL;
  			pr_err("Can't get reg_tf201_2v85_cam.\n");
  			goto fail_to_get_reg;
  		}
  		regulator_set_voltage(reg_tf201_2v85_cam, 2850000, 2850000);
  		regulator_enable(reg_tf201_2v85_cam);
  	}
  }
	/* cam_power_en, powdn*/
	tegra_gpio_enable(CAM3_POWER_DWN_GPIO);
	ret = gpio_request(CAM3_POWER_DWN_GPIO, "cam3_power_dwn");
	if(ret == -EBUSY)
		printk("%s: gpio %s has been requested?\n", __func__, "CAM3_POWER_DWN_GPIO");
	else if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s, ret= %d\n",
			__func__, "CAM3_POWER_DWN_GPIO", ret);
		goto fail_to_request_gpio;
	}
	pr_info("gpio %d: %d",CAM3_POWER_DWN_GPIO, gpio_get_value(CAM3_POWER_DWN_GPIO));
	gpio_set_value(CAM3_POWER_DWN_GPIO, 0);
	gpio_direction_output(CAM3_POWER_DWN_GPIO, 0);
	pr_info("--> %d\n", gpio_get_value(CAM3_POWER_DWN_GPIO));

	/* yuv_sensor_rst_lo*/
	tegra_gpio_enable(FRONT_YUV_SENSOR_RST_GPIO);
	ret = gpio_request(FRONT_YUV_SENSOR_RST_GPIO, "yuv_sensor_rst_lo");
	if(ret == -EBUSY)
		printk("%s: gpio %s has been requested?\n", __func__, "FRONT_YUV_SENSOR_RST_GPIO");
	else if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s, ret= %d\n",
			__func__, "FRONT_YUV_SENSOR_RST_GPIO", ret);
		goto fail_to_request_gpio;
	}
	pr_info("gpio %d: %d", FRONT_YUV_SENSOR_RST_GPIO, gpio_get_value(FRONT_YUV_SENSOR_RST_GPIO));
	gpio_set_value(FRONT_YUV_SENSOR_RST_GPIO, 1);
	gpio_direction_output(FRONT_YUV_SENSOR_RST_GPIO, 1);
	pr_info("--> %d\n", gpio_get_value(FRONT_YUV_SENSOR_RST_GPIO));

	printk("yuv_front_sensor_power_on-\n");
	return 0;

fail_to_request_gpio:
	gpio_free(FRONT_YUV_SENSOR_RST_GPIO);
	gpio_free(CAM3_POWER_DWN_GPIO);

fail_to_get_reg:
	if (reg_tf201_2v85_cam) {
		regulator_put(reg_tf201_2v85_cam);
		reg_tf201_2v85_cam = NULL;
	}
	if (reg_tf201_1v8_cam) {
		regulator_put(reg_tf201_1v8_cam);
		reg_tf201_1v8_cam = NULL;
	}

	camera_busy = false;
	printk("yuv_front_sensor_power_on- : -ENODEV\n");
	return -ENODEV;
}

static int yuv_front_sensor_power_off(void)
{
	printk("yuv_front_sensor_power_off+\n");

	gpio_set_value(FRONT_YUV_SENSOR_RST_GPIO, 0);
	gpio_direction_output(FRONT_YUV_SENSOR_RST_GPIO, 0);
	gpio_free(FRONT_YUV_SENSOR_RST_GPIO);

	gpio_set_value(CAM3_POWER_DWN_GPIO, 1);
	gpio_direction_output(CAM3_POWER_DWN_GPIO, 1);
	gpio_free(CAM3_POWER_DWN_GPIO);

  if (IsTF200XorTF200XG()) {  //VDD_CAM1_ldo
  	gpio_set_value(CAM1_LDO_EN_GPIO, 0);
  	gpio_direction_output(CAM1_LDO_EN_GPIO, 0);
  	gpio_free(CAM1_LDO_EN_GPIO);
  }
	if (reg_tf201_2v85_cam) {
		regulator_disable(reg_tf201_2v85_cam);
		regulator_put(reg_tf201_2v85_cam);
		reg_tf201_2v85_cam = NULL;
	}
	if (reg_tf201_1v8_cam) {
		regulator_disable(reg_tf201_1v8_cam);
		regulator_put(reg_tf201_1v8_cam);
		reg_tf201_1v8_cam = NULL;
	}

	camera_busy = false;
	printk("yuv_front_sensor_power_off-\n");
	return 0;
}

#define OV5640_RST_GPIO TEGRA_GPIO_PBB0
#define OV5640_PWR_DN_GPIO TEGRA_GPIO_PBB5
#define OV5640_AF_PWR_DN_GPIO TEGRA_GPIO_PBB3

static int ov5640_power_on(void)
{
    int ret;

    printk("ov5640_power_on+\n");

    if (!reg_tf201_1v8_cam) {
        reg_tf201_1v8_cam = regulator_get(NULL, "vdd_1v8_cam1");
        if (IS_ERR_OR_NULL(reg_tf201_1v8_cam)) {
            pr_err("TF201_m6mo_power_on PBB4: vdd_1v8_cam1 failed\n");
            reg_tf201_1v8_cam = NULL;
            return PTR_ERR(reg_tf201_1v8_cam);
        }
        regulator_set_voltage(reg_tf201_1v8_cam, 1800000, 1800000);
        regulator_enable(reg_tf201_1v8_cam);
    }

    //VDD_CAM1_ldo
    pr_info("gpio %d read as %d\n",CAM1_LDO_EN_GPIO, gpio_get_value(CAM1_LDO_EN_GPIO));
    tegra_gpio_enable(CAM1_LDO_EN_GPIO);
    ret = gpio_request(CAM1_LDO_EN_GPIO, "cam1_ldo_en");
    if (ret < 0)
        pr_err("%s: gpio_request failed for gpio %s\n",
        __func__, "CAM1_LDO_EN_GPIO");
    gpio_direction_output(CAM1_LDO_EN_GPIO, 1);
    pr_info("gpio %d set to %d\n",CAM1_LDO_EN_GPIO, gpio_get_value(CAM1_LDO_EN_GPIO));


	/* CAM VCM controlled by CAM2/3_LDO_EN(GPIO_PS0)*/
	if (!reg_tf201_2v85_cam) {
		reg_tf201_2v85_cam = regulator_get(NULL, "vdd_cam3");
		if (IS_ERR_OR_NULL(reg_tf201_2v85_cam)) {
			reg_tf201_2v85_cam = NULL;
			pr_err("Can't get reg_tf201_2v85_cam.\n");
//			goto fail_to_get_reg;
		}
		regulator_set_voltage(reg_tf201_2v85_cam, 2850000, 2850000);
		regulator_enable(reg_tf201_2v85_cam);
	}

    if (!reg_tf201_cam) {
        reg_tf201_cam = regulator_get(NULL, "avdd_dsi_csi");
        if (IS_ERR_OR_NULL(reg_tf201_cam)) {
            pr_err("TF201_m6mo_power_on LDO6: p_tegra_cam failed\n");
            reg_tf201_cam = NULL;
            return PTR_ERR(reg_tf201_cam);
        }
        regulator_set_voltage(reg_tf201_cam, 1200000, 1200000);
        regulator_enable(reg_tf201_cam);
    }

	/* cam_power_en, powdn*/
	tegra_gpio_enable(OV5640_PWR_DN_GPIO);
	ret = gpio_request(OV5640_PWR_DN_GPIO, "cam1_power_dwn");
	if(ret == -EBUSY)
		printk("%s: gpio %s has been requested?\n", __func__, "OV5640_PWR_DN_GPIO");
	else if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s, ret= %d\n",
			__func__, "OV5640_PWR_DN_GPIO", ret);
//		goto fail_to_request_gpio;
	}
	pr_info("gpio %d: %d",OV5640_PWR_DN_GPIO, gpio_get_value(OV5640_PWR_DN_GPIO));
	gpio_set_value(OV5640_PWR_DN_GPIO, 0);
	gpio_direction_output(OV5640_PWR_DN_GPIO, 0);
	pr_info("--> %d\n", gpio_get_value(OV5640_PWR_DN_GPIO));

	/* cam_af_powdn*/
	tegra_gpio_enable(OV5640_AF_PWR_DN_GPIO);
	ret = gpio_request(OV5640_AF_PWR_DN_GPIO, "cam1_af_power_dwn");
	if(ret == -EBUSY)
		printk("%s: gpio %s has been requested?\n", __func__, "OV5640_AF_PWR_DN_GPIO");
	else if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s, ret= %d\n",
			__func__, "OV5640_AF_PWR_DN_GPIO", ret);
//		goto fail_to_request_gpio;
	}
	pr_info("gpio %d: %d",OV5640_AF_PWR_DN_GPIO, gpio_get_value(OV5640_AF_PWR_DN_GPIO));
	gpio_set_value(OV5640_AF_PWR_DN_GPIO, 0);
	gpio_direction_output(OV5640_AF_PWR_DN_GPIO, 0);
	pr_info("--> %d\n", gpio_get_value(OV5640_AF_PWR_DN_GPIO));

	/* yuv_sensor_rst_lo*/
	tegra_gpio_enable(OV5640_RST_GPIO);
	ret = gpio_request(OV5640_RST_GPIO, "cam_sensor_rst_lo");
	if(ret == -EBUSY)
		printk("%s: gpio %s has been requested?\n", __func__, "OV5640_RST_GPIO");
	else if (ret < 0) {
		pr_err("%s: gpio_request failed for gpio %s, ret= %d\n",
			__func__, "OV5640_RST_GPIO", ret);
//		goto fail_to_request_gpio;
	}
	pr_info("gpio %d: %d", OV5640_RST_GPIO, gpio_get_value(OV5640_RST_GPIO));
	gpio_set_value(OV5640_RST_GPIO, 1);
	gpio_direction_output(OV5640_RST_GPIO, 1);
	pr_info("--> %d\n", gpio_get_value(OV5640_RST_GPIO));

    return 0;

fail_to_get_reg:
	if (reg_tf201_2v85_cam) {
		regulator_put(reg_tf201_2v85_cam);
		reg_tf201_2v85_cam = NULL;
	}
	if (reg_tf201_1v8_cam) {
		regulator_put(reg_tf201_1v8_cam);
		reg_tf201_1v8_cam = NULL;
	}

	printk("yuv_front_sensor_power_on- : -ENODEV\n");
	return -ENODEV;
}
static int ov5640_power_off(void)
{
	printk("ov5640_power_off+\n");
	gpio_set_value(OV5640_RST_GPIO, 0);
	gpio_direction_output(OV5640_RST_GPIO, 0);
	gpio_free(OV5640_RST_GPIO);

	gpio_set_value(OV5640_AF_PWR_DN_GPIO, 1);
	gpio_direction_output(OV5640_AF_PWR_DN_GPIO, 1);
	gpio_free(OV5640_AF_PWR_DN_GPIO);

	gpio_set_value(OV5640_PWR_DN_GPIO, 1);
	gpio_direction_output(OV5640_PWR_DN_GPIO, 1);
	gpio_free(OV5640_PWR_DN_GPIO);

	if (reg_tf201_2v85_cam) {
		regulator_disable(reg_tf201_2v85_cam);
		regulator_put(reg_tf201_2v85_cam);
		reg_tf201_2v85_cam = NULL;
	}
	gpio_set_value(CAM1_LDO_EN_GPIO, 0);
	gpio_direction_output(CAM1_LDO_EN_GPIO, 0);
	gpio_free(CAM1_LDO_EN_GPIO);

	if (reg_tf201_1v8_cam) {
		regulator_disable(reg_tf201_1v8_cam);
		regulator_put(reg_tf201_1v8_cam);
		reg_tf201_1v8_cam = NULL;
	}

	printk("ov5640_power_off-\n");
  return 0;
}

struct yuv_sensor_platform_data yuv_front_sensor_data = {
	.power_on = yuv_front_sensor_power_on,
	.power_off = yuv_front_sensor_power_off,
};
struct yuv_sensor_platform_data ov5640_data = {
	.power_on = ov5640_power_on,
	.power_off = ov5640_power_off,
};
#endif  /* CONFIG_VIDEO_YUV */

#ifdef CONFIG_VIDEO_OV5650
static int tf201_left_ov5650_power_on(void)
{
	regulator_enable(tf201_vdd_2v8_cam1);
	mdelay(5);
	}

	/* Enable VDD_1V8_Cam1 */
	if (tf201_1v8_cam1 == NULL) {
		tf201_1v8_cam1 = regulator_get(NULL, "vdd_1v8_cam1");
		if (WARN_ON(IS_ERR(tf201_1v8_cam1))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam1: %ld\n",
				__func__, PTR_ERR(tf201_1v8_cam1));
			goto reg_alloc_fail;
		}
	}
	regulator_enable(tf201_1v8_cam1);

	mdelay(5);

	gpio_direction_output(CAM1_RST_L_GPIO, 0);
	mdelay(100);
	gpio_direction_output(CAM1_RST_L_GPIO, 1);

	return 0;

reg_alloc_fail:
	if (tf201_1v8_cam1) {
		regulator_put(tf201_1v8_cam1);
		tf201_1v8_cam1 = NULL;
	}
	if (tf201_vdd_2v8_cam1) {
		regulator_put(tf201_vdd_2v8_cam1);
		tf201_vdd_2v8_cam1 = NULL;
	}

	return -ENODEV;

}

static int tf201_left_ov5650_power_off(void)
{

	if (tf201_1v8_cam1)
		regulator_disable(tf201_1v8_cam1);
	if (tf201_vdd_2v8_cam1)
		regulator_disable(tf201_vdd_2v8_cam1);

	return 0;
}

struct ov5650_platform_data tf201_left_ov5650_data = {
	.power_on = tf201_left_ov5650_power_on,
	.power_off = tf201_left_ov5650_power_off,
};

#ifdef CONFIG_VIDEO_OV14810
static int tf201_ov14810_power_on(void)
{
	return 0;
}

static int tf201_ov14810_power_off(void)
{
	return 0;
}

struct ov14810_platform_data tf201_ov14810_data = {
	.power_on = tf201_ov14810_power_on,
	.power_off = tf201_ov14810_power_off,
};

struct ov14810_platform_data tf201_ov14810uC_data = {
	.power_on = NULL,
	.power_off = NULL,
};

struct ov14810_platform_data tf201_ov14810SlaveDev_data = {
	.power_on = NULL,
	.power_off = NULL,
};

static struct i2c_board_info tf201_i2c_board_info_e1214[] = {
	{
		I2C_BOARD_INFO("ov14810", 0x36),
		.platform_data = &tf201_ov14810_data,
	},
	{
		I2C_BOARD_INFO("ov14810uC", 0x67),
		.platform_data = &tf201_ov14810uC_data,
	},
	{
		I2C_BOARD_INFO("ov14810SlaveDev", 0x69),
		.platform_data = &tf201_ov14810SlaveDev_data,
	}
};
#endif

static int tf201_right_ov5650_power_on(void)
{
	/* CSI-B and front sensor are muxed on tf201 */
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 0);

	/* Enable VDD_1V8_Cam2 */
	if (tf201_1v8_cam2 == NULL) {
		tf201_1v8_cam2 = regulator_get(NULL, "vdd_1v8_cam2");
		if (WARN_ON(IS_ERR(tf201_1v8_cam2))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam2: %ld\n",
				__func__, PTR_ERR(tf201_1v8_cam2));
			goto reg_alloc_fail;
		}
	}
	regulator_enable(tf201_1v8_cam2);

	mdelay(5);

	gpio_direction_output(CAM2_RST_L_GPIO, 0);
	mdelay(100);
	gpio_direction_output(CAM2_RST_L_GPIO, 1);

	return 0;

reg_alloc_fail:
	if (tf201_1v8_cam2) {
		regulator_put(tf201_1v8_cam2);
		tf201_1v8_cam2 = NULL;
	}
	if (tf201_vdd_2v8_cam2) {
		regulator_put(tf201_vdd_2v8_cam2);
		tf201_vdd_2v8_cam2 = NULL;
	}

	return -ENODEV;

}

static int tf201_right_ov5650_power_off(void)
{
	/* CSI-B and front sensor are muxed on tf201 */
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 0);

	if (tf201_1v8_cam2)
		regulator_disable(tf201_1v8_cam2);
	if (tf201_vdd_2v8_cam2)
		regulator_disable(tf201_vdd_2v8_cam2);

	return 0;
}

static void tf201_ov5650_synchronize_sensors(void)
{
		pr_err("%s: UnSupported BoardId\n", __func__);
}

struct ov5650_platform_data tf201_right_ov5650_data = {
	.power_on = tf201_right_ov5650_power_on,
	.power_off = tf201_right_ov5650_power_off,
	.synchronize_sensors = tf201_ov5650_synchronize_sensors,
};
#endif

#ifdef CONFIG_VIDEO_OV2710

static int tf201_ov2710_power_on(void)
{
	/* CSI-B and front sensor are muxed on tf201 */
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 1);

	/* Enable VDD_1V8_Cam3 */
	if (tf201_1v8_cam3 == NULL) {
		tf201_1v8_cam3 = regulator_get(NULL, "vdd_1v8_cam3");
		if (WARN_ON(IS_ERR(tf201_1v8_cam3))) {
			pr_err("%s: couldn't get regulator vdd_1v8_cam3: %ld\n",
				__func__, PTR_ERR(tf201_1v8_cam3));
			goto reg_alloc_fail;
		}
	}
	regulator_enable(tf201_1v8_cam3);
	mdelay(5);

	return 0;

reg_alloc_fail:
	if (tf201_1v8_cam3) {
		regulator_put(tf201_1v8_cam3);
		tf201_1v8_cam3 = NULL;
	}
	if (tf201_vdd_cam3) {
		regulator_put(tf201_vdd_cam3);
		tf201_vdd_cam3 = NULL;
	}

	return -ENODEV;
}

static int tf201_ov2710_power_off(void)
{
	/* CSI-B and front sensor are muxed on tf201 */
	gpio_direction_output(CAMERA_CSI_MUX_SEL_GPIO, 1);

	if (tf201_1v8_cam3)
		regulator_disable(tf201_1v8_cam3);
	if (tf201_vdd_cam3)
		regulator_disable(tf201_vdd_cam3);

	return 0;
}

struct ov2710_platform_data tf201_ov2710_data = {
	.power_on = tf201_ov2710_power_on,
	.power_off = tf201_ov2710_power_off,
};

static const struct i2c_board_info tf201_i2c3_board_info[] = {
	{
		I2C_BOARD_INFO("pca9546", 0x70),
		.platform_data = &tf201_pca954x_data,
	},
};

static struct sh532u_platform_data sh532u_left_pdata = {
	.num		= 1,
	.sync		= 2,
	.dev_name	= "focuser",
	.gpio_reset	= TEGRA_GPIO_PBB0,
};

static struct sh532u_platform_data sh532u_right_pdata = {
	.num		= 2,
	.sync		= 1,
	.dev_name	= "focuser",
	.gpio_reset	= TEGRA_GPIO_PBB0,
};

static struct nvc_torch_pin_state tf201_tps61050_pinstate = {
	.mask		= 0x0008, /*VGP3*/
	.values		= 0x0008,
};

static struct tps61050_platform_data tf201_tps61050_pdata = {
	.dev_name	= "torch",
	.pinstate	= &tf201_tps61050_pinstate,
};
#endif

#ifdef CONFIG_VIDEO_OV5650

static const struct i2c_board_info tf201_i2c_board_info_tps61050[] = {
	{
		I2C_BOARD_INFO("tps61050", 0x33),
		.platform_data = &tf201_tps61050_pdata,
	},
};

static struct i2c_board_info tf201_i2c6_board_info[] = {
	{
		I2C_BOARD_INFO("ov5650L", 0x36),
		.platform_data = &tf201_left_ov5650_data,
	},
	{
		I2C_BOARD_INFO("sh532u", 0x72),
		.platform_data = &sh532u_left_pdata,
	},
};

static struct i2c_board_info tf201_i2c7_board_info[] = {
	{
		I2C_BOARD_INFO("ov5650R", 0x36),
		.platform_data = &tf201_right_ov5650_data,
	},
	{
		I2C_BOARD_INFO("sh532u", 0x72),
		.platform_data = &sh532u_right_pdata,
	},
};
#endif
#ifdef CONFIG_VIDEO_OV2710
static struct i2c_board_info tf201_i2c8_board_info[] = {
	{
		I2C_BOARD_INFO("ov2710", 0x36),
		.platform_data = &tf201_ov2710_data,
	},
};
#endif

#ifndef CONFIG_TEGRA_INTERNAL_TSENSOR_EDP_SUPPORT
static int nct_get_temp(void *_data, long *temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_get_temp(data, temp);
}

static int nct_get_temp_low(void *_data, long *temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_get_temp_low(data, temp);
}

static int nct_set_limits(void *_data,
			long lo_limit_milli,
			long hi_limit_milli)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_limits(data,
					lo_limit_milli,
					hi_limit_milli);
}

static int nct_set_alert(void *_data,
				void (*alert_func)(void *),
				void *alert_data)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_alert(data, alert_func, alert_data);
}

static int nct_set_shutdown_temp(void *_data, long shutdown_temp)
{
	struct nct1008_data *data = _data;
	return nct1008_thermal_set_shutdown_temp(data, shutdown_temp);
}

static void nct1008_probe_callback(struct nct1008_data *data)
{
	struct tegra_thermal_device *thermal_device;

	thermal_device = kzalloc(sizeof(struct tegra_thermal_device),
					GFP_KERNEL);
	if (!thermal_device) {
		pr_err("unable to allocate thermal device\n");
		return;
	}

	thermal_device->name = "nct1008";
	thermal_device->data = data;
	thermal_device->offset = TDIODE_OFFSET;
	thermal_device->get_temp = nct_get_temp;
	thermal_device->get_temp_low = nct_get_temp_low;
	thermal_device->set_limits = nct_set_limits;
	thermal_device->set_alert = nct_set_alert;
	thermal_device->set_shutdown_temp = nct_set_shutdown_temp;

	tegra_thermal_set_device(thermal_device);
}
#endif

static struct nct1008_platform_data tf201_nct1008_pdata = {
	.supported_hwrev = true,
	.ext_range = true,
	.conv_rate = 0x08,
	.offset = 8, /* 4 * 2C. Bug 844025 - 1C for device accuracies */
#ifndef CONFIG_TEGRA_INTERNAL_TSENSOR_EDP_SUPPORT
	.probe_callback = nct1008_probe_callback,
#endif
};

static struct i2c_board_info tf201_i2c4_bq27510_board_info[] = {
	{
		I2C_BOARD_INFO("bq27510", 0x55),
	},
};

static struct i2c_board_info tf201_i2c4_pad_bat_board_info[] = {
	{
		I2C_BOARD_INFO("pad-battery", 0xb),
	},
};

static struct i2c_board_info tf201_i2c4_nct1008_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &tf201_nct1008_pdata,
		.irq = -1,
	}
};

#ifdef CONFIG_VIDEO_YUV
static struct i2c_board_info rear_sensor_i2c3_board_info[] = {  //ddebug
    {
        I2C_BOARD_INFO("fjm6mo", 0x1F),
        .platform_data = &yuv_rear_sensor_data,
    },
};

static struct i2c_board_info front_sensor_i2c2_board_info[] = {  //ddebug
	{
		I2C_BOARD_INFO("mi1040", 0x48),
		.platform_data = &yuv_front_sensor_data,
	},
};
static struct i2c_board_info ov5640_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO("ov5640", 0x3C),
		.platform_data = &ov5640_data,
	},
};
#endif /* CONFIG_VIDEO_YUV */

static int tf201_nct1008_init(void)
{
	int nct1008_port = -1;
	int ret;

	if (nct1008_port >= 0) {
		/* FIXME: enable irq when throttling is supported */
		tf201_i2c4_nct1008_board_info[0].irq = TEGRA_GPIO_TO_IRQ(nct1008_port);

		ret = gpio_request(nct1008_port, "temp_alert");
		if (ret < 0)
			return ret;

		ret = gpio_direction_input(nct1008_port);
		if (ret < 0)
			gpio_free(nct1008_port);
		else
			tegra_gpio_enable(nct1008_port);
	}

	return ret;
}

static const struct i2c_board_info tf201_i2c1_board_info_al3010[] = {
	{
		I2C_BOARD_INFO("al3010",0x1C),
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PZ2),
	},
};

#if defined(CONFIG_GPIO_PCA953X)
static struct pca953x_platform_data tf201_pmu_tca6416_data = {
	.gpio_base      = PMU_TCA6416_GPIO_BASE,
};

static const struct i2c_board_info tf201_i2c4_board_info_tca6416[] = {
	{
		I2C_BOARD_INFO("tca6416", 0x20),
		.platform_data = &tf201_pmu_tca6416_data,
	},
};

static struct pca953x_platform_data tf201_cam_tca6416_data = {
	.gpio_base      = CAM_TCA6416_GPIO_BASE,
};

static const struct i2c_board_info tf201_i2c2_board_info_tca6416[] = {
	{
		I2C_BOARD_INFO("tca6416", 0x20),
		.platform_data = &tf201_cam_tca6416_data,
	},
};

static int __init pmu_tca6416_init(void)
{

	pr_info("Registering pmu pca6416\n");
	i2c_register_board_info(4, tf201_i2c4_board_info_tca6416,
		ARRAY_SIZE(tf201_i2c4_board_info_tca6416));
	return 0;
}

static int __init cam_tca6416_init(void)
{
	pr_info("Registering cam pca6416\n");
	i2c_register_board_info(2, tf201_i2c2_board_info_tca6416,
		ARRAY_SIZE(tf201_i2c2_board_info_tca6416));
	return 0;
}
#else
static int __init pmu_tca6416_init(void)
{
	return 0;
}

static int __init cam_tca6416_init(void)
{
	return 0;
}
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
#define SENSOR_MPU_NAME "mpu3050"
static struct mpu_platform_data mpu3050_data = {
	.int_config	= 0x10,
	.level_shifter	= 0,
	.orientation	= MPU_GYRO_ORIENTATION,	/* Located in board_[platformname].h	*/
};

static struct ext_slave_platform_data mpu3050_accel_data = {
	.address	= MPU_ACCEL_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_ACCEL_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_SECONDARY,
	.orientation	= MPU_ACCEL_ORIENTATION,	/* Located in board_[platformname].h	*/
};

static struct ext_slave_platform_data mpu_compass_data = {
	.address	= MPU_COMPASS_ADDR,
	.irq		= 0,
	.adapt_num	= MPU_COMPASS_BUS_NUM,
	.bus		= EXT_SLAVE_BUS_PRIMARY,
	.orientation	= MPU_COMPASS_ORIENTATION,	/* Located in board_[platformname].h	*/
};

static struct i2c_board_info __initdata inv_mpu_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO(MPU_GYRO_NAME, MPU_GYRO_ADDR),
		.irq = TEGRA_GPIO_TO_IRQ(MPU_GYRO_IRQ_GPIO),
		.platform_data = &mpu3050_data,
	},
	{
		I2C_BOARD_INFO(MPU_ACCEL_NAME, MPU_ACCEL_ADDR),
#if	MPU_ACCEL_IRQ_GPIO
		.irq = TEGRA_GPIO_TO_IRQ(MPU_ACCEL_IRQ_GPIO),
#endif
		.platform_data = &mpu3050_accel_data,
	},
	{
		I2C_BOARD_INFO(MPU_COMPASS_NAME, MPU_COMPASS_ADDR),
#if	MPU_COMPASS_IRQ_GPIO
		.irq = TEGRA_GPIO_TO_IRQ(MPU_COMPASS_IRQ_GPIO),
#endif
		.platform_data = &mpu_compass_data,
	},
};

static void mpuirq_init(void)
{
	pr_info("*** MPU START *** tf201_mpuirq_init...\n");
	signed char orientationGyroTF200X [9] = { -1, 0, 0, 0, 1, 0, 0, 0, -1 };
	signed char orientationAccelTF200X [9] = { 0, 1, 0, 1, 0, 0, 0, 0, -1 };
	signed char orientationMagTF200X [9] = { 0, -1, 0, -1, 0, 0, 0, 0, -1 };
	signed char orientationGyroTF200XG[9] = { -1, 0, 0, 0, 1, 0, 0, 0, -1 };
	signed char orientationAccelTF200XG[9] = { 0, 1, 0, 1, 0, 0, 0, 0, -1 };
	signed char orientationMagTF200XG[9] = { 1, 0, 0, 0, -1, 0, 0, 0, -1 };

	int ret = 0;

#if	MPU_ACCEL_IRQ_GPIO
	/* ACCEL-IRQ assignment */
	tegra_gpio_enable(MPU_ACCEL_IRQ_GPIO);
	ret = gpio_request(MPU_ACCEL_IRQ_GPIO, MPU_ACCEL_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_ACCEL_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_ACCEL_IRQ_GPIO);
		return;
	}
#endif

	/* MPU-IRQ assignment */
	tegra_gpio_enable(MPU_GYRO_IRQ_GPIO);
	ret = gpio_request(MPU_GYRO_IRQ_GPIO, MPU_GYRO_NAME);
	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(MPU_GYRO_IRQ_GPIO);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(MPU_GYRO_IRQ_GPIO);
		return;
	}
	pr_info("*** MPU END *** mpuirq_init...\n");

	i2c_register_board_info(MPU_GYRO_BUS_NUM, inv_mpu_i2c2_board_info,
		ARRAY_SIZE(inv_mpu_i2c2_board_info));

}
#endif


int __init tf201_sensors_init(void)
{
	int err;

	tegra_get_board_info(&board_info);

	tf201_camera_init();
	cam_tca6416_init();

	i2c_register_board_info(2, tf201_i2c1_board_info_al3010,
		ARRAY_SIZE(tf201_i2c1_board_info_al3010));

#ifdef CONFIG_I2C_MUX_PCA954x
	i2c_register_board_info(2, tf201_i2c3_board_info,
		ARRAY_SIZE(tf201_i2c3_board_info));

	i2c_register_board_info(2, tf201_i2c_board_info_tps61050,
		ARRAY_SIZE(tf201_i2c_board_info_tps61050));

//#ifdef CONFIG_VIDEO_OV14810
	/* This is disabled by default; To enable this change Kconfig;
	 * there should be some way to detect dynamically which board
	 * is connected (E1211/E1214), till that time sensor selection
	 * logic is static;
	 * e1214 corresponds to ov14810 sensor */
	i2c_register_board_info(2, tf201_i2c_board_info_e1214,
		ARRAY_SIZE(tf201_i2c_board_info_e1214));
//#else
	/* Left  camera is on PCA954x's I2C BUS0, Right camera is on BUS1 &
	 * Front camera is on BUS2 */
	i2c_register_board_info(PCA954x_I2C_BUS0, tf201_i2c6_board_info,
		ARRAY_SIZE(tf201_i2c6_board_info));

	i2c_register_board_info(PCA954x_I2C_BUS1, tf201_i2c7_board_info,
		ARRAY_SIZE(tf201_i2c7_board_info));

	i2c_register_board_info(PCA954x_I2C_BUS2, tf201_i2c8_board_info,
		ARRAY_SIZE(tf201_i2c8_board_info));
#endif

//+ m6mo rear camera
#ifdef CONFIG_VIDEO_YUV
    pr_info("fjm6mo i2c_register_board_info");
    i2c_register_board_info(2, rear_sensor_i2c3_board_info,
        ARRAY_SIZE(rear_sensor_i2c3_board_info));

/* Front Camera mi1040 + */
    pr_info("mi1040 i2c_register_board_info");
	i2c_register_board_info(2, front_sensor_i2c2_board_info,
		ARRAY_SIZE(front_sensor_i2c2_board_info));
/* Front Camera mi1040 - */
/* Back Camera ov5640 + */
    pr_info("ov5640 i2c_register_board_info");
	i2c_register_board_info(2, ov5640_i2c2_board_info,
		ARRAY_SIZE(ov5640_i2c2_board_info));
/* Back Camera ov5640 - */
#endif /* CONFIG_VIDEO_YUV */
//-
	pmu_tca6416_init();

	i2c_register_board_info(4, tf201_i2c4_pad_bat_board_info,
		ARRAY_SIZE(tf201_i2c4_pad_bat_board_info));


	err = tf201_nct1008_init();
	if (err)
		return err;

	i2c_register_board_info(4, tf201_i2c4_nct1008_board_info,
		ARRAY_SIZE(tf201_i2c4_nct1008_board_info));

#ifdef CONFIG_MPU_SENSORS_MPU3050
	mpuirq_init();
#endif
	return 0;
}

#if defined(CONFIG_GPIO_PCA953X)
struct ov5650_gpios {
	const char *name;
	int gpio;
	int enabled;
};

#ifdef CONFIG_VIDEO_OV5650
#define OV5650_GPIO(_name, _gpio, _enabled)		\
	{						\
		.name = _name,				\
		.gpio = _gpio,				\
		.enabled = _enabled,			\
	}

static struct ov5650_gpios ov5650_gpio_keys[] = {
	[0] = OV5650_GPIO("cam1_pwdn", CAM1_PWR_DN_GPIO, 0),
	[1] = OV5650_GPIO("cam1_rst_lo", CAM1_RST_L_GPIO, 1),
	[2] = OV5650_GPIO("cam1_af_pwdn_lo", CAM1_AF_PWR_DN_L_GPIO, 0),
	[3] = OV5650_GPIO("cam1_ldo_shdn_lo", CAM1_LDO_SHUTDN_L_GPIO, 1),
	[4] = OV5650_GPIO("cam2_pwdn", CAM2_PWR_DN_GPIO, 0),
	[5] = OV5650_GPIO("cam2_rst_lo", CAM2_RST_L_GPIO, 1),
	[6] = OV5650_GPIO("cam2_af_pwdn_lo", CAM2_AF_PWR_DN_L_GPIO, 0),
	[7] = OV5650_GPIO("cam2_ldo_shdn_lo", CAM2_LDO_SHUTDN_L_GPIO, 1),
	[8] = OV5650_GPIO("cam3_pwdn", CAM_FRONT_PWR_DN_GPIO, 0),
	[9] = OV5650_GPIO("cam3_rst_lo", CAM_FRONT_RST_L_GPIO, 1),
	[10] = OV5650_GPIO("cam3_af_pwdn_lo", CAM_FRONT_AF_PWR_DN_L_GPIO, 0),
	[11] = OV5650_GPIO("cam3_ldo_shdn_lo", CAM_FRONT_LDO_SHUTDN_L_GPIO, 1),
	[12] = OV5650_GPIO("cam_led_exp", CAM_FRONT_LED_EXP, 1),
	[13] = OV5650_GPIO("cam_led_rear_exp", CAM_SNN_LED_REAR_EXP, 1),
	[14] = OV5650_GPIO("cam_i2c_mux_rst", CAM_I2C_MUX_RST_EXP, 1),
};

int __init tf201_ov5650_late_init(void)
{
	int ret;
	int i;

	if (!machine_is_tf201())
		return 0;

	printk("%s: \n", __func__);
	for (i = 0; i < ARRAY_SIZE(ov5650_gpio_keys); i++) {
		ret = gpio_request(ov5650_gpio_keys[i].gpio,
			ov5650_gpio_keys[i].name);
		if (ret < 0) {
			printk("%s: gpio_request failed for gpio #%d\n",
				__func__, i);
			goto fail;
		}
		printk("%s: enable - %d\n", __func__, i);
		gpio_direction_output(ov5650_gpio_keys[i].gpio,
			ov5650_gpio_keys[i].enabled);
		gpio_export(ov5650_gpio_keys[i].gpio, false);
	}

	return 0;

fail:
	while (i--)
		gpio_free(ov5650_gpio_keys[i].gpio);
	return ret;
}

late_initcall(tf201_ov5650_late_init);
#endif
#endif
