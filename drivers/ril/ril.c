#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <../../arch/arm/mach-tegra/include/mach/board-cardhu-misc.h>

#include "pm-irq.h"
#include "ril.h"
#include "ril_proximity.h"
#include "ril_wakeup.h"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

//**** external symbols


//**** constants

#define _ATTR_MODE S_IRUSR | S_IWUSR | S_IRGRP


//**** local variable declaration

static struct workqueue_struct *workqueue;
static struct device *dev;
static struct class *ril_class;
static dev_t ril_dev;
static int ril_major = 0;
static int ril_minor = 0;
int project_id = 0;

static struct gpio ril_gpios_TF300TG[] = {
	{ MOD_VBUS_ON,    GPIOF_OUT_INIT_LOW,  "BB_VBUS"    },
	{ USB_SW_SEL,     GPIOF_OUT_INIT_LOW,  "BB_SW_SEL"  },
	{ SAR_DET_3G,     GPIOF_IN,            "BB_SAR_DET" },
	{ SIM_CARD_DET,   GPIOF_IN,            "BB_SIM_DET" },
};

static struct gpio ril_gpios_TF300TL[] = {
	{ MOD_VBAT_ON,    GPIOF_OUT_INIT_LOW,  "BB_VBAT"},
	{ MOD_VBUS_ON,    GPIOF_OUT_INIT_LOW,  "BB_VBUS"},
	{ USB_SW_SEL,     GPIOF_OUT_INIT_LOW,  "BB_SW_SEL"},
	{ SAR_DET_3G,     GPIOF_IN,            "BB_SAR_DET" },
	{ SIM_CARD_DET,   GPIOF_IN,            "BB_SIM_DET" },
	{ MOD_POWER_KEY,  GPIOF_OUT_INIT_LOW,  "BB_MOD_PWR"},
	{ DL_MODE,        GPIOF_OUT_INIT_LOW,  "BB_DL_MODE"},
	{ AP_TO_MOD_RST,  GPIOF_OUT_INIT_LOW,  "BB_MOD_RST"},
	{ MOD_WAKE_AP,    GPIOF_IN,            "BB_MOD_WAKE_AP"},
	{ MOD_WAKE_IND,   GPIOF_OUT_INIT_HIGH, "BB_MOD_WAKE_IND"},
	{ MOD_HANG,       GPIOF_IN,            "BB_MOD_HANG"},
	/* no use now */
	{ DL_COMPLETE,    GPIOF_OUT_INIT_LOW,  "BB_DL_COMPLETE"},
	{ MOD_SUS_REQ,    GPIOF_OUT_INIT_LOW,  "BB_MOD_SUS_REQ"},
	{ AP_WAKE_IND,    GPIOF_OUT_INIT_LOW,  "BB_AP_WAKE_IND"},
	{ AP_WAKE_MOD,    GPIOF_OUT_INIT_LOW,  "BB_AP_WAKE_MOD"},
};

//**** IRQ event handler

irqreturn_t ril_ipc_sar_det_irq(int irq, void *dev_id)
{
	return ril_proximity_interrupt_handle(irq, dev_id);
}

irqreturn_t ril_ipc_sim_det_irq(int irq, void *dev_id)
{
	// TODO: implement SIM hot-plug here
	return IRQ_HANDLED;
}

//**** sysfs callback functions
static int store_gpio(size_t count, const char *buf, int gpio, char *gpio_name)
{
	int enable;

	if (sscanf(buf, "%u", &enable) != 1)
		return -EINVAL;

	if ((enable != 1) && (enable != 0))
		return -EINVAL;

	gpio_set_value(gpio, enable);
	RIL_INFO("Set %s to %d\n", gpio_name, enable);
	return count;
}

/* TF300TG sysfs functions */
static ssize_t show_vbus_state(struct device *class,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(MOD_VBUS_ON));
}

static ssize_t store_vbus_state(struct device *class, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return store_gpio(count, buf, MOD_VBUS_ON, "MOD_VBUS_ON");
}

/* TF300TL sysfs functions */
static ssize_t show_power_state(struct device *class, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(MOD_POWER_KEY));
}

static ssize_t store_power_state(struct device *class, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return store_gpio(count, buf, MOD_POWER_KEY, "MOD_POWER_KEY");
}

static ssize_t show_vbat_state(struct device *class, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(MOD_VBAT_ON));
}

static ssize_t store_vbat_state(struct device *class, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return store_gpio(count, buf, MOD_VBAT_ON, "MOD_VBAT_ON");
}

static ssize_t show_reset_state(struct device *class, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(AP_TO_MOD_RST));
}

static ssize_t store_reset_state(struct device *class, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return store_gpio(count, buf, AP_TO_MOD_RST, "AP_TO_MOD_RST");
}

