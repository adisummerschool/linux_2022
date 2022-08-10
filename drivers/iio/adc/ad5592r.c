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
        .name = "iio-adi-ad5592r",
    }
};
module_spi_driver(adi_ad5592r_driver);
MODULE_AUTHOR("Oance Sergiu <oance_sergiu@yahoo.com>");
MODULE_DESCRIPTION("ad5592r Driver");
MODULE_LICENSE("GPL v2");