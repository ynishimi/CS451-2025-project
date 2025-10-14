// C++ Program to illustrate how we can serialize and
// deserialize an object
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

class Msg
{
private:
    unsigned long id;
    string m;

public:
    Msg() {};
    // Constructor to initialize the data members
    Msg(unsigned long id, const string &m)
        : id(id), m(m)
    {
    }

    // Getter methods for the class
    unsigned long getId() const { return id; }
    string getM() const { return m; }

    //  Function for Serialization
    string serialize()
    {
        return to_string(id) + ':' + m;
    }

    //  Function for Deserialization
    void deserialize(string serialized_m)
    {
        auto delim = serialized_m.find(':');
        this->id = stoul(serialized_m.substr(0, delim));
        this->m = serialized_m.substr(delim + 1);
    }
};