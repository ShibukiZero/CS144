#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    std::deque<std::pair<EthernetAddress, uint32_t>>::iterator ite;
    bool arp_cashed = false;
    EthernetAddress cashed_mac;
    for (ite = _arp_table.begin(); ite != _arp_table.end(); ite++) {
        const uint32_t cashed_ip = ite->second;
        if (cashed_ip == next_hop_ip) {
            cashed_mac = ite->first;
            arp_cashed = true;
            break;
        }
    }
    EthernetFrame frame_out;
    if (arp_cashed) {
        frame_out.payload() = dgram.serialize();
        frame_out.header().type = EthernetHeader::TYPE_IPv4;
        frame_out.header().src = _ethernet_address;
        frame_out.header().dst = cashed_mac;
    } else {
        ARPMessage arp_message;
        arp_message.opcode = ARPMessage::OPCODE_REQUEST;
        arp_message.sender_ethernet_address = _ethernet_address;
        arp_message.sender_ip_address = _ip_address.ipv4_numeric();
        arp_message.target_ethernet_address = ETHERNET_LOCALHOST;
        arp_message.target_ip_address = next_hop_ip;
        frame_out.payload() = arp_message.serialize();
        frame_out.header().type = EthernetHeader::TYPE_ARP;
        frame_out.header().src = _ethernet_address;
        frame_out.header().dst = ETHERNET_BROADCAST;
        _arp_request.push(std::make_pair(frame_out, _current_timer));
    }
    _frames_out.push(frame_out);
    return;
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram ipv4_datagram;
        if (ipv4_datagram.parse(frame.payload()) == ParseResult::NoError) {
            return ipv4_datagram;
        } else {
            cerr << "ipv4 datagram failed to parse!\n";
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_message;
        if (arp_message.parse(frame.payload()) == ParseResult::NoError) {
            if (arp_message.opcode == ARPMessage::OPCODE_REQUEST) {
                arp_update(arp_message.sender_ethernet_address, arp_message.sender_ip_address);
                if (arp_message.target_ip_address == _ip_address.ipv4_numeric()) {
                    ARPMessage arp_reply;
                    arp_reply.opcode = ARPMessage::OPCODE_REPLY;
                    arp_reply.sender_ethernet_address = _ethernet_address;
                    arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
                    arp_reply.target_ethernet_address = arp_message.sender_ethernet_address;
                    arp_reply.target_ip_address = arp_message.sender_ip_address;
                    EthernetFrame frame_out;
                    frame_out.payload() = arp_message.serialize();
                    frame_out.header().type = EthernetHeader::TYPE_ARP;
                    frame_out.header().src = _ethernet_address;
                    frame_out.header().dst = arp_message.sender_ethernet_address;
                    _frames_out.push(frame_out);
                }
            } else if (arp_message.opcode == ARPMessage::OPCODE_REPLY) {
                arp_update(arp_message.sender_ethernet_address, arp_message.sender_ip_address);
            } else {
                cerr << "arp type unknown!\n";
            }
        } else {
            cerr << "arp message failed ro parse!\n";
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _current_timer = _current_timer + ms_since_last_tick;
    while(!_arp_table.empty()) {
        if (_current_timer - _arp_table_timer.front() > 30e3) {
            _arp_table.pop_front();
            _arp_table_timer.pop();
            continue;
        } else {
            break;
        }
    }
    while (!_arp_request.empty()) {
        if (_current_timer - _arp_request.front().second > 5e3) {
            _frames_out.push(_arp_request.front().first);
            _arp_request.push(std::make_pair(_arp_request.front().first, _current_timer));
            _arp_request.pop();
            continue;
        } else {
            break;
        }
    }
}

//! \param[in] mac the Ethernet address of sender of received arp message
//! \param[in] ip the ip address of sender of received arp message
void NetworkInterface::arp_update(const EthernetAddress mac, const uint32_t ip) {
    bool arp_cashed = false;
    std::deque<std::pair<EthernetAddress, uint32_t>>::iterator ite_arp = _arp_table.begin();
    while(ite_arp != _arp_table.end()) {
        if (ite_arp->first == mac && ite_arp->second == ip) {
            arp_cashed = true;
            break;
        } else if (ite_arp->first == mac || ite_arp->second == ip) {
            _arp_table.erase(ite_arp);
        } else {
            ite_arp++;
        }
    }
    if (!arp_cashed) {
        _arp_table_timer.push(_current_timer);
        _arp_table.push_back(std::make_pair(mac, ip));
    }
    if (!_ipv4_queue.empty()) {
        std::deque<std::pair<InternetDatagram, uint32_t>>::iterator ite_ipv4 = _ipv4_queue.begin();
        while (ite_ipv4 != _ipv4_queue.end()) {
            if (ip == ite_ipv4->second) {
                EthernetFrame frame_out;
                frame_out.payload() = ite_ipv4->first.serialize();
                frame_out.header().type = EthernetHeader::TYPE_IPv4;
                frame_out.header().src = _ethernet_address;
                frame_out.header().dst = mac;
                _frames_out.push(frame_out);
                _ipv4_queue.erase(ite_ipv4);
            } else {
                ite_ipv4++;
            }
        }
    }
}

//!
