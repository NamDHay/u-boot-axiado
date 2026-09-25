/*
 * Helper functions for working with the builtin symbol table
 *
 * Copyright (c) 2008-2009 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#include <dm.h>
#include <linux/types.h>
#include <linux/string.h>

/* We need the weak marking as this symbol is provided specially */
extern const char system_map[] __attribute__((weak));

/* Given an address, return a pointer to the symbol name and store
 * the base address in caddr.  So if the symbol map had an entry:
 *		03fb9b7c_spi_cs_deactivate
 * Then the following call:
 *		unsigned long base;
 *		const char *sym = symbol_lookup(0x03fb9b80, &base);
 * Would end up setting the variables like so:
 *		base = 0x03fb9b7c;
 *		sym = "_spi_cs_deactivate";
 */
const char *symbol_lookup(unsigned long addr, unsigned long *caddr)
{
    const char *sym, *csym;
    char *esym;
    unsigned long sym_addr;

    *caddr = 0;

    if (!system_map)
        return NULL;

    sym = system_map;
    csym = NULL;

    while (*sym) {
        sym_addr = hextoul(sym, &esym);
        sym = esym;

        if (sym_addr > addr)
            break;

        *caddr = sym_addr;
        csym = sym;
        sym += strlen(sym) + 1;
    }

    return csym;
}
