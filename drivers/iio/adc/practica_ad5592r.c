// SPDX-License-Identifier: GPL-2.0+
/*
 * AD5592R SPI ADC driver
 *
 * Copyright 2018 Analog Devices Inc.
 */
#include <asm/unaligned.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#define ADI_AD5592R_REG_ADC_SEQ         0x2
#define   ADI_AD5592R_MASK_REPEAT	BIT(9)
#define ADI_AD5592R_REG_GP_CTL		0x3
#define   ADI_AD5592R_MASK_ADC_RANGE	BIT(5)
#define ADI_AD5592R_REG_ADC_PIN      	0x4   
#define ADI_AD5592R_REG_READBACK     	0x7
#define   ADI_AD5592R_MASK_RB_EN     	BIT(6)
#define   ADI_AD5592R_MASK_REG_RB    	GENMASK(5,2)
#define ADI_AD5592R_REG_POWER_REF       0xB
#define ADI_AD5592R_MASK_EN_REF         BIT(9)
#define ADI_AD5592R_REG_RESET        	0xF
#define ADI_AD5592R_VAL_RESET        	0X5AC

#define   ADI_AD5592R_MASK_ADC_PIN(x)   BIT(x) 
#define ADI_AD5592R_MASK_ADC_RESP_ADDR	GENMASK(14,12)
#define ADI_AD5592R_MASK_ADC_RESP_VAL 	GENMASK(11,0)
#define ADI_AD5592R_ADDR_MASK        	GENMASK(14,11)
#define ADI_AD5592R_VAL_MASK         	GENMASK(10,0)

#define ADI_AD5592R_MAX_NR_OF_ADC	0x7
#define ADI_AD5592R_DEFAULT_ADC_PIN_CFG ADI_AD5592R_MASK_ADC_PIN(0) |\
					ADI_AD5592R_MASK_ADC_PIN(1) |\
					ADI_AD5592R_MASK_ADC_PIN(2) |\
					ADI_AD5592R_MASK_ADC_PIN(3) 

static struct adi_ad5592r_state{

	struct spi_device *spi;
	bool double_gain;
	u8 nr_active_scan;
	

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

static int adi_ad5592r_nop(struct adi_ad5592r_state *st, __be16 *rx )
{
	struct spi_transfer xfer = {
		.tx_buf = 0,
		.rx_buf= rx,
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
		dev_err(&st->spi->dev, "Fail read ctrl reg at SPI write");
		return ret;
	}

	*val = get_unaligned_be16(&rx);

	return 0;
}

static int adi_ad5592r_read_adc(struct iio_dev *indio_dev, u8 chan, u16 *val)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);

	u16 msg = 0;
	u16 resp = 0;
	u16 resp_addr;
	__be16 tx;
	__be16 rx;
	int ret;

	if(chan > ADI_AD5592R_MAX_NR_OF_ADC)
	{
		dev_dbg(&st->spi->dev, "ADC channel exceeds maximum number!");
		return -EINVAL;
	}
	msg |= FIELD_PREP(ADI_AD5592R_ADDR_MASK, ADI_AD5592R_REG_ADC_SEQ);
	msg |= ADI_AD5592R_MASK_ADC_PIN(chan);

	put_unaligned_be16(msg, &tx);

	ret = spi_write(st->spi, &tx, sizeof(tx));
	if(ret)
	{
		dev_err(&st->spi->dev, "Failed write sequencer register!");
		return ret;
	}
	
	ret = adi_ad5592r_nop(st , NULL);
	if(ret)
	{
		dev_err(&st->spi->dev, "Failed at read adc first nop!");
		return ret;
	}

	ret = adi_ad5592r_nop(st, &rx);
	if(ret)
	{
		dev_err(&st->spi->dev, "Failed at read adc second nop!");
		return ret;
	}

	resp = get_unaligned_be16(&rx);

	resp_addr = ADI_AD5592R_MASK_ADC_RESP_ADDR & resp;
	resp_addr = (resp_addr >> 12);

	dev_info(&st->spi->dev, "ADC response addr = %d", resp_addr);
	if(resp_addr != chan)
	{
		dev_err(&st->spi->dev, "Response doesnt match requested chan");
		return -EIO;
	}

	*val = resp & ADI_AD5592R_MASK_ADC_RESP_VAL;
	return 0;

}

static irqreturn_t adi_ad5592r_trigger_thread(int irq, void *p )
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	__be16 rx;
	u16 sample;
	int ret;
	u8 i;
	
	for(i=0; i < st->nr_active_scan;i++)
	{
		ret = adi_ad5592r_nop(st, &rx);
		if(ret)
		{
			dev_err(&st->spi->dev, "Failed buffer at nop");
			return IRQ_HANDLED;
		}
		sample = get_unaligned_be16(&rx);
		iio_push_to_buffers(indio_dev, &sample);
	}

	iio_trigger_notify_done(indio_dev->trig);
	
	return IRQ_HANDLED;
}

