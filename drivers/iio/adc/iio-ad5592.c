// SPDX-License-Identifier: GPL-2.0-only
/*
 * IIO Analog Devices emulator driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

#define ADI_AD5592R_REG_RDBACK 		0x7
#define   ADI_AD5592R_MASK_RB_EN	BIT(6)		
#define   ADI_AD5592R_MASK_RB_REG	GENMASK(5,2)		


#define ADI_AD5592R_ADDR_MASK		GENMASK(14,11)
#define ADI_AD5592R_VAL_MASK		GENMASK(10,0)


static struct iio_ad5592r_state {
	struct spi_device *spi;
};

static int iio_ad5592r_write_ctrl(struct iio_ad5592r_state *st,
	u8 reg,u16 val
){
	u16 msg = 0;
	__be16 tx;
	
	// msg |= (((u16)reg & 0xF)<< 11)
	// msg |= val & 0x7FF

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK,reg);
	msg |= FIELD_PREP(ADI_AD5592R_VAL_MASK,val);
	
	put_unaligned_be16(msg, &tx);

	return spi_write(st->spi, &tx, sizeof(tx)); 
};

static int iio_ad5592r_nop(struct iio_ad5592r_state *st, __be16 *rx){
	struct spi_transfer xfer = {
		.tx_buf = 0x00,
		.rx_buf = rx,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int iio_ad5592r_read_ctrl(struct iio_ad5592r_state *st,u8 reg, u16 *val){
	u16 msg=0;
	__be16 tx;
	__be16 rx;
	int ret;

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, ADI_AD5592R_REG_RDBACK);
	msg |= ADI_AD5592R_MASK_RB_EN;
	msg |= FIELD_PREP(ADI_AD5592R_MASK_RB_REG, reg);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if(ret)
	{
		dev_err(&st->spi->dev,"Fail reading ctrl reg at spi write");
		return ret;
	}

	ret = iio_ad5592r_nop(st, &rx);
	if(ret)
	{
		dev_err(&st->spi->dev,"Fail reading ctrl reg at nop");
		return ret;
	}

	*val = get_unaligned_be16(rx);

	return 0;
}

int ad5592r_read_raw(struct iio_dev *indio_dev,
		     struct iio_chan_spec const *chan, int *val, int *val2,
		     long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->channel) {
			//in order to read ADC channel 0 we have to write to the ADC sequence register + 2xNOP
			// 0(D15) 0010(seq addr) 0(reserved) 0(rep) 0(temp) 00000001 (channel 0 bit set)
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
}

static struct iio_info iio_ad5592_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
};

static const struct iio_chan_spec ad5592r_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 0,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_ENABLE)|
			BIT(IIO_CHAN_INFO_HARDWAREGAIN),
	},

	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 1,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 2,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},

	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 3,
		.indexed = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
	},
};

static int iio_ad5592_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	static struct iio_ad5592r_state *st;
	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));

	if (indio_dev == NULL) {
		return -ENOMEM;
	}
	st = iio_priv(indio_dev);
	indio_dev->name = "iio-ad5592";
	indio_dev->info = &iio_ad5592_info;
	indio_dev->channels = ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

	st->spi = spi;

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