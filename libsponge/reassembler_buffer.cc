#include "reassembler_buffer.hh"

Substring::Substring(const size_t index, const string &data)
    : start_index(index)
    , end_index(index + data.length())
    , data_string(data){}

bool operator<(const Substring &A, const Substring &B) {
    return A.start_index < B.start_index;
}

bool operator==(const Substring &A, const Substring &B) {
    return A.start_index == B.start_index;
}

optional<Substring> operator+(const Substring &A, const Substring &B) {
    if (B.start_index > A.end_index || A.start_index > B.end_index ) {
        return {};
    } else {
        string new_substring;
        size_t new_index;
        if (A.start_index <= B.start_index) {
            new_index = A.start_index;
            if (B.end_index > A.end_index){
                new_substring = A.data_string + B.data_string.substr(A.end_index - B.start_index);
            }
            else{
                new_substring = A.data_string;
            }
        } else{
            new_index = B.start_index;
            if (A.end_index > B.end_index){
                new_substring = B.data_string + A.data_string.substr(B.end_index - A.start_index);
            }
            else{
                new_substring = B.data_string;
            }
        }
        return Substring(new_index, new_substring);
    }
}

bool ReassemblerBuffer::empty() {
    return _buffer.empty();
}

void ReassemblerBuffer::push(const Substring &data) {
    if (empty()){
        _buffer.insert(pair<size_t, Substring>(data.start_index, data));
    }
    else{
        Substring new_substring = data;
        for (auto ite = _buffer.begin(); ite != _buffer.end();){
            if ((data + ite->second).has_value()){
                new_substring = (new_substring + ite->second).value();
                _buffer.erase(ite++);
            }
            else{
                ite++;
            }
        }
        _buffer.insert(pair<size_t, Substring>(new_substring.start_index, new_substring));
    }
}

optional<Substring> ReassemblerBuffer::front() {
    if (empty()){
        return {};
    }
    else{
        Substring data = _buffer.begin()->second;
        return data;
    }
}

void ReassemblerBuffer::pop() {
    if (empty()){
        return;
    }
    else{
        _buffer.erase(_buffer.begin());
    }
}