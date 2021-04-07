#include "stream_reassembler.hh"
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _unassembled_bytes(0), _tracker(0), _eof_index(UINT16_MAX) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    if (eof){
        _eof_index = index;
    }
    if (_tracker > index){
        return;
    }
    if (_output.input_ended()){
        return;
    }
    if (_output.buffer_size() + _unassembled_bytes + data.size() <= _capacity){
        if (! _unassembled.count(index)){
            _unassembled[index] = data;
            _unassembled_bytes += data.size();
            while (_tracker <= _eof_index) {
                if (_unassembled.count(_tracker)) {
                    _output.write(_unassembled[_tracker]);
                    _tracker += 1;
                    _unassembled_bytes -= _unassembled[_tracker].size();
                    _unassembled.erase(_tracker);
                }
                else {
                    break;
                }
            }
            if (_tracker > _eof_index){
                _output.end_input();
                return;
            }
        }
    }
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
