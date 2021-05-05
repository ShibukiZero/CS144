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
    // Note: for either end of the TCP link, the link is considered
    // to be established when SYN is received.
    if (seg.header().syn) {
        _connected = true;
    }
    // Note: if connection has not been established, ignore all reset segments.
    if (seg.header().rst && (_sender.next_seqno_absolute() || _connected)) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _connected = false;
        _linger_after_streams_finish = false;
        return;
    }
    if (active()) {
        // Receiver receives new segment and update window size and ack information.
        _receiver.segment_received(seg);
        if (_receiver.ackno().has_value()) {
            // _new_ack checks whether we have new ack to send.
            bool _new_ack = !(_ackno == _receiver.ackno());
            _ackno = _receiver.ackno();
            if (seg.header().ack) {
                _sender.ack_received(seg.header().ackno, seg.header().win);
            }
            // Note: when SYN has not been sent, fill_window() will generate an SYN.
            _sender.fill_window();
            if (!_sender.segments_out().empty()) {
                _send_segment();
            } else if (_new_ack || seg.length_in_sequence_space()) {
                // Note: if local peer need to send some new ack to peer or there are some new data arrived,
                // local peer need to generate an empty segment for acking.
                _sender.send_empty_segment();
                _send_segment();
            }
            if (_receiver.stream_out().input_ended() && !_sender.stream_in().input_ended()) {
                // If local peer finished input before peer does, we don't need to linger.
                _linger_after_streams_finish = false;
            }
            _timer = 0;
        }
    } else if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        _send_rst();
    }
    return;
}

bool TCPConnection::active() const {
    bool err = (_sender.stream_in().error() || _receiver.stream_out().error());
    bool ended =
        (_sender.stream_in().input_ended() && _sender.stream_in().buffer_empty() &&
         _receiver.stream_out().buffer_empty() && _receiver.stream_out().input_ended() && !_sender.bytes_in_flight());
    return ((!err && !ended && _connected) || _linger_after_streams_finish);
}

size_t TCPConnection::write(const string &data) {
    size_t previous_written = _sender.stream_in().bytes_written();
    if (active()) {
        // Sender get the data and generates segments to send.
        _sender.stream_in().write(data);
        _sender.fill_window();
        _send_segment();
    } else if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        _send_rst();
    }
    return _sender.stream_in().bytes_written() - previous_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (active()) {
        _timer = _timer + ms_since_last_tick;
        if (_sender.consecutive_retransmissions() >= TCPConfig::MAX_RETX_ATTEMPTS) {
            // When number of consecutive retransmissions exceeds maximum, abort connection.
            _send_rst();
            return;
        } else {
            // Sender checks whether it needs to retransmit segments.
            _sender.tick(ms_since_last_tick);
            if (!_sender.segments_out().empty()) {
                _send_segment();
            }
        }
        bool stream_finished = _sender.stream_in().input_ended() && _sender.stream_in().buffer_empty();
        if (stream_finished && _linger_after_streams_finish) {
            // If remote peer finished before us, local peer linger 10 times the initial retransmission timeout
            // ( cfg.rt timeout) since the local peer has received any segments from the remote peer.
            _linger_after_streams_finish = (_timer < 10 * _cfg.rt_timeout);
        }
    } else if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        _send_rst();
    }
    return;
}

void TCPConnection::end_input_stream() {
    if (active()) {
        _sender.stream_in().end_input();
        _sender.fill_window();
        _send_segment();
    } else if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        _send_rst();
    }
    return;
}

void TCPConnection::connect() {
    if (!_connected && !_sender.stream_in().error() && !_receiver.stream_out().error()) {
        _sender.fill_window();
        _send_segment();
    } else if (_sender.stream_in().error() || _receiver.stream_out().error()) {
        _send_rst();
    }
    return;
}

void TCPConnection::_send_rst() {
    _sender.send_empty_segment();
    TCPSegment rst_seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    rst_seg.header().rst = true;
    _segments_out.push(rst_seg);
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _connected = false;
    _linger_after_streams_finish = false;
}

void TCPConnection::_send_segment() {
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_ackno.has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _ackno.value();
        }
        // Set window size as large as possible.
        if (_receiver.window_size() < std::numeric_limits<uint16_t>::max()) {
            seg.header().win = _receiver.window_size();
        } else {
            seg.header().win = std::numeric_limits<uint16_t>::max();
        }
        _segments_out.push(seg);
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
