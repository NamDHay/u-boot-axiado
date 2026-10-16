// SPDX-License-Identifier: GPL-2.0+
#include "eth_common_api.h"

/* Packet API (test helpers) */
int generate_ethernet_packet(unsigned char *tx_buffer, unsigned char *dest_mac,
			     unsigned char *src_mac, int payload_len)
{
	int fit;
	unsigned short ether_type = ETH_TEST_ETHER_TYPE;
	memcpy(tx_buffer, dest_mac, ETH_TEST_MAC_SIZE);
	memcpy(tx_buffer + ETH_TEST_MAC_SIZE, src_mac, ETH_TEST_MAC_SIZE);
	tx_buffer[12] = (ether_type >> 8) & 0xFF;
	tx_buffer[13] = ether_type & 0xFF;

	payload_len = (payload_len < ETH_TEST_MIN_PAYLOAD) ?
			      ETH_TEST_MIN_PAYLOAD :
			      payload_len;
	payload_len = (payload_len > ETH_TEST_MAX_PAYLOAD) ?
			      ETH_TEST_MAX_PAYLOAD :
			      payload_len;

	/* Align to a 4-byte boundary due to a h/w bug: exact len-bits not set */
	fit = (ETH_TEST_HEADER_SIZE + payload_len) % 4;
	payload_len = payload_len - fit;

	for (int i = 0; i < payload_len; i++) {
		tx_buffer[ETH_TEST_HEADER_SIZE + i] =
			ETH_TEST_TEST_TRAFFIC_PATTERN + i;
	}

	printf("Ethernet packet generated successfully.\n");
	return payload_len + ETH_TEST_HEADER_SIZE;
}
int compare_packets(uint8_t mac_idx, unsigned char *received, int received_len,
			  unsigned char *sent, int sent_len)
{
    int status = 1;
    uint8_t count = 0;

    if (received_len != sent_len) {
	    printf("Package Drop\n");
	    // return sgmii_link_try(mac_idx, true);
	    status = 0;
	    return status;
    }
	// comparisons of tx/rx data byte by byte
	for (int i = 0; i < received_len; i++) {
		if (received[i] != sent[i]) {
			printf("Sent: %x | Received: %x\n", sent[i],
				received[i]);
			count++;
		}
	}

	if (count != 0) {
	    status = 0;
	    return status;
	}

	return status;
}
