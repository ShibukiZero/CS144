#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout(retx_timeout)
    , _stream(capacity)
    , _connected(0)
    , _syn_sent(false)
    , _fin_sent(false)
    , _ack_correct(true)
    , _timer(_initial_retransmission_timeout)
    , _receiver_window_size(SIZE_MAX)
    , _outstanding_segments()
    , _bytes_unacknowledged(0)
    , _consecutive_retransmissions(0) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_unacknowledged; }

void TCPSender::fill_window() {
    // When there is space available in the window, transmit as much data as possible.
    while (_bytes_unacknowledged <= _receiver_window_size) {
        if (!_ack_correct) {
            // Reset the ack correct flag to avoid crashes.
            _ack_correct = true;
            return;
        }
        TCPSegment segment = TCPSegment();
        // Note: When SYN has already sent, don't send it again.
        segment.header().syn = (!_connected && !_syn_sent);
        if (segment.header().syn) {
            _syn_sent = true;
        }
        // Note: If connection is not established or this is an SYN segment, TCP sender shouldn't
        // send any data in payload.
        // Note: payload size should not be larger than MAX_PAYLOAD_SIZE.
        size_t payload_size = (!segment.header().syn && _connected) *
                              min((_receiver_window_size == 0) + _receiver_window_size - _bytes_unacknowledged,
                                  TCPConfig::MAX_PAYLOAD_SIZE);
        segment.payload() = _stream.read(payload_size);
        segment.header().seqno = wrap(_next_seqno, _isn);
        // Note: FIN should not be sent when it will exceed receiver's window size.
        // Note: FIN flag should be set when FIN has never sent before and connection has not tore down yet.
        segment.header().fin = _stream.buffer_empty() && _stream.input_ended() && !_fin_sent && _connected &&
                               segment.length_in_sequence_space() <
                                   _receiver_window_size + (_receiver_window_size == 0) - _bytes_unacknowledged;
        if (segment.header().fin) {
            _fin_sent = true;
        }
        if (segment.length_in_sequence_space() == 0) {
            return;
        }
        _bytes_unacknowledged = _bytes_unacknowledged + segment.length_in_sequence_space();
        _next_seqno = _next_seqno + segment.length_in_sequence_space();
        _outstanding_segments.insert(pair<uint64_t, TCPSegment>(_next_seqno, segment));
        _segments_out.push(segment);
        if (!_timer.timing()) {
            _timer.start();
        }
    }
    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_abs_seqno = unwrap(ackno, _isn, _next_seqno);
    // Acknowledge sequence number should be valid, it should be larger than 1 and
    // segment that hasn't been transmitted should not be acked.
    if (ackno_abs_seqno >= 1 && ackno_abs_seqno <= _next_seqno) {
        _ack_correct = true;
        // Note: when acking SYN segment, absolute sequence number of ack should equal to 1.
        if (ackno_abs_seqno >= !_connected + _next_seqno - _bytes_unacknowledged) {
            if (ackno_abs_seqno == 1) {
                _connected = true;
            }
            // Delete segments that are fully acked.
            for (auto ite = _outstanding_segments.begin(); ite != _outstanding_segments.end();) {
                if (ackno_abs_seqno >= ite->first) {
                    _outstanding_segments.erase(ite++);
                } else {
                    ite++;
                }
            }
            size_t previous_bytes_unacknowledged = _bytes_unacknowledged;
            _bytes_unacknowledged = _next_seqno - ackno_abs_seqno;
            _receiver_window_size = window_size;
            // If any data has been acked, set retransmission time to initial value and
            // number of consecutive retransmission to 0. Then, if all outstanding segments
            // are fully acked, stop the timer, else, restart it.
            if (previous_bytes_unacknowledged - _bytes_unacknowledged > 0) {
                _timer.reset();
                _consecutive_retransmissions = 0;
                if (_bytes_unacknowledged == 0) {
                    _timer.stop();
                } else {
                    _timer.start();
                }
            }
        } else {
            return;
        }
    } else {
        _ack_correct = false;
    }
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // If retransmission times out, retransmit segment that is not fully acked and double the retransmission
    // time, after that, restart timer.
    if (_timer.timeout(ms_since_last_tick)) {
        if (_outstanding_segments.empty()) {
            return;
        }
        // Retransmit the first segment that is not fully acknowledged.
        _segments_out.push(_outstanding_segments.upper_bound(_next_seqno - _bytes_unacknowledged)->second);
        // If window size in not 0, update number of consecutive retransmissions and backoff, restart the timer.
        if (_receiver_window_size != 0) {
            _consecutive_retransmissions = _consecutive_retransmissions + 1;
            _timer.backoff();
        }
        _timer.start();
    }
    return;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_segment = TCPSegment();
    empty_segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(empty_segment);
    return;
}
