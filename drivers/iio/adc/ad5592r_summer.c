// SPDX-License-Identifier: GPL-2.0+
/*
 * IIO Analog Devices AD5592r Driver
 *
 * Copyright 2022 Analog Devices Inc.
 */

#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/bitfield.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>

#define AD5592R_REG_GP_CTR 0x3
#define AD5592R_MASK_ADC_RANGE BIT(5)

#define AD5592R_REG_ADC_SEQ 0x2

#define AD5592R_MASK_REPEAT BIT(9)

#define AD5592R_REG_ADC_PIN 0x4
#define AD5592R_MASK_ADC_PIN(x) BIT(x)

#define AD5592R_MAX_NR_OF_ADC 7

#define AD5592R_REG_READBACK 0x7
#define AD5592R_MASK_RB_EN BIT(6)
#define AD5592R_MASK_REG_RB GENMASK(5, 2)

#define AD5592R_REG_POWER_REF 0xB
#define AD5592R_MASK_POWER_REF BIT(9)

#define AD5592R_REG_RESET 0xF
#define AD5592R_VAL_RESET 0x5AC

#define AD5592R_MASK_ADC_RESP_ADDR GENMASK(14, 12)
#define AD5592R_MASK_ADC_RESP_VAL GENMASK(11, 0)
#define AD5592R_ADDR_MASK GENMASK(14, 11)
#define AD5592R_VAL_MASK GENMASK(10, 0)
#define AD5592R_DEFAULT_ADC_PIN_CFG                                            \
	AD5592R_MASK_ADC_PIN(0) | AD5592R_MASK_ADC_PIN(1) |                    \
		AD5592R_MASK_ADC_PIN(2) | AD5592R_MASK_ADC_PIN(3)

static struct ad5592r_state {
	struct spi_device *spi;
	bool double_gain;
	u8 nr_active_scans;
};

/*
ad5592r_write_ctr(st,0xF,0x5AC)
0x7DAC
0xAC7D
*/

static int ad5592r_write_ctr(struct ad5592r_state *st, u8 reg, u16 val)
{
	u16 msg = 0;
	__be16 tx;

	msg |= FIELD_PREP(AD5592R_ADDR_MASK, reg);
	msg |= FIELD_PREP(AD5592R_VAL_MASK, val);

	put_unaligned_be16(msg, &tx);

	return spi_write(st->spi, &tx, sizeof(tx));
}

static int ad5592r_nop(struct ad5592r_state *st, __be16 *rx)
{
	struct spi_transfer xfer = {
		.tx_buf = 0,
		.rx_buf = rx,
		.len = 2,
	};

	return spi_sync_transfer(st->spi, &xfer, 1);
}

static int ad5592r_read_ctr(struct ad5592r_state *st, u8 reg, u16 *val)
{
	u16 msg = 0;
	__be16 tx;
	__be16 rx;
	int ret;

	msg |= FIELD_PREP(AD5592R_ADDR_MASK, AD5592R_REG_READBACK);
	msg |= AD5592R_MASK_RB_EN;
	msg |= FIELD_PREP(AD5592R_MASK_REG_RB, reg);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if (ret) {
		dev_err(&st->spi->dev, "Fail to read ctrl reg at SPI write");
		return ret;
	}

	ret = ad5592r_nop(st, &rx);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to read ctrl reg at nop");
		return ret;
	}

	*val = get_unaligned_be16(&rx);

	return 0;
}
static int ad5592r_read_adc(struct iio_dev *indio_dev, u8 chan, u16 *val)
{
	struct ad5592r_state *st = iio_priv(indio_dev);
	u16 msg = 0;
	u16 resp = 0;
	u16 resp_addr = 0;
	__be16 tx;
	__be16 rx;
	int ret;

	if (chan > AD5592R_MAX_NR_OF_ADC) {
		dev_dbg(&st->spi->dev, "ADC channel exceeds maximum number");
		return -EINVAL;
	}
	msg |= FIELD_PREP(AD5592R_ADDR_MASK, AD5592R_REG_ADC_SEQ);
	msg |= AD5592R_MASK_ADC_PIN(chan);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if (ret) {
		dev_err(&st->spi->dev, "Fail to write ADC Sequence Register");
		return ret;
	}

	ret = ad5592r_nop(st, NULL);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to read adc first nop");
		return ret;
	}

	ret = ad5592r_nop(st, &rx);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to read adc second nop");
		return ret;
	}

	resp = get_unaligned_be16(&rx);

	resp_addr = AD5592R_MASK_ADC_RESP_ADDR & resp;
	resp_addr = (resp_addr >> 12);


	dev_info(&st->spi->dev, "ADC response = %d", resp);
	dev_info(&st->spi->dev, "ADC response addr = %d", resp_addr);
	if (resp_addr != chan) {
		dev_err(&st->spi->dev,
			"Response does not match requested channel");
		return -EIO;
	}

	*val = resp & AD5592R_MASK_ADC_RESP_VAL;
	dev_info(&st->spi->dev, "ADC response value = %d", val);

	return 0;
}

static int ad5592r_update_gain(struct iio_dev *indio_dev, bool double_gain)
{
	u16 rx;
	int ret;
	struct ad5592r_state *st = iio_priv(indio_dev);

	ret = ad5592r_read_ctr(st, AD5592R_REG_GP_CTR, &rx);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to read range from gp register");
		return ret;
	}

	if (double_gain)
		rx |= AD5592R_MASK_ADC_RANGE;
	else
		rx &= ~AD5592R_MASK_ADC_RANGE;

	return ad5592r_write_ctr(st, AD5592R_REG_GP_CTR, rx);
}