static ssize_t show_download_state(struct device *class, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value(DL_MODE));
}

static ssize_t store_download_state(struct device *class, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return store_gpio(count, buf, DL_MODE, "DL_MODE");
}


//**** sysfs list
/* TF300TG sysfs array */
static struct device_attribute device_attr_TF300TG[] = {
	__ATTR(bb_vbus, _ATTR_MODE, show_vbus_state, store_vbus_state),
	__ATTR_NULL,
};

/* TF300TL sysfs array */
static struct device_attribute device_attr_TF300TL[] = {
	__ATTR(mod_power, _ATTR_MODE, show_power_state, store_power_state),
	__ATTR(vbat, _ATTR_MODE, show_vbat_state, store_vbat_state),
	__ATTR(mod_rst, _ATTR_MODE, show_reset_state, store_reset_state),
	__ATTR(dl_mode, _ATTR_MODE, show_download_state, store_download_state),
	__ATTR_NULL,
};

//**** initialize and finalize

static int create_ril_files(void)
{
	int rc = 0, i = 0;

	rc = alloc_chrdev_region(&ril_dev, ril_minor, 1, "ril");
	ril_major = MAJOR(ril_dev);
	if (rc < 0) {
		RIL_ERR("allocate char device failed\n");
		goto failed;
	}
	RIL_INFO("rc = %d, ril_major = %d\n", rc, ril_major);

	ril_class = class_create(THIS_MODULE, "ril");
	if (IS_ERR(ril_class)) {
		RIL_ERR("ril_class create fail\n");
		rc = PTR_ERR(ril_class);
		goto create_class_failed;
	}

	dev = device_create(ril_class, NULL, MKDEV(ril_major, ril_minor),
			NULL, "files");
	if (IS_ERR(dev)) {
		RIL_ERR("dev create fail\n");
		rc = PTR_ERR(dev);
		goto create_device_failed;
	}

	if (project_id == TEGRA3_PROJECT_TF300TG) {
		for (i = 0; i < (ARRAY_SIZE(device_attr_TF300TG) - 1); i++) {
			rc = device_create_file(dev, &device_attr_TF300TG[i]);
			if (rc < 0) {
				RIL_ERR("create file of [%d] failed, err = %d\n", i, rc);
				goto create_files_failed;
			}
		}
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		for (i = 0; i < (ARRAY_SIZE(device_attr_TF300TL) - 1); i++) {
			rc = device_create_file(dev, &device_attr_TF300TL[i]);
			if (rc < 0) {
				RIL_ERR("create file of [%d] failed, err = %d\n", i, rc);
				goto create_files_failed;
			}
		}
	}

	RIL_INFO("add_ril_dev success\n");
	return 0;

create_files_failed:
	if (project_id == TEGRA3_PROJECT_TF300TG) {
		while (i--)
			device_remove_file(dev, &device_attr_TF300TG[i]);
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		while (i--)
			device_remove_file(dev, &device_attr_TF300TL[i]);
	}
create_device_failed:
	class_destroy(ril_class);
create_class_failed:
	unregister_chrdev_region(ril_dev, 1);
failed:
	return rc;
}

static void remove_ril_files(void)
{
	int i;
	if (project_id == TEGRA3_PROJECT_TF300TG) {
		for (i = 0; i < (ARRAY_SIZE(device_attr_TF300TG) - 1); i++)
			device_remove_file(dev, &device_attr_TF300TG[i]);
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		for (i = 0; i < (ARRAY_SIZE(device_attr_TF300TL) - 1); i++)
			device_remove_file(dev, &device_attr_TF300TL[i]);
	}
	device_destroy(ril_class, MKDEV(ril_major, ril_minor));
	class_destroy(ril_class);
	unregister_chrdev_region(ril_dev, 1);
}

static void power_on_TF300TL(void)
{
	RIL_INFO("TF300TL power_on\n");
	gpio_set_value(MOD_VBAT_ON, 1);
	RIL_INFO("Set MOD_VBAT_ON to %d\n", gpio_get_value(MOD_VBAT_ON));
	mdelay(100);
	gpio_set_value(MOD_POWER_KEY, 1);
	RIL_INFO("Set MOD_POWER_KEY to %d\n", gpio_get_value(MOD_POWER_KEY));
	msleep(200);
	gpio_set_value(USB_SW_SEL, 1);
	RIL_INFO("Set USB_SW_SEL to %d\n", gpio_get_value(USB_SW_SEL));
	mdelay(50);
	gpio_set_value(MOD_VBUS_ON, 1);
	RIL_INFO("Set MOD_VBUS_ON to %d\n", gpio_get_value(MOD_VBUS_ON));
}

