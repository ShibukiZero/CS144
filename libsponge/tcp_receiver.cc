#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader header = seg.header();
    // If it's a syn segment, set connection and record isn.
    if (header.syn == 1) {
        _connected = 1;
        _isn = header.seqno;
    }
    // If connection is built, set eof and omit syn index,
    // If in window, push them into reassembler, and update the left edge.
    if (_connected) {
        bool eof = header.fin;
        uint64_t abs_seqno = unwrap(header.seqno, _isn, _first_unassembled);
        // Note: if segment has syn flag, stream index starts right after that.
        uint64_t stream_index = abs_seqno - 1 + header.syn;
        size_t pre_window_size = this->window_size();
        _reassembler.push_substring(seg.payload().copy(), stream_index, eof);
        // Note: decrease of window size is bytes pushed into byte stream, update left
        // edge by adding it.
        _first_unassembled = _first_unassembled + pre_window_size - this->window_size();
        return;
    } else {
        return;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // If there is a connection, return accumulative acknowledgements, ack fin as one byte.
    if (_connected) {
        // Return sequence number of first byte unassembled.
        // Note: if all data is pushed into byte stream, ack fin.
        return wrap(_first_unassembled + _reassembler.stream_out().input_ended(), _isn);
    } else {
        return {};
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
