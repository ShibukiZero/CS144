#ifndef SPONGE_REASSEMBLER_BUFFER_HH
#define SPONGE_REASSEMBLER_BUFFER_HH
#include <string>
#include <map>

using namespace std;

struct Substring{
    size_t start_index;
    size_t end_index;
    string data_string;
    Substring(const size_t index, const string &data);
};

inline optional<Substring> operator+(const Substring &A, const Substring &B);
inline bool operator<(const Substring &A, const Substring &B);
inline bool operator==(const Substring &A, const Substring &B);

class ReassemblerBuffer{
    map<size_t, Substring> _buffer{};
  public:
    ReassemblerBuffer() = default;
    void push(const Substring &data);
    optional<Substring> front();
    void pop();
    bool empty();
};

#endif //SPONGE_REASSEMBLER_BUFFER_HH
