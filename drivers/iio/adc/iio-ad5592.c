// SPDX-License-Identifier: GPL-2.0-only
/*
 * IIO Analog Devices emulator driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>

static struct spi_driver iio_ad5592_driver = {
    .driver = {
        .name = "iio-ad5592",
    }
};

module_spi_driver(iio_ad5592_driver);

MODULE_AUTHOR("Ceclan Dumitru-Ioan <mitrutzceclan@gmail.com>");
MODULE_DESCRIPTION("IIO Analog Devices emulator driver");
MODULE_LICENSE("GPL v2");