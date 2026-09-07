// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __SPI_H
#define __SPI_H

struct ax_spi_ops {
    int (*setup)(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode);
    int (*xfer)(unsigned int bus, unsigned int bitlen, const void *dout,
		void *din);
};
extern struct ax_spi_ops spi;

static inline int ax_spi_setup(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode) {  
    if (spi.setup == NULL) return -ENOSYS;

    return spi.setup(bus, cs, max_hz, mode); 
}

static inline int ax_spi_xfer(unsigned int bus, unsigned int bitlen, const void *dout,
		void *din) {  
    if (spi.xfer == NULL) return -ENOSYS;

    return spi.xfer(bus, bitlen, dout, din); 
}

#endif /* __SPI_H */

