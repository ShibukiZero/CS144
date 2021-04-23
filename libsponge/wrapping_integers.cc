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
    WrappingInt32 seqno = WrappingInt32(isn.raw_value());
    uint32_t offset = n % (1ul << 32);
    seqno = seqno + offset;
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
    uint32_t offset = uint32_t(n - isn);
    uint32_t quotient = checkpoint / (1ul << 32);
    uint32_t remainder = checkpoint % (1ul << 32);
    uint64_t abs_seqno;
    if (offset > remainder && quotient > 0 && uint32_t(offset - remainder) > (1ul << 31)){
        abs_seqno = (quotient - 1) * (1ul << 32) + offset;
    }
    else if (remainder > offset && uint32_t(remainder - offset) > (1ul << 31)){
        abs_seqno = (quotient + 1) * (1ul << 32) + offset;
    }
    else{
        abs_seqno = quotient * (1ul << 32) + offset;
    }
    return abs_seqno;
}
