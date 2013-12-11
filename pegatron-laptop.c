/*
 * ACPI-WMI driver for Pegatron platform
 *
 * Copyright (C) 2013 Marco Antonio Benatto <benatto@mandriva.com.br>
 *
 * Development of this program was funded by Philco Informática.
 * Most of this driver was written following the ACPI specification provided
 * by Pegatron Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of GNU General Public License version 2 as
 * published by Free Software Foundation
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/input-polldev.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

#define PEGATRON_DEVICE_ID "PTK0001"
#define PEGATRON_FILE KBUILD_MODNAME


MODULE_AUTHOR("Marco Antonio Benatto");
MODULE_DESCRIPTION("Pegatron ACPI/WMI platform device driver");
MODULE_LICENSE("GPL");

static int pegatron_acpi_add(struct acpi_device*);
static int pegatron_acpi_remove(struct acpi_device*);
static void pegatron_acpi_notify(struct acpi_device*, u32);

static const struct acpi_device_id pegatron_device_ids[] = {
	{ PEGATRON_DEVICE_ID, 0 },
	{ "", 0}
};
MODULE_DEVICE_TABLE(acpi, pegatron_device_ids);

/*
 * Pegatron specific hotkeys mapping
 */
static const struct key_entry pegatron_keymap[] = {
	{KE_KEY, 0xf1, {KEY_WLAN} }, /* WLAN on/off hotkey */
	{KE_KEY, 0xf3, {KEY_PROG2} }, /* Smart battery hotkey */
	{KE_KEY, 0xf8, {KEY_TOUCHPAD_TOGGLE} }, /* TouchPad lock hotkey */
};

/*
 * Pegatron laptop information
 */

struct pegatron_laptop {
	struct acpi_device *dev;

	struct input_dev *inputdev; /* Pegatron input device */
	struct key_entry *keymap; /* Pegatron hotkeys sparse keymap */

	int wlan_status; /* WLAN card status (1: ON, 0: OFF) */

	acpi_handle handle;	
};


/*
 * Pegatron ACPI driver information
 */

static struct acpi_driver pegatron_acpi_driver = {
	.name =			"Pegatron ACPI",
	.class = 		"Pegatron",
	.ids = 			pegatron_device_ids,
	.ops =			{
						.add = pegatron_acpi_add,
						.remove = pegatron_acpi_remove,
						.notify = pegatron_acpi_notify,
					},
	.owner =		THIS_MODULE,
};


/*
 * init functions
 */

static int pegatron_acpi_init(struct pegatron_laptop *pegatron) {
	acpi_status status = AE_OK;

	if (!acpi_has_method(pegatron->handle, "INIT")){
		dev_err(&pegatron->dev->dev, "[Pegatron] INIT method not found on DSDT\n");
		return -ENODEV;
	}

	status = acpi_execute_simple_method(pegatron->handle, "INIT", 0x55AA66BB);
	if (ACPI_FAILURE(status)){
		dev_err(&pegatron->dev->dev, "[Pegatron] error calling ACPI INIT method\n");
		return -ENODEV;
	}

	return 0;
}

static int pegatron_input_init(struct pegatron_laptop *pegatron) {
	struct input_dev *input;
	int failed;

	pr_info("[Pegatron] Initializing input devices\n");

	input = input_allocate_device();

	if (!input){
		pr_warn("[Pegatron] Failed to allocate input device\n");
		return -ENOMEM;
	}

	input->name = "Pegatron laptop extra buttons";
	input->phys = PEGATRON_FILE "/input0";
	input->id.bustype = BUS_HOST;
	
	/*Generic device from Pegatron ACPI device*/
	input->dev.parent = &pegatron->dev->dev; 
	
	/* Setting up sparse keymap */
	failed = sparse_keymap_setup(input, pegatron_keymap, NULL);

	if (failed) {
		pr_warn("[Pegatron] Unable to setup Pegatron keymap\n");
		input_free_device(input);
		return failed;
	}

	failed = input_register_device(input);

	if (failed) {
		pr_warn("[Pegatron] Unable to register Pegatron input\n");
		sparse_keymap_free(input);
		input_free_device(input);
		return failed;
	}

	pegatron->inputdev = input;
	
	return 0;
}

/*****************************************************************************/



/*
 * exit functions
 */

static void pegatron_input_exit(struct pegatron_laptop *pegatron) {
	if (pegatron->inputdev) {
		sparse_keymap_free(pegatron->inputdev);
		input_unregister_device(pegatron->inputdev);
	}

	pegatron->inputdev = NULL;
}

/*****************************************************************************/

static int pegatron_acpi_add(struct acpi_device *dev) {
	/* We should call INIT function from acpi sending 0x55AA66BB
	 * as arg0 following Pegatron ACPI spec */
	struct pegatron_laptop *pegatron;
	int result;

	pegatron = kzalloc(sizeof(struct pegatron_laptop), GFP_KERNEL);

	if (!pegatron){
		dev_err(&dev->dev, "[Pegatron] Error allocating memory for pegatron laptop\n");
		return -ENOMEM;
	}

	pegatron->handle = dev->handle;
	dev->driver_data = pegatron;
	pegatron->dev = dev;

	/* Initializing acpi */
	result = pegatron_acpi_init(pegatron);

	/* If we fail to init something we release allocated memory */
	if (result) {
		kfree(pegatron);
		return result;
	}

	/* Initializing Pegatron specific input device */
	result = pegatron_input_init(pegatron);

	if (result) {
		kfree(pegatron);
		return result;
	}
	

	return result;
}

static void pegatron_acpi_notify(struct acpi_device *dev, u32 event) {
	pr_info("[Pegatron] event found: 0x%.2x\n", event);
}

static int pegatron_acpi_remove(struct acpi_device *dev) {
	struct pegatron_laptop *pegatron = acpi_driver_data(dev);

	pr_info("[Pegatron] removing acpi data\n");
	
	/* TODO: exit all other stuff to be done */

	pegatron_input_exit(pegatron);

	kfree(pegatron);
	
	return 0;
}

static int __init pegatron_laptop_init(void) {
	int result = 0;

	pr_info("[Pegatron] ACPI/WMI module loaded\n");

	result = acpi_bus_register_driver(&pegatron_acpi_driver);
	if (result < 0) {
		pr_err("[Pegatron] Could not insert Pegatron device driver. Exiting...\n");
		return -ENODEV;
	}
	
	return 0;
}

static void __exit pegatron_acpi_exit(void) {
	pr_info("[Pegatron] Unloading ACPI/WMI device\n");
	acpi_bus_unregister_driver(&pegatron_acpi_driver);
}

module_init(pegatron_laptop_init);
module_exit(pegatron_acpi_exit);
