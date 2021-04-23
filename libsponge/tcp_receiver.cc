#include "tcp_receiver.hh"
#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader tcpHeader = seg.header();
    Buffer payLoad = seg.payload();
    cout << tcpHeader.seqno << payLoad.size() << endl;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    return {};
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
