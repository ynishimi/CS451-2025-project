#pragma once
// C++ Program to illustrate how we can serialize and
// deserialize an object

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

enum class MessageType
{
    DATA,
    ACK,
};

inline string enum_to_string(MessageType type)
{
    return to_string(static_cast<int>(type));
}

template <typename T>
class Msg
{
public:
    Msg() {};
    // Constructor to initialize the data members
    Msg(MessageType type, unsigned long src_id, unsigned int seq_id, unsigned long relay_id, const T &p)
        : type(type), src_id(src_id), seq_id(seq_id), relay_id(relay_id), payload(p) {}

    // Getter methods for the class
    // unsigned long getId() const { return src_id; }
    // string getM() const { return m; }

    //  Function for Serialization
    string serialize()
    {
        string result = "";
        result = enum_to_string(type) + ':' + to_string(src_id) + ':' + to_string(seq_id) + ':' + to_string(relay_id) + ':';
        result += payload.serialize();
        return result;
    }

    //  Function for Deserialization
    void deserialize(string serialized_m)
    {
        auto delim1 = serialized_m.find(':');
        this->type = static_cast<MessageType>(stoi(serialized_m.substr(0, delim1)));
        string remaining_str = serialized_m.substr(delim1 + 1);
        auto delim2 = remaining_str.find(':');
        this->src_id = stoul(remaining_str.substr(0, delim2));
        remaining_str = remaining_str.substr(delim2 + 1);
        auto delim3 = remaining_str.find(':');
        this->seq_id = static_cast<unsigned int>(stoi(remaining_str.substr(0, delim3)));
        remaining_str = remaining_str.substr(delim3 + 1);
        auto delim4 = remaining_str.find(':');
        this->relay_id = stoul(remaining_str.substr(0, delim4));

        // payload
        string payload_str = remaining_str.substr(delim4 + 1);
        this->payload.deserialize(payload_str);
    }

    bool operator<(const Msg &r) const
    {
        return std::tie(this->src_id, this->seq_id) < std::tie(r.src_id, r.seq_id);
    }

public:
    MessageType type;
    unsigned long src_id;
    unsigned int seq_id;
    unsigned long relay_id;
    // string m;
    T payload;
};
