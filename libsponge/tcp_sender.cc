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
    , _ack_correct(true)
    , _timer(_initial_retransmission_timeout)
    , _receiver_window_size(1)
    , _outstanding_segments(TCPSegment())
    , _bytes_unacknowledged(0)
    , _consecutive_retransmissions(0){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_unacknowledged; }

void TCPSender::fill_window() {
    if (!_ack_correct){
        return;
    }
    // if the last byte of segment are all acknowledged, fill the window and send new segment,
    // else, resend the segment and start timer.
    if (_bytes_unacknowledged == 0){
        // if SYN segment is acknowledged, do nothing
        if (_outstanding_segments.header().syn == 1) {
            return;
        }
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
        _next_seqno = _next_seqno + _bytes_unacknowledged;
        _timer.start();
        _segments_out.push(segment);
    }
    else{
        _segments_out.push(_outstanding_segments);
        _timer.start();
    }
    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // receiving an ack means connection established, reset the timer.
    _connected = true;
    _timer.reset();
    // update window size, if window size is 0, set it to 1
    // otherwise TCP sender don't know when to send segment.
    _receiver_window_size = (window_size == 0) + window_size;
    // update number of bytes unacknowledged by computing how many bytes have been acknowledged.
    uint64_t ackno_abs_seqno = unwrap(ackno, _isn, _next_seqno);
    uint64_t outstanding_abs_seqno = unwrap(_outstanding_segments.header().seqno, _isn, _next_seqno);
    if (ackno_abs_seqno > outstanding_abs_seqno && ackno_abs_seqno \
        <= outstanding_abs_seqno + _outstanding_segments.length_in_sequence_space()){
        uint64_t offset = ackno - _outstanding_segments.header().seqno;
        _bytes_unacknowledged = _bytes_unacknowledged - offset;
        _ack_correct = true;
    }
    else{
        _ack_correct = false;
    }
    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // if retransmission times out, resend the segment and exponential backoff the timer.
    if(_timer.timeout(ms_since_last_tick)){
        _segments_out.push(_outstanding_segments);
        _timer.backoff();
        _timer.start();
    }
    return;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    // create a empty segment and push it into queue.
    TCPSegment empty_segment = TCPSegment();
    empty_segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(empty_segment);
    return;
}
