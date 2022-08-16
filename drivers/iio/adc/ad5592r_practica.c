#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define AD5592R_READBACK_REG 0x7
#define AD5592R_MASK_READBACK_EN BIT(6)
#define AD5592R_MASK_READBACK_REG GENMASK(5, 2)

#define AD5592R_ADDR_MASK GENMASK(14, 11)
#define AD5592R_VAL_MASK GENMASK(10, 0)

static struct ad5592r_state {
	struct spi_device *spi;
};

static int ad5592r_write_control(struct ad5592r_state *st,
				 u8 reg, 
				 u16 val)
{
	u16 msg = 0;
	__be16 tx;

	//msg = msg | ((U16)reg << 11) & AD5592R_PRACTICA_ADDR_MASK;
	//msg = msg | val & AD5592R_PRACTICA_VAL_MASK;

	msg = msg | FIELD_PREP(AD5592R_ADDR_MASK, reg);
	msg = msg | FIELD_PREP(AD5592R_VAL_MASK, val);

	put_unaligned_be16(msg, &tx);

	return spi_write(st->spi, &msg, sizeof(tx));
}

static int ad5592r_nop(struct ad5592r_state *st, 
		       __be16 *rx)
{
	struct spi_transfer xfer = 
	{
		.tx_buf = 0,
		.rx_buf = rx,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_read_control(struct ad5592r_state *st,
				u8 reg, 
				u16 *val)
{
	u16 msg = 0;
	__be16 tx;
	__be16 rx;
	int ret;

	msg = msg | FIELD_PREP(AD5592R_ADDR_MASK, AD5592R_READBACK_REG);
	msg = msg | AD5592R_MASK_READBACK_EN;
	msg = msg | FIELD_PREP(AD5592R_MASK_READBACK_REG, reg);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));

	if(ret)
	{
		dev_err(&st->spi->dev, "Fail read control register at SPI write");
		return ret;
	}

	ret = ad5592r_nop(st, &rx);

	if(ret)
	{
		dev_err(&st->spi->dev, "Fail read control register at NOP");
		return ret;
	}

	*val = get_unaligned_be16(&rx);

	return 0;
}

int ad5592r_read_raw(struct iio_dev *indio_dev,
		     struct iio_chan_spec const *chan, 
		     int *val,
		     int *val2, 
		     long mask)
{
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

int ad5592r_write_raw(struct iio_dev *indio_dev,
		      struct iio_chan_spec const *chan, 
		      int val,
		      int val2, 
		      long mask)
{
	return 0;
}

static const struct iio_info ad5592r_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
};

static const struct iio_chan_spec ad5592r_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 0,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | 
				      BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 1,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | 
				      BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 2,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | 
				      BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 3,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | 
				      BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	}
};

static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592r_state *st;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev) {
		return -ENOMEM;
	}

	st = iio_priv(indio_dev);

	indio_dev->name = "ad5592r_practica";
	indio_dev->info = &ad5592r_info;
	indio_dev->channels = ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

	st->spi = spi;

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_driver = {
	.driver = {
		.name = "ad5592r_practica",
	},
	.probe = ad5592r_probe,
};
module_spi_driver(ad5592r_driver);

MODULE_AUTHOR("Coroian Razvan <razvan.coroian@gmail.com>");
MODULE_DESCRIPTION("IIO ADI Emulator Driver");
MODULE_LICENSE("GPL v2");