#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/iio/iio.h>

static const struct iio_info adi_adi5592r_info = {};

static int adi_ad5592r_probe(struct spi_device *spi)
{
    struct iio_dev *indio_dev;

    indio_dev = devm_iio_device_alloc(&spi->dev,0);

    if (!indio_dev) return -ENOMEM;

    indio_dev->name ="adi5592r";
    indio_dev->info = &adi_adi5592r_info;
    dev_info(&spi->dev,"adi5592r Probed");
    
    return devm_iio_device_register(&spi->dev,indio_dev);  
}

static struct spi_driver adi_adi5592r_driver = {
    .driver = {
        .name = "adi5592r",
        
    },
    .probe = adi_ad5592r_probe,

};
module_spi_driver(adi_adi5592r_driver);
MODULE_AUTHOR("Furnigel Andrei <dafurnigel@gmail.com>");
MODULE_DESCRIPTION("Analog Devices adi5592 Driver");
MODULE_LICENSE("GPL v2");