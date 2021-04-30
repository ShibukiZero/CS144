#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
//
using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity), _buffer(""), _bytes_written(0), _bytes_read(0) {}

size_t ByteStream::write(const string &data) {
    // The length of data string shouldn't be larger than capacity.
    size_t len = min(remaining_capacity(), data.length());
    if (len) {
        _buffer += data.substr(0, len);
        _bytes_written += len;
    }
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    if (!buffer_empty()) {
        return _buffer.substr(0, min(buffer_size(), len));
    } else {
        return "";
    }
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (!buffer_empty()) {
        _bytes_read += min(buffer_size(), len);
        _buffer.erase(0, min(buffer_size(), len));
    }
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    const string data = peek_output(min(buffer_size(), len));
    pop_output(min(buffer_size(), len));
    return data;
}

void ByteStream::end_input() {
    _end = true;
    return;
}

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return _buffer.length(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const {
    return input_ended() && buffer_empty();
}

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer.length(); }
