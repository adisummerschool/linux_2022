// SPDX-License-Identifier: GPL-2.0
/*
 * IIO Analog Devices, Inc. Emulator Driver
 *
 * Copyright (C) 2022 Analog Devices, Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static const struct iio_info adi_ad5592r_info = {
};

static int adi_ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "ad5592r";
	indio_dev->info = &adi_ad5592r_info;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver adi_ad5592r_driver = {
	.driver = {
		.name = "ad5592r",
	},
	.probe = adi_ad5592r_probe,
};
module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Laurentiu Popa <laurentiu.popa@analog.com>");
MODULE_DESCRIPTION("AD5592R IIO ADI Driver");
MODULE_LICENSE("GPL v2");