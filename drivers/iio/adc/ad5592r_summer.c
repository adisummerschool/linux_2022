#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/iio/iio.h>

static const struct iio_info adi_ad5592r_info = {

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
