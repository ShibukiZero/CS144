#ifndef SPONGE_RETRANSMISSION_TIMER_HH
#define SPONGE_RETRANSMISSION_TIMER_HH

//! \brief The timer which goes off if transmission times out.

//!

class RetransmissionTimer{
  private:
    unsigned int _initial_retransmission_timeout;
    unsigned int _retransmission_timeout;

};

#endif //SPONGE_RETRANSMISSION_TIMER_HH
