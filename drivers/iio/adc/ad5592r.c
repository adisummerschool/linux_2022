// SPDX-License-Identifier: GPL-2.0+
/*
 * AD7124 SPI ADC driver
 *
 * Copyright 2018 Analog Devices Inc.
 */
#include <linux/module.h>
#include <linux/spi/spi.h>
static struct spi_driver ad5592r_driver = {
    .driver = {
        .name = "iio-adi-ad5592r",
    }
};
module_spi_driver(ad5592r_driver);
MODULE_AUTHOR("Muresan Mihai <mymihai22@yahoo.com>");
MODULE_DESCRIPTION("IIO Analog Devices ad5592r Driver");
MODULE_LICENSE("GPL v2");