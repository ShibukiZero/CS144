#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _timer; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _receiver.segment_received(seg);
    if (seg.header().rst){
        _sender.stream_in().error();
        _receiver.stream_out().error();
        return;
    }
    if (_receiver.ackno().has_value()){
        _ackno = _receiver.ackno();
        if (seg.header().ack){
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }
    }
    _timer = 0;
    return;
}

bool TCPConnection::active() const {
    bool err = _sender.stream_in().
    return (_sender.stream_in().input_ended() && _receiver.stream_out().input_ended());
}

size_t TCPConnection::write(const string &data) {
    size_t previous_written = _sender.stream_in().bytes_written();
    _sender.stream_in().write(data);
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    if (_ackno.has_value()){
        seg.header().ack = true;
        seg.header().ackno = _ackno.value();
        seg.header().win = _receiver.window_size();
    }
    _segments_out.push(seg);
    return _sender.stream_in().bytes_written() - previous_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _timer = _timer + ms_since_last_tick;
    if (_sender.consecutive_retransmissions() < TCPConfig::MAX_RETX_ATTEMPTS){
        _sender.tick(ms_since_last_tick);
    }
    else{
        _sender.send_empty_segment();
        TCPSegment empty_seg = _sender.segments_out().front();
        empty_seg.header().rst = true;
        _sender.stream_in().error();
        _receiver.stream_out().error();
    }

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    _sender.fill_window();
    TCPSegment syn_seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    syn_seg.header().win = _receiver.window_size();
    _segments_out.push(syn_seg);

}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}