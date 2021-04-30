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
    // If two substrings are not adjacent, return nothing
    if (B.start_index > A.end_index || A.start_index > B.end_index ) {
        return {};
    }
    else {
        string new_substring;
        size_t new_index;
        if (A.start_index <= B.start_index) {
            new_index = A.start_index;
            if (B.end_index > A.end_index){
                // new_substring concatenates A and adjacent parts B.
                new_substring = A.data_string + B.data_string.substr(A.end_index - B.start_index);
            }
            else{
                new_substring = A.data_string;
            }
        } else{
            new_index = B.start_index;
            if (A.end_index > B.end_index){
                // new_substring concatenates B and adjacent parts A.
                new_substring = B.data_string + A.data_string.substr(B.end_index - A.start_index);
            }
            else{
                new_substring = B.data_string;
            }
        }
        return Substring(new_index, new_substring);
    }
}

void ReassemblerBuffer::push(const Substring &data) {
    if (empty()){
        _unassembled_bytes += data.data_string.length();
        _buffer.insert(pair<size_t, Substring>(data.start_index, data));
    }
    else{
        Substring new_substring = data;
        // Traverse buffer to check whether any substring is adjacent to data.
        for (auto ite = _buffer.begin(); ite != _buffer.end();){
            if ((data + ite->second).has_value()){
                // Concatenate adjacent substrings, store the result in new_substring
                // and erase old substrings.
                new_substring = (new_substring + ite->second).value();
                _unassembled_bytes -= ite->second.data_string.length();
                _buffer.erase(ite++);
            }
            else{
                ite++;
            }
        }
        // After traversal is finished, push concatenated substring into buffer.
        _unassembled_bytes += new_substring.data_string.length();
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
        _unassembled_bytes -= _buffer.begin()->second.data_string.length();
        _buffer.erase(_buffer.begin());
    }
}
