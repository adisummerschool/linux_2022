// SPDX-License-Identifier: GPL-2.0-only
/*
 * Analog Devices AD3552R
 * Analog to Digital converter driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define ADI_AD5592R_REG_READBACK	0x7
#define	  ADI_AD5592R_MASK_RB_EN	BIT(6)
#define	  ADI_AD5592R_MASK_REG_RB	GENMASK(5, 2)
#define ADI_AD5592R_REG_RESET		0xF
#define   ADI_AD5592R_VAL_RESET		0x5AC

#define ADI_AD5592R_ADDR_MASK		GENMASK(14, 11)
#define ADI_AD5592R_VAL_MASK		GENMASK(10, 0)

static struct adi_ad5592r_state {
	struct spi_device *spi;
};

static int adi_ad5592r_write_ctr(struct adi_ad5592r_state *st,
				 u8 reg,
				 u16 val)
{
	u16 msg = 0;
	__be16 tx;

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, reg);
	msg |= FIELD_PREP(ADI_AD5592R_VAL_MASK, val);

	put_unaligned_be16(msg, &tx);

	return spi_write(st->spi, &tx, sizeof(tx));
}

static int adi_ad5592r_nop(struct adi_ad5592r_state *st, __be16 *rx)
{
	struct spi_transfer xfer = {
		.tx_buf = 0,
		.rx_buf = rx,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int adi_ad5592r_read_ctr(struct adi_ad5592r_state *st,
				u8 reg,
				u16 *val)
{
	u16 msg = 0;
	__be16 tx;
	__be16 rx;
	int ret;

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, ADI_AD5592R_REG_READBACK);
	msg |= ADI_AD5592R_MASK_RB_EN;
	msg |= FIELD_PREP(ADI_AD5592R_MASK_REG_RB, reg);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if(ret)
	{
		dev_err(&st->spi->dev, "Fail read ctrl reg at SPI write");
		return ret;
	}

	ret = adi_ad5592r_nop(st, &rx);
	if(ret)
	{
		dev_err(&st->spi->dev, "Fail read ctrl reg at nop");
		return ret;
	}

	*val = get_unaligned_be16(&rx);
	
	return 0;
}

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

static int adi_ad5592r_reg_access(struct iio_dev *indio_dev,
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	u16 read;
	int ret;

	if (readval) {
		ret = adi_ad5592r_read_ctr(st, reg, &read);
		if(ret){
			dev_err(&st->spi->dev, "DBG read failed");
			return ret;
		}
		dev_info(&st->spi->dev, "read_reg=0x%x", read);
		*readval = read;
		return ret;
	}

	return 	adi_ad5592r_write_ctr(st, reg, writeval);
}

static const struct iio_info adi_ad5592r_info = {
	.read_raw = &adi_ad5592r_read_raw,
	.write_raw = &adi_ad5592r_write_raw,
	.debugfs_reg_access = &adi_ad5592r_reg_access,
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

static int adi_ad5592r_init(struct iio_dev *indio_dev)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	int ret;

	//reset
	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_RESET, 
				    ADI_AD5592R_VAL_RESET);
	if(ret)
	{
		dev_err(&st->spi->dev, "Reset Failed");
		return ret;
	}
	usleep_range(250, 300);

	return 0;	
}

static int adi_ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct adi_ad5592r_state *st;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if(!indio_dev)
	{
		return -ENOMEM;
	}
	st = iio_priv(indio_dev);

	indio_dev->name = "ad5592r";
	indio_dev->channels = adi_ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(adi_ad5592r_channels);
	indio_dev->info = &adi_ad5592r_info;

	st->spi = spi;

	ret = adi_ad5592r_init(indio_dev);
	if(ret)
	{
		dev_err(&st->spi->dev, "Init Failed");
		return ret;
	}

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
