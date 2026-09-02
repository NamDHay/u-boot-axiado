// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __TPM_H
#define __TPM_H

struct ax_tpm_ops {
    int (*open)(uint chan);
    int (*close)(uint chan);
    int (*send)(uint chan, const void *data, size_t len);
    int (*recv)(uint chan, void *data, size_t len);
};

extern struct ax_tpm_ops tpm;

static inline int ax_tpm_open(uint chan) {  
    if (tpm.open == NULL) return -ENOSYS;

    return tpm.open(chan); 
}

static inline int ax_tpm_close(uint chan) {  
    if (tpm.close == NULL) return -ENOSYS;

    return tpm.close(chan); 
}

static inline int ax_tpm_send(uint chan, const void *data, size_t len) {
    if (tpm.send == NULL) return -ENOSYS;

    return tpm.send(chan, data, len);
}

static inline int ax_tpm_recv(uint chan, void *data, size_t len) {
    if (tpm.recv == NULL) return -ENOSYS;

    return tpm.recv(chan, data, len);
}

#endif /* __TPM_H */