static irqreturn_t ad5592r_trigger_thread(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ad5592r_state *st = iio_priv(indio_dev);
	__be16 rx;
	u16 sample;
	u16 buff[AD5592R_MAX_NR_OF_ADC];
	int ret;
	u8 i;

	for (i = 0; i < st->nr_active_scans; i++) {
		ret = ad5592r_nop(st, &rx);
		if (ret) {
			dev_err(&st->spi->dev, "Fail buffer at nop");
			return IRQ_HANDLED;
		}
		sample = get_unaligned_be16(&rx);
		sample &= AD5592R_MASK_ADC_RESP_VAL;
		buff[i] = sample;
	}
	
	iio_push_to_buffers(indio_dev, buff);
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static int ad5592r_preenable(struct iio_dev *indio_dev)
{
	struct ad5592r_state *st = iio_priv(indio_dev);
	int ret;
	u16 msg;
	u16 active_scan;

	active_scan = *(indio_dev->active_scan_mask);
	st->nr_active_scans = hweight16(active_scan);

	msg = AD5592R_MASK_REPEAT | active_scan;

	ret = ad5592r_write_ctr(st, AD5592R_REG_ADC_SEQ, msg);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to preenable at SPI write");
		return ret;
	}

	ret = ad5592r_nop(st, NULL);
	if (ret) {
		dev_err(&st->spi->dev, "Fail to preenable at nop");
		return ret;
	}

	return 0;
}

static const struct iio_buffer_setup_ops ad5592r_buffer_ops ={
	.preenable = &ad5592r_preenable,
};


static int ad5592r_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan, int *val,
			    int *val2, long mask)
{
	int ret;
	struct ad5592r_state *st = iio_priv(indio_dev);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ad5592r_read_adc(indio_dev, chan->channel, (u16 *)val);
		if (ret)
			return ret;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		*val = st->double_gain;
		return IIO_VAL_INT;
	}
	return -EINVAL;
}

static int ad5592r_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan, int val,
			     int val2, long mask)
{
	struct ad5592r_state *st = iio_priv(indio_dev);
	switch (mask) {
	case IIO_CHAN_INFO_HARDWAREGAIN:
		st->double_gain = val;
		return ad5592r_update_gain(indio_dev, val);
	}
	return -EINVAL;
}

static const struct iio_chan_spec ad5592r_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 0,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_index = 0,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		}
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 1,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_index = 1,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 2,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_index = 2,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	},
	{
		.type = IIO_VOLTAGE,
		.output = 0,
		.channel = 3,
		.indexed = 1,
		.info_mask_separate =
			BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_ENABLE),
		.info_mask_shared_by_all = BIT(IIO_CHAN_INFO_HARDWAREGAIN),
		.scan_index = 3,
		.scan_type = {
			.sign = 'u',
			.realbits = 12,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_LE,
		},
	}
};

static int ad5592r_reg_access(struct iio_dev *indio_dev, unsigned reg,
			      unsigned writeval, unsigned *readval)
{
	int ret;
	u16 read;
	struct ad5592r_state *st = iio_priv(indio_dev);

	if (readval) {
		ret = ad5592r_read_ctr(st, reg, &read);
		if (ret) {
			dev_err(&st->spi->dev, "DBG read failed");
			return ret;
		}
		dev_info(&st->spi->dev, "Read reg  = %x\n", read);

		*readval = read;
		return ret;
	}
	return ad5592r_write_ctr(st, reg, writeval);
}

static const struct iio_info ad5592r_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
	.debugfs_reg_access = &ad5592r_reg_access,
};

static int ad5592r_init(struct iio_dev *indio_dev)
{
	struct ad5592r_state *st = iio_priv(indio_dev);
	int ret;

	//reset
	ret = ad5592r_write_ctr(st, AD5592R_REG_RESET, AD5592R_VAL_RESET);
	if (ret) {
		dev_err(&st->spi->dev, "Reset failed");
		return ret;
	}
	usleep_range(250, 300);

	ret = ad5592r_write_ctr(st, AD5592R_REG_POWER_REF,
				AD5592R_MASK_POWER_REF);
	if (ret) {
		dev_err(&st->spi->dev, "Power ref write failed");
		return ret;
	}

	ret = ad5592r_write_ctr(st, AD5592R_REG_ADC_PIN,
				AD5592R_DEFAULT_ADC_PIN_CFG);
	if (ret) {
		dev_err(&st->spi->dev, "ADC pin config write failed");
		return ret;
	}

	return 0;
}

static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct ad5592r_state *st;
	int ret;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev) {
		return -ENOMEM;
	}
	st = iio_priv(indio_dev);

	indio_dev->name = "ad5592r";
	indio_dev->info = &ad5592r_info;
	indio_dev->channels = ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(ad5592r_channels);

	st->spi = spi;
	st->double_gain = false;
	st->nr_active_scans = 0;

	ret = ad5592r_init(indio_dev);
	if (ret) {
		dev_err(&st->spi->dev, "Init failed");
		return ret;
	}

	devm_iio_triggered_buffer_setup(&spi->dev, indio_dev, NULL,
					ad5592r_trigger_thread, &ad5592r_buffer_ops);

	dev_info(&spi->dev, "PROBED");

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_driver = {
  .driver = {
        .name = "ad5592r_summer",
    },
    .probe = ad5592r_probe,
};

module_spi_driver(ad5592r_driver);

MODULE_AUTHOR("Bindea Cristian <cristian.bindea@analog.com>");
MODULE_DESCRIPTION("IIO Analog Devices AD5592r Drivers");
MODULE_LICENSE("GPL_V2");