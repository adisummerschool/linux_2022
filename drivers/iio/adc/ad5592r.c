// SPDX-License-Identifier: GPL-2.0+
/*
 * AD7124 SPI ADC driver
 *
 * Copyright 2018 Analog Devices Inc.
 */
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>

struct ad5592r_state{
    struct regmap *regmap
};

static int ad5592r_reg_access(struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
    struct ad5592r_state *st = iio_priv(indio_dev);
    if(readval)
        return regmap_read(st->regmap, reg, readval);
    return regmap_write(st->regmap, reg, writeval);
}
static const struct iio_info adi_ad5592r_info = {
    .debugfs_reg_access = &ad5592r_reg_access
};

static const struct regmap_config ad5592r_regmap_cfg = {
       .reg_bits = 5,
       .val_bits = 11,
};

static int ad5592r_probe(struct spi_device *spi)
{   
    struct iio_dev *indio_dev;
    struct ad5592r_state *st;

    indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
    if(!indio_dev)
    {
     return -ENOMEM;
    }

    st = iio_priv(indio_dev);

    st->regmap = regmap_init_spi(spi,&ad5592r_regmap_cfg);
    if(IS_ERR(st->regmap))
       return PTR_ERR(st->regmap);


    indio_dev->name = "iio-adi-ad5592r";
    indio_dev->info = &adi_ad5592r_info;

    dev_info(&spi->dev, "iio-adi-ad5592r Probed");

    return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    },
    .probe = ad5592r_probe,
};
module_spi_driver(ad5592r_driver);
MODULE_AUTHOR("Muresan Mihai <mymihai22@yahoo.com>");
MODULE_DESCRIPTION("IIO Analog Devices ad5592r Driver");
MODULE_LICENSE("GPL v2");