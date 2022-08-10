// SPDX-License-Identifier: GPL-2.0+
/*
 * AD7124 SPI ADC driver
 *
 * Copyright 2018 Analog Devices Inc.
 */

#include <linux/module.h> 
#include <linux/spi/spi.h>

static struct spi_driver adi_ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    }
};

module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Bianca Maxim <bmaxim55@yahoo.com>");
MODULE_DESCRPIPTION("iio Analog Devices ad5592r Driver");
MODULE_LICENSE("GPL v2");