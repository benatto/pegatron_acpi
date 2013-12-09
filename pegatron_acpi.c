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
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

#define PEGATRON_DEVICE_ID "PTK0001"


MODULE_AUTHOR("Marco Antonio Benatto");
MODULE_DESCRIPTION("Pegatron ACPI/WMI platform device driver");
MODULE_LICENSE("GPL");

static int pegatron_acpi_add(struct acpi_device*);
static int pegatron_acpi_remove(struct acpi_device*);

static const struct acpi_device_id pegatron_device_ids[] = {
	{ PEGATRON_DEVICE_ID, 0 },
	{ "", 0}
};
MODULE_DEVICE_TABLE(acpi, pegatron_device_ids);

static struct acpi_driver pegatron_acpi_driver = {
	.name =			"Pegatron ACPI",
	.class = 		"Pegatron",
	.ids = 			pegatron_device_ids,
	.ops =			{
						.add = pegatron_acpi_add,
						.remove = pegatron_acpi_remove,
					},
	.owner =		THIS_MODULE,
};

static int pegatron_acpi_add(struct acpi_device *dev) {
	/* We should call INIT function from acpi sending 0x55AA66BB
	 * as arg0 following Pegatron ACPI spec */
	acpi_status status = AE_OK;


	if (!acpi_has_method(dev->handle, "INIT")){
		dev_err(&dev->dev, "[Pegatron] INIT method not found in ACPI implementation\n");
		return -ENODEV;
	}

	status = acpi_execute_simple_method(dev->handle, "INIT", 0x55AA66BB);
	if (ACPI_FAILURE(status)){
		dev_err(&dev->dev, "[Pegatron] error calling ACPI INIT method\n");
		return -ENODEV;
	}


	return 0;
}

static int pegatron_acpi_remove(struct acpi_device *device){
	return 0;
}

static int __init pegatron_acpi_init(void) {
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
}

module_init(pegatron_acpi_init);
module_exit(pegatron_acpi_exit);
