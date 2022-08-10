// SPDX-License-Identifier: GPL-2.0+
/*
 * IIO Analog Devices AD5592r Driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>

static struct spi_driver ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    },
};

module_spi_driver(ad5592r_driver);

MODULE_AUTHOR ("Bindea Cristian <cristian.bindea@analog.com>");
MODULE_DESCRIPTION("IIO Analog Devices AD5592r Drivers");
MODULE_LICENSE("GPL_V2");