#pragma once
#include <stdint.h>
void poll_eth();
void init_ethernet();
int print_network_status();
void tcp_connect(uint32_t remote_ip, uint16_t remote_port);
int isethernetactive();
