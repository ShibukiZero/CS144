#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // receive the TCP segment header before further processing.
    TCPHeader header = seg.header();
    // if it's a syn segment, set connection and record isn.
    if (header.syn == 1){
        _connected = 1;
        _isn = header.seqno;
    }
    // if connection is built, set eof and omit syn index,
    // if in window, push them into reassembler, and update the left edge.
    if (_connected){
        bool eof = header.fin;
        uint64_t abs_seqno = unwrap(header.seqno, _isn, _first_unassembled);
        uint64_t stream_index = max(_first_unassembled, abs_seqno) - 1;
        size_t pre_window_size = this->window_size();
        _reassembler.push_substring(seg.payload().copy(), stream_index, eof);
        _first_unassembled = _first_unassembled + this->window_size() - pre_window_size;
        return;
        }
    // if there is no connection, do nothing.
    else{
        return;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_connected){
        return wrap(_first_unassembled, _isn);
    }
    else{
        return {};
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
