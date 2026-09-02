// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __MAILBOX_H
#define __MAILBOX_H

struct ax_mbox_ops {
    int (*request)(uint chan);
    int (*free)(uint chan);
    int (*send)(uint chan, const void *data);
    int (*recv)(uint chan, void *data, ulong timeout);
};

extern struct ax_mbox_ops mbox;

static inline int ax_mbox_request(uint chan) {  
    if (mbox.request == NULL) return -ENOSYS;

    return mbox.request(chan); 
}

static inline int ax_mbox_free(uint chan) {  
    if (mbox.free == NULL) return -ENOSYS;

    return mbox.free(chan); 
}

static inline int ax_mbox_send(uint chan, const void *data) {
    if (mbox.send == NULL) return -ENOSYS;

    return mbox.send(chan, data);
}

static inline int ax_mbox_recv(uint chan, void *data, ulong timeout) {
    if (mbox.recv == NULL) return -ENOSYS;

    return mbox.recv(chan, data, timeout);
}

#endif /* __MAILBOX_H */
