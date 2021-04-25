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
    , _connected(false)
    , _timer(_initial_retransmission_timeout)
    , _receiver_window_size(1)
    , _outstanding_segments()
    , _bytes_unacknowledged(0)
    , _consecutive_retransmissions(0){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_unacknowledged; }

void TCPSender::fill_window() {
    // initialize TCP segment
    TCPSegment segment = TCPSegment();
    // set the SYN flag, if connection is established, set SYN flag true.
    segment.header().syn = !_connected;
    // set the payload size as big as possible, but no larger than 1452.
    size_t payload_size = min(_receiver_window_size - segment.header().syn, TCPConfig::MAX_PAYLOAD_SIZE);
    // read data from stream and assign it to payload
    segment.payload() = _stream.read(payload_size);
    // set sequence number of segment
    segment.header().seqno = wrap(_next_seqno, _isn);
    // if stream input is ended and window size is not full, set FIN flag to true.
    segment.header().fin = _stream.input_ended() && \
            segment.length_in_sequence_space() < _receiver_window_size - segment.header().syn;
    // store the segment, update number of bytes unacknowledged,
    // start the timer and push segment into queue.
    _outstanding_segments = segment;
    _bytes_unacknowledged = _bytes_unacknowledged + segment.length_in_sequence_space();
    _timer.start();
    _segments_out.push(segment);

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_abs_seqno = unwrap(ackno, _isn, _next_seqno);
    _timer.reset();
    _receiver_window_size = (window_size == 0) + window_size;
    _next_seqno = ack_abs_seqno;
    uint64_t offset = ackno - _outstanding_segments.header().seqno;
    _bytes_unacknowledged = _bytes_unacknowledged - offset;
    if (_bytes_unacknowledged == 0){
        fill_window();
    }
    else{
        _segments_out.push(_outstanding_segments);
        _timer.start();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(!_timer.alarm(ms_since_last_tick)){

    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_segment = TCPSegment();
    _segments_out.push(empty_segment);
    return;
}
