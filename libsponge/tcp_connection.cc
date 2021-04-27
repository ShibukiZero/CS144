#include "tcp_connection.hh"

#include <iostream>
#include <limits>

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
    // handle rst flag, set both stream error and connection error.
    if (seg.header().rst){
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _err = true;
        return;
    }
    // if there is no error occurs in TCP connection, receiver will receive the segment and ack it.
    _receiver.segment_received(seg);
    // usually, receiver will generate an ack, unless some segments come earlier than
    // SYN segment. in that case, do nothing.
    if (_receiver.ackno().has_value()){
        // generate an ack sequence number for sender to send.
        _ackno = _receiver.ackno();
        // if received segment has acked something
        if (seg.header().ack){
            _sender.ack_received(seg.header().ackno, seg.header().win);
            _sender.fill_window();
            TCPSegment send_seg = _sender.segments_out().front();
            _sender.segments_out().pop();
            if (_receiver.window_size() < std::numeric_limits<uint16_t>::max()){
                send_seg.header().win = _receiver.window_size();
            }
            else{
                send_seg.header().win = std::numeric_limits<uint16_t>::max();
            }
            send_seg.header().ack = true;
            send_seg.header().ackno = _ackno.value();
        }
        if (_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()){
            _linger_after_streams_finish = false;
        }
        _timer = 0;
    }
    return;
}

bool TCPConnection::active() const {
    bool ended = (_sender.stream_in().input_ended() && _receiver.stream_out().input_ended());
    return ((!_err && !ended) || _linger_after_streams_finish);
}

size_t TCPConnection::write(const string &data) {
    // record the previous number of bytes written into outbound stream
    size_t previous_written = _sender.stream_in().bytes_written();
    // write data into outbound stream and generate a segment
    _sender.stream_in().write(data);
    _sender.fill_window();
    // if new segment is generated, set flags right and send it,
    // else, do nothin.
    if (!_sender.segments_out().empty()){
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        // if TCP connection hasn't received an ack (usually for client part), set ack
        // flag to false, else, set it to true and assign ackno to segment.
        if (_ackno.has_value()){
            seg.header().ack = true;
            seg.header().ackno = _ackno.value();
        }
        // set window size as large as possible.
        if (_receiver.window_size() < std::numeric_limits<uint16_t>::max()){
            seg.header().win = _receiver.window_size();
        }
        else {
            seg.header().win = std::numeric_limits<uint16_t>::max();
        }
        // send out the segment
        _segments_out.push(seg);
    }
    // calculate how many bytes are written into outbound stream and return.
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
        TCPSegment rst_seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        rst_seg.header().rst = true;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _err = true;
    }

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    // create a SYN segment with no ack flag and ackno, set window as large as possible.
    // initialize a SYN segment
    _sender.fill_window();
    TCPSegment syn_seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    // set window size as large as possible
    if (_receiver.window_size() < std::numeric_limits<uint16_t>::max()){
        syn_seg.header().win = _receiver.window_size();
    }
    else{
        syn_seg.header().win = std::numeric_limits<uint16_t>::max();
    }
    // send segment out
    _segments_out.push(syn_seg);

}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            TCPSegment rst_seg = _sender.segments_out().front();
            _sender.segments_out().pop();
            rst_seg.header().rst = true;
            _segments_out.push(rst_seg);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
