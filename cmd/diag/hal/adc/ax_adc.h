// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __ADC_H
#define __ADC_H

struct ax_adc_ops {
    int (*start_channel)(int channel);
    int (*start_channels)(unsigned int channel_mask);
    
    int (*channel_data)(int channel, unsigned int *data);
    int (*channels_data)(unsigned int channel_mask, unsigned int *data);

    int (*stop)(void);
};

extern struct ax_adc_ops adc;

static inline int ax_adc_start_channel(int channel) {
    if (adc.start_channel == NULL) return -ENOSYS;

    return adc.start_channel(channel); 
}

static inline int ax_adc_start_channels(unsigned int channel_mask) {
    if (adc.start_channels == NULL) return -ENOSYS;

    return adc.start_channels(channel_mask); 
}

static inline int ax_adc_channel_data(int channel, unsigned int *data) {  
    if (adc.channel_data == NULL) return -ENOSYS;

    return adc.channel_data(channel, data); 
}

static inline int ax_adc_channels_data(unsigned int channel_mask, unsigned int *data) {  
    if (adc.channels_data == NULL) return -ENOSYS;

    return adc.channels_data(channel_mask, data); 
}

static inline int ax_adc_stop(void) {
    if (adc.stop == NULL) return -ENOSYS;

    return adc.stop(); 
}

#endif /* __ADC_H */
