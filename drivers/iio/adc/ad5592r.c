// SPDX-License-Identifier: GPL-2.0
/*
 * IIO Analog Devices, Inc. Emulator Driver
 *
 * Copyright (C) 2022 Analog Devices, Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>

static struct spi_driver adi_ad5592r_driver = {
	.driver = {
		.name = "ad5592r",
	},
};
module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Laurentiu Popa <laurentiu.popa@analog.com>");
MODULE_DESCRIPTION("AD5592R IIO ADI Driver");
MODULE_LICENSE("GPL v2");