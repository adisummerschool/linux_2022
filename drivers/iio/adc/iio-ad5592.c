// SPDX-License-Identifier: GPL-2.0-only
/*
 * IIO Analog Devices emulator driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static struct iio_info iio_ad5592_info = {
};

static int iio_ad5592_probe(struct spi_device *spi){
    struct iio_dev *indio_dev;

    indio_dev = devm_iio_device_alloc(&spi->dev, 0);

    if(indio_dev==NULL){
        return -ENOMEM;
    }

    indio_dev->name = "iio-ad5592";
    indio_dev->info = &iio_ad5592_info;

    dev_info(&spi->dev, "iio_ad5592 Probed man");

    return devm_iio_device_register(&spi->dev, indio_dev);
    
}


static struct spi_driver iio_ad5592_driver = {
    .driver = {
        .name = "iio-ad5592",
    },
    .probe = iio_ad5592_probe,
};

module_spi_driver(iio_ad5592_driver);

MODULE_AUTHOR("Ceclan Dumitru-Ioan <mitrutzceclan@gmail.com>");
MODULE_DESCRIPTION("IIO Analog Devices ad5592 driver");
MODULE_LICENSE("GPL v2");