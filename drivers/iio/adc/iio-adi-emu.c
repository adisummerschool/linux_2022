#include <linux/spi/spi.h>
#include <linux/module.h>
static struct spi_driver adi_emu_driver = {
    .driver = {
        .name = "iio-adi-emu",
    }
};
module_spi_driver(adi_emu_driver);
MODULE_AUTHOR("Coroian Razvan <razvan.coroian@gmail.com>");
MODULE_DESCRIPTION("IIO Analog Devices Emulator Driver");
MODULE_LICENSE("GPL v2");