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
    // if stream has already been reassembled, do nothing.
    if (_output.input_ended()){
        return;
    }
    // if the substring is the last piece of stream, record the index of it.
    if (eof){
        _eof_index = index + data.length();
        // special case, where tracker is already at eof index.
        // usually, this situation occurs when substring is an empty string.
        if (_eof_index == _tracker){
            _output.end_input();
            return;
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
            if (_unassembled.find(i) != _unassembled.end()){    // if buffer has key i.
                // delete element i in buffer.
                _unassembled.erase(i);
                // number of unassembled bytes minus 1.
                _unassembled_bytes = _unassembled_bytes - 1;
            }
        }
        // after writing continuous data chunk into byte stream, pop out data in buffer
        // that is continuous to current data and write into byte stream.
        while (_unassembled.find(_tracker) != _unassembled.end()){      // can find key tracker in buffer.
            // write key tracker into byte stream.
            _output.write(_unassembled[_tracker]);
            // delete data in buffer.
            _unassembled.erase(_tracker);
            // update tracker.
            _tracker = _tracker + 1;
            // update number of unassembled bytes.
            _unassembled_bytes = _unassembled_bytes - 1;
        }
        // if tracker is beyond eof index, it means all data have been received. Set byte stream end_input.
        if (_tracker == _eof_index){
            _output.end_input();
        }
        return;
    }
    // if substring is ahead of current tracker, store them in buffer.
    else {
        for (uint64_t i = start_index; i < end_index; i++){
            if (_unassembled.find(i) == _unassembled.end()){        // if can't find key i in buffer.
                // insert data into buffer.
                _unassembled[i] = data.substr(i - index, 1);
                // update number of unassembled bytes.
                _unassembled_bytes = _unassembled_bytes + 1;
            }
        }
        return;
    }

}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
