#include "test/tcp/tests/unit/unit.h"
#include "test/utility.h"
#include "handlers/tcp/tcp_shared.h"


bool tcp_test_checksum(struct handler_t* handler, struct test_config_t* config) {
    struct tcp_header_t* tcp_header = get_tcp_header_from_package(checksum_packet_no_options);
    struct ipv4_header_t* ipv4_header = get_ipv4_header_from_package(checksum_packet_no_options);
    uint16_t payload_size = get_tcp_payload_length_from_package(checksum_packet_no_options);

    uint16_t expected_checksum = tcp_header->checksum;
    tcp_header->checksum = 0;

    uint16_t checksum = _tcp_calculate_checksum(tcp_header, ipv4_header->source_ip, ipv4_header->destination_ip, payload_size);

    if(expected_checksum != checksum) {
        return false;
    }

    tcp_header = get_tcp_header_from_package(checksum_packet_with_data_non_aligned);
    ipv4_header = get_ipv4_header_from_package(checksum_packet_with_data_non_aligned);
    payload_size = get_tcp_payload_length_from_package(checksum_packet_with_data_non_aligned);

    expected_checksum = tcp_header->checksum;
    tcp_header->checksum = 0;

    checksum = _tcp_calculate_checksum(tcp_header, ipv4_header->source_ip, ipv4_header->destination_ip, payload_size);

    return expected_checksum == checksum;
}