static int __init ril_init(void)
{
	int err, i;
	RIL_INFO("RIL init\n");

	project_id = tegra3_get_project_id();

	/* enable and request gpio(s) */
	if (project_id == TEGRA3_PROJECT_TF300TG) {
		RIL_INFO("project_id = TF300TG\n");
		for (i = 0; i < ARRAY_SIZE(ril_gpios_TF300TG); i++) {
			tegra_gpio_enable(ril_gpios_TF300TG[i].gpio);
		}
		err = gpio_request_array(ril_gpios_TF300TG,
				ARRAY_SIZE(ril_gpios_TF300TG));
		if (err < 0) {
			pr_err("%s - request gpio(s) failed\n", __func__);
			return err;
		}
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		RIL_INFO("project_id = TF300TL\n");
		for (i = 0; i < ARRAY_SIZE(ril_gpios_TF300TL); i++) {
			tegra_gpio_enable(ril_gpios_TF300TL[i].gpio);
		}
		err = gpio_request_array(ril_gpios_TF300TL,
				ARRAY_SIZE(ril_gpios_TF300TL));
		if (err < 0) {
			pr_err("%s - request gpio(s) failed\n", __func__);
			return err;
		}
		mdelay(100);
		power_on_TF300TL();
	} else {
		RIL_ERR("Ril driver doesn't support this project\n");
		return -1;
	}

	/* create device file(s) */
	err = create_ril_files();
	if (err < 0)
		goto failed1;

	/* request ril irq(s) */
	err = request_irq(gpio_to_irq(SAR_DET_3G),
		ril_ipc_sar_det_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"IPC_SAR_DET_3G",
		NULL);
	if (err < 0) {
		pr_err("%s - request irq IPC_SAR_DET_3G failed\n",
			__func__);
		goto failed2;
	}
	disable_irq(gpio_to_irq(SAR_DET_3G));

	err = request_irq(gpio_to_irq(SIM_CARD_DET),
		ril_ipc_sim_det_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		"IPC_SIM_CARD_DET",
		NULL);
	if (err < 0) {
		pr_err("%s - request irq IPC_SIM_CARD_DET failed\n",
			__func__);
		goto failed3;
	}
	tegra_pm_irq_set_wake_type(gpio_to_irq(SIM_CARD_DET),
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING);
	enable_irq_wake(gpio_to_irq(SIM_CARD_DET));

	/* init work queue */
	workqueue = create_singlethread_workqueue
		("ril_workqueue");
	if (!workqueue) {
		pr_err("%s - cannot create workqueue\n", __func__);
		err = -1;
		goto failed4;
	}

	/* register proximity switch */
	err = ril_proximity_init(dev, workqueue);
	if (err < 0) {
		pr_err("%s - register proximity switch failed\n",
			__func__);
		goto failed5;
	}

	/* wakeup control (TF-300TL only) */
	if (TEGRA3_PROJECT_TF300TL == project_id) {
		err = init_wakeup_control(workqueue);
		if (err < 0) {
			RIL_ERR("init wakeup_control failed\n");
			goto failed6;
		}
	}

	RIL_INFO("RIL init successfully\n");
	return 0;

failed6:
	ril_proximity_exit();
failed5:
	destroy_workqueue(workqueue);
failed4:
	free_irq(gpio_to_irq(SIM_CARD_DET), NULL);
failed3:
	free_irq(gpio_to_irq(SAR_DET_3G), NULL);
failed2:
	remove_ril_files();
failed1:
	if (project_id == TEGRA3_PROJECT_TF300TG) {
		gpio_free_array(ril_gpios_TF300TG,
				ARRAY_SIZE(ril_gpios_TF300TG));
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		gpio_free_array(ril_gpios_TF300TL,
				ARRAY_SIZE(ril_gpios_TF300TL));
	}
	return err;
}

static void __exit ril_exit(void)
{
	RIL_INFO("RIL exit\n");

	/* free irq(s) */
	free_irq(gpio_to_irq(SAR_DET_3G), NULL);
	free_irq(gpio_to_irq(SIM_CARD_DET), NULL);

	/* free gpio(s) */
	if (project_id == TEGRA3_PROJECT_TF300TG) {
		gpio_free_array(ril_gpios_TF300TG,
				ARRAY_SIZE(ril_gpios_TF300TG));
	} else if (project_id == TEGRA3_PROJECT_TF300TL) {
		gpio_free_array(ril_gpios_TF300TL,
				ARRAY_SIZE(ril_gpios_TF300TL));
	}

	/* delete device file(s) */
	remove_ril_files();

	/* unregister proximity switch */
	ril_proximity_exit();

	/* destroy workqueue */
	destroy_workqueue(workqueue);

	/* free resources for wakeup control*/
	if (TEGRA3_PROJECT_TF300TL == project_id) {
		free_wakeup_control();
	}

}

module_init(ril_init);
module_exit(ril_exit);
