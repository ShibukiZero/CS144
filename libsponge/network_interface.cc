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

    // Traverse the whole arp table to find whether there is a match.
    bool arp_cashed = false;
    EthernetAddress cashed_mac;
    for (auto ite = _arp_table.begin(); ite != _arp_table.end(); ite++) {
        const uint32_t cashed_ip = ite->second;
        if (cashed_ip == next_hop_ip) {
            cashed_mac = ite->first;
            arp_cashed = true;
            break;
        }
    }
    // If there is a match, forward the ipv4 datagram, else generate an arp request.
    EthernetFrame frame_out;
    if (arp_cashed) {
        frame_out.payload() = dgram.serialize();
        frame_out.header().type = EthernetHeader::TYPE_IPv4;
        frame_out.header().src = _ethernet_address;
        frame_out.header().dst = cashed_mac;
    } else {
        // Note: need to queue the ipv4 datagram if network interface don't know how to forward it, if
        // arp message is already sent, don't send it again to avoid broadcast flooding.
        _ipv4_queue.push_back(std::make_pair(dgram, next_hop_ip));
        for (auto ite_arpreq = _arp_request.begin(); ite_arpreq != _arp_request.end(); ite_arpreq++) {
            ARPMessage arp_outgoing;
            if (arp_outgoing.parse(ite_arpreq->first.payload()) == ParseResult::NoError &&
                arp_outgoing.target_ip_address == next_hop_ip) {
                return;
            }
        }
        // Generate arp request and broadcast it.
        ARPMessage arp_request;
        arp_request.opcode = ARPMessage::OPCODE_REQUEST;
        arp_request.sender_ethernet_address = _ethernet_address;
        arp_request.sender_ip_address = _ip_address.ipv4_numeric();
        // Note: in arp request message, target ethernet address should be (00:00:00:00:00:00) because
        // network interface don't know mac address of target yet.
        arp_request.target_ethernet_address = ETHERNET_LOCALHOST;
        arp_request.target_ip_address = next_hop_ip;
        frame_out.payload() = arp_request.serialize();
        frame_out.header().type = EthernetHeader::TYPE_ARP;
        frame_out.header().src = _ethernet_address;
        frame_out.header().dst = ETHERNET_BROADCAST;
        _arp_request.push_back(std::make_pair(frame_out, _current_timer));
    }
    _frames_out.push(frame_out);
    return;
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // If frame is not for local host, ignore it.
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST) {
        return {};
    }
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        // If the frame carries a ipv4 datagram and is successfully parsed, return it to its caller.
        InternetDatagram ipv4_datagram;
        if (ipv4_datagram.parse(frame.payload()) == ParseResult::NoError) {
            return ipv4_datagram;
        } else {
            cerr << "ipv4 datagram failed to parse!\n";
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        // If the frame carries an arp, learn from it. Generate proper reply to it if necessary.
        ARPMessage arp_message;
        if (arp_message.parse(frame.payload()) == ParseResult::NoError) {
            arp_update(arp_message.sender_ethernet_address, arp_message.sender_ip_address);
            if (arp_message.opcode == ARPMessage::OPCODE_REQUEST) {
                if (arp_message.target_ip_address == _ip_address.ipv4_numeric()) {
                    ARPMessage arp_reply;
                    arp_reply.opcode = ARPMessage::OPCODE_REPLY;
                    arp_reply.sender_ethernet_address = _ethernet_address;
                    arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
                    arp_reply.target_ethernet_address = arp_message.sender_ethernet_address;
                    arp_reply.target_ip_address = arp_message.sender_ip_address;
                    EthernetFrame frame_out;
                    frame_out.payload() = arp_reply.serialize();
                    frame_out.header().type = EthernetHeader::TYPE_ARP;
                    frame_out.header().src = _ethernet_address;
                    frame_out.header().dst = arp_message.sender_ethernet_address;
                    _frames_out.push(frame_out);
                }
            } else if (arp_message.opcode != ARPMessage::OPCODE_REPLY) {
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
    // Delete arp table if it's out of date.
    while (!_arp_table.empty()) {
        if (_current_timer - _arp_table_timer.front() > 30e3) {
            _arp_table.pop_front();
            _arp_table_timer.pop_front();
            continue;
        } else {
            break;
        }
    }
    // Retransmit arp request if it times out.
    while (!_arp_request.empty()) {
        if (_current_timer - _arp_request.front().second > 5e3) {
            _frames_out.push(_arp_request.front().first);
            _arp_request.push_back(std::make_pair(_arp_request.front().first, _current_timer));
            _arp_request.pop_front();
            continue;
        } else {
            break;
        }
    }
}

//! \param[in] mac the Ethernet address of sender of received arp message
//! \param[in] ip the ip address of sender of received arp message
void NetworkInterface::arp_update(const EthernetAddress mac, const uint32_t ip) {
    // Traverse the arp table to check whether arp mapping is cashed, if not, queue it.
    bool arp_cashed = false;
    auto ite_arp = _arp_table.begin();
    while (ite_arp != _arp_table.end()) {
        if (ite_arp->first == mac && ite_arp->second == ip) {
            arp_cashed = true;
            break;
        } else if (ite_arp->first == mac || ite_arp->second == ip) {
            // Note: arp mapping changed in this case, need to update it.
            _arp_table.erase(ite_arp);
        } else {
            ite_arp++;
        }
    }
    if (!arp_cashed) {
        _arp_table_timer.push_back(_current_timer);
        _arp_table.push_back(std::make_pair(mac, ip));
    }
    // Update arp request buffer if outgoing arp request is replied.
    for (auto ite_arpreq = _arp_request.begin(); ite_arpreq != _arp_request.end(); ite_arpreq++) {
        ARPMessage arp_outgoing;
        if (arp_outgoing.parse(ite_arpreq->first.payload()) == ParseResult::NoError &&
            arp_outgoing.target_ip_address == ip) {
            _arp_request.erase(ite_arpreq);
            break;
        }
    }
    // If received arp mapping is enough to forward an ipv4 datagram that was cashed, transmit it.
    if (!_ipv4_queue.empty()) {
        auto ite_ipv4 = _ipv4_queue.begin();
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
