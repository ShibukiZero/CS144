#ifndef SPONGE_REASSEMBLER_BUFFER_HH
#define SPONGE_REASSEMBLER_BUFFER_HH
#include <map>
#include <string>

using namespace std;

//! \brief A class that contains information about index and substring itself. It is used as a member
//! of ReassemblerBuffer class.
struct Substring {
    //! The start index of substring
    size_t start_index;
    //! The end index of substring
    //! \note the byte that end index is indicating is not contained in this substring.
    size_t end_index;
    //! The string itself
    string data_string;
    //! \brief Construct a substring that combines index information and substring itself.
    Substring(const size_t index, const string &data);
};

//! \name Helper functions
//! @{

//! \brief The overloading operator + combines two adjacent substring with information updated,
//! \return If two substring is adjacent, it returns a Substring class with new information,
//! else, it return nothing.
inline optional<Substring> operator+(const Substring &A, const Substring &B);

//! \brief The overloading operator < compares start index of two substring
//! \note This operator is not used for map sorting only, don't use it in any
//! other cases.
inline bool operator<(const Substring &A, const Substring &B);

//! \brief The overloading operator == compares start index of two substring
//! \note This operator is not used by user.
inline bool operator==(const Substring &A, const Substring &B);
//! @}

//! The ReassemblerBuffer class receives a Substring and store them, if possible, it will also
//! concatenate adjacent substring and update storage.
class ReassemblerBuffer {
    //! The buffer for storing incoming substrings
    map<size_t, Substring> _buffer{};
    //! The number of unassembled bytes in buffer
    size_t _unassembled_bytes{0};

  public:
    //! \brief Default constructor of ReassemblerBuffer class.
    ReassemblerBuffer() = default;

    //! \name Accessors to reassembler
    //! @{

    //! \brief This method is used to push a substring into Buffer. If possible, this method will
    //! concatenate adjacent substrings and update storage.
    void push(const Substring &data);
    //! \brief This method return the substring with smallest start index in buffer. If buffer is
    //! empty, return nothing.
    optional<Substring> front();
    //! \brief This method will delete the substring with smallest start index in buffer.
    void pop();
    //! @}

    //! \brief This method returns whether buffer is empty.
    bool empty() const { return _buffer.empty(); };
    //! \brief This method returns number of bytes unassembled in buffer.
    size_t buffer_size() const { return _unassembled_bytes; };
};

#endif  // SPONGE_REASSEMBLER_BUFFER_HH
