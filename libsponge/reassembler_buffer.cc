#include "reassembler_buffer.hh"

Substring::Substring(const size_t index, const string &data)
    : start_index(index)
    , data(data) {
    end_index = start_index + data.length();
}

bool Substring::operator<(const Substring &A, const Substring &B) {
    return A.start_index < B.start_index;
}

bool Substring::operator==(const Substring &A, const Substring &B) {
    return A.start_index == B.start_index;
}

optional<Substring> Substring::operator+(const Substring &A, const Substring &B) {
    if (B.start_index > A.end_index || A.start_index > B.end_index){
        return;
    }else{
        string new_substring;
        if (A.start_index <= B.start_index && B.start_index <= A.end_index){
            if (B.end_index > A.end_index){
                new_substring = A.data + B.data.substr(B.end_index - A.end_index, :);
                return Substring(A.start_index, new_substring);
            }else{
                return A;
            }
        }else if (B.start_index <= A.start_index && A.start_index <= B.end_index){
            if (A.end_index > B.start_index){
                new_substring = B.data + A.data.substr(A.end_index);
                return Substring(B.start_index, new_substring);
            }else{
                return B;
            }
        }
    }
}

bool ReassemblerBuffer::empty() {
    return _buffer.empty();
}

void ReassemblerBuffer::push(const Substring &data) {
    if (empty()){
        _buffer[data.start_index] = data;
    }else{
        Substring new_substring = data;
        for (auto ite = _buffer.begin(); ite != _buffer.end();){
            if ((data + ite->second).has_value()){
                new_substring = (new_substring + ite->second).value();
                _buffer.erase(ite++);
            }else{
                ite++;
            }
        }
        _buffer[new_substring.start_index] = new_substring;
    }
}

optional<Substring> ReassemblerBuffer::front() {
    if (empty()){
        return;
    }else{
        Substring data = _buffer.begin()->second;
        return data;
    }
}

void ReassemblerBuffer::pop() {
    if (empty()){
        return;
    }else{
        _buffer.erase(_buffer.begin());
    }
}