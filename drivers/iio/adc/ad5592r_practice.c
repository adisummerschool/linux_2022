// SPDX-License-Identifier: GPL-2.0
/*
 * IIO Analog Devices, Inc. Emulator Driver
 *
 * Copyright (C) 2022 Analog Devices, Inc.
 */

#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define ADI_AD5592R_REG_ADC_SEQ		0x2
#define ADI_AD5592R_REG_ADC_PIN		0x4
#define ADI_AD5592R_REG_READBACK	0x7
#define ADI_AD5592R_MASK_RB_EN		BIT(6)
#define ADI_AD5592R_MASK_REG_RB 	GENMASK(5, 2)
#define ADI_AD5592R_REG_POWER_REF	0xB
#define ADI_AD5592R_MASK_EN_REF		BIT(9)
#define ADI_AD5592R_REG_RESET		0XF
#define ADI_AD5592R_VAL_RESET		0x5AC

#define ADI_AD5592R_MASK_ADC_PIN(x)	BIT(x)
#define ADI_AD5592R_MASK_ADC_RESP_ADDR	GENMASK(14, 12)
#define ADI_AD5592R_MASK_ADC_RESP_VAL 	GENMASK(11, 0)
#define ADI_AD5592R_ADDR_MASK 		GENMASK(14, 11)
#define ADI_AD5592R_VAL_MASK 		GENMASK(10, 0)

#define ADI_AD5592R_MAX_NR_OF_ADC	7
#define ADI_AD5592R_DEFAULT_ADC_PIN_CFG ADI_AD5592R_MASK_ADC_PIN(0) |\
					ADI_AD5592R_MASK_ADC_PIN(1) |\
					ADI_AD5592R_MASK_ADC_PIN(2) |\
					ADI_AD5592R_MASK_ADC_PIN(3)

static struct adi_ad5592r_state {
	struct spi_device *spi;
};

static int adi_ad5592r_write_ctr(struct adi_ad5592r_state *st, 
				 u8 reg, 
				 u16 val)
{
	u16 msg = 0;
	__be16 tx;

	// msg |= (u16)((reg << 11) & ADI_AD5592R_ADDR_MASK);
	// msg |= (val & ADI_AD5592R_VAL_MASK);

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, reg);
	msg |= FIELD_PREP(ADI_AD5592R_VAL_MASK, val);

	put_unaligned_be16(msg, &tx);

	return spi_write(st->spi, &tx, sizeof(tx));
};

static int adi_ad5592r_nop(struct adi_ad5592r_state *st, __be16 *rx)
{
	struct spi_transfer xfer = {
		.tx_buf = 0,
		.rx_buf = rx,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
};

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
	if (ret) {
		dev_err(&(st->spi->dev), "Fail read ctrl reg at SPI write");
		return ret;
	}

	ret = adi_ad5592r_nop(st, &rx);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail read ctrl reg at nop");
		return ret;
	}

	*val = get_unaligned_be16(&rx);

	return 0;
};

static int adi_ad5592r_read_adc(struct iio_dev *indio_dev, u8 chan, u16 *val)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);

	u16 msg = 0;
	u16 resp;
	u16 resp_addr;
	__be16 tx;
	__be16 rx;
	int ret;

	if(chan > ADI_AD5592R_MAX_NR_OF_ADC) {
		dev_dbg(&(st->spi->dev),"ADC Channel exceeds maximum number");
		return -EINVAL;
	}

	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, ADI_AD5592R_REG_ADC_SEQ);
	msg |= FIELD_PREP(ADI_AD5592R_VAL_MASK, ADI_AD5592R_MASK_ADC_PIN(chan));

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if (ret) {
		dev_err(&(st->spi->dev), "Failed to write sequencer reg");
		return ret;
	}

	// 1st nop
	ret = adi_ad5592r_nop(st, NULL);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail at read ADC 1st nop");
		return ret;
	}	

	// 2nd nop
	ret = adi_ad5592r_nop(st, &rx);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail at read ADC 2st nop");
		return ret;
	}

	resp = get_unaligned_be16(&rx);

	resp_addr = ADI_AD5592R_MASK_ADC_RESP_ADDR & resp;
	resp_addr = (resp_addr >> 12);

	dev_info(&(st->spi->dev),"ADC response addr = %d", resp_addr);
	if(resp_addr != chan) {
		dev_err(&(st->spi->dev),"Response does not match requested chan");
		return -EIO;
	}

	*val = resp & ADI_AD5592R_MASK_ADC_RESP_VAL;

	return 0;
}

static int ad5592r_read_raw(struct iio_dev *indio_dev,
		     struct iio_chan_spec const *chan, int *val, int *val2,
		     long mask)
{
	int ret;
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = adi_ad5592r_read_adc(indio_dev,chan->channel,(u16 *)val);
		if(ret)
			return ret;
		return IIO_VAL_INT;
		}
	return -EINVAL;
};

int ad5592r_write_raw(struct iio_dev *indio_dev,
		      struct iio_chan_spec const *chan, int val, int val2,
		      long mask)
{
	return 0;
};

static int adi_ad5592r_reg_access(struct iio_dev *indio_dev, 
				  unsigned reg, unsigned writeval,
				  unsigned *readval)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	u16 read;
	int ret;

	if(readval) {
		ret = adi_ad5592r_read_ctr(st,reg, &read);
		if(ret) {
			dev_err(&(st->spi->dev), "DBG read failed");
			return ret;
		}
		*readval = read;
		return ret;
	}

	return adi_ad5592r_write_ctr(st, reg, writeval);
}

static const struct iio_info adi_ad5592r_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
	.debugfs_reg_access = &adi_ad5592r_reg_access,
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

static int adi_ad5592r_init(struct iio_dev *indio_dev)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	int ret;

	// reset
	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_RESET,
					ADI_AD5592R_VAL_RESET);

	if (ret) {
		dev_err(&(st->spi->dev), "Fail write RESET reg");
		return ret;
	}

	usleep_range(250, 300);

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_POWER_REF,
					ADI_AD5592R_MASK_EN_REF);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail write power reg ref en");
		return ret;
	}

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_ADC_PIN,
					ADI_AD5592R_DEFAULT_ADC_PIN_CFG);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail write ADC PIN config");
		return ret;
	}		

	return 0;
}

static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct adi_ad5592r_state *st;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	st = iio_priv(indio_dev);

	indio_dev->name = "ad5592r_practice";
	indio_dev->info = &adi_ad5592r_info;
	indio_dev->channels = ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

	st->spi = spi;

	ret = adi_ad5592r_init(indio_dev);
	if (ret) {
		dev_err(&(st->spi->dev), "Fail Init");
		return ret;
	}

	return devm_iio_device_register(&spi->dev, indio_dev);
};

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
