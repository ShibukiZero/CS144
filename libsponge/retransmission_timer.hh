#ifndef SPONGE_RETRANSMISSION_TIMER_HH
#define SPONGE_RETRANSMISSION_TIMER_HH

#include <cstdint>
#include <cstddef>

//! \brief The timer which goes off if transmission times out.

class RetransmissionTimer{
  private:
    //! initial retruansmission timeout
    unsigned int _initial_retransmission_timeout;

    //! current retransmission timeout
    unsigned int _retransmission_timeout;

    //! timer records how much millisecond has elapsed after start time
    unsigned int _timer;

  public:
    //! Initialize a timer
    RetransmissionTimer(const uint16_t initial_retransmission_timeout);

    //! \name "Input" interface for the writer
    //!@{

    //! \brief Start the timer
    void start();

    //! \brief Timing, when timeout, returns false,
    //! else return true
    bool alarm(const size_t ms_since_last_tick);

    //! \brief If conflict occurs, double the retransmission time
    void backoff();

    //! \brief After a success transmission, reset the retransmission time
    void reset();
    //!@}
};

#endif //SPONGE_RETRANSMISSION_TIMER_HH
