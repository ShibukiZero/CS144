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
    cout << data << endl;
    // if stream has already been reassembled, do nothing.
    if (_output.input_ended()){
        return;
    }
    // if the substring is the last piece of stream, record the index of it.
    if (eof){
        _eof_index = index + data.length();
        // very special case
        if (_eof_index == 0){
            _output.end_input();
        }
    }
    // if there are some data that have already written into byte stream, reassembler drops those data.
    const uint64_t start_index = max(_tracker, index);
    // if there are some data that beyond capacity, reassembler drops those data.
    const uint64_t end_index = min(index + data.length(), _tracker + _output.remaining_capacity());
    // if end_index is less or equal to start_index, either substring is already written into
    // byte stream or the substring is beyond capacity. In both case, do nothing.
    if (end_index <= start_index){
        return;
    }
    // if substring contains a continuous chunk of stream from tracker, write them into byte stream.
    else if (start_index == _tracker){
        // write data into byte stream.
        _output.write(data.substr(start_index - index, end_index - start_index));
        // update the tracker.
        _tracker = end_index;
        // for any data that have already written, delete them in buffer.
        for (uint64_t i = start_index; i < end_index; i++){
            if (_unassembled.find(i) != _unassembled.end()){
                _unassembled.erase(i);
                _unassembled_bytes = _unassembled_bytes - 1;
            }
        }
        while (_unassembled.find(_tracker) != _unassembled.end()){
            _output.write(_unassembled[_tracker]);
            _unassembled.erase(_tracker);
            _tracker = _tracker + 1;
            _unassembled_bytes = _unassembled_bytes - 1;
        }
        // if tracker is beyond eof index, it means all data have been received. Set byte stream end_input.
        if (_tracker == _eof_index){
            _output.end_input();
        }
    }
    // if substring is ahead of current tracker.
    else {
        for (uint64_t i = start_index; i < end_index; i++){
            if (_unassembled.find(i) == _unassembled.end()){
                _unassembled[i] = data.substr(i - index, 1);
                _unassembled_bytes = _unassembled_bytes + 1;
            }
        }
    }

}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
