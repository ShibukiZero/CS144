#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // compute the remainder of absolute sequence number during one round trip(2^32 bits)
    uint32_t offset = n % (1ul << 32);
    // compute sequence number, which equals isn plus offset
    WrappingInt32 seqno = isn + offset;
    return seqno;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // compute the offset between n and isn
    uint32_t offset = uint32_t(n - isn);
    // initialize absolute sequence number
    uint64_t abs_seqno;
    // if offset is already larger than checkpoint, which is rare to see,
    // then return offset as absolute sequence number.
    if (offset >= checkpoint){
        abs_seqno = offset;
        return abs_seqno;
    }
    // if checkpoint is larger than offset, find the right round trip so that
    // absolute sequence number is closest to checkpoint.
    else{
        // compute quotient and remainder of checkpoint with 2^32
        uint32_t quotient = checkpoint / (1ul << 32);
        uint32_t remainder = checkpoint % (1ul << 32);
        // if offset is larger than remainder and distance between offset and remainder
        // is larger than half the round trip, set absolute sequence number in previous
        // round trip.
        if (offset > remainder && uint32_t(offset - remainder) > (1ul << 31)){
            abs_seqno = (quotient - 1) * (1ul << 32) + offset;
        }
        // if remainder is larger than offset and distance between offset and remainder
        // is larger than half the round trip, set absolute sequence number in next
        // round trip.
        else if (remainder > offset && uint32_t(remainder - offset) > (1ul << 31)){
            abs_seqno = (quotient + 1) * (1ul << 32) + offset;
        }
        // if distance between offset and remainder is less than half the round trip, no
        // matter which one is larger, we set absolute sequence number in current round trip.
        else{
            abs_seqno = quotient * (1ul << 32) + offset;
        }
        return abs_seqno;
    }
}
