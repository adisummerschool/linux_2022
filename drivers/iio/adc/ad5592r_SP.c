#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

struct adi_ad5592r_priv {
	bool enable;
};

int adi_ad5592r_read_raw(struct iio_dev *indio_dev,
			 struct iio_chan_spec const *chan, int *val, int *val2,
			 long mask)
{
	struct adi_ad5592r_priv *priv = iio_priv(indio_dev);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->channel) {
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

int adi_ad5592r_write_raw(struct iio_dev *indio_dev,
			  struct iio_chan_spec const *chan, int val, int val2,
			  long mask)
{
	return 0;
};

static const struct iio_info adi_ad5592r_info = {
	.read_raw = &adi_ad5592r_read_raw,
	.write_raw = &adi_ad5592r_write_raw,
};

static const struct iio_chan_spec adi_ad5592r_channels[] = {
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

static int adi_ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	

	indio_dev = devm_iio_device_alloc(&spi->dev,0);
	if (!indio_dev)
		return -ENOMEM;

	indio_dev->name = "ad5592r";
	indio_dev->info = &adi_ad5592r_info;
	indio_dev->channels = adi_ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(adi_ad5592r_channels);

	dev_info(&spi->dev,indio_dev);

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver adi_ad5592r_driver = {
	.driver = {
		.name = "ad5592r_SP",
	},
	.probe = adi_ad5592r_probe,
};
module_spi_driver(adi_ad5592r_driver);

MODULE_AUTHOR("Furnigel Andrei <dafurnigel@gmail.com>");
MODULE_DESCRIPTION("Analog Devices adi5592 Driver");
MODULE_LICENSE("GPL v2");