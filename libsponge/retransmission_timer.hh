#ifndef SPONGE_RETRANSMISSION_TIMER_HH
#define SPONGE_RETRANSMISSION_TIMER_HH
#include <cstdint>

//! \brief The timer which goes off if transmission times out.

class RetransmissionTimer{
  private:
    unsigned int _initial_retransmission_timeout;
    unsigned int _retransmission_timeout;
    unsigned int _timer;

  public:
    RetransmissionTimer(const uint16_t initial_retransmission_timeout);

    void start();

    bool alart(const size_t ms_since_last_tick);

    void double_RTO();

    void reset_RTO();
};

#endif //SPONGE_RETRANSMISSION_TIMER_HH
