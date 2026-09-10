// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __SPI_H
#define __SPI_H

/**
 * @brief SPI mode numbers (combinations of polarity and phases)
 */
typedef enum spi_mode {
	SPI_MODE_0 =
		0x0, /**< CPOL = 0, CPHA = 0, SCK held low in the inactive state and Sampling of data occurs at odd edges. */
	SPI_MODE_1, /**< CPOL = 0, CPHA = 1, SCK held low in the inactive state and Sampling of data occurs at even edges. */
	SPI_MODE_2, /**< CPOL = 1, CPHA = 0, SCK held high in the in active and Sampling of data occurs at odd edges. */
	SPI_MODE_3, /**< CPOL = 1, CPHA = 1, SCK held high in the in active Sampling of data occurs at even edges. */
	SPI_INVALID_MODE
} SPI_MODE;

struct ax_spi_ops {
    int (*init)(unsigned int bus);

    int (*set_speed)(unsigned int bus, uint hz);
    int (*set_cs)(unsigned int bus, uint cs);
    int (*set_mode)(unsigned int bus, uint mode);
    
    int (*xfer)(unsigned int bus, unsigned int txlen, const void *dout,
        unsigned int rx_len, void *din);
};
extern struct ax_spi_ops spi;

static inline int ax_spi_init(unsigned int bus) {  
    if (spi.init == NULL) return -ENOSYS;

    return spi.init(bus); 
}

static inline int ax_spi_set_speed(unsigned int bus, uint hz) {  
    if (spi.set_speed == NULL) return -ENOSYS;

    return spi.set_speed(bus, hz); 
}

static inline int ax_spi_set_cs(unsigned int bus, uint cs) {  
    if (spi.set_cs == NULL) return -ENOSYS;

    return spi.set_cs(bus, cs); 
}

static inline int ax_spi_set_mode(unsigned int bus, uint mode) {  
    if (spi.set_mode == NULL) return -ENOSYS;

    return spi.set_mode(bus, mode); 
}

static inline int ax_spi_xfer(unsigned int bus, unsigned int tx_len, const void *dout,
        unsigned int rx_len, void *din) {
    if (spi.xfer == NULL) return -ENOSYS;

    return spi.xfer(bus, tx_len, dout, rx_len, din); 
}

#endif /* __SPI_H */

