#include <linux/spi/spi.h>
#include <linux/module.h>
static struct spi_driver adi_ad5592r_driver = {
    .driver = {
        .name = "ad5592r",
    }
};
module_spi_driver(adi_ad5592r_driver);
MODULE_AUTHOR("Coroian Razvan <razvan.coroian@gmail.com>");
MODULE_DESCRIPTION("IIO Analog Devices Emulator Driver");
MODULE_LICENSE("GPL v2");