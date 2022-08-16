// SPDX-License-Identifier: GPL-2.0
/*
 * IIO Analog Devices, Inc. Emulator Driver
 *
 * Copyright (C) 2022 Analog Devices, Inc.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

struct ad5592r_priv {
	bool enable
};

int ad5592r_read_raw(struct iio_dev *indio_dev,
		     struct iio_chan_spec const *chan, int *val, int *val2,
		     long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->channel) {
			//in order to read ADC channel 0 we have to write
			//to the ADC sequence register + 2xNOP
			// 0(D15) 0010(seq addr) 0(reserved) 0(rep)
			// 0(temp) 00000001 (channel 0 bit set)
		case 0:
			*val = 0;
			break;
		case 1:
			*val = 1;
			break;
		case 2:
			*val = 2;
			break;
		case 3:
			*val = 3;
			break;
		}
		return IIO_VAL_INT;
	}
	return -EINVAL;
}

int ad5592r_write_raw(struct iio_dev *indio_dev,
		      struct iio_chan_spec const *chan, int val, int val2,
		      long mask)
{
	return 0;
};

static const struct iio_info adi_ad5592r_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
};

static const struct iio_chan_spec ad5592r_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 0,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 1,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 2,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 3,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
};

static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	//struct ad5592r_priv *priv;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "ad5592r_practice";
	indio_dev->info = &adi_ad5592r_info;
	indio_dev->channels = ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_driver = {
	.driver = {
		.name = "ad5592r_practice",
	},
	.probe = ad5592r_probe,
};

module_spi_driver(ad5592r_driver);

MODULE_AUTHOR("Laurentiu Popa <laurentiu.popa@analog.com>");
MODULE_DESCRIPTION("AD5592R IIO ADI Driver");
MODULE_LICENSE("GPL v2");