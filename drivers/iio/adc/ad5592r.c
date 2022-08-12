// SPDX-License-Identifier: GPL-2.0+
/*
 * AD7124 SPI ADC driver
 *
 * Copyright 2018 Analog Devices Inc.
 */
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

struct adi_ad5592r_priv {
    bool enable;
};

int adi_ad5592r_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val,
			int *val2,
			long mask)

{ 
     struct adi_ad5592r_priv *priv = iio_priv(indio_dev);

    switch (mask){
    case IIO_CHAN_INFO_RAW:
    if(chan->channel)
       *val = 2;
    else
       *val = 5;
    return IIO_VAL_INT;

    case IIO_CHAN_INFO_ENABLE:
       *val = priv->enable;
       return IIO_VAL_INT;

    }
    return -EINVAL;
}
int adi_ad5592r_write_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int val,
			int val2,
			long mask)
{    
    struct adi_ad5592r_priv *priv = iio_priv(indio_dev);
    switch (mask){
    case IIO_CHAN_INFO_ENABLE:
        priv->enable = val;
    }
    return 0;

}

static const struct iio_info adi_ad5592r_info = {
    .read_raw = &adi_ad5592r_read_raw,
    .write_raw = &adi_ad5592r_write_raw,
};

static const struct iio_chan_spec adi_ad5592r_channels[] = {
{
     .type = IIO_VOLTAGE,
     .output = 0,
     .channel = 0,
     .indexed = 1 ,
     .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
     .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
     
},

{
     .type = IIO_VOLTAGE,
     .output = 0,
     .channel = 1,
     .indexed = 1,
     .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
     .info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)
}

};

static int adi_ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
    struct adi_ad5592r_priv *priv;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

    indio_dev->name = "iio-adi-ad5592r";
	indio_dev->info = &adi_ad5592r_info;
    indio_dev->channels = adi_ad5592r_channels;
    indio_dev->num_channels = ARRAY_SIZE(adi_ad5592r_channels);

    priv = iio_priv(indio_dev);
    priv->enable = false;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver adi_ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    }
    .probe = adi_ad5592r_probe,
};

module_spi_driver(adi_ad5592r_driver);
MODULE_AUTHOR("Radu Alina <alexandrinaradu23@yahoo.com>");
MODULE_DESCRPIPTION("iio Analog Devices ad5592r Driver");
MODULE_LICENSE("GPL v2");