#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

static int adi_ad5592r_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val,
				int *val2,
				long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		*val = 0;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_RAW:
		if (chan->channel)
			*val = 0;
		else
			*val = 0;
		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static int adi_ad5592r_write_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int val,
				 int val2,
				 long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_ENABLE:
		return 0;
	}

	return -EINVAL;
}
static const struct iio_info adi_ad5592r_info = {
	.read_raw = &adi_ad5592r_read_raw,
	.write_raw = &adi_ad5592r_write_raw,
};

static const struct iio_chan_spec adi_ad5592r_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.output = 0,
		.indexed = 1,
		.channel = 0,
	}, 
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.output = 0,
		.indexed = 1,
		.channel = 1,
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.output = 0,
		.indexed = 1,
		.channel = 2,
	},
	{
		.type = IIO_VOLTAGE,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.output = 0,
		.indexed = 1,
		.channel = 3,
	},
};

static int adi_ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, 0);
	if(!indio_dev)
	{
		return -ENOMEM;
	}

	indio_dev->name = "ad5592r";
	indio_dev->channels = adi_ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(adi_ad5592r_channels);
	indio_dev->info = &adi_ad5592r_info;

	dev_info(&spi->dev, "ad5592r Probed");

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver adi_ad5592r_driver = {
    .driver = {
        .name = "ad5592r_summer",
    },
    .probe = adi_ad5592r_probe,
};

module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Ciprian Hegbeli <ciprian.hegbeli@analog.com>");
MODULE_DESCRIPTION("IIO Analog Devices Emulator Driver");
MODULE_LICENSE("GPL v2");
