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
    if (header.syn == 1) {
        _connected = 1;
        _isn = header.seqno;
    }
    // if connection is built, set eof and omit syn index,
    // if in window, push them into reassembler, and update the left edge.
    if (_connected) {
        bool eof = header.fin;
        // get the absolute sequence number of incoming segment
        uint64_t abs_seqno = unwrap(header.seqno, _isn, _first_unassembled);
        // stream index is absolute sequence number minus 1,
        // if segment has syn flag, stream index starts right after that.
        uint64_t stream_index = abs_seqno - 1 + header.syn;
        // record previous window size
        size_t pre_window_size = this->window_size();
        // push the data into reassembler
        _reassembler.push_substring(seg.payload().copy(), stream_index, eof);
        // the decrease of window size is bytes pushed into byte stream, update left
        // edge by adding it.
        _first_unassembled = _first_unassembled + pre_window_size - this->window_size();
        return;
    }
    // if there is no connection, do nothing.
    else {
        return;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // if there is a connection, return accumulative acknowledgements, ack fin as one byte.
    if (_connected) {
        // return sequence number of first byte unassembled, if all data is pushed into
        // byte stream, ack fin.
        return wrap(_first_unassembled + _reassembler.stream_out().input_ended(), _isn);
    }
    // if there is no connection, return empty.
    else {
        return {};
    }
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
