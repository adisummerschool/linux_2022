// SPDX-License-Identifier: GPL-2.0
/*
* IIO Analog Devices Emulator Driver
*
* Copyright 2022 Analog Devices Inc.
*/
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static const struct iio_info ad5592r_info = {

};
static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev)
		return -ENOMEM;
	indio_dev->name = "ad5592r";
	indio_dev->info = &ad5592r_info;
	dev_info(&spi->dev, "ad5592r Probed");
	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver adi_ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    },
    .probe = ad5592r_probe,
};

module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Andras Eunicia <andraseunicia@yahoo.com>");
MODULE_DESCRIPTION("Analog Devices Emulator Driver");
MODULE_LICENSE("GPL v2");


