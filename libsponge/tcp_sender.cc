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
    , _receiver_window_size(1)
    , _outstanding_segments()
    , _bytes_unacknowledged(0)
    , _consecutive_retransmissions(0){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_unacknowledged; }

void TCPSender::fill_window() {
    if (!_ack_correct){
        return;
    }
    TCPSegment segment = TCPSegment();
    segment.header().syn = !_connected;
    if (segment.header().syn){
        _syn_sent = true;
    }
    size_t payload_size = min(_receiver_window_size - _bytes_unacknowledged - segment.header().syn, TCPConfig::MAX_PAYLOAD_SIZE);
    segment.payload() = _stream.read(payload_size);
    segment.header().seqno = wrap(_next_seqno, _isn);
    segment.header().fin = _stream.buffer_empty() && _stream.input_ended() && !_fin_sent\
        && segment.length_in_sequence_space() < _receiver_window_size - _bytes_unacknowledged - segment.header().syn;
    if (segment.header().fin){
        _fin_sent = true;
    }
    if (segment.length_in_sequence_space() == 0){
        return;
    }
    _bytes_unacknowledged = _bytes_unacknowledged + segment.length_in_sequence_space();
    _next_seqno = _next_seqno + segment.length_in_sequence_space();
    _outstanding_segments[_next_seqno] = segment;
    _segments_out.push(segment);
    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ackno_abs_seqno = unwrap(ackno, _isn, _next_seqno);
    if (ackno_abs_seqno >= _syn_sent + _next_seqno - _bytes_unacknowledged && ackno_abs_seqno <= _next_seqno){
        if (ackno_abs_seqno == 1){
            _connected = true;
            _syn_sent = false;
        }
        for (auto ite = _outstanding_segments.begin(); ite != _outstanding_segments.end();){
            if (ackno_abs_seqno >= ite->first){
                _outstanding_segments.erase(ite++);
            }
            else{
                ite++;
            }
        }
        _ack_correct = true;
        _bytes_unacknowledged = _next_seqno - ackno_abs_seqno;
        _receiver_window_size = (window_size == 0) + window_size;
    }
    else{
        _ack_correct = false;
    }
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_timer.timeout(ms_since_last_tick)){
        _segments_out.push(_outstanding_segments.upper_bound(_next_seqno - _bytes_unacknowledged)->second);
        _timer.backoff();
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