static int adi_ad5592r_preenable(struct iio_dev *indio_dev)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	u16 msg= 0;
	u16 active_scan;
	int ret;

	active_scan = *(indio_dev->active_scan_mask);
	st->nr_active_scan = hweight16(active_scan);

	msg |= ADI_AD5592R_MASK_REPEAT | active_scan;

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_ADC_SEQ, msg);
        if(ret)
        {
                dev_err(&st->spi->dev, "Fail preenable at SPI write");
                return ret;
        }
        ret = adi_ad5592r_nop(st, NULL);
        if(ret)
        {
                dev_err(&st->spi->dev, "Fail preenable at nop");
                return ret;
        }
	return 0;
}

static const struct iio_buffer_setup_ops adi_add5592r_buffer_ops = {
	.preenable = &adi_ad5592r_preenable
};

int ad5592r_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int *val, int *val2,
		long mask)
{	
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	int ret;
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = adi_ad5592r_read_adc(indio_dev, chan->channel,(u16*)val);
		if(ret)
			return ret;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_HARDWAREGAIN:
		*val = st->double_gain;
		return IIO_VAL_INT;
	}
	return -EINVAL;
}
static int adi_ad5592r_update_gain(struct iio_dev *indio_dev, bool double_gain)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	u16 rx;
	int ret;

	ret = adi_ad5592r_read_ctr(st, ADI_AD5592R_REG_GP_CTL, &rx);
	if(ret){
		dev_err(&st->spi->dev,"Fail to read range from register!");
		return ret;
	}
	if(double_gain)
		rx |=ADI_AD5592R_MASK_ADC_RANGE;
	else
		rx &= ADI_AD5592R_MASK_ADC_RANGE;

	return adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_GP_CTL, rx); 
}

int ad5592r_write_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int val, int val2,
		long mask)
{
	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	
	switch (mask) {
	case IIO_CHAN_INFO_HARDWAREGAIN:
		st->double_gain = val;
		return adi_ad5592r_update_gain(indio_dev, val );
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

	if(readval){
		ret = adi_ad5592r_read_ctr(st, reg, &read);
		if(ret){
	
		dev_err(&(st->spi->dev), "DBG read failed");
		return ret;
	}
	*readval = read;
	return ret;
	}

	return adi_ad5592r_write_ctr(st, reg, writeval);
}


static const struct iio_info ad5592r_info = {
	.read_raw = &ad5592r_read_raw,
	.write_raw = &ad5592r_write_raw,
	.debugfs_reg_access = &adi_ad5592r_reg_access,
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
		}
		
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
		}
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
		}
	}
};

static int adi_ad5592r_init(struct iio_dev *indio_dev){

	struct adi_ad5592r_state *st = iio_priv(indio_dev);
	int ret;

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_RESET,
				    ADI_AD5592R_VAL_RESET);

	if(ret)
	{
		dev_err(&st->spi->dev, "Reset failed!");
		return ret;
	}

	usleep_range(250, 300);

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_POWER_REF,
				    ADI_AD5592R_MASK_EN_REF);
	if(ret)
	{
		dev_err(&st->spi->dev, "Power reg write failed!");
		return ret;
	}

	ret = adi_ad5592r_write_ctr(st, ADI_AD5592R_REG_ADC_PIN,
				    ADI_AD5592R_DEFAULT_ADC_PIN_CFG);
	if(ret)
	{
		dev_err(&st->spi->dev, "ADC pin reg write failed!");
		return ret;
	}


	return 0;			    
}

static int ad5592r_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	static struct adi_ad5592r_state *st;
	int ret;
	
	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));

	if (!indio_dev)
	{
		return -ENOMEM;
	}
	st = iio_priv(indio_dev);

	indio_dev->name = "practica_ad5592r";
	indio_dev->info = &ad5592r_info;
	indio_dev->channels = adi_ad5592r_channels;
	indio_dev->num_channels = ARRAY_SIZE(adi_ad5592r_channels);

	st->spi = spi;
	st->double_gain = false;
	st->nr_active_scan = 0;

	ret = adi_ad5592r_init(indio_dev);
	if(ret)
	{
		dev_err(&st->spi->dev, "Initialization Failed");
		return ret;
	}

	devm_iio_triggered_buffer_setup(&spi->dev,indio_dev, NULL,
	                                &adi_ad5592r_trigger_thread, NULL);
	
	dev_info(&spi->dev, "ad5592r Probed");

	return devm_iio_device_register(&spi->dev, indio_dev);
}

static struct spi_driver ad5592r_driver = {
	.driver = {
	.name ="practica_ad5592r",
},
	.probe = ad5592r_probe,
};

module_spi_driver(ad5592r_driver);

MODULE_AUTHOR("Rusu Razvan <rusurazvan779@gmail.com>");
MODULE_DESCRIPTION("IIO ADI Emulator Driver");
MODULE_LICENSE("GPL v2");
