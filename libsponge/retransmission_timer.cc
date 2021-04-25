#include "retransmission_timer.hh"

RetransmissionTimer::RetransmissionTimer(const uint16_t initial_retransmission_timeout)
    : _initial_retransmission_timeout(initial_retransmission_timeout)
    , _retransmission_timeout(initial_retransmission_timeout)
    , _timer(0){};

void RetransmissionTimer::start() {
    _timer = 0;
}

bool RetransmissionTimer::alarm(const std::size_t ms_since_last_tick) {
    _timer = _timer + ms_since_last_tick;
    return (_timer >= _retransmission_timeout);
}

void RetransmissionTimer::backoff() {
    _retransmission_timeout = 2 * _retransmission_timeout;
}

void RetransmissionTimer::reset() {
    _retransmission_timeout = _initial_retransmission_timeout;
}
