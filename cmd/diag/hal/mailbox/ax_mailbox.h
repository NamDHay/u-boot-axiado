// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __MAILBOX_H
#define __MAILBOX_H

struct ax_mbox_ops {
    int (*request)(void);
    int (*free)(void);
    int (*startup)(uint chan);
    int (*shutdown)(uint chan);
    int (*send)(uint chan, const void *data);
    int (*recv)(uint chan, void *data, ulong timeout);
};

extern struct ax_mbox_ops mbox;

static inline int ax_mbox_request(void) {  
    if (mbox.request == NULL) return -ENOSYS;

    return mbox.request(); 
}

static inline int ax_mbox_free(void) {  
    if (mbox.free == NULL) return -ENOSYS;

    return mbox.free(); 
}

static inline int ax_mbox_startup(uint chan) {  
    if (mbox.startup == NULL) return -ENOSYS;

    return mbox.startup(chan); 
}

static inline int ax_mbox_shutdown(uint chan) {  
    if (mbox.shutdown == NULL) return -ENOSYS;

    return mbox.shutdown(chan); 
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
