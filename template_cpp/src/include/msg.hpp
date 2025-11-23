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

class Msg
{
public:
    Msg() {};
    // Constructor to initialize the data members
    Msg(MessageType type, long src_id, const string &m)
        : type(type), src_id(src_id), m(m)
    {
    }

    // Getter methods for the class
    unsigned long getId() const { return src_id; }
    string getM() const { return m; }

    //  Function for Serialization
    string serialize()
    {
        return enum_to_string(type) + ':' + to_string(src_id) + ':' + m;
    }

    //  Function for Deserialization
    void deserialize(string serialized_m)
    {
        auto delim1 = serialized_m.find(':');
        this->type = static_cast<MessageType>(stoi(serialized_m.substr(0, delim1)));
        string remaining_str = serialized_m.substr(delim1 + 1);
        auto delim2 = remaining_str.find(':');
        this->src_id = stoul(remaining_str.substr(0, delim2));
        this->m = remaining_str.substr(delim2 + 1);
    }

public:
    MessageType type;
    unsigned long src_id;
    string m;
};