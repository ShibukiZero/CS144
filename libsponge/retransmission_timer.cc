#include "retransmission_timer.hh"

using namespace std;

//! \param[in] initial retransmission timeout of the timer
RetransmissionTimer::RetransmissionTimer(const uint16_t initial_retransmission_timeout)
    : _initial_retransmission_timeout(initial_retransmission_timeout)
    , _retransmission_timeout(initial_retransmission_timeout)
    , _timer(0) {}

void RetransmissionTimer::start() {
    _timer = 0;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
bool RetransmissionTimer::timeout(const size_t ms_since_last_tick) {
    _timer = _timer + ms_since_last_tick;
    return (_timer >= _retransmission_timeout);
}

void RetransmissionTimer::backoff() {
    _retransmission_timeout = 2 * _retransmission_timeout;
}

void RetransmissionTimer::reset() {
    _retransmission_timeout = _initial_retransmission_timeout;
}
