#pragma once
#include <set>
#include <functional>

using namespace std;

typedef std::set<int> proposalSet;

enum class LatticeMessageType
{
    PROPOSAL,
    ACK,
    NACK,
};
inline string enum_to_string(LatticeMessageType type)
{
    return to_string(static_cast<int>(type));
}

// data used in Lattice Agreement
struct LatticePayload
{
    LatticeMessageType type;
    proposalSet proposed_value;
    int active_proposal_number;

    // PROPOSAL:{1,2,3,}:1
    string serialize() const
    {
        string res = "";
        res += enum_to_string(type) + ":{";
        for (auto &val : proposed_value)
        {
            res += to_string(val);
            res += ",";
        }
        res += "}:" + to_string(active_proposal_number);

        return res;
    };

    void deserialize(string payload_m)
    {
        // type
        auto delim1 = payload_m.find(':');
        this->type = static_cast<LatticeMessageType>(stoi(payload_m.substr(0, delim1)));

        string remaining_str = payload_m.substr(delim1 + 1);

        // proposed_value
        auto delim2 = remaining_str.find(':');
        string proposed_value_string = remaining_str.substr(0, delim2);
        if (proposed_value_string != "")
        {
            // todo: add each element to set
        }

        // active_proposal_number
        string payload_str = remaining_str.substr(delim2 + 1);
    };
};

class LatticeProposer
{
public:
    LatticeProposer(int lattice_shot_num, int f, function<void(const int lattice_shot_num, const LatticePayload &)> broadcastCallback)
        : lattice_shot_num_(lattice_shot_num), f_(f), broadcastPayloadCallback_(broadcastCallback)
    {
        Init();
    }
    void Init();
    void Propose(proposalSet proposal);
    void Receive(const LatticePayload &p);

private:
    bool active_;
    int ack_count_;
    int nack_count_;
    // active_proposal_number_ denotes the number used to distinguish between proposals WITHIN this shot (a single line of proposal in config)
    int active_proposal_number_;
    proposalSet proposed_value_;

    int lattice_shot_num_;
    // n = 2f + 1
    int f_;

    // broadcast (using broadcastCallback)
    function<void(const int lattice_shot_num, const LatticePayload &)> broadcastPayloadCallback_;
};

class LatticeAcceptor
{
public:
    LatticeAcceptor(int lattice_shot_num, function<void(unsigned long src_id, const int lattice_shot_num, const LatticePayload &)> sendCallback)
        : lattice_shot_num_(lattice_shot_num), sendPayloadCallback_(sendCallback)
    {
        Init();
    }
    void Init();
    void Receive(unsigned long src_id, const LatticePayload &p);

private:
    proposalSet accepted_value_;
    int lattice_shot_num_;
    function<void(unsigned long src_id, const int lattice_shot_num, LatticePayload)> sendPayloadCallback_;
};